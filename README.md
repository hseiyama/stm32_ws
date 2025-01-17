# stm32_ws

## STM32CubeIDE 環境設定
### (1) STM32CubeMXのコード生成時にUTF-8が文字化けしないようにする
環境変数に以下を追加する。
変数名 | 変数値
--- | ---
JAVA_TOOL_OPTIONS | -Dfile.encoding=UTF-8

## 参考になる情報
### (1) ARM-ASM-Tutorial (ARMプロセッサのアセンブラ理解の一助となる)
[Mikrocontroller.net のリンクはこちら。](https://www.mikrocontroller.net/articles/ARM-ASM-Tutorial)

## NUCLEO-H755ZI-Q 注意事項
### (1) 上限のクロック設定
- SupplySource = PWR_DIRECT_SMPS_SUPPLY  
CPU1 (CM7) = 400 MHz  
CPU2 (CM4) = 200 MHz  
[情報元のリンク](https://community.st.com/t5/stm32cubemx-mcus/stm32h755-nucleo-and-pwr-direct-smps-supply/td-p/113752)
### (2) 起動不能状態（クロック設定ミス）からの復帰
- STM32CubeProgrammer で操作する。  
(1) Mode="Power down" にして "Connect" を実行する。  
(2) "Full chip erase" を実行する。  
[情報元のリンク](https://community.st.com/t5/stm32-mcus/how-to-unbrick-an-stm32h7-after-setting-the-wrong-power-mode/ta-p/49691)
