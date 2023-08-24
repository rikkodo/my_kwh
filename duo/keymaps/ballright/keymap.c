// Copyright 2021 Hayashi (@w_vwbw)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "analog.h"
#include "split_util.h"
#include "math.h"

// clang-format off
// レイヤー名
enum layer_number
{
    BASE = 0,
    ONOFF, OFFON, ONON,                     // トグルスイッチで変更するレイヤー
    LOWER, UPPER, UTIL,                     // 長押しで変更するレイヤー
    MOUSE, BALL_SETTINGS, LIGHT_SETTINGS    // 自動マウスレイヤー切り替えや設定用のレイヤー
};

// トラックボール設定用のキーコード
enum ball_keycodes
{
    BALL_SAFE_RANGE = SAFE_RANGE,
    CPI_I, CPI_D, L_ANG_I, L_ANG_D, L_INV, R_ANG_I, R_ANG_D, R_INV,
    L_CHMOD, R_CHMOD, INV_SCRL, MOD_SCRL, AUTO_MOUSE,
    OLED_MOD
};

// 自作エリア
enum my_ball_keycodes
{
    MY_BALL_SAFE_RANGE = SAFE_RANGE + 200 - 64,
    L_OBL_I, L_OBL_D, R_OBL_I, R_OBL_D,
    MY_BALL_SAFE_RANGE_END,

    /* TODO
    MY_CONFIG_SAFE_RANGE = SAFE_RANGE + 250 - 64,
    ORTHO_SCROLL, DEBUG_KC,
    MY_CONFIG_SAFE_RANGE_END,
    // ORTHO_SCROLLはマウス側
    */
};

// トラックボールの定数、変数
#define CPI_OPTIONS {800, 1100, 1400, 1500, 1600, 1900, 2200}
#define CPI_DEFAULT 3
#define ANGLE_OPTIONS { \
      0,  10,  20,  30,  40,  50,  60,  70,  80, \
     90, 100, 110, 120, 130, 140, 150, 160, 170, \
    180, 190, 200, 210, 220, 230, 240, 250, 260, \
    270, 280, 290, 300, 310, 320, 330, 340, 350, \
}
#define ANGLE_DEFAULT 31
#define OBLIQUE_DEFAULT 5
#define CPI_OPTION_SIZE (sizeof(cpi_array) / sizeof(uint16_t))
#define ANGLE_OPTION_SIZE (sizeof(angle_array) / sizeof(uint16_t))
#define SCROLL_DIVISOR 150.0

typedef union
{
    uint32_t raw;
    struct
    {
        uint8_t cpi_idx         :4; //  4
        uint8_t angle_idx       :6; // 10
        bool inv                :1; // 11
        bool scmode             :1; // 12
        bool inv_sc             :1; // 13
        bool auto_mouse         :1; // 14
        uint8_t oblique_idx     :6; // 20
        bool ortho_scroll       :1; // 21
        bool kc_debug           :1; // 22
    };
} ballconfig_t;
// clang-format on

ballconfig_t ballconfig;
uint16_t cpi_array[] = CPI_OPTIONS;
uint16_t angle_array[] = ANGLE_OPTIONS;
bool scrolling;
float scroll_accumulated_h;
float scroll_accumulated_v;
uint8_t pre_layer;
uint8_t cur_layer;
bool oled_mode;

static uint16_t prev_kc = 0;

// ジョイスティックの定数、変数
#define OFFSET 300
#define NO_JOYSTICK_VAL 100
int16_t gp27_newt;
int16_t gp28_newt;
keypos_t key_up;
keypos_t key_left;
keypos_t key_right;
keypos_t key_down;
bool pressed_up;
bool pressed_down;
bool pressed_left;
bool pressed_right;
bool joystick_attached;

