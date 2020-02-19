# Precision Pro to PlayStation2 Adapter

マイクロソフト製フライトスティック SideWinder Precision Pro を、SONY PlayStation2に接続するアダプタです。

アナログスティックの傾きは、デュアルショック２の左アナログスティックではなく、方向キーにパルスを加えて疑似アナログスティックにしています。
（タイトーメモリーズⅡ 下巻に収録されているナイトストライカーをスムーズにプレイするための仕様です）

## ハードウェア

アダプタ本体となるマイコンボードと電子部品をブレッドボードに組み付けます。
ブレッドボードの配線図は、PP2PS2_Breadboard.pdf をご覧ください。

 - ブレッドボード ハーフサイズ x1
 - マイコンボード Arduino Nano x1
 - 3ステートバッファIC TC74HC125AP x1
 - ジャンパワイヤ 適宜
 - Dサブ15ピン メスコネクタ (to Precision Pro) x1
 − PlayStation2 コントローラケーブル x1
 - USBバッテリ (アダプタ電源)

## ソフトウェア

Arduino IDEを使用して、PP2PS2 スケッチをArduino Nanoに書き込みます。

 - PP2PS2.ino           スケッチ本体
 - PrecisionPro.h       PrecisionProクラス
 - DualShock2Talker.cpp DualShock2Talker本体
 - DualShock2Talker.h   DualShock2Talkerクラス
 - PP2DS2Talker.h       PP2DS2Talkerクラス
 - PWM.h                PWMクラス
 - sw_data_t.h          sw_data_t 共用体
 - portmacro.h          digitalWrite に相当する、portOn/portOffマクロを定義

## 使用方法

アダプタボードに Precision Pro とゲーム機を接続したら、
USBバッテリをArduino Nanoに接続して、ゲーム機の電源を入れます。

Precision Pro のスティック・ボタンと、PlayStation2 のコントローラの対応は以下の通りです。

 - スティック        十字キー（疑似アナログ）
 - スティックひねり  なし
 - スロットル        なし

 - Fireボタン        ×ボタン cross
 - Topボタン         ○ボタン circle
 - Top Upボタン      △ボタン triangle
 - Top Downボタン    □ボタン square
 - HATスイッチ       十字キー

 - Aボタン           STARTボタン
 - Bボタン           R1ボタン
 - Cボタン           L1ボタン
 - Dボタン           SELECTボタン


## 参考

1. [MaZderMind/SidewinderInterface](https://github.com/MaZderMind/SidewinderInterface/tree/master/software) Precision Proの通信プロトコルについて、オシロスコープ画像つきで詳細に説明されています。union sw_data_t の定義を引用しています。
2. [PS2専用ローリングスイッチ](http://magicpuppet.org/rollingswitch.html) DualShock の通信仕様や Arduino によるエミュレートについて、詳細に説明されています。rolling_switch.ino より DualShock2 との通信部分を抜き出して、 DualShock2Talker クラスとして実装しています。

