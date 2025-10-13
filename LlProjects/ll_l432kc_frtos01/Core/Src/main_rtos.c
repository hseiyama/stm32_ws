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
#include "timers.h"
#include "semphr.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
QueueHandle_t xLedQueue;
QueueHandle_t xUartQueue;
SemaphoreHandle_t xBinarySemaphore;

/* Private function prototypes -----------------------------------------------*/
static void prvAutoReloadTimerCallback(TimerHandle_t xTimer);

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
	uint8_t ucValueToSend = 0;
	TickType_t xLastWakeTime;

	/* Tickカウントを取得する */
	xLastWakeTime = xTaskGetTickCount();
	/* タスク処理は無限ループ */
	for (;;) {
		/* LED出力値を反転する */
		ucValueToSend = !ucValueToSend;
		/* LED出力値をキューに送信する */
		xQueueSendToBack(xLedQueue, &ucValueToSend, 0);

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
	uint8_t ucValueToSend = '.';
	TickType_t xLastWakeTime;
	TimerHandle_t xAutoReloadTimer;

	/* Tickカウントを取得する */
	xLastWakeTime = xTaskGetTickCount();
	/* 自動リロードタイマーを生成する */
	xAutoReloadTimer = xTimerCreate("AutoReload", 2000, pdTRUE, 0,
		prvAutoReloadTimerCallback);
	if (xAutoReloadTimer != NULL) {
		/* 自動リロードタイマーを開始する */
		xTimerStart(xAutoReloadTimer, 0);
	}
	/* タスク処理は無限ループ */
	for (;;) {
		/* UART出力値をキューに送信する */
		xQueueSendToBack(xUartQueue, &ucValueToSend, 0);

		/* 時間待ち(1000ms) 1000tick */
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
	uint8_t ucReceivedValue;
	BaseType_t xStatus;

	/* タスク処理は無限ループ */
	for (;;) {
		/* LED出力値をキューから受信する */
		xStatus = xQueueReceive(xLedQueue, &ucReceivedValue, 5000);
		if (xStatus == pdPASS) {
			if (ucReceivedValue != 0) {
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
  * @brief  UART出力タスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
void vUartOutTask(void *pvParameters)
{
	uint8_t ucReceivedValue;
	BaseType_t xStatus;

	/* タスク処理は無限ループ */
	for (;;) {
		/* LED出力値をキューから受信する */
		xStatus = xQueueReceive(xUartQueue, &ucReceivedValue, 5000);
		if (xStatus == pdPASS) {
			/* UART送信レジスタが空の場合 */
			if (LL_USART_IsActiveFlag_TXE(USART2) != 0) {
				/* UARTデータを送信する */
				LL_USART_TransmitData8(USART2, ucReceivedValue);
			}
		}
	}
}

/**
  * @brief  外部入力割り込みハンドラー
  * @param  None
  * @retval None
  */
void vExtiIrqHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* バイナリーセマフォを提供する */
	xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);

	/* コンテキストの切替えを要求する */
	xHigherPriorityTaskWoken = pdTRUE;
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
  * @brief  外部入力ハンドラータスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
void vExtiHandlerTask(void *pvParameters)
{
	uint8_t ucValueToSend = 'i';

	/* タスク処理は無限ループ */
	for (;;) {
		/* バイナリーセマフォを取得する */
		xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);

		/* UART出力値をキューに送信する */
		xQueueSendToBack(xUartQueue, &ucValueToSend, 0);
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
	xLedQueue = xQueueCreate(5, sizeof(uint8_t));
	xUartQueue = xQueueCreate(5, sizeof(uint8_t));
	/* バイナリーセマフォを生成する */
	xBinarySemaphore = xSemaphoreCreateBinary();

	/* タスクを生成する */
	xTaskCreate(vLedCtrlTask, "LedCtrl", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(vUartCtrlTask, "UartCtrl", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(vLedOutTask, "LedOut", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(vUartOutTask, "UartOut", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(vExtiHandlerTask, "ExtiHandler", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 4, NULL);

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

/**
  * @brief  自動リロードタイマーコールバック関数
  * @param  xTimer: タイマーのハンドル
  * @retval None
  */
static void prvAutoReloadTimerCallback(TimerHandle_t xTimer)
{
	uint8_t ucValueToSend = 't';

	/* UART出力値をキューに送信する */
	xQueueSendToBack(xUartQueue, &ucValueToSend, 0);
}

