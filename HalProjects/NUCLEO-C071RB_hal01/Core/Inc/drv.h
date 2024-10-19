/**
  ******************************************************************************
  * @file           : drv.h
  * @brief          : ドライバー共通定義
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_H
#define __DRV_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* キュー制御情報 */
typedef struct _QueueControl {
	uint16_t u16_head;				/* 先頭データのインデックス				*/
	uint16_t u16_tail;				/* 末尾データのインデックス				*/
	uint16_t u16_count;				/* データの登録数						*/
} QueueControl;

/* ADCチャネル情報 */
enum AdcChannelInfo {
	ADC_CHANNEL_ADC1_IN0 = 0,		/* ADCのチャネルADC1_IN0				*/
	ADC_CHANNEL_ADC1_IN1,			/* ADCのチャネルADC1_IN1				*/
	ADC_CHANNEL_ADC1_IN4,			/* ADCのチャネルADC1_IN4				*/
	ADC_CHANNEL_MAX					/* ADCのチャネル上限					*/
};

/* PWMチャネル情報 */
enum PwmChannelInfo {
	PWM_CHANNEL_TIM3_CH1 = 0,		/* PWMのチャネルTIM3_CH1				*/
	PWM_CHANNEL_MAX					/* PWMのチャネル上限					*/
};

/* I2Cレジスタ情報 */
enum I2cRegisterInfo {
	I2C_REGISTER_PORTA = 0,			/* I2CレジスタPORTA(入力用)				*/
	I2C_REGISTER_PORTB,				/* I2CレジスタPORTA(出力用)				*/
	I2C_REGISTER_MAX				/* I2Cレジスタの上限					*/
};

/* Exported constants --------------------------------------------------------*/
#define ADC_FAILURE_VALUE		(0xFFFF)	/* ADCフェール値				*/
#define UART_RX_BLOCK_SIZE		(4)			/* UART受信ブロックサイズ		*/
#define FLASH_BLOCK_SIZE		(64)		/* FLASHブロックサイズ			*/
#define FLASH_MARK_SIZE			(4)			/* FLASHマークサイズ			*/
#define FLASH_DATA_SIZE			(FLASH_BLOCK_SIZE - FLASH_MARK_SIZE)
											/* FLASHデータサイズ			*/

/* Exported macro ------------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim3;
extern I2C_HandleTypeDef hi2c1;

/* Exported functions prototypes ---------------------------------------------*/

/* drv_adc.c */
extern void taskAdcDriverInit(void);										/* ADCドライバー初期化処理				*/
extern void taskAdcDriverInput(void);										/* ADCドライバー入力処理				*/
extern void taskAdcDriverOutput(void);										/* ADCドライバー出力処理				*/
extern uint16_t adcGetData(uint16_t u16_Channel);							/* ADCデータを取得する					*/

/* drv_uart.c */
extern void taskUartDriverInit(void);										/* UARTドライバー初期化処理				*/
extern void taskUartDriverInput(void);										/* UARTドライバー入力処理				*/
extern void taskUartDriverOutput(void);										/* UARTドライバー出力処理				*/
extern uint16_t uartSetTxData(const uint8_t *pu8_Data, uint16_t u16_Size);	/* UART送信データを登録する				*/
extern uint16_t uartGetRxData(uint8_t *pu8_Data, uint16_t u16_Size);		/* UART受信データを取得する				*/
extern uint16_t uartGetRxCount(void);										/* UART受信データの数を取得する			*/
extern void uartEchoHex8(uint8_t u8_Data);									/* Hex1Byte表示処理						*/
extern void uartEchoHex16(uint16_t u16_Data);								/* Hex2Byte表示処理						*/
extern void uartEchoHex32(uint32_t u32_Data);								/* Hex4Byte表示処理						*/
extern void uartEchoStr(const char *ps8_Data);								/* 文字列表示処理						*/
extern void uartEchoStrln(const char *ps8_Data);							/* 文字列表示処理(改行付き)				*/

/* drv_pwm.c */
extern void taskPwmDriverInit(void);										/* PWMドライバー初期化処理				*/
extern void taskPwmDriverInput(void);										/* PWMドライバー入力処理				*/
extern void taskPwmDriverOutput(void);										/* PWMドライバー出力処理				*/
extern uint8_t pwmSetDuty(uint16_t u16_Channel, uint16_t u16_Duty);			/* PWMのDuty値を設定する				*/

/* drv_flash.c */
extern void taskFlashDriverInit(void);										/* FLASHドライバー初期化処理			*/
extern void taskFlashDriverInput(void);										/* FLASHドライバー入力処理				*/
extern void taskFlashDriverOutput(void);									/* FLASHドライバー出力処理				*/
extern uint16_t flashGetData(uint8_t *pu8_Data, uint16_t u16_Size);			/* FLASHデータを取得する				*/
extern uint16_t flashSetData(uint8_t *pu8_Data, uint16_t u16_Size);			/* FLASHデータを更新する				*/
extern void flashReadData(void);											/* FLASHデータを読み出す				*/
extern void flashWriteDataRequest(void);									/* FLASHデータの書き込みを要求する		*/
extern uint8_t flashGetReadResult(void);									/* FLASH読み出し結果を取得する			*/
extern uint8_t flashGetWriteResult(void);									/* FLASH書き込み結果を取得する			*/

/* drv_i2c.c */
extern void taskI2cDriverInit(void);										/* I2Cドライバー初期化処理				*/
extern void taskI2cDriverInput(void);										/* I2Cドライバー入力処理				*/
extern void taskI2cDriverOutput(void);										/* I2Cドライバー出力処理				*/
extern void i2cComEnable(void);												/* I2C通信を有効化						*/
extern void i2cComDisable(void);											/* I2C通信を無効化						*/
extern uint8_t i2cGetData(uint16_t u16_Register, uint8_t *pu8_Data);		/* I2Cデータを取得する					*/
extern uint8_t i2cSetData(uint16_t u16_Register, uint8_t u8_Data);			/* I2Cデータを登録する					*/

#endif /* __DRV_H */
