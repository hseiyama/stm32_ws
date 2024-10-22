/**
  ******************************************************************************
  * @file           : drv_flash.c
  * @brief          : FLASHドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* FLASH状態情報 */
enum FlashStatelInfo {
	FLASH_STATE_IDLE = 0,			/* 動作なし								*/
	FLASH_STATE_WRITE,				/* 書き込み中							*/
	FLASH_STATE_ERASE,				/* 消去中								*/
	FLASH_STATE_MAX					/* FLASH状態の上限						*/
};

/* FLASH結果情報 */
enum FlashResultlInfo {
	FLASH_RESULT_NONE = 0,			/* 結果なし								*/
	FLASH_RESULT_BUSY,				/* 動作中								*/
	FLASH_RESULT_OK,				/* 結果OK								*/
	FLASH_RESULT_NG,				/* 結果NG								*/
	FLASH_RESULT_MAX				/* FLASH結果の上限						*/
};

/* FLASHブロックアドレス情報 */
typedef struct _FlashBlockAddress {
	uint32_t u32_start;				/* ブロック開始アドレス					*/
	uint32_t u32_end;				/* ブロック終了アドレス					*/
} FlashBlockAddress;

/* Private define ------------------------------------------------------------*/
#define DATA_PAGE_START			(63)						/* FLASHデータ開始ページ			*/
#define DATA_PAGE_NUM			(1)							/* FLASHデータページ数				*/
#define DATA_PAGE_END			(DATA_PAGE_START + DATA_PAGE_NUM - 1)
															/* FLASHデータ終了ページ			*/
#define DATA_START_ADDR			(FLASH_BASE + (DATA_PAGE_START * FLASH_PAGE_SIZE))
															/* FLASHデータ開始アドレス			*/
#define DATA_END_ADDR			(DATA_START_ADDR + (DATA_PAGE_NUM * FLASH_PAGE_SIZE) - 1)
															/* FLASHデータ終了アドレス			*/
#define DATA_BLOCK_NUM			(FLASH_PAGE_SIZE / FLASH_BLOCK_SIZE)
															/* FLASHデータブロック数			*/
#define DATA_MARK_OFFSET		FLASH_DATA_SIZE				/* FLASHデータマークオフセット		*/
#define MARK_OFF				(0xFFFFFFFF)				/* マークOFF						*/
#define MARK_ON					(0xAA55AA55)				/* マークON							*/
#define DEBUG_FLASH				ON							/* デバッグ用マクロの切り替え		*/

/* Private macro -------------------------------------------------------------*/

/* デバッグ用マクロ */
#if DEBUG_FLASH == ON
#define DBG_VALUE32(a, b)		{uartEchoStr(a); uartEchoHex32(b); uartEchoStrln("");}
#define DBG_MESSAGE(a)			uartEchoStrln(a)
#else
#define DBG_VALUE32(a, b)
#define DBG_MESSAGE(a)
#endif

/* Private variables ---------------------------------------------------------*/

/* FLASHデータ初期値 */
static const uint8_t FlashDataInit[FLASH_BLOCK_SIZE] __ALIGNED(4) = {
	0x00, 0x00, 'P',  'W',  'M',  '=',  '_',  '_',  '0',  '%',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t u8s_FlashDataBuffer[FLASH_BLOCK_SIZE] __ALIGNED(4);	/* FLASHデータバッファ			*/
volatile static uint8_t u8s_FlashState;							/* FLASH状態					*/
volatile static uint8_t u8s_FlashIrqEnd;						/* FLASH割り込み終了フラグ		*/
volatile static uint8_t u8s_FlashIrqError;						/* FLASH割り込みエラーフラグ	*/
static FlashBlockAddress sts_FlashBlockAddr;					/* FLASHブロックアドレス		*/
static uint8_t u8s_FlashWriteRequest;							/* FLASH書き込み要求			*/
static uint32_t u32s_FlashWriteAddr;							/* FLASH書き込みアドレス		*/
static uint64_t *pu64s_FlashData;								/* FLASHデータ(64bit)のポインタ	*/
static uint8_t u8s_FlashReadResult;								/* FLASH読み出し結果			*/
static uint8_t u8s_FlashWriteResult;							/* FLASH書き込み結果			*/

/* Private function prototypes -----------------------------------------------*/
static uint8_t getReadDataAddr(FlashBlockAddress *pst_Address);		/* FLASH読み出しアドレスを取得する		*/
static uint8_t getWriteDataAddr(FlashBlockAddress *pst_Address);	/* FLASH書き込みアドレスを取得する		*/
static void updateFlashState(void);									/* FLASH状態の更新処理					*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief FLASH end of operation interrupt callback.
  * @param ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                  Page Erase: Page which has been erased
  *                  Program: Address which was selected for data program
  * @retval None
  */
void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
	switch (u8s_FlashState) {
	/* 書き込み中 */
	case FLASH_STATE_WRITE:
		u8s_FlashIrqEnd = ON;
		break;
	/* 消去中 */
	case FLASH_STATE_ERASE:
		if (ReturnValue >= DATA_PAGE_END) {
			u8s_FlashIrqEnd = ON;
		}
		break;
	}
}

