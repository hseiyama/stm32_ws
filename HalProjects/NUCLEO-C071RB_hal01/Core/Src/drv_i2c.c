/**
  ******************************************************************************
  * @file           : drv_i2c.c
  * @brief          : I2Cドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* I2C状態情報 */
enum I2cStatelInfo {
	I2C_STATE_IDLE = 0,				/* 動作なし								*/
	I2C_STATE_READ1,				/* 読み込み中1							*/
	I2C_STATE_READ2,				/* 読み込み中2							*/
	I2C_STATE_WRITE,				/* 書き込み中							*/
	I2C_STATE_MAX					/* I2C状態の上限						*/
};

/* Private define ------------------------------------------------------------*/

/* MCP23017 */
#define MCP23017_ADDR			(0x20)		/* デバイスのアドレス								*/
#define TX_BUFFER_SIZE			(2)			/* I2C送信バッファサイズ							*/
#define RX_BUFFER_SIZE			(2)			/* I2C受信バッファサイズ							*/
/* MCP23017 Register Address */
#define REG_IODIRA				(0x00)		/* I/O方向レジスタ									*/
#define REG_IODIRB				(0x01)
#define REG_IPOLA				(0x02)		/* 入力極性ポートレジスタ							*/
#define REG_IPOLB				(0x03)
#define REG_GPINTENA			(0x04)		/* 状態変化割り込みピン								*/
#define REG_GPINTENB			(0x05)
#define REG_DEFVALA				(0x06)		/* 既定値レジスタ									*/
#define REG_DEFVALB				(0x07)
#define REG_INTCONA				(0x08)		/* 状態変化割り込み制御レジスタ						*/
#define REG_INTCONB				(0x09)
#define REG_IOCON				(0x0A)		/* I/Oエクスパンダコンフィグレーションレジスタ		*/
//#define REG_IOCON				(0x0B)
#define REG_GPPUA				(0x0C)		/* GPIOプルアップ抵抗レジスタ						*/
#define REG_GPPUB				(0x0D)
#define REG_INTFA				(0x0E)		/* 割り込みフラグレジスタ							*/
#define REG_INTFB				(0x0F)
#define REG_INTCAPA				(0x10)		/* 割り込み時にキャプチャしたポート値を示すレジスタ	*/
#define REG_INTCAPB				(0x11)
#define REG_GPIOA				(0x12)		/* 汎用I/Oポ ートレジスタ							*/
#define REG_GPIOB				(0x13)
#define REG_OLATA				(0x14)		/* 出力ラッチレジスタ								*/
#define REG_OLATB				(0x15)

/* Private macro -------------------------------------------------------------*/
static uint8_t sendI2cData(uint8_t u8_RegAddr, uint8_t u8_Data);		/* I2Cデータを送信する(同期)			*/
static uint8_t sendI2cDataReq(uint8_t *pu8_Data, uint16_t u16_Size);	/* I2Cデータを送信する(非同期)			*/
static uint8_t recvI2cDataReq(uint8_t *pu8_Data, uint16_t u16_Size);	/* I2Cデータを受信する(非同期)			*/
static void updateI2cState(void);										/* I2C状態の更新処理					*/

/* Private variables ---------------------------------------------------------*/

/* I2Cレジスタアドレステーブル */
static const uint8_t I2cRegAddrTable[I2C_REGISTER_MAX] = {
	REG_GPIOA,								/* I2C_REGISTER_PORTA			*/
	REG_OLATB								/* I2C_REGISTER_PORTB			*/
};

volatile static uint8_t u8s_I2cTxBuffer[TX_BUFFER_SIZE];		/* I2C送信バッファ				*/
volatile static uint8_t u8s_I2cRxBuffer[RX_BUFFER_SIZE];		/* I2C受信バッファ				*/
volatile static uint8_t u8s_I2cIrqTxEnd;						/* I2C送信割り込み終了フラグ	*/
volatile static uint8_t u8s_I2cIrqRxEnd;						/* I2C受信割り込み終了フラグ	*/
volatile static uint8_t u8s_I2cIrqError;						/* I2C割り込みエラーフラグ		*/
static uint8_t u8s_I2cState;									/* I2C状態						*/
static uint8_t u8s_I2cData[I2C_REGISTER_MAX];					/* I2Cデータ					*/
static uint8_t u8s_I2cEnable;									/* I2C有効フラグ				*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief Master Tx Transfer completed callback.
  * @param hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == &hi2c1) {
		u8s_I2cIrqTxEnd = ON;
	}
}

/**
  * @brief Master Rx Transfer completed callback.
  * @param hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == &hi2c1) {
		u8s_I2cIrqRxEnd = ON;
	}
}

/**
  * @brief I2C error callback.
  * @param hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == &hi2c1) {
		u8s_I2cIrqError = ON;
	}
}

/**
  * @brief I2Cドライバー初期化処理
  * @param None
  * @retval None
  */
