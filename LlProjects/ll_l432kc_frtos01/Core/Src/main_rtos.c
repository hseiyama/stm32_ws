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
  * @brief  MallocFailedフック関数
  * @param  None
  * @retval None
  */
void vApplicationMallocFailedHook(void)
{
	for (;;) {
		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		/* 100ms待つ */
		LL_mDelay(100);
	}
}

/**
  * @brief  StackOverflowフック関数
  * @param  xTask: タスクのハンドル
            pcTaskName: タスク名の文字列
  * @retval None
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	for (;;) {
		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		/* 100ms待つ */
		LL_mDelay(100);
	}
}

/**
  * @brief  LED制御タスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
void vLedCtrlTask(void *pvParameters)
{
	TickType_t xLastWakeTime;

	/* Tickカウントを取得する */
	xLastWakeTime = xTaskGetTickCount();
	/* タスク処理は無限ループ */
	for (;;) {
 		/* ユーザーLEDを反転出力する */
		LL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);

		/* 時間待ち(500ms) 500tick */
		vTaskDelayUntil(&xLastWakeTime, 500);
	}
}

/**
  * @brief  UART制御タスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
void vUartCtrlTask(void *pvParameters)
{
	TickType_t xLastWakeTime;

	/* Tickカウントを取得する */
	xLastWakeTime = xTaskGetTickCount();
	/* タスク処理は無限ループ */
	for (;;) {
 		/* UARTデータを送信する */
		LL_USART_TransmitData8(USART2, '.');

		/* 時間待ち(1000ms) 500tick */
		vTaskDelayUntil(&xLastWakeTime, 1000);
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
		NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(vUartCtrlTask, "UartCtrl", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY, NULL);

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

