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

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;							/* 1秒タイマー				*/

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
	syncEchoStr("Exti13");
	/* ユーザーLEDを反転出力する */
	LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);

	/* タイマーを再開する */
	startTimer(&sts_Timer1s);
}

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	/* タイマーを開始する */
	startTimer(&sts_Timer1s);
}

/**
  * @brief  周期処理関数
  * @param  None
  * @retval None
  */
void loop(void)
{
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

