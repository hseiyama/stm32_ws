/**
  ******************************************************************************
  * @file           : drv_sync.c
  * @brief          : SYNCドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define __SECTION_SYNC		__attribute__((section(".share")))

#define HSEM_ID_30			(30)			/* HW semaphore 30				*/
#define HSEM_ID_31			(31)			/* HW semaphore 31				*/
#define TX_QUEUE_SIZE		(64)			/* SYNC送信Queueサイズ			*/
#define RX_QUEUE_SIZE		(64)			/* SYNC受信Queueサイズ			*/
#define SYNC_INIT_DATA		(0xAA55AA55)	/* SYNC同期データ(初期化時)		*/

#ifdef CORE_CM7
#define TX_HSEM_ID			HSEM_ID_30		/* SYNC送信用セマフォID			*/
#define RX_HSEM_ID			HSEM_ID_31		/* SYNC受信用セマフォID			*/
#else  /* CORE_CM4 */
#define RX_HSEM_ID			HSEM_ID_30		/* SYNC受信用セマフォID			*/
#define TX_HSEM_ID			HSEM_ID_31		/* SYNC送信用セマフォID			*/
#endif

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile static uint32_t u32s_SyncInitData __SECTION_SYNC = 0;			/* SYNC同期用データ(初期化時)	*/

#ifdef CORE_CM7
volatile static uint8_t u8s_SyncTxBuffer[TX_QUEUE_SIZE] __SECTION_SYNC;	/* SYNC送信Queueデータ			*/
volatile static uint8_t u8s_SyncRxBuffer[RX_QUEUE_SIZE] __SECTION_SYNC;	/* SYNC受信Queueデータ			*/
volatile static QueueControl sts_SyncTxQueue __SECTION_SYNC;			/* SYNC送信Queue情報			*/
volatile static QueueControl sts_SyncRxQueue __SECTION_SYNC;			/* SYNC受信Queue情報			*/
#else  /* CORE_CM4 */
volatile static uint8_t u8s_SyncRxBuffer[RX_QUEUE_SIZE] __SECTION_SYNC;	/* SYNC受信Queueデータ			*/
volatile static uint8_t u8s_SyncTxBuffer[TX_QUEUE_SIZE] __SECTION_SYNC;	/* SYNC送信Queueデータ			*/
volatile static QueueControl sts_SyncRxQueue __SECTION_SYNC;			/* SYNC受信Queue情報			*/
volatile static QueueControl sts_SyncTxQueue __SECTION_SYNC;			/* SYNC送信Queue情報			*/
#endif

/* Private function prototypes -----------------------------------------------*/
static uint16_t setSyncTxQueue(const uint8_t *pu8_Data, uint16_t u16_Size);	/* SYNC送信Queueに登録する				*/
static uint16_t getSyncRxQueue(uint8_t *pu8_Data, uint16_t u16_Size);		/* SYNC受信Queueから取得する			*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  SYNCドライバー初期化処理
  * @param  None
  * @retval None
  */
void taskSyncDriverInit(void)
{
#ifdef CORE_CM7
	mem_set08((uint8_t *)&u8s_SyncTxBuffer[0], 0x00, TX_QUEUE_SIZE);
	mem_set08((uint8_t *)&u8s_SyncRxBuffer[0], 0x00, RX_QUEUE_SIZE);
	mem_set08((uint8_t *)&sts_SyncTxQueue, 0x00, sizeof(sts_SyncTxQueue));
	mem_set08((uint8_t *)&sts_SyncRxQueue, 0x00, sizeof(sts_SyncRxQueue));
	/* 初期化タイミングの同期(更新) */
	u32s_SyncInitData = SYNC_INIT_DATA;
#else  /* CORE_CM4 */
	/* 初期化タイミングの同期(参照) */
	while (u32s_SyncInitData != SYNC_INIT_DATA);
#endif
}

/**
  * @brief  SYNCドライバー入力処理
  * @param  None
  * @retval None
  */
void taskSyncDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief  SYNCドライバー出力処理
  * @param  None
  * @retval None
  */
void taskSyncDriverOutput(void)
{
	/* 処理なし */
}

/**
  * @brief  セマフォを取得する
  * @param  u32_SemId: セマフォのID
  * @retval OK/NG
  */