// 初期化関係
void eeconfig_init_user(void)
{
    ballconfig.cpi_idx = CPI_DEFAULT;
    ballconfig.angle_idx = ANGLE_DEFAULT;
    ballconfig.inv = false;
    ballconfig.scmode = false;
    ballconfig.inv_sc = false;
    ballconfig.auto_mouse = true;
    ballconfig.oblique_idx = OBLIQUE_DEFAULT;
    ballconfig.ortho_scroll = false;
    ballconfig.kc_debug = false;
    eeconfig_update_user(ballconfig.raw);
}

void matrix_init_user(void)
{
    gp27_newt = analogReadPin(GP27);
    gp28_newt = analogReadPin(GP28);
    key_up.row = 3;
    key_up.col = 6;
    key_down.row = 1;
    key_down.col = 6;
    key_left.row = 2;
    key_left.col = 6;
    key_right.row = 4;
    key_right.col = 6;
    if (gp27_newt < NO_JOYSTICK_VAL || gp28_newt < NO_JOYSTICK_VAL)
    {
        joystick_attached = false;
    }
    else
    {
        joystick_attached = true;
    }
    scrolling = false;
    scroll_accumulated_h = 0;
    scroll_accumulated_v = 0;
    pre_layer = 0;
    cur_layer = 0;
    oled_mode = true;
}

void keyboard_post_init_user(void)
{
    ballconfig.raw = eeconfig_read_user();
    pointing_device_set_cpi(cpi_array[ballconfig.cpi_idx]);
    set_auto_mouse_enable(ballconfig.auto_mouse);
}

// スクロールレイヤーかを判定する。
static bool isScrollLayer(void)
{
    return IS_LAYER_ON(8) || IS_LAYER_ON(9);
}

// カーソル調整
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report)
{
    double theta = angle_array[ballconfig.angle_idx] * (M_PI / 180);
    double phi = angle_array[ballconfig.oblique_idx] * (M_PI / 180);
    int8_t x_rev = +mouse_report.x * cos(theta) + mouse_report.y * cos(phi);
    int8_t y_rev = +mouse_report.x * sin(theta) + mouse_report.y * sin(phi);

    if (ballconfig.inv)
    {
        x_rev = -1 * x_rev;
    }
    if ((isScrollLayer() != scrolling) || ballconfig.scmode)
    {
        if (ballconfig.ortho_scroll)
        {
            if (abs(x_rev) > abs(y_rev))
            {
                y_rev = 0;
            }
            else
            {
                x_rev = 0;
            }
        }

        mouse_report.h = x_rev;
        mouse_report.v = y_rev;
        if (!ballconfig.inv_sc)
        {

            mouse_report.h = -1 * mouse_report.h;
            mouse_report.v = -1 * mouse_report.v;
        }

        scroll_accumulated_h += (float)mouse_report.h / SCROLL_DIVISOR;
        scroll_accumulated_v += (float)mouse_report.v / SCROLL_DIVISOR;
        mouse_report.h = (int8_t)scroll_accumulated_h;
        mouse_report.v = (int8_t)scroll_accumulated_v;
        scroll_accumulated_h -= (int8_t)scroll_accumulated_h;
        scroll_accumulated_v -= (int8_t)scroll_accumulated_v;

        mouse_report.x = 0;
        mouse_report.y = 0;
    }
    else
    {
        mouse_report.x = -1 * x_rev;
        mouse_report.y = y_rev;
    }
    return mouse_report;
}