/**
  * @brief FLASH operation error interrupt callback.
  * @param ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                 Page Erase: Page number which returned an error
  *                 Program: Address which was selected for data program
  * @retval None
  */
void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue)
{
	u8s_FlashIrqError = ON;
}

/**
  * @brief FLASHドライバー初期化処理
  * @param None
  * @retval None
  */
void taskFlashDriverInit(void)
{
	mem_cpy32((uint32_t *)&u8s_FlashDataBuffer[0], (uint32_t *)&FlashDataInit[0], FLASH_BLOCK_SIZE / 4);
	mem_set08((uint8_t *)&sts_FlashBlockAddr, 0x00, sizeof(sts_FlashBlockAddr));
	u8s_FlashState= FLASH_STATE_IDLE;
	u8s_FlashIrqEnd = OFF;
	u8s_FlashIrqError = OFF;
	u8s_FlashWriteRequest = OFF;
	u32s_FlashWriteAddr = 0;
	pu64s_FlashData = NULL;
	u8s_FlashReadResult = FLASH_RESULT_NONE;
	u8s_FlashWriteResult = FLASH_RESULT_NONE;
}

/**
  * @brief FLASHドライバー入力処理
  * @param None
  * @retval None
  */
void taskFlashDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief FLASHドライバー出力処理
  * @param None
  * @retval None
  */
void taskFlashDriverOutput(void)
{
	/* FLASH状態の更新処理 */
	updateFlashState();
}

/**
  * @brief FLASHデータを取得する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval データの取得数
  */
uint16_t flashGetData(uint8_t *pu8_Data, uint16_t u16_Size)
{
	u16_Size = (u16_Size <= FLASH_BLOCK_SIZE) ? u16_Size : FLASH_BLOCK_SIZE;
	mem_cpy08(&pu8_Data[0], &u8s_FlashDataBuffer[0], u16_Size);
	return u16_Size;
}

/**
  * @brief FLASHデータを更新する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval データの更新数
  */
uint16_t flashSetData(uint8_t *pu8_Data, uint16_t u16_Size)
{
	u16_Size = (u16_Size <= FLASH_BLOCK_SIZE) ? u16_Size : FLASH_BLOCK_SIZE;
	mem_cpy08(&u8s_FlashDataBuffer[0], &pu8_Data[0], u16_Size);
	return u16_Size;
}

/**
  * @brief FLASHデータを読み出す
  * @param None
  * @retval None
  */
void flashReadData(void)
{
	FlashBlockAddress st_FlashBlockAddr;

	/* FLASH読み出しアドレスの取得が成功した場合 */
	if (getReadDataAddr(&st_FlashBlockAddr) == OK) {
		DBG_VALUE32("ReadAddr = ", st_FlashBlockAddr.u32_start);				/* DEBUG */
		mem_cpy32((uint32_t *)&u8s_FlashDataBuffer[0], (uint32_t *)st_FlashBlockAddr.u32_start, FLASH_BLOCK_SIZE / 4);
		u8s_FlashReadResult = FLASH_RESULT_OK;
	}
	/* 上記以外 */
	else {
		mem_cpy32((uint32_t *)&u8s_FlashDataBuffer[0], (uint32_t *)&FlashDataInit[0], FLASH_BLOCK_SIZE / 4);
		u8s_FlashReadResult = FLASH_RESULT_NG;
	}
}

/**
  * @brief FLASHデータの書き込みを要求する
  * @param None
  * @retval None
  */
void flashWriteDataRequest(void)
{
	u8s_FlashWriteRequest = ON;
}