void taskI2cDriverInit(void)
{
	mem_set08((uint8_t *)&u8s_I2cTxBuffer[0], 0x00, TX_BUFFER_SIZE);
	mem_set08((uint8_t *)&u8s_I2cRxBuffer[0], 0x00, RX_BUFFER_SIZE);
	mem_set08(&u8s_I2cData[0], 0x00, I2C_REGISTER_MAX);
	u8s_I2cIrqTxEnd = OFF;
	u8s_I2cIrqRxEnd = OFF;
	u8s_I2cIrqError = OFF;
	u8s_I2cState = I2C_STATE_IDLE;
	u8s_I2cEnable = ON;

#if I2C_ACTIVATE == ON
	/* IOCON コンフィグレーションを初期化 */
	sendI2cData(REG_IOCON, 0x00);
	/* PORTA 0-3ピンを入力として設定 */
	sendI2cData(REG_IODIRA, 0xFF);
	sendI2cData(REG_GPPUA, 0x0F);
	/* PORTB 0-3ピンを出力として設定 */
	sendI2cData(REG_IODIRB, 0xF0);
	sendI2cData(REG_OLATB, 0x00);
#endif
}

/**
  * @brief I2Cドライバー入力処理
  * @param None
  * @retval None
  */
void taskI2cDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief I2Cドライバー出力処理
  * @param None
  * @retval None
  */
void taskI2cDriverOutput(void)
{
#if I2C_ACTIVATE == ON
	/* I2C有効フラグがONの場合 */
	if (u8s_I2cEnable == ON) {
		/* I2C状態の更新処理 */
		updateI2cState();
	}
#endif
}

/**
  * @brief I2C通信を有効化
  * @param None
  * @retval None
  */
void i2cComEnable(void)
{
	u8s_I2cEnable = ON;
}

/**
  * @brief I2C通信を無効化
  * @param None
  * @retval None
  */
void i2cComDisable(void)
{
	u8s_I2cEnable = OFF;
}

/**
  * @brief I2Cデータを取得する
  * @param u16_Register デバイスのレジスタ
  * @param pu8_Data データのアドレス
  * @retval OK/NG
  */
uint8_t i2cGetData(uint16_t u16_Register, uint8_t *pu8_Data)
{
	/* I2Cのレジスタが指定レジスタでない場合 */
	if (u16_Register >= I2C_REGISTER_MAX) {
		return NG;
	}

	*pu8_Data = u8s_I2cData[u16_Register] & 0x0F;
	return OK;
}

/**
  * @brief I2Cデータを登録する
  * @param u16_Register デバイスのレジスタ
  * @param u8_Data データ
  * @retval OK/NG
  */
