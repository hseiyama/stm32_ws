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
#define TIME_SLEEP_WAIT		(1000)					/* スリープ待ち時間[ms]		*/
#define UART_BUFF_SIZE		(8)						/* UARTバッファサイズ		*/
#define MESSAGE_SIZE		(8)						/* メッセージサイズ		*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Timer sts_Timer1s;									/* 1秒タイマー				*/
static Timer sts_TimerSleepWait;							/* スリープ待ちタイマー		*/
static uint8_t u8s_TxBuffer[UART_BUFF_SIZE];				/* UART送信バッファ			*/
static uint8_t u8s_RxBuffer[UART_BUFF_SIZE];				/* UART受信バッファ			*/
static uint16_t u16s_AdcData[ADC_CHANNEL_MAX];				/* ADCデータ(全チャネル)	*/
static uint8_t u8s_DisplayState;							/* 表示状態					*/
static uint8_t u8s_FlashDataBuffer[FLASH_DATA_SIZE];		/* FLASHデータバッファ		*/
static uint16_t u16s_PwmDutyValue;							/* PWMデューティ値			*/
static uint16_t u16s_PwmDutyValue_prev;						/* PWMデューティ値(前回値)	*/
static uint8_t u8s_MessageData[MESSAGE_SIZE + 1];			/* メッセージデータ			*/
static uint16_t u16s_MessageCount;							/* メッセージカウント		*/
static uint8_t u8s_MessageUpdate;							/* メッセージ更新フラグ		*/
volatile static uint8_t u8s_Exti0Event;						/* 外部割込み0発生フラグ	*/

/* Private function prototypes -----------------------------------------------*/
static void getFlashData(void);								/* FLASHデータを取得する				*/
static void setFlashData(void);								/* FLASHデータを更新する				*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_0) {
		u8s_Exti0Event = ON;
	}
}

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
	mem_set08(&u8s_MessageData[0], 0x00, sizeof(u8s_MessageData));
	u8s_DisplayState = ON;
	u16s_PwmDutyValue = 0;
	u16s_PwmDutyValue_prev = 0;
	u16s_MessageCount = 0;
	u8s_MessageUpdate = OFF;
	u8s_Exti0Event = OFF;

	/* UART送信バッファに改行コードをセット */
	u8s_TxBuffer[UART_RX_BLOCK_SIZE] = '\r';			/* CRコード					*/
	u8s_TxBuffer[UART_RX_BLOCK_SIZE + 1] = '\n';		/* LFコード					*/

	/* FLASHデータを取得する */
	getFlashData();

	/* タイマーを開始する */
	startTimer(&sts_Timer1s);
	/* タイマーを停止する */
	stopTimer(&sts_TimerSleepWait);

	/* プログラム開始メッセージを表示する */
	uartEchoStrln("Start ADC/UART sample!!");
	/* メッセージデータを表示する */
	uartEchoStrln((char *)&u8s_MessageData[0]);
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
	uint8_t u8_I2cData;
	uint8_t u8_SpiData;

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
				mem_cpy08(&u8s_MessageData[u16s_MessageCount], &u8s_RxBuffer[0], u16_RxSize);
				u16s_MessageCount += u16_RxSize;
			}
			if (u16s_MessageCount >= MESSAGE_SIZE) {
				u16s_MessageCount = 0;
				u8s_MessageUpdate = OFF;
			}
		}
		/* 表示状態の切替え(OFF) */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"dsp0", UART_RX_BLOCK_SIZE) == 0) {
			u8s_DisplayState = OFF;
		}
		/* 表示状態の切替え(ON) */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"dsp1", UART_RX_BLOCK_SIZE) == 0) {
			u8s_DisplayState = ON;
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
			uartEchoStrln((char *)&u8s_MessageData[0]);
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
		/* スリープ移行の要求 */
		else if (mem_cmp08(&u8s_RxBuffer[0], (uint8_t *)"slp0", UART_RX_BLOCK_SIZE) == 0) {
			uartEchoStrln("GotoSleep!");
			/* タイマーを停止する */
			stopTimer(&sts_Timer1s);
			/* I2C通信を無効化 */
			i2cComDisable();
			/* SPI通信を無効化 */
			spiComDisable();
			/* タイマーを開始する */
			startTimer(&sts_TimerSleepWait);
		}
	}

	for (u16_Index = 0; u16_Index < ADC_CHANNEL_MAX; u16_Index++) {
		/* ADCデータを取得する */
		u16s_AdcData[u16_Index] = adcGetData(u16_Index);
	}

	/* I2Cデータを取得する */
	if (i2cGetData(I2C_REGISTER_PORTA, &u8_I2cData) == OK) {
		/* SPIデータを登録する */
		spiSetData(SPI_REGISTER_PORTB, u8_I2cData);
	}

	/* SPIデータを取得する */
	if (spiGetData(SPI_REGISTER_PORTA, &u8_SpiData) == OK) {
		/* I2Cデータを登録する */
		i2cSetData(I2C_REGISTER_PORTB, u8_SpiData);
	}

	/* 1秒判定時間が満了した場合 */
	if (checkTimer(&sts_Timer1s, TIME_1S)) {
		/* LEDを反転出力する */
//		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

		/* 表示状態がONの場合 */
		if (u8s_DisplayState == ON) {
			/* ADCデータを表示する */
			uartEchoStr("ADC =");
			for (u16_Index = 0; u16_Index < ADC_CHANNEL_MAX; u16_Index++) {
				uartEchoStr(" ");
				uartEchoHex16(u16s_AdcData[u16_Index]);
			}
			/* I2Cデータを表示する */
			uartEchoStr(", I2C = ");
			uartEchoHex8(u8_I2cData);
			/* SPIデータを表示する */
			uartEchoStr(", SPI = ");
			uartEchoHex8(u8_SpiData);
			uartEchoStrln("");
		}

		/* タイマーを再開する */
		startTimer(&sts_Timer1s);
	}

	/* ユーザーSWが押下された場合 */
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
		/* LED_GREENをHIGH出力する */
//		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	}

	/* PWMデューティ値が変化した場合 */
	if (u16s_PwmDutyValue != u16s_PwmDutyValue_prev) {
		pwmSetDuty(PWM_CHANNEL_TIM3_CH1, u16s_PwmDutyValue);
	}

	if (u8s_Exti0Event == ON) {
		uartEchoStrln("Exti0Falling!");
		u8s_Exti0Event = OFF;
	}

	/* 前回値を更新 */
	u16s_PwmDutyValue_prev = u16s_PwmDutyValue;

	/* スリープ待ち時間が満了した場合 */
	if (checkTimer(&sts_TimerSleepWait, TIME_SLEEP_WAIT)) {
		/* タイマーを停止する */
		stopTimer(&sts_TimerSleepWait);

		/* スリープへ移行する */
		sleep();

		uartEchoStrln("Wakeup!");
		/* タイマーを開始する */
		startTimer(&sts_Timer1s);
		/* I2C通信を有効化 */
		i2cComEnable();
		/* SPI通信を有効化 */
		spiComEnable();
	}
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
	mem_cpy08(&u8s_MessageData[0], &u8s_FlashDataBuffer[2], MESSAGE_SIZE);
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
	mem_cpy08(&u8s_FlashDataBuffer[2], &u8s_MessageData[0], MESSAGE_SIZE);

	/* FLASHデータを更新する */
	flashSetData(&u8s_FlashDataBuffer[0], FLASH_DATA_SIZE);
	/* FLASHデータの書き込みを要求する */
	flashWriteDataRequest();
}
