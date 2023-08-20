# Killer Whale


## About

KillerWhaleの自分用ファームウェア

やりたいこと

* Keyboard Quantizer用のMASTER\_LEFT化
* スクロール中はマウスレイヤ変更を受け付けないようにする
* LCDにレイヤ情報、モディファイアキー情報を表示する

## 参照

[goropikariの備忘録 keyball39 を組み立てた 検証2: MASTER_LEFT で固定してしまう](https://goropikari.hatenablog.com/entry/keyball39_build_log#検証2-MASTER_LEFT-で固定してしまう)

## キーボード情報

<https://github.com/Taro-Hayashi/KillerWhale/blob/main/README.md>

## ビルド方法

1. qmk環境を構築する (0.21.6で動作を確認)
2. <qmk\_firmware>/keyboardsにkillerwhaleという名前で本ディレクトリへのシンボリックリンクを貼る
3. <qmk\_firmware>下で、`make killerwhale/duo:ballright`

