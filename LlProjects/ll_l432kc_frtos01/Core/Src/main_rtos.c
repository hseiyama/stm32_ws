/**
  ******************************************************************************
  * @file           : main_rtos.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
QueueHandle_t xQueue;

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
	uint32_t ulValueToSend;
	BaseType_t xStatus;
	TickType_t xLastWakeTime;

	/* LED出力値を初期化する */
	ulValueToSend = 0;

	/* Tickカウントを取得する */
	xLastWakeTime = xTaskGetTickCount();
	/* タスク処理は無限ループ */
	for (;;) {
		/* 時間待ち(500ms) 500tick */
		vTaskDelayUntil(&xLastWakeTime, 500);

		/* LED出力値を反転する */
		ulValueToSend = !ulValueToSend;
		/* LED出力値をキューに送信する */
		xStatus = xQueueSendToBack(xQueue, &ulValueToSend, 0);
		if (xStatus != pdPASS) {
			/* 時間待ち(1000ms) 1000tick */
			vTaskDelayUntil(&xLastWakeTime, 1000);
		}
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
  * @brief  LED出力タスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
void vLedOutTask(void *pvParameters)
{
	uint32_t ulReceivedValue;
	BaseType_t xStatus;

	/* タスク処理は無限ループ */
	for (;;) {
		/* LED出力値をキューから受信する */
		xStatus = xQueueReceive(xQueue, &ulReceivedValue, 5000);
		if (xStatus == pdPASS) {
			if (ulReceivedValue != 0) {
				/* ユーザーLEDをON出力する */
				LL_GPIO_SetOutputPin(LD3_GPIO_Port, LD3_Pin);
			}
			else {
				/* ユーザーLEDをOFF出力する */
				LL_GPIO_ResetOutputPin(LD3_GPIO_Port, LD3_Pin);
			}
		}
	}
}

/**
  * @brief  初期化関数
  * @param  None
  * @retval None
  */
void setup(void)
{
	/* キューを生成する */
	xQueue = xQueueCreate(5, sizeof(uint32_t));

	/* タスクを生成する */
	xTaskCreate(vLedCtrlTask, "LedCtrl", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(vUartCtrlTask, "UartCtrl", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(vLedOutTask, "LedOut", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 2, NULL);

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

