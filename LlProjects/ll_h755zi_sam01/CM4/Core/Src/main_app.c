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
#define TIME_500MS			(500)					/* 500ms判定時間[ms]		*/
#define RCV_BUFF_SIZE		(64)					/* 受信バッファサイズ		*/

/* SYNC命令 */
#define SYNC_LED_BRINK_A	(0x01)					/* LED点滅A(全Core点滅)		*/
#define SYNC_LED_BRINK_B	(0x02)					/* LED点滅B(時間差点滅)		*/
#define SYNC_LED_BRINK_C	(0x03)					/* LED点滅C(各Core反転)		*/
#define SYNC_CM4_RESET		(0x04)					/* リセット(CM4)			*/
#define SYNC_CM4_SLEEP		(0x05)					/* スリープ(CM4)			*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;							/* 1秒タイマー				*/
static Timer sts_Timer500ms;						/* 500msタイマー			*/
static uint8_t u8s_RcvData[RCV_BUFF_SIZE];			/* 受信データ				*/
static uint16_t u16_RcvDataSize;					/* 受信データサイズ			*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  EXTI13イベント発生コールバック関数
  * @param  None
  * @retval None
  */
void EXTI13_EventOccurred_Callback(void)
{
	/* 文字を出力する */
	syncEchoStr("<EXTI13_EventOccurred>");
}

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	mem_set08(&u8s_RcvData[0], 0x00, RCV_BUFF_SIZE);
	u16_RcvDataSize = 0;

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);
	/* タイマーを停止する */
	stopTimer(&sts_Timer500ms);

	/* プログラム開始メッセージを表示する */
	syncEchoStrln("Core-M4 start!!");
}

/**
  * @brief  周期処理関数
  * @param  None
  * @retval None
  */
void loop(void)
{
	/* SYNC受信データを取得する */
	u16_RcvDataSize = syncGetRxData(&u8s_RcvData[0], RCV_BUFF_SIZE);
	/* SYNC受信データが存在する場合 */
	if (u16_RcvDataSize > 0) {
		/* LED点滅の同期制御 */
		switch (u8s_RcvData[0]) {
		/* LED点滅A(全Core点滅) */
		case SYNC_LED_BRINK_A:
			/* ユーザーLEDを点灯する */
			LL_GPIO_SetOutputPin(LD1_GPIO_Port, LD1_Pin);
			LL_GPIO_SetOutputPin(LD3_GPIO_Port, LD3_Pin);
			/* 文字を出力する */
			syncEchoStr("<SYNC_LedBrinkA>");
			/* タイマーを再開する */
			startTimer(&sts_Timer1s);
			break;
		/* LED点滅B(時間差点滅) */
		case SYNC_LED_BRINK_B:
			/* ユーザーLEDを点灯/消灯する */
			LL_GPIO_SetOutputPin(LD1_GPIO_Port, LD1_Pin);
			LL_GPIO_ResetOutputPin(LD3_GPIO_Port, LD3_Pin);
			/* 文字を出力する */
			syncEchoStr("<SYNC_LedBrinkB>");
			/* タイマーを停止する */
			stopTimer(&sts_Timer1s);
			/* タイマーを開始する */
			startTimer(&sts_Timer500ms);
			break;
		/* LED点滅C(各Core反転) */
		case SYNC_LED_BRINK_C:
			/* ユーザーLEDを点灯する */
			LL_GPIO_SetOutputPin(LD1_GPIO_Port, LD1_Pin);
			LL_GPIO_SetOutputPin(LD3_GPIO_Port, LD3_Pin);
			/* 文字を出力する */
			syncEchoStr("<SYNC_LedBrinkC>");
			/* タイマーを再開する */
			startTimer(&sts_Timer1s);
			break;
		/* リセット(CM4) */
		case SYNC_CM4_RESET:
			/* リセット処理 */
			NVIC_SystemReset();
			break;
		/* スリープ(CM4) */
		case SYNC_CM4_SLEEP:
			/* SysTickタイマー停止 */
			LL_SYSTICK_DisableIT();
			/* イベント待機 */
			__WFE();
			__WFE();
			/* SysTickタイマー開始 */
			LL_SYSTICK_EnableIT();
			/* 文字を出力する */
			syncEchoStr("<SYNC_WakeupCM4>");
			break;
		default:
			break;
		}
	}

	/* 500ms判定時間が満了した場合 */
	if (checkTimer(&sts_Timer500ms, TIME_500MS)) {
		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		/* タイマーを停止する */
		stopTimer(&sts_Timer500ms);
		/* タイマーを開始する */
		startTimer(&sts_Timer1s);
	}

	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}
}

/* Private functions ---------------------------------------------------------*/