uint8_t i2cSetData(uint16_t u16_Register, uint8_t u8_Data)
{
	/* I2Cのレジスタが指定レジスタでない場合 */
	if (u16_Register >= I2C_REGISTER_MAX) {
		return NG;
	}

	u8s_I2cData[u16_Register] = u8_Data & 0x0F;
	return OK;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief I2Cデータを送信する(同期)
  * @param u8_RegAddr レジスタアドレス
  * @param u8_Data データ
  * @retval OK/NG
  */
static uint8_t sendI2cData(uint8_t u8_RegAddr, uint8_t u8_Data)
{
	uint16_t u16_I2cAddress;
	uint8_t u8_RetCode = NG;

	u8s_I2cIrqTxEnd = OFF;
	u8s_I2cIrqError = OFF;
	/* I2C送信の準備 */
	u16_I2cAddress = (MCP23017_ADDR << 1) & 0x0FE;
	u8s_I2cTxBuffer[0] = u8_RegAddr;
	u8s_I2cTxBuffer[1] = u8_Data;
	/* I2C送信(割り込み)を開始 */
	if (HAL_I2C_Master_Transmit_IT(&hi2c1, u16_I2cAddress, (uint8_t *)&u8s_I2cTxBuffer[0], 2) != HAL_OK) {
		/* Transmitting Error */
		Error_Handler();
	}

	while ((u8s_I2cIrqTxEnd == OFF) && (u8s_I2cIrqError == OFF)) {
		/* 何もしない */
	}
	if (u8s_I2cIrqTxEnd == ON) {
		u8_RetCode = OK;
	}
	u8s_I2cIrqTxEnd = OFF;
	u8s_I2cIrqError = OFF;

	return u8_RetCode;
}

/**
  * @brief I2Cデータを送信する(非同期)
  * @param pu8_Data データのアドレス
  * @param u16_Size データのサイズ
  * @retval OK/NG
  */
static uint8_t sendI2cDataReq(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_I2cAddress;

	u16_I2cAddress = (MCP23017_ADDR << 1) & 0x0FE;
	/* I2C送信(割り込み)を開始 */
	if (HAL_I2C_Master_Transmit_IT(&hi2c1, u16_I2cAddress, pu8_Data, u16_Size) != HAL_OK) {
		/* Transmitting Error */
		Error_Handler();
	}
	return OK;
}

/**
  * @brief I2Cデータを受信する(非同期)
  * @param pu8_Data データのアドレス
  * @param u16_Size データのサイズ
  * @retval OK/NG
  */
static uint8_t recvI2cDataReq(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_I2cAddress;

	u16_I2cAddress = (MCP23017_ADDR << 1) & 0x0FE;
	/* I2C受信(割り込み)を開始 */
	if (HAL_I2C_Master_Receive_IT(&hi2c1, u16_I2cAddress, pu8_Data, u16_Size) != HAL_OK) {
		/* Receiving Error */
		Error_Handler();
	}
	return OK;
}

/**
  * @brief I2C状態の更新処理
  * @param None
  * @retval None
  */
static void updateI2cState(void)
{
	switch (u8s_I2cState) {
	/* 動作なし */
	case I2C_STATE_IDLE:
		u8s_I2cIrqTxEnd = OFF;
		u8s_I2cIrqRxEnd = OFF;
		u8s_I2cIrqError = OFF;
		/* I2C受信の準備 */
		u8s_I2cRxBuffer[0] = I2cRegAddrTable[I2C_REGISTER_PORTA];
		u8s_I2cRxBuffer[1] = 0;
		/* I2Cデータを送信する(非同期) */
		sendI2cDataReq((uint8_t *)&u8s_I2cRxBuffer[0], 1);
		u8s_I2cState = I2C_STATE_READ1;
		break;
	/* 読み込み中1 */
	case I2C_STATE_READ1:
		if (u8s_I2cIrqTxEnd == ON) {
			u8s_I2cIrqTxEnd = OFF;
			/* I2Cデータを受信する(非同期) */
			recvI2cDataReq((uint8_t *)&u8s_I2cRxBuffer[1], RX_BUFFER_SIZE - 1);
			u8s_I2cState = I2C_STATE_READ2;
		}
		else if (u8s_I2cIrqError == ON) {
			u8s_I2cIrqError = OFF;
			/* 動作なしに遷移する */
			u8s_I2cState = I2C_STATE_IDLE;
		}
		break;
	/* 読み込み中2 */
	case I2C_STATE_READ2:
		if (u8s_I2cIrqRxEnd == ON) {
			u8s_I2cIrqRxEnd = OFF;
			/* 受信したI2Cデータを格納する */
			u8s_I2cData[I2C_REGISTER_PORTA] = u8s_I2cRxBuffer[1];
			/* I2C送信の準備 */
			u8s_I2cTxBuffer[0] = I2cRegAddrTable[I2C_REGISTER_PORTB];
			u8s_I2cTxBuffer[1] = u8s_I2cData[I2C_REGISTER_PORTB];
			/* I2Cデータを送信する(非同期) */
			sendI2cDataReq((uint8_t *)&u8s_I2cTxBuffer[0], TX_BUFFER_SIZE);
			u8s_I2cState = I2C_STATE_WRITE;
		}
		else if (u8s_I2cIrqError == ON) {
			u8s_I2cIrqError = OFF;
			/* 動作なしに遷移する */
			u8s_I2cState = I2C_STATE_IDLE;
		}
		break;
	/* 書き込み中 */
	case I2C_STATE_WRITE:
		if (u8s_I2cIrqTxEnd == ON) {
			u8s_I2cIrqTxEnd = OFF;
			/* 動作なしに遷移する */
			u8s_I2cState = I2C_STATE_IDLE;
		}
		else if (u8s_I2cIrqError == ON) {
			u8s_I2cIrqError = OFF;
			/* 動作なしに遷移する */
			u8s_I2cState = I2C_STATE_IDLE;
		}
		break;
	/* 上記以外 */
	default:
		/* I2Cドライバー初期化処理 */
		taskI2cDriverInit();
		break;
	}
}