uint8_t syncTakeSemaphore(uint32_t u32_SemId)
{
	uint8_t u8_RetCode = NG;

	/* Read the RLR register to take the semaphore */
	if (HSEM->RLR[u32_SemId] == (HSEM_CR_COREID_CURRENT | HSEM_RLR_LOCK)) {
		/* take success when MasterID match and take bit set */
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  セマフォを開放する
  * @param  u32_SemId: セマフォのID
  * @retval OK
  */
uint8_t syncReleaseSemaphore(uint32_t u32_SemId)
{
	/* Clear the semaphore by writing to the R register : the MasterID and take bit = 0 */
	HSEM->R[u32_SemId] = HSEM_CR_COREID_CURRENT;
	return OK;
}

/**
  * @brief  セマフォの状態を確認する
  * @param  u32_SemId: セマフォのID
  * @retval ON/OFF
  */
uint8_t syncIsTakenSemaphore(uint32_t u32_SemId)
{
	uint8_t u8_RetCode = OFF;

	if ((HSEM->R[u32_SemId] & HSEM_R_LOCK) != 0) {
		u8_RetCode = ON;
	}
	return u8_RetCode;
}

/**
  * @brief  SYNC送信データを登録する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 登録した数
  */
uint16_t syncSetTxData(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	/* SYNC送信Queueに登録する */
	return setSyncTxQueue(pu8_Data, u16_Size);
}

/**
  * @brief  SYNC受信データを取得する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 取得した数
  */
uint16_t syncGetRxData(uint8_t *pu8_Data, uint16_t u16_Size)
{
	/* SYNC受信Queueから取得する */
	return getSyncRxQueue(pu8_Data, u16_Size);
}

/**
  * @brief  SYNC受信データの数を取得する
  * @param  None
  * @retval データの数
  */
uint16_t syncGetRxCount(void)
{
	/* SYNC受信Queueデータの登録数 */
	return sts_SyncRxQueue.u16_count;
}

/**
  * @brief  Hex1Byte表示処理(SYNC)
  * @param  u8_Data: データ
  * @retval None
  */
void syncEchoHex8(uint8_t u8_Data) {
	const uint8_t HexTable[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
	setSyncTxQueue(&HexTable[(u8_Data >> 4) & 0x0F], 1);
	setSyncTxQueue(&HexTable[u8_Data & 0x0F], 1);
}

/**
  * @brief  Hex2Byte表示処理(SYNC)
  * @param  u16_Data: データ
  * @retval None
  */
void syncEchoHex16(uint16_t u16_Data) {
	syncEchoHex8((u16_Data >> 8) & 0xFF);
	syncEchoHex8(u16_Data & 0xFF);
}

/**
  * @brief  Hex4Byte表示処理(SYNC)
  * @param  u32_Data: データ
  * @retval None
  */
void syncEchoHex32(uint32_t u32_Data) {
	syncEchoHex16((u32_Data >> 16) & 0xFFFF);
	syncEchoHex16(u32_Data & 0xFFFF);
}

/**
  * @brief  文字列表示処理(SYNC)
  * @param  pu8_Data: データのポインタ
  * @retval None
  */
void syncEchoStr(const char *ps8_Data) {
	while (*ps8_Data != 0x00) {
		setSyncTxQueue((uint8_t *)ps8_Data, 1);
		ps8_Data++;
	}
}

/**
  * @brief  文字列表示処理(SYNC改行付き)
  * @param  pu8_Data: データのポインタ
  * @retval None
  */
void syncEchoStrln(const char *ps8_Data) {
	syncEchoStr(ps8_Data);
	syncEchoStr("\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  SYNC送信Queueに登録する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 登録した数
  */
static uint16_t setSyncTxQueue(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_RemainSize;
	uint16_t u16_RetValue = 0;

	/* 上限を超えるQueueデータの登録は破棄する */
	if ((sts_SyncTxQueue.u16_count + u16_Size) <= TX_QUEUE_SIZE) {
		/* セマフォを取得する */
		while (syncTakeSemaphore(TX_HSEM_ID) != OK);
		u16_RemainSize = TX_QUEUE_SIZE - sts_SyncTxQueue.u16_head;
		/* バッファが循環する場合 */
		if (u16_Size > u16_RemainSize) {
			mem_cpy08((uint8_t *)&u8s_SyncTxBuffer[sts_SyncTxQueue.u16_head], &pu8_Data[0], u16_RemainSize);
			mem_cpy08((uint8_t *)&u8s_SyncTxBuffer[0], &pu8_Data[u16_RemainSize], u16_Size - u16_RemainSize);
		}
		/* 上記以外 */
		else {
			mem_cpy08((uint8_t *)&u8s_SyncTxBuffer[sts_SyncTxQueue.u16_head], &pu8_Data[0], u16_Size);
		}
		sts_SyncTxQueue.u16_head = (sts_SyncTxQueue.u16_head + u16_Size) % TX_QUEUE_SIZE;
		sts_SyncTxQueue.u16_count += u16_Size;
		/* セマフォを開放する */
		syncReleaseSemaphore(TX_HSEM_ID);
		u16_RetValue = u16_Size;
	}
	return u16_RetValue;
}

/**
  * @brief  SYNC受信Queueから取得する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 取得した数
  */
static uint16_t getSyncRxQueue(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_RemainSize;
	uint16_t u16_GetDataSize = 0;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_SyncRxQueue.u16_count > 0) {
		/* セマフォを取得する */
		while (syncTakeSemaphore(RX_HSEM_ID) != OK);
		u16_GetDataSize = (u16_Size <= sts_SyncRxQueue.u16_count) ? u16_Size : sts_SyncRxQueue.u16_count;
		u16_RemainSize = RX_QUEUE_SIZE - sts_SyncRxQueue.u16_tail;
		/* バッファが循環する場合 */
		if (u16_GetDataSize > u16_RemainSize) {
			mem_cpy08(&pu8_Data[0], (uint8_t *)&u8s_SyncRxBuffer[sts_SyncRxQueue.u16_tail], u16_RemainSize);
			mem_cpy08(&pu8_Data[u16_RemainSize], (uint8_t *)&u8s_SyncRxBuffer[0], u16_GetDataSize - u16_RemainSize);
		}
		/* 上記以外 */
		else {
			mem_cpy08(&pu8_Data[0], (uint8_t *)&u8s_SyncRxBuffer[sts_SyncRxQueue.u16_tail], u16_GetDataSize);
		}
		sts_SyncRxQueue.u16_tail = (sts_SyncRxQueue.u16_tail + u16_GetDataSize) % RX_QUEUE_SIZE;
		sts_SyncRxQueue.u16_count -= u16_GetDataSize;
		/* セマフォを開放する */
		syncReleaseSemaphore(RX_HSEM_ID);
	}
	return u16_GetDataSize;
}
