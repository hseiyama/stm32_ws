# stm32_ws

## STM32CubeIDE 環境設定
### (1) STM32CubeMXのコード生成時にUTF-8が文字化けしないようにする
環境変数に以下を追加する。
変数名 | 変数値
--- | ---
JAVA_TOOL_OPTIONS | -Dfile.encoding=UTF-8
