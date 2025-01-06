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

/* drv_sync.c */
extern void taskSyncDriverInit(void);										/* SYNCドライバー初期化処理				*/
extern void taskSyncDriverInput(void);										/* SYNCドライバー入力処理				*/
extern void taskSyncDriverOutput(void);										/* SYNCドライバー出力処理				*/
extern uint8_t syncTakeSemaphore(uint32_t u32_SemId);						/* セマフォを取得する					*/
extern uint8_t syncReleaseSemaphore(uint32_t u32_SemId);					/* セマフォを開放する					*/
extern uint8_t syncIsTakenSemaphore(uint32_t u32_SemId);					/* セマフォの状態を確認する				*/
extern uint16_t syncSetTxData(const uint8_t *pu8_Data, uint16_t u16_Size);	/* SYNC送信データを登録する				*/
extern uint16_t syncGetRxData(uint8_t *pu8_Data, uint16_t u16_Size);		/* SYNC受信データを取得する				*/
extern uint16_t syncGetRxCount(void);										/* SYNC受信データの数を取得する			*/
extern void syncEchoHex8(uint8_t u8_Data);									/* Hex1Byte表示処理(SYNC)				*/
extern void syncEchoHex16(uint16_t u16_Data);								/* Hex2Byte表示処理(SYNC)				*/
extern void syncEchoHex32(uint32_t u32_Data);								/* Hex4Byte表示処理(SYNC)				*/
extern void syncEchoStr(const char *ps8_Data);								/* 文字列表示処理(SYNC)					*/
extern void syncEchoStrln(const char *ps8_Data);							/* 文字列表示処理(SYNC改行付き)			*/

#endif /* __DRV_H */
