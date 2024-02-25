[In English](README_en_US.md)

# M5Stackシリーズ用イメージビュワー

M5Stackシリーズでファイルシステム（LittleFS）上にある画像を次々と表示するプログラムです。

以下の2種類の表示方法があります。

* 手動（Manul）モード  
  Aボタン（正順）・Cボタン（逆順）を押すたびに画像を切り替えて表示します。M5Dialの場合はダイヤルを左右（逆順・正順）に回すことで画像を切り替えます。
* 自動（Auto）モード  
  設定ファイルで指定した間隔（ミリ秒）で表示します。指定した表示間隔を最大値としたランダムの間隔で表示することもできます。

## コンパイル方法

[PlatformIO IDE](https://platformio.org/platformio-ide)環境でコンパイルします。機種に合わせて環境を選択してください。

| 機種            | 環境                  |
| :-------------- | :---------------------|
| M5Stack BASIC   | env:m5stack-basic     |
| M5Stack Fire    | env:m5stack-fire      |
| M5Go            | env:m5stack-m5go      |
| M5Stack Core2   | env:m5stack-core2     |
| M5Stack Core3   | env:m5stack-core3     |
| M5Stick C       | env:m5stick-c         |
| M5Stick C Plus  | env:m5stick-c-plus    |
| M5Stick C Plus2 | env:m5stick-c-plus2   |
| M5ATOM S3       | env:m5stack-atoms3    |
| M5Dial          | env:m5stack-dial      |
| M5Cardputer     | env:m5stack-cardputer |
| CoreInk         | env:m5stack-coreink   |
| M5Paper         | env:m5stack-paper     |

## 設定ファイル

設定ファイル`data/image-viewer.json`で以下を設定できます。

* `AutoMode`  
  自動表示モードのオン（`true`）・オフ（`false`）
* `AutoModeInterval`  
  自動表示モードのときの画像の切り替え間隔（ミリ秒）
* `AutoModeRandomized`  
  ランダム切り替え間隔モードのオン（`true`）・オフ（`false`）

設定ファイルがない場合は，自動モードはオフ（`false`），切り替え間隔は 3 秒（3000 ミリ秒），ランダム切り替え間隔モードはオフ（`false`）になります。

```json
{
  "AutoMode": false,
  "AutoModeInterval": 3000,
  "AutoModeRandomized": false
}
```

ランダム切り替え間隔モードをオンにすると，0 ミリ秒から`AutoModeInterval`で指定したミリ秒の間のランダムな間隔で画像を切り替えます。

この設定ファイルは次の「表示する画像のアップロード」の際に画像と一緒に実機のファイルシステムにアップロードされます。

## 表示する画像のアップロード

表示する画像ファイル（PNG，JPEG，BMP）を`data`ディレクトリに置き，以下のいずれかの方法で実機にアップロードします。

* PlatformIO メニューから「Upload Filesystem Image」を選択する。
* コマンドラインから`pio run --target uploadfs`を実行する。

このとき，設定ファイル`data/image-viewer.json`も含め，`data`ディレイクトリに置かれているファイルはすべて実機にアップロードされます。

## 実行方法

プログラムを起動すると，ファイルシステムにある画像ファイル（PNG，JPEG，BMP）を順に表示します。通常は設定ファイルで指定したモードで起動します。Aボタンを押しながら起動すると，設定にかかわらず自動モードになります。

IMUが使える場合は，画面の向きに合わせて表示が自動的に切り替わります。

起動すると以下の画面が表示されます。設定ファイルがない場合`Config:`の情報は表示されません。

```text
Image Viewer v1.0.0
Config:
 /image-viewer.json
 AutoMode: false
 Interval: 3000ms
 Randomized: false
Mode:
 Manual, Auto or Auto(Forced)
Image Files:
 画像ファイル1
 画像ファイル2
 ...
 画像ファイルN
```

ファイルシステム上に画像ファイルがない場合は，以下のように表示されます。

```text
Image Viewer v1.0.0
Config:
 /image-viewer.json
 AutoMode: false
 Interval: 3000ms
 Randomized: false
Mode:
 Manual, Auto or Auto(Forced)
No image files found
```

画面に画像一覧が表示されてから一定時間（デフォルトは 3 秒）が経過すると表示モードに応じて画面に画像が表示されます。

## 独自ファームウェアの作成方法

配布用の独自ファームウェアファイルを生成するには，以下のいずれかの方法で生成できます。

* PlatformIO メニューから「Custom/Generate User Custom」を選択する。
* コマンドラインから`pio run --target firmware`を実行する。

`firmware`ディレクトリに`[機種名]_[名前]_firmware_[バージョン].bin`ファイルが生成されます。

生成されるファームウェアの設定に関しては，`platformio.ini`ファイルの`image-viewer`セクションにある以下の項目を参照してください。

* 機種名  
  `custom_firmware_target`
  各機種を表す文字列が各環境ごとに定義されています。  
  例：M5Stack Basic = m5basic
* ファームウェアの名前  
  `custom_firmware_name`（初期値：`image_viewer`）
* ファームウェアのバージョンが書かれているファイル名  
  `custom_firmware_version_file` （初期値：`ImageViewer.cpp`）  
  ソースコードの書かれている`"vX.Y.Z"`から`X.Y.Z`の値を抽出します。
* ファームウェアの拡張子（ドットは不要）  
  `custom_firmware_suffix`（初期値：`bin`）
* 生成先ディレクトリ  
  `custom_firmware_dir`（初期値：`firmware`）
