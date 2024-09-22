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
#define TIME_1S				(1000)					/* 1秒判定時間[ms]			*/
#define UART_BUFF_SIZE		(8)						/* UARTバッファサイズ		*/
#define UART_TXBF_SIZE		(UART_BUFF_SIZE + 2)	/* UART送信用サイズ(+CRLF)	*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* プログラム開始メッセージ */
const uint8_t OpeningMsg[] = "Start UART sample!!\r\n";

static Timer sts_Timer1s;							/* 1秒タイマー				*/
static uint8_t u8s_TxBuffer[UART_TXBF_SIZE];		/* UART送信バッファ			*/
static uint8_t u8s_RxBuffer[UART_BUFF_SIZE];		/* UART受信バッファ			*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief Rx Transfer completed callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		mem_cpy08(&u8s_TxBuffer[0], &u8s_RxBuffer[0], UART_BUFF_SIZE);
		/* UART送信(DMA)を開始 */
		if (HAL_UART_Transmit_DMA(&huart2, &u8s_TxBuffer[0], UART_TXBF_SIZE) != HAL_OK) {
			/* Transmission Error */
			Error_Handler();
		}
		/* UART受信(DMA)を開始 */
		if (HAL_UART_Receive_DMA(&huart2, &u8s_RxBuffer[0], UART_BUFF_SIZE) != HAL_OK) {
			/* Reception Error */
			Error_Handler();
		}
	}
}

/**
  * @brief Tx Transfer completed callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	/* LED_GREENを反転出力する */
	HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
}

/**
  * @brief UART error callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	/* Error */
	Error_Handler();
}

/**
  * @brief 初期化関数
  * @param None
  * @retval None
  */
void setup(void)
{
	mem_set08(&u8s_TxBuffer[0], 0x00, UART_TXBF_SIZE);
	mem_set08(&u8s_RxBuffer[0], 0x00, UART_BUFF_SIZE);
	/* UART送信バッファに改行コードをセット */
	u8s_TxBuffer[UART_TXBF_SIZE - 2] = '\r';		/* CRコード					*/
	u8s_TxBuffer[UART_TXBF_SIZE - 1] = '\n';		/* LFコード					*/

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* UART送信(DMA)を開始 */
	if (HAL_UART_Transmit_DMA(&huart2, &OpeningMsg[0], sizeof(OpeningMsg)) != HAL_OK) {
		/* Transmission Error */
		Error_Handler();
	}

	/* UART受信(DMA)を開始 */
	if (HAL_UART_Receive_DMA(&huart2, &u8s_RxBuffer[0], UART_BUFF_SIZE) != HAL_OK) {
		/* Reception Error */
		Error_Handler();
	}
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
		/* LED_GREENを反転出力する */
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}

	/* ユーザーSWが押下された場合 */
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		/* LED_GREENをHIGH出力する */
		HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
	}
}

/* Private functions ---------------------------------------------------------*/

