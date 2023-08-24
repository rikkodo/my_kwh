# Killer Whale

## About

KillerWhaleの自分用ファームウェア

以下に対応

* Keyboard Quantizer用のMASTER\_LEFT化
  * 起動後にbootを押下すれば良いらしいが、ファーム書くならこっちの方が楽
* スクロール中はマウスレイヤ変更を受け付けないようにする
  * <https://github.com/qmk/qmk_firmware/blob/master/docs/feature_pointing_device.md#adding-mouse-keys>
* [斜行座標系](https://ja.wikipedia.org/wiki/%E6%96%9C%E4%BA%A4%E5%BA%A7%E6%A8%99%E7%B3%BB)を用いたボール処理
  * $\theta = 310, \phi = 50$ が好みだったため結局直交座標
  * CUSTOM(203), CUSTOM(204)で $\phi$ を操作

やりたいこと

* LCDにレイヤ情報、モディファイアキー情報を表示する
* スクロールの刻みにfloatを使わないようにする
  * 引っかかるため
* 直交スクロールのモード変更
* デバッグ用にキーコード表示

## 参照

[goropikariの備忘録 keyball39 を組み立てた 検証2: MASTER_LEFT で固定してしまう](https://goropikari.hatenablog.com/entry/keyball39_build_log#検証2-MASTER_LEFT-で固定してしまう)

## キーボード情報

<https://github.com/Taro-Hayashi/KillerWhale/blob/main/README.md>

## ビルド方法

1. qmk環境を構築する (0.21.6で動作を確認)
2. <qmk\_firmware>/keyboardsにkillerwhaleという名前で本ディレクトリへのシンボリックリンクを貼る
3. <qmk\_firmware>下で、`make killerwhale/duo:ballright`
