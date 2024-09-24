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
#define ADC_CHANEL_NUM		(3)						/* ADCチェネル数			*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* プログラム開始メッセージ */
const uint8_t OpeningMsg[] = "Start UART sample!!\r\n";

static Timer sts_Timer1s;									/* 1秒タイマー				*/
volatile static uint8_t u8s_TxBuffer[UART_TXBF_SIZE];		/* UART送信バッファ			*/
volatile static uint8_t u8s_RxBuffer[UART_BUFF_SIZE];		/* UART受信バッファ			*/
volatile static uint16_t u16s_AdcData[ADC_CHANEL_NUM];		/* ADC変換データ			*/

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
		mem_cpy08((uint8_t *)&u8s_TxBuffer[0], (const uint8_t *)&u8s_RxBuffer[0], UART_BUFF_SIZE);
		/* UART送信(割り込み)を開始 */
		if (HAL_UART_Transmit_IT(&huart2, (const uint8_t *)&u8s_TxBuffer[0], UART_TXBF_SIZE) != HAL_OK) {
			/* Transmission Error */
			Error_Handler();
		}
		/* UART受信(割り込み)を開始 */
		if (HAL_UART_Receive_IT(&huart2, (uint8_t *)&u8s_RxBuffer[0], UART_BUFF_SIZE) != HAL_OK) {
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
  * @brief Conversion complete callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	__NOP();
}

/**
  * @brief Conversion DMA half-transfer callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
	__NOP();
}

/**
  * @brief  ADC error callback in non-blocking mode
  * @param hadc ADC handle
  * @retval None
  */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
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
	mem_set08((uint8_t *)&u8s_TxBuffer[0], 0x00, UART_TXBF_SIZE);
	mem_set08((uint8_t *)&u8s_RxBuffer[0], 0x00, UART_BUFF_SIZE);
	mem_set16((uint16_t *)&u16s_AdcData[0], 0x00, ADC_CHANEL_NUM);

	/* UART送信バッファに改行コードをセット */
	u8s_TxBuffer[UART_TXBF_SIZE - 2] = '\r';		/* CRコード					*/
	u8s_TxBuffer[UART_TXBF_SIZE - 1] = '\n';		/* LFコード					*/

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* UART送信(割り込み)を開始 */
	if (HAL_UART_Transmit_IT(&huart2, &OpeningMsg[0], sizeof(OpeningMsg)) != HAL_OK) {
		/* Transmission Error */
		Error_Handler();
	}

	/* UART受信(割り込み)を開始 */
	if (HAL_UART_Receive_IT(&huart2, (uint8_t *)&u8s_RxBuffer[0], UART_BUFF_SIZE) != HAL_OK) {
		/* Reception Error */
		Error_Handler();
	}

	/* ADCキャリブレーション開始 */
	if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
		/* Calibration Error */
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

		/* ADC(DMA)を開始 */
		if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&u16s_AdcData[0], ADC_CHANEL_NUM) != HAL_OK) {
			/* ADC conversion start Error */
			Error_Handler();
		}

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

