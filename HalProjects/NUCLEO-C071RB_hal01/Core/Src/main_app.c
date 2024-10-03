/**
  ******************************************************************************
  * @file           : main_app.c
  * @brief          : MAINアプリケーション
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TIME_1S				(1000)					/* 1秒判定時間[ms]			*/
#define UART_BUFF_SIZE		(8)						/* UARTバッファサイズ		*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;									/* 1秒タイマー				*/
static uint8_t u8s_TxBuffer[UART_BUFF_SIZE];				/* UART送信バッファ			*/
static uint8_t u8s_RxBuffer[UART_BUFF_SIZE];				/* UART受信バッファ			*/
static uint16_t u16s_AdcData[ADC_CHANNEL_MAX];				/* ADCデータ(全チャネル)	*/
static uint8_t u8s_AdcDispState;							/* ADC表示状態				*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief 初期化関数
  * @param None
  * @retval None
  */
void setup(void)
{
	mem_set08(&u8s_TxBuffer[0], 0x00, UART_BUFF_SIZE);
	mem_set08(&u8s_RxBuffer[0], 0x00, UART_BUFF_SIZE);
	mem_set16(&u16s_AdcData[0], ADC_FAILURE_VALUE, ADC_CHANNEL_MAX);
	u8s_AdcDispState = ON;

	/* UART送信バッファに改行コードをセット */
	u8s_TxBuffer[UART_RX_BLOCK_SIZE] = '\r';			/* CRコード					*/
	u8s_TxBuffer[UART_RX_BLOCK_SIZE + 1] = '\n';		/* LFコード					*/

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* プログラム開始メッセージを表示する */
	uartEchoStr("Start ADC/UART sample!!\r\n");
}

/**
  * @brief 周期処理関数
  * @param None
  * @retval None
  */
void loop(void)
{
	uint16_t u16_Index;
	uint16_t u16_RxSize;

	/* UART受信データの数を取得する */
	if (uartGetRxCount() >= UART_RX_BLOCK_SIZE) {
		/* UART受信データを取得する */
		u16_RxSize = uartGetRxData(&u8s_RxBuffer[0], UART_RX_BLOCK_SIZE);
		/* 受信データを送信データ(+CR/LF)にセットする */
		mem_cpy08(&u8s_TxBuffer[0], &u8s_RxBuffer[0], u16_RxSize);
		/* UART送信データを登録する */
		uartSetTxData(&u8s_TxBuffer[0], u16_RxSize + 2);

		/* ADC表示状態の切替え(OFF) */
		if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"adc0", UART_RX_BLOCK_SIZE) == 0) {
			u8s_AdcDispState = OFF;
		}
		/* ADC表示状態の切替え(ON) */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"adc1", UART_RX_BLOCK_SIZE) == 0) {
			u8s_AdcDispState = ON;
		}
		/* システムリセットの要求 */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"rst0", UART_RX_BLOCK_SIZE) == 0) {
			NVIC_SystemReset();
		}
		/* PWMのDuty値を設定する */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"pwm", 3) == 0) {
			if ((u8s_RxBuffer[3] >= '0') && (u8s_RxBuffer[3] <= '9')) {
				pwmSetDuty(PWM_CHANNEL_TIM3_CH1, (u8s_RxBuffer[3] - '0') * 100);
			}
			else if (u8s_RxBuffer[3] == 'a') {
				pwmSetDuty(PWM_CHANNEL_TIM3_CH1, PWM_PERIOD);
			}
		}
	}

	for (u16_Index = 0; u16_Index < ADC_CHANNEL_MAX; u16_Index++) {
		/* ADCデータを取得する */
		u16s_AdcData[u16_Index] = adcGetData(u16_Index);
	}

	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* LEDを反転出力する */
		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

		/* ADC表示状態がONの場合 */
		if (u8s_AdcDispState == ON) {
			/* ADCデータを表示する */
			uartEchoStr("ADC =");
			for (u16_Index = 0; u16_Index < ADC_CHANNEL_MAX; u16_Index++) {
				uartEchoStr(" ");
				uartEchoHex16(u16s_AdcData[u16_Index]);
			}
			uartEchoStr("\r\n");
		}

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}

	/* ユーザーSWが押下された場合 */
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		/* LED_GREENをHIGH出力する */
		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	}
}

/* Private functions ---------------------------------------------------------*/