// キースキャン関係
bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
    prev_kc = keycode;
    if (keycode == CPI_I && record->event.pressed)
    {
        ballconfig.cpi_idx = ballconfig.cpi_idx + 1;
        if (ballconfig.cpi_idx >= CPI_OPTION_SIZE)
        {
            ballconfig.cpi_idx = CPI_OPTION_SIZE - 1;
        }
        eeconfig_update_user(ballconfig.raw);
        pointing_device_set_cpi(cpi_array[ballconfig.cpi_idx]);
    }
    if (keycode == CPI_D && record->event.pressed)
    {
        if (ballconfig.cpi_idx > 0)
        {
            ballconfig.cpi_idx = ballconfig.cpi_idx - 1;
        }
        eeconfig_update_user(ballconfig.raw);
        pointing_device_set_cpi(cpi_array[ballconfig.cpi_idx]);
    }
    if (keycode == L_ANG_I && record->event.pressed)
    {
        ballconfig.angle_idx = (ballconfig.angle_idx + 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == L_ANG_D && record->event.pressed)
    {
        ballconfig.angle_idx = (ANGLE_OPTION_SIZE + ballconfig.angle_idx - 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == R_ANG_I && record->event.pressed)
    {
        ballconfig.angle_idx = (ballconfig.angle_idx + 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == R_ANG_D && record->event.pressed)
    {
        ballconfig.angle_idx = (ANGLE_OPTION_SIZE + ballconfig.angle_idx - 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == L_OBL_I && record->event.pressed)
    {
        ballconfig.oblique_idx = (ballconfig.oblique_idx + 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == L_OBL_D && record->event.pressed)
    {
        ballconfig.oblique_idx = (ANGLE_OPTION_SIZE + ballconfig.oblique_idx - 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == R_OBL_I && record->event.pressed)
    {
        ballconfig.oblique_idx = (ballconfig.oblique_idx + 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == R_OBL_D && record->event.pressed)
    {
        ballconfig.oblique_idx = (ANGLE_OPTION_SIZE + ballconfig.oblique_idx - 1) % ANGLE_OPTION_SIZE;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == R_INV && record->event.pressed)
    {
        ballconfig.inv = !ballconfig.inv;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == L_INV && record->event.pressed)
    {
        ballconfig.inv = !ballconfig.inv;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == L_CHMOD && record->event.pressed)
    {
        ballconfig.scmode = !ballconfig.scmode;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == R_CHMOD && record->event.pressed)
    {
        ballconfig.scmode = !ballconfig.scmode;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == INV_SCRL && record->event.pressed)
    {
        ballconfig.inv_sc = !ballconfig.inv_sc;
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == MOD_SCRL)
    {
        scrolling = record->event.pressed;
    }
    if (keycode == AUTO_MOUSE && record->event.pressed)
    {
        ballconfig.auto_mouse = !ballconfig.auto_mouse;
        set_auto_mouse_enable(ballconfig.auto_mouse);
        eeconfig_update_user(ballconfig.raw);
    }
    if (keycode == OLED_MOD && record->event.pressed)
    {
        oled_mode = !oled_mode;
        oled_clear();
    }
    return true;
}

void matrix_scan_user(void)
{
    if (joystick_attached)
    {
        int16_t gp27_val = analogReadPin(GP27);
        int16_t gp28_val = analogReadPin(GP28);
        float x_val = ((float)gp27_val - (float)gp27_newt) * (float)511 / ((float)1023 - (float)gp27_newt);
        float y_val = ((float)gp28_val - (float)gp28_newt) * (float)511 / ((float)1023 - (float)gp28_newt);
        if (x_val < -511)
        {
            x_val = -511;
        }
        if (y_val < -511)
        {
            y_val = -511;
        }

        int8_t layer = layer_switch_get_layer(key_up);
        int16_t keycode_up = keymap_key_to_keycode(layer, key_up);
        int16_t keycode_left = keymap_key_to_keycode(layer, key_left);
        int16_t keycode_right = keymap_key_to_keycode(layer, key_right);
        int16_t keycode_down = keymap_key_to_keycode(layer, key_down);

        if (pressed_left == false && x_val > (float)OFFSET)
        {
            register_code(keycode_left);
            pressed_left = true;
            wait_ms(100);
        }
        else if (pressed_left == true && x_val < (float)OFFSET)
        {
            unregister_code(keycode_left);
            pressed_left = false;
            wait_ms(100);
        }
        else if (pressed_right == false && x_val < (float)-1 * (float)OFFSET)
        {
            register_code(keycode_right);
            pressed_right = true;
            wait_ms(100);
        }
        else if (pressed_right == true && x_val > (float)-1 * (float)OFFSET)
        {
            unregister_code(keycode_right);
            pressed_right = false;
            wait_ms(100);
        }

        if (pressed_up == false && y_val > (float)OFFSET)
        {
            register_code(keycode_up);
            pressed_up = true;
            wait_ms(100);
        }
        else if (pressed_up == true && y_val < (float)OFFSET)
        {
            unregister_code(keycode_up);
            pressed_up = false;
            wait_ms(100);
        }
        else if (pressed_down == false && y_val < (float)-1 * (float)OFFSET)
        {
            register_code(keycode_down);
            pressed_down = true;
            wait_ms(100);
        }
        else if (pressed_down == true && y_val > (float)-1 * (float)OFFSET)
        {
            unregister_code(keycode_down);
            pressed_down = false;
            wait_ms(100);
        }
    }
}

// keymap, oled表示をheaderに移動
#include "consts.h"

// OLED表示
bool oled_task_user(void)
{
    cur_layer = get_highest_layer(layer_state);
    oled_set_cursor(0, 0);
    if (is_keyboard_master())
    {
        if (!oled_mode)
        {
            if (is_keyboard_left())
            {
                oled_write_raw_P(number[cur_layer], sizeof(number[cur_layer]));
            }
            else
            {
                oled_write_raw_P(reverse_number[cur_layer], sizeof(reverse_number[cur_layer]));
            }
        }
        else
        {
            oled_write_P(PSTR("CPI:   "), false);
            oled_write(get_u16_str(cpi_array[ballconfig.cpi_idx], ' '), false);

            if (ballconfig.auto_mouse)
            {
                oled_write_P(PSTR(" AUTO"), false);
            }
            else
            {
                oled_write_P(PSTR("     "), false);
            }

            oled_set_cursor(0, 1);
            oled_write_P(PSTR("ANGLE: "), false);
            oled_write(get_u16_str(angle_array[ballconfig.angle_idx], ' '), false);
            oled_write_P(PSTR("/"), false);
            oled_write(get_u16_str(angle_array[ballconfig.oblique_idx], ' '), false);

            oled_set_cursor(0, 2);
            oled_write_P(PSTR("INV AXIS:"), false);
            if (ballconfig.inv)
            {
                oled_write_P(PSTR("YES"), false);
            }
            else
            {
                oled_write_P(PSTR(" NO"), false);
            }
            if (ballconfig.scmode)
            {
                oled_write_P(PSTR(" SCRL"), false);
            }
            else
            {
                oled_write_P(PSTR(" CRSL"), false);
            }

            oled_set_cursor(0, 3);
            oled_write_P(PSTR("INV SCRL:"), false);
            if (ballconfig.inv_sc)
            {
                oled_write_P(PSTR("YES "), false);
            }
            else
            {
                oled_write_P(PSTR(" NO "), false);
            }

            if (ballconfig.kc_debug)
            {
                oled_write(get_u16_str(prev_kc, ' '), false);
            }
        }
    }
    else
    {
        if (is_keyboard_left())
        {
            oled_write_raw_P(number[cur_layer], sizeof(number[cur_layer]));
        }
        else
        {
            oled_write_raw_P(reverse_number[cur_layer], sizeof(reverse_number[cur_layer]));
        }
    }

    return false;
}

// トラックボール関係のキーはmouse recordキーとする
bool is_mouse_record_kb(uint16_t keycode, keyrecord_t *record)
{
    switch (keycode)
    {
    case KC_ENT:
        return true;
    case KC_RIGHT ... KC_UP:
        return true;
    case BALL_SAFE_RANGE ... OLED_MOD:
        return true;
    case MY_BALL_SAFE_RANGE ... MY_BALL_SAFE_RANGE_END:
        return true;
    default:
        return is_mouse_record_user(keycode, record);
    }
}