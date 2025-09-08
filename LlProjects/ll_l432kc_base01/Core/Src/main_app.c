/**
  ******************************************************************************
  * @file           : main_app.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIME_1S				(1000)					/* 1秒判定時間[ms]			*/
#define UART_BUFF_SIZE		(58)					/* UARTバッファサイズ		*/

/* UART命令 */
#define UART_CMD_HELP		(0x08)					/* ヘルプ表示(^H)			*/
#define UART_CMD_RESET		(0x12)					/* リセット(^R)				*/
#define UART_CMD_SLEEP		(0x13)					/* スリープ(^S)				*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;							/* 1秒タイマー				*/
static uint8_t u8s_RcvData[UART_BUFF_SIZE];			/* UART受信データ			*/
static uint16_t u16s_RcvDataSize;					/* UART受信データサイズ		*/


/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  EXTI12立ち上がりコールバック関数
  * @param  None
  * @retval None
  */
void EXTI12_Rising_Callback(void)
{
	/* 文字を出力する */
	uartEchoStr("Exti12");
}

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	mem_set08(&u8s_RcvData[0], 0x00, UART_BUFF_SIZE);
	u16s_RcvDataSize = 0;

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* プログラム開始メッセージを表示する */
	uartEchoStrln("");
	uartEchoStrln("Start UART/GPIO sample!!");
}

/**
  * @brief  周期処理関数
  * @param  None
  * @retval None
  */
void loop(void)
{
	/* UART受信データを取得する */
	u16s_RcvDataSize = uartGetRxData(&u8s_RcvData[0], UART_BUFF_SIZE);
	/* UART受信データが存在する場合 */
	if (u16s_RcvDataSize > 0) {
		/* UART送信データを登録する */
		uartSetTxData(&u8s_RcvData[0], u16s_RcvDataSize);

		/* UART命令解析 */
		switch (u8s_RcvData[0]) {
		/* ヘルプ表示(^H) */
		case UART_CMD_HELP:
			/* UART命令表示 */
			uartEchoStrln("");
			uartEchoStrln("^H :Help");
			uartEchoStrln("^R :Reset");
			uartEchoStrln("^S :Sleep");
			break;
		/* リセット(^R) */
		case UART_CMD_RESET:
			/* リセット処理 */
			NVIC_SystemReset();
			break;
		/* スリープ(^S) */
		case UART_CMD_SLEEP:
			/* SysTickタイマー停止 */
			LL_SYSTICK_DisableIT();
			/* イベント待機 */
			__WFE();
			__WFE();
			/* SysTickタイマー開始 */
			LL_SYSTICK_EnableIT();
			/* 文字を出力する */
			uartEchoStr("<Wakeup!!>");
			break;
		}
	}

	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		/* 文字を出力する */
		uartEchoStr(".");

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}
}

/* Private functions ---------------------------------------------------------*/

