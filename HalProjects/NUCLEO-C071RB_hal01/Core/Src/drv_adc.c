/**
  ******************************************************************************
  * @file           : drv_adc.c
  * @brief          : ADCドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile static uint16_t u16s_AdcBuffer[ADC_CHANNEL_MAX];		/* ADCバッファ					*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief Conversion complete callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	/* 処理なし */
}

/**
  * @brief Conversion DMA half-transfer callback in non-blocking mode.
  * @param hadc ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
	/* 処理なし */
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
  * @brief ADCドライバー初期化処理
  * @param None
  * @retval None
  */
void taskAdcDriverInit(void)
{
	mem_set16((uint16_t *)&u16s_AdcBuffer[0], ADC_FAILURE_VALUE, ADC_CHANNEL_MAX);

	/* ADCキャリブレーション開始 */
	if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
		/* Calibration Error */
		Error_Handler();
	}
}

/**
  * @brief ADCドライバー入力処理
  * @param None
  * @retval None
  */
void taskAdcDriverInput(void)
{
	mem_set16((uint16_t *)&u16s_AdcBuffer[0], ADC_FAILURE_VALUE, ADC_CHANNEL_MAX);

	/* ADC(DMA)を開始 */
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&u16s_AdcBuffer[0], ADC_CHANNEL_MAX) != HAL_OK) {
		/* ADC conversion start Error */
		Error_Handler();
	}
	/* ADC(DMA)の終了を待つ(最小1msを確保) */
	if (HAL_ADC_PollForConversion(&hadc1, 2) != HAL_OK) {
		/* ADC conversion polling Error */
		Error_Handler();
	}
}

/**
  * @brief ADCドライバー出力処理
  * @param None
  * @retval None
  */
void taskAdcDriverOutput(void)
{
	/* 処理なし */
}

/**
  * @brief ADCデータを取得する
  * @param u16_Channel ADCのチャネル
  * @retval 取得した値
  */
uint16_t adcGetData(uint16_t u16_Channel)
{
	/* ADCのチャネルが指定チャネルでない場合 */
	if (u16_Channel >= ADC_CHANNEL_MAX) {
		return ADC_FAILURE_VALUE;
	}

	return u16s_AdcBuffer[u16_Channel];
}

/* Private functions ---------------------------------------------------------*/

