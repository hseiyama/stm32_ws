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

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

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

#endif /* __DRV_H */