/**
  * @brief FLASH読み出し結果を取得する
  * @param None
  * @retval FlashResultlInfo
  */
uint8_t flashGetReadResult(void)
{
	return u8s_FlashReadResult;
}

/**
  * @brief FLASH書き込み結果を取得する
  * @param None
  * @retval FlashResultlInfo
  */
uint8_t flashGetWriteResult(void)
{
	return u8s_FlashWriteResult;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief FLASH読み出しアドレスを取得する
  * @param pst_Address FLASHブロックアドレス情報のポインタ
  * @retval OK/NG
  */
static uint8_t getReadDataAddr(FlashBlockAddress *pst_Address)
{
	uint32_t u32_StartAddress;
	uint32_t u32_Mark;
	uint8_t u8_RetCode = NG;

	u32_StartAddress = DATA_START_ADDR;
	/* マークONのブロックを探索する */
	while (u32_StartAddress < DATA_END_ADDR) {
		u32_Mark = *(uint32_t *)(u32_StartAddress + DATA_MARK_OFFSET);
		if (u32_Mark == MARK_ON) {
			/* FLASHブロックアドレス情報を設定する */
			pst_Address->u32_start = u32_StartAddress;
			pst_Address->u32_end = u32_StartAddress + FLASH_BLOCK_SIZE - 1;
			u8_RetCode = OK;
		}
		else {
			/* 探索を終了する */
			break;
		}
		u32_StartAddress += FLASH_BLOCK_SIZE;
	}

	return u8_RetCode;
}

/**
  * @brief FLASH書き込みアドレスを取得する
  * @param pst_Address FLASHブロックアドレス情報のポインタ
  * @retval OK/NG
  */
static uint8_t getWriteDataAddr(FlashBlockAddress *pst_Address)
{
	uint32_t u32_StartAddress;
	uint32_t u32_Mark;
	uint8_t u8_RetCode = NG;

	u32_StartAddress = DATA_START_ADDR;
	/* マークOFFのブロックを探索する */
	while (u32_StartAddress < DATA_END_ADDR) {
		u32_Mark = *(uint32_t *)(u32_StartAddress + DATA_MARK_OFFSET);
		if (u32_Mark == MARK_OFF) {
			/* FLASHブロックアドレス情報を設定する */
			pst_Address->u32_start = u32_StartAddress;
			pst_Address->u32_end = u32_StartAddress + FLASH_BLOCK_SIZE - 1;
			u8_RetCode = OK;
			/* 探索を終了する */
			break;
		}
		u32_StartAddress += FLASH_BLOCK_SIZE;
	}

	return u8_RetCode;
}

/**
  * @brief FLASH状態の更新処理
  * @param None
  * @retval None
  */
static void updateFlashState(void)
{
	FLASH_EraseInitTypeDef st_EraseInitInfo;

	switch (u8s_FlashState) {
	/* 動作なし */
	case FLASH_STATE_IDLE:
		/* FLASH書き込み要求がONの場合 */
		if (u8s_FlashWriteRequest == ON) {
			u8s_FlashWriteRequest = OFF;
			/* 割り込み各種フラグの初期化 */
			u8s_FlashIrqEnd = OFF;
			u8s_FlashIrqError = OFF;
			/* Unlock the Flash to enable the flash control register access */
			HAL_FLASH_Unlock();
			/* Clear OPTVERR bit set on virgin samples */
			__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
			/* FLASH書き込みアドレスの取得が成功した場合 */
			if (getWriteDataAddr(&sts_FlashBlockAddr) == OK) {
				u8s_FlashState = FLASH_STATE_WRITE;
				u8s_FlashWriteResult = FLASH_RESULT_BUSY;
				DBG_VALUE32("WriteAddr = ", sts_FlashBlockAddr.u32_start);		/* DEBUG */
				/* FLASH書き込みの準備 */
				u32s_FlashWriteAddr = sts_FlashBlockAddr.u32_start;
				mem_set32((uint32_t *)&u8s_FlashDataBuffer[DATA_MARK_OFFSET], MARK_ON, 1);
				pu64s_FlashData = (uint64_t *)&u8s_FlashDataBuffer[0];
				/* FLASH書き込み(割り込み)を開始 */
				if (HAL_FLASH_Program_IT(FLASH_TYPEPROGRAM_DOUBLEWORD, u32s_FlashWriteAddr, *pu64s_FlashData) == HAL_OK) {
					u32s_FlashWriteAddr += 8;
					pu64s_FlashData++;
				}
				else {
					/* Programing Error */
				    Error_Handler();
				}
			}
			/* 上記以外 */
			else {
				u8s_FlashState = FLASH_STATE_ERASE;
				u8s_FlashWriteResult = FLASH_RESULT_BUSY;
				/* FLASH消去の準備 */
				st_EraseInitInfo.TypeErase   = FLASH_TYPEERASE_PAGES;
				st_EraseInitInfo.Page        = DATA_PAGE_START;
				st_EraseInitInfo.NbPages     = DATA_PAGE_NUM;
				/* FLASH消去(割り込み)を開始 */
				if (HAL_FLASHEx_Erase_IT(&st_EraseInitInfo) != HAL_OK) {
					/* Erasing Error */
				    Error_Handler();
				}
			}
		}
		break;
	/* 書き込み中 */
	case FLASH_STATE_WRITE:
		if (u8s_FlashIrqEnd == ON) {
			u8s_FlashIrqEnd = OFF;
			/* FLASH書き込みが継続中である場合 */
			if (u32s_FlashWriteAddr < sts_FlashBlockAddr.u32_end) {
				/* FLASH書き込み(割り込み)を開始 */
				if (HAL_FLASH_Program_IT(FLASH_TYPEPROGRAM_DOUBLEWORD, u32s_FlashWriteAddr, *pu64s_FlashData) == HAL_OK) {
					u32s_FlashWriteAddr += 8;
					pu64s_FlashData++;
				}
				else {
					/* Programing Error */
				    Error_Handler();
				}
			}
			/* 上記以外 */
			else {
				/* Lock the Flash to disable the flash control register access */
				HAL_FLASH_Lock();
				u8s_FlashState = FLASH_STATE_IDLE;
				u8s_FlashWriteResult = FLASH_RESULT_OK;
				DBG_MESSAGE("Flash Write OK!");									/* DEBUG */
			}
		}
		else if (u8s_FlashIrqError == ON) {
			u8s_FlashIrqError = OFF;
			/* Lock the Flash to disable the flash control register access */
			HAL_FLASH_Lock();
			u8s_FlashState = FLASH_STATE_IDLE;
			u8s_FlashWriteResult = FLASH_RESULT_NG;
			DBG_MESSAGE("Flash Write NG!");										/* DEBUG */
		}
		break;
	/* 消去中 */
	case FLASH_STATE_ERASE:
		if (u8s_FlashIrqEnd == ON) {
			u8s_FlashIrqEnd = OFF;
			DBG_MESSAGE("Flash Clear OK!");										/* DEBUG */
			/* FLASH状態を「消去中→書き込み中」に移行する */
			u8s_FlashState = FLASH_STATE_WRITE;
			/* FLASH書き込みの準備 */
			sts_FlashBlockAddr.u32_start = DATA_START_ADDR;
			sts_FlashBlockAddr.u32_end = DATA_START_ADDR + FLASH_BLOCK_SIZE - 1;
			DBG_VALUE32("WriteAddr = ", sts_FlashBlockAddr.u32_start);			/* DEBUG */
			u32s_FlashWriteAddr = sts_FlashBlockAddr.u32_start;
			mem_set32((uint32_t *)&u8s_FlashDataBuffer[DATA_MARK_OFFSET], MARK_ON, 1);
			pu64s_FlashData = (uint64_t *)&u8s_FlashDataBuffer[0];
			/* FLASH書き込み(割り込み)を開始 */
			if (HAL_FLASH_Program_IT(FLASH_TYPEPROGRAM_DOUBLEWORD, u32s_FlashWriteAddr, *pu64s_FlashData) == HAL_OK) {
				u32s_FlashWriteAddr += 8;
				pu64s_FlashData++;
			}
			else {
				/* Programing Error */
			    Error_Handler();
			}
		}
		else if (u8s_FlashIrqError == ON) {
			u8s_FlashIrqError = OFF;
			/* Lock the Flash to disable the flash control register access */
			HAL_FLASH_Lock();
			u8s_FlashState = FLASH_STATE_IDLE;
			u8s_FlashWriteResult = FLASH_RESULT_NG;
			DBG_MESSAGE("Flash Clear NG!");										/* DEBUG */
		}
		break;
	/* 上記以外 */
	default:
		/* FLASHドライバー初期化処理 */
		taskFlashDriverInit();
		break;
	}
}
