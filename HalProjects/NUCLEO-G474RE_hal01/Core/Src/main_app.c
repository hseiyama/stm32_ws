/**
  ******************************************************************************
  * @file           : main_app.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIME_1S					(1000)		/* 1秒判定時間[ms]			*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;					/* 1秒タイマー				*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief 初期化関数
  * @param None
  * @retval None
  */
void setup(void)
{
	/* タイマーを開始する */
	startTimer(&sts_Timer1s);
}

/**
  * @brief 周期処理関数
  * @param None
  * @retval None
  */
void loop(void)
{
	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* LED2を反転出力する */
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}

	/* ユーザーSWが押下された場合 */
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET) {
		/* LED2をHIGH出力する */
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	}
}

/* Private functions ---------------------------------------------------------*/

