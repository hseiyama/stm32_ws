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
#define MESSAGE_SIZE		(8)						/* メッセージサイズ		*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;									/* 1秒タイマー				*/
static uint8_t u8s_TxBuffer[UART_BUFF_SIZE];				/* UART送信バッファ			*/
static uint8_t u8s_RxBuffer[UART_BUFF_SIZE];				/* UART受信バッファ			*/
static uint16_t u16s_AdcData[ADC_CHANNEL_MAX];				/* ADCデータ(全チャネル)	*/
static uint8_t u8s_AdcDispState;							/* ADC表示状態				*/
static uint8_t u8s_FlashDataBuffer[FLASH_DATA_SIZE];		/* FLASHデータバッファ		*/
static uint16_t u16s_PwmDutyValue;							/* PWMデューティ値			*/
static uint16_t u16s_PwmDutyValue_prev;						/* PWMデューティ値(前回値)	*/
static uint8_t u8s_MessageDate[MESSAGE_SIZE + 3];			/* メッセージデータ			*/
static uint16_t u16s_MessageCount;							/* メッセージカウント		*/
static uint8_t u8s_MessageUpdate;							/* メッセージ更新フラグ		*/

/* Private function prototypes -----------------------------------------------*/
static void getFlashData(void);								/* FLASHデータを取得する				*/
static void setFlashData(void);								/* FLASHデータを更新する				*/

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
	mem_set08(&u8s_FlashDataBuffer[0], 0x00, FLASH_DATA_SIZE);
	mem_set08(&u8s_MessageDate[0], 0x00, sizeof(u8s_MessageDate));
	u8s_AdcDispState = ON;
	u16s_PwmDutyValue = 0;
	u16s_PwmDutyValue_prev = 0;
	u16s_MessageCount = 0;
	u8s_MessageUpdate = OFF;

	/* UART送信バッファに改行コードをセット */
	u8s_TxBuffer[UART_RX_BLOCK_SIZE] = '\r';			/* CRコード					*/
	u8s_TxBuffer[UART_RX_BLOCK_SIZE + 1] = '\n';		/* LFコード					*/
	/* メッセージデータに改行コードをセット */
	u8s_MessageDate[MESSAGE_SIZE] = '\r';				/* CRコード					*/
	u8s_MessageDate[MESSAGE_SIZE + 1] = '\n';			/* LFコード					*/

	/* FLASHデータを取得する */
	getFlashData();

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);

	/* プログラム開始メッセージを表示する */
	uartEchoStr("Start ADC/UART sample!!\r\n");
	/* メッセージデータを表示する */
	uartEchoStr((char *)&u8s_MessageDate[0]);
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

		/* メッセージ更新フラグがONの場合 */
		if (u8s_MessageUpdate == ON) {
			if (u16s_MessageCount < MESSAGE_SIZE) {
				mem_cpy08(&u8s_MessageDate[u16s_MessageCount], &u8s_RxBuffer[0], u16_RxSize);
				u16s_MessageCount += u16_RxSize;
			}
			if (u16s_MessageCount >= MESSAGE_SIZE) {
				u16s_MessageCount = 0;
				u8s_MessageUpdate = OFF;
			}
		}
		/* ADC表示状態の切替え(OFF) */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"adc0", UART_RX_BLOCK_SIZE) == 0) {
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
		/* PWMのDuty値を更新する */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"pwm", 3) == 0) {
			if ((u8s_RxBuffer[3] >= '0') && (u8s_RxBuffer[3] <= '9')) {
				u16s_PwmDutyValue = (u8s_RxBuffer[3] - '0') * 100;
			}
			else if (u8s_RxBuffer[3] == 'a') {
				u16s_PwmDutyValue = PWM_PERIOD;
			}
		}
		/* メッセージデータを表示する */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"msgr", UART_RX_BLOCK_SIZE) == 0) {
			uartEchoStr((char *)&u8s_MessageDate[0]);
		}
		/* メッセージデータを更新する */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"msgw", UART_RX_BLOCK_SIZE) == 0) {
			u16s_MessageCount = 0;
			u8s_MessageUpdate = ON;
		}
		/* FLASHデータを取得する */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"nvmr", UART_RX_BLOCK_SIZE) == 0) {
			getFlashData();
		}
		/* FLASHデータを更新する */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"nvmw", UART_RX_BLOCK_SIZE) == 0) {
			setFlashData();
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

	/* PWMデューティ値が変化した場合 */
	if (u16s_PwmDutyValue != u16s_PwmDutyValue_prev) {
		pwmSetDuty(PWM_CHANNEL_TIM3_CH1, u16s_PwmDutyValue);
	}

	/* 前回値を更新 */
	u16s_PwmDutyValue_prev = u16s_PwmDutyValue;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief FLASHデータを取得する
  * @param None
  * @retval None
  */
static void getFlashData(void)
{
	/* FLASHデータを読み出す */
	flashReadData();
	/* FLASHデータを取得する */
	flashGetData(&u8s_FlashDataBuffer[0], FLASH_DATA_SIZE);

	/* PWMデューティ値 */
	u16s_PwmDutyValue = *(uint16_t *)&u8s_FlashDataBuffer[0];
	/* メッセージデータ */
	mem_cpy08(&u8s_MessageDate[0], &u8s_FlashDataBuffer[2], MESSAGE_SIZE);
}

/**
  * @brief FLASHデータを更新する
  * @param None
  * @retval None
  */
static void setFlashData(void)
{
	uint16_t *pu16_Data;

	/* PWMデューティ値 */
	pu16_Data = (uint16_t *)&u8s_FlashDataBuffer[0];
	*pu16_Data = u16s_PwmDutyValue;
	/* メッセージデータ */
	mem_cpy08(&u8s_FlashDataBuffer[2], &u8s_MessageDate[0], MESSAGE_SIZE);

	/* FLASHデータを更新する */
	flashSetData(&u8s_FlashDataBuffer[0], FLASH_DATA_SIZE);
	/* FLASHデータの書き込みを要求する */
	flashWriteDataRequest();
}
