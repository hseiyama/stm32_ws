/**
  ******************************************************************************
  * @file           : main_rtos.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  LED制御タスク
  * @param  pvParam: パラメータのポインタ
  * @retval None
  */
void vLedCtrlTask(void *pvParam)
{
	/* タスク処理は無限ループ */
	for (;;) {
 		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);

		/* 時間待ち(500ms) 500tick */
		vTaskDelay(500);
	}
}

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	/* タスクを生成する */
	xTaskCreate(vLedCtrlTask, "LedCtrl", configMINIMAL_STACK_SIZE,
		(void *)NULL, tskIDLE_PRIORITY, NULL);

	/* スケジュールを開始する */
	vTaskStartScheduler();
}

/**
  * @brief  周期処理関数
  * @param  None
  * @retval None
  */
void loop(void)
{
}

/* Private functions ---------------------------------------------------------*/

