# Buildにあたっての注意

## これは何?

F303K8用プロジェクト
F411用はDrill01

## コンパイラの入手

ヘッダとバイナリをインストールするには
```
 brew install --cask gcc-arm-embedded
```
でインストールする。 もしarm-none-eabi-gcc, arm-none-eabi-binutilsを
単体ででインストールしてしまっていたら
``` bash
brew uninstall arm-none-eabi-gcc
brew uninstall arm-none-eabi-bintuils
brew autoremove
```
等でクリーンアップしておく。これらツールはバイナリのみでヘッダは含まれていない。

## CubeMXのインストール
最近、Drill3.iocを起動する際にCubeCLTをダウンロードしてパスを指定しないといけなくなった。
リリースのzipをダウンロードして展開。
プロジェクトブラウザのDrill03.iocをクリック
CubeMXが立ち上がるので、GenerateCodeでコード生成をする。
CMakeとか、ライブラリファイルのコピーが行われる。
その後Buildする。
```
OpenOCDの場所
/Users/kikuchi/Library/Arduino15/packages/arduino/tools/openocd/0.11.0-arduino2/bin/openocd
CubeMXの場所
/Applications/STMicroelectronics/STM32CubeMX.app/Contents/Resources/STM32CubeMX
CubeCLTの場所
/opt/ST/STM32CubeCLT_1.18.0
```

## CLionのToolchainの設定
mac用のtoolchainのみ用意しておけばよい。
CubeMXが作成するCMakefile.txtの中でコンパイラが指定される。
arm-none-eabi-gcc等にパスを通しておく必要があるが、前述の
brewで入れると 自動でパスが通っているようだ。

## CMakeの設定
実行環境、デバッグ環境で使うCMakeのオプションを指定する。 どちらもCMakeオプションに
```
-DCMAKE_SYSTEM_NAME=Generic
```
を指定する。これをしないと-archオプションが自動付加されてエラーになる。
Cmakeのテンプレートでやると、テストコンパイルでエラーがでるのでコマンドラインオプションのほうがよい。

## 実行環境の設定
