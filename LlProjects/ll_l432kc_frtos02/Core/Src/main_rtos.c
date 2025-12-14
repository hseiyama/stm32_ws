/**
  ******************************************************************************
  * @file           : main_rtos.c
  * @brief          : MAINリアルタイムOS層
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static TaskHandle_t xHandlerTask;						/* タスク通知用のハンドル	*/

/* Private function prototypes -----------------------------------------------*/
static void prvHighCycleTask(void *pvParameters);		/* High周期タスク			*/
static void prvMiddleCycleTask(void *pvParameters);		/* Middle周期タスク			*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  MallocFailedフック関数
  * @param  None
  * @retval None
  */
void vApplicationMallocFailedHook(void)
{
	/* エラー処理関数 */
	Error_Handler();
}

/**
  * @brief  StackOverflowフック関数
  * @param  xTask: タスクのハンドル
            pcTaskName: タスク名の文字列
  * @retval None
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	/* エラー処理関数 */
	Error_Handler();
}

/**
  * @brief  初期化関数(RTOS)
  * @param  None
  * @retval None
  */
void vRtosSetup(void)
{
	/* タイマー初期化処理 */
	taskTimerInit();
	/* UARTドライバー初期化処理 */
	taskUartDriverInit();
	/* 初期化関数 */
	setup();

	/* SysTickタイマー開始(1msタイマー割り込み用) */
	LL_SYSTICK_EnableIT();

	/* タスクを生成する */
	xTaskCreate(prvHighCycleTask, "HighCycle", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(prvMiddleCycleTask, "MiddleCycle", configMINIMAL_STACK_SIZE,
		NULL, tskIDLE_PRIORITY + 1, &xHandlerTask);

	/* スケジュールを開始する */
	vTaskStartScheduler();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  High周期タスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
static void prvHighCycleTask(void *pvParameters)
{
	TickType_t xLastWakeTime;
	uint32_t u32s_CycleTimeCounter = 0;

	/* Tickカウントを取得する */
	xLastWakeTime = xTaskGetTickCount();
	/* タスク処理は無限ループ */
	for (;;) {
		u32s_CycleTimeCounter++;
		/* 周期時間カウンターがシステムの周期時間[ms]に達した場合 */
		if (u32s_CycleTimeCounter >= SYS_CYCLE_TIME) {
			u32s_CycleTimeCounter = 0;
			/* タスク通知を送信する */
			xTaskNotifyGive(xHandlerTask);
		}
		/* 時間待ち(1ms) 1tick */
		vTaskDelayUntil(&xLastWakeTime, 1);
	}
}

/**
  * @brief  Middle周期タスク
  * @param  pvParameters: パラメータのポインタ
  * @retval None
  */
static void prvMiddleCycleTask(void *pvParameters)
{
	uint32_t ulEventsToProcess;

	/* タスク処理は無限ループ */
	for (;;) {
		/* タスク通知の受信を待つ */
		ulEventsToProcess = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(ulEventsToProcess != 0) {
			/* タイマー更新処理 */
			taskTimerUpdate();
			/* UARTドライバー入力処理 */
			taskUartDriverInput();
			/* 周期処理関数 */
			loop();
			/* UARTドライバー出力処理 */
			taskUartDriverOutput();
		}
	}
}

