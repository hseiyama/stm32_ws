/**
  ******************************************************************************
  * @file           : drv_pwm.c
  * @brief          : PWMドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"

/* Private typedef -----------------------------------------------------------*/

/* PWM制御情報 */
typedef struct _PwmControl {
	TIM_HandleTypeDef *pst_handle;	/* TIMのハンドル(ポインタ)				*/
	uint32_t u32_channel;			/* TIMのチャネル						*/
} PwmControl;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* PWM制御テーブル */
static const PwmControl PwmControlTable[PWM_CHANNEL_MAX] = {
	/*	pst_handle,	u32_channel		*/
	{	&htim3,		TIM_CHANNEL_1	}		/* PWM_CHANNEL_TIM3_CH1			*/
};

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief PWMドライバー初期化処理
  * @param None
  * @retval None
  */
void taskPwmDriverInit(void)
{
	TIM_HandleTypeDef *pst_TimHandle;
	uint32_t u32_TimChannel;
	uint16_t u16_Index;

	for (u16_Index = 0; u16_Index < PWM_CHANNEL_MAX; u16_Index++) {
		pst_TimHandle = PwmControlTable[u16_Index].pst_handle;
		u32_TimChannel = PwmControlTable[u16_Index].u32_channel;
		/* PWM出力を開始 */
		if (HAL_TIM_PWM_Start(pst_TimHandle, u32_TimChannel) != HAL_OK) {
			/* PWM generation start Error */
			Error_Handler();
		}
	}
}

/**
  * @brief PWMドライバー入力処理
  * @param None
  * @retval None
  */
void taskPwmDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief PWMドライバー出力処理
  * @param None
  * @retval None
  */
void taskPwmDriverOutput(void)
{
	/* 処理なし */
}

/**
  * @brief PWMのDuty値を設定する
  * @param u16_Channel PWMのチャネル
  * @param u16_Duty PWMのDuty値
  * @retval OK/NG
  */
uint8_t pwmSetDuty(uint16_t u16_Channel, uint16_t u16_Duty)
{
	TIM_OC_InitTypeDef st_ConfigOC = {0};
	TIM_HandleTypeDef *pst_TimHandle;
	uint32_t u32_TimChannel;

	/* PWMのチャネルが指定チャネルでない場合 */
	if (u16_Channel >= PWM_CHANNEL_MAX) {
		return NG;
	}

	pst_TimHandle = PwmControlTable[u16_Channel].pst_handle;
	u32_TimChannel = PwmControlTable[u16_Channel].u32_channel;

	/* PWM出力を停止 */
	if (HAL_TIM_PWM_Stop(pst_TimHandle, u32_TimChannel) != HAL_OK) {
		/* PWM generation stop Error */
		Error_Handler();
	}

	/* PWMコンフィグレーションを実行 */
	st_ConfigOC.OCMode = TIM_OCMODE_PWM1;
	st_ConfigOC.Pulse = u16_Duty;
	st_ConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	st_ConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(pst_TimHandle, &st_ConfigOC, u32_TimChannel) != HAL_OK)
	{
		/* PWM configuration Error */
		Error_Handler();
	}
	/* PWM出力を開始 */
	if (HAL_TIM_PWM_Start(pst_TimHandle, u32_TimChannel) != HAL_OK) {
		/* PWM generation start Error */
		Error_Handler();
	}
	return OK;
}

/* Private functions ---------------------------------------------------------*/

