/**
  ******************************************************************************
  * @file           : drv_spi.c
  * @brief          : SPIドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* SPI状態情報 */
enum SpiStatelInfo {
	SPI_STATE_IDLE = 0,				/* 動作なし								*/
	SPI_STATE_READ,					/* 読み込み中							*/
	SPI_STATE_WRITE,				/* 書き込み中							*/
	SPI_STATE_MAX					/* SPI状態の上限						*/
};

/* Private define ------------------------------------------------------------*/

/* MCP23S17 */
#define MCP23S17_ADDR			(0x40)		/* デバイスのアドレス								*/
#define TX_BUFFER_SIZE			(3)			/* SPI送信バッファサイズ							*/
#define RX_BUFFER_SIZE			(3)			/* SPI受信バッファサイズ							*/
/* MCP23S17 Register Address */
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
static uint8_t sendSpiData(uint8_t u8_RegAddr, uint8_t u8_Data);			/* SPIデータを送信する(同期)			*/
static uint8_t sendSpiDataReq(const uint8_t *pu8_Data, uint16_t u16_Size);	/* SPIデータを送信する(非同期)			*/
static uint8_t recvSpiDataReq(const uint8_t *pu8_TxData, uint8_t *pu8_RxData, uint16_t u16_Size);
																			/* SPIデータを受信する(非同期)			*/
static void updateSpiState(void);											/* SPI状態の更新処理					*/

/* Private variables ---------------------------------------------------------*/

/* SPIレジスタアドレステーブル */
static const uint8_t SpiRegAddrTable[SPI_REGISTER_MAX] = {
	REG_GPIOA,								/* SPI_REGISTER_PORTA			*/
	REG_OLATB								/* SPI_REGISTER_PORTB			*/
};

volatile static uint8_t u8s_SpiTxBuffer[TX_BUFFER_SIZE] __ALIGNED(4);	/* SPI送信バッファ				*/
volatile static uint8_t u8s_SpiRxBuffer[RX_BUFFER_SIZE] __ALIGNED(4);	/* SPI受信バッファ				*/
volatile static uint8_t u8s_SpiIrqTxEnd;						/* SPI送信割り込み終了フラグ	*/
volatile static uint8_t u8s_SpiIrqRxEnd;						/* SPI受信割り込み終了フラグ	*/
volatile static uint8_t u8s_SpiIrqError;						/* SPI割り込みエラーフラグ		*/
static uint8_t u8s_SpiState;									/* SPI状態						*/
static uint8_t u8s_SpiData[SPI_REGISTER_MAX];					/* SPIデータ					*/
static uint8_t u8s_SpiEnable;									/* SPI有効フラグ				*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief Tx Transfer completed callback.
  * @param hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == &hspi1) {
		/* SPI1_NSSを非アクティブに設定する */
		HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);
		u8s_SpiIrqTxEnd = ON;
	}
}

/**
  * @brief Tx and Rx Transfer completed callback.
  * @param hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == &hspi1) {
		/* SPI1_NSSを非アクティブに設定する */
		HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);
		u8s_SpiIrqRxEnd = ON;
	}
}

/**
  * @brief SPI error callback.
  * @param hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == &hspi1) {
		/* SPI1_NSSを非アクティブに設定する */
		HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);
		u8s_SpiIrqError = ON;
	}
}

/**
  * @brief SPIドライバー初期化処理
  * @param None
  * @retval None
  */
void taskSpiDriverInit(void)
{
	mem_set08((uint8_t *)&u8s_SpiTxBuffer[0], 0x00, TX_BUFFER_SIZE);
	mem_set08((uint8_t *)&u8s_SpiRxBuffer[0], 0x00, RX_BUFFER_SIZE);
	mem_set08(&u8s_SpiData[0], 0x00, SPI_REGISTER_MAX);
	u8s_SpiIrqTxEnd = OFF;
	u8s_SpiIrqRxEnd = OFF;
	u8s_SpiIrqError = OFF;
	u8s_SpiState = SPI_STATE_IDLE;
	u8s_SpiEnable = ON;

#if SPI_ACTIVATE == ON
	/* IOCON コンフィグレーションを初期化 */
	sendSpiData(REG_IOCON, 0x00);
	/* PORTA 0-3ピンを入力として設定 */
	sendSpiData(REG_IODIRA, 0xFF);
	sendSpiData(REG_GPPUA, 0x0F);
	/* PORTB 0-3ピンを出力として設定 */
	sendSpiData(REG_IODIRB, 0xF0);
	sendSpiData(REG_OLATB, 0x00);
#endif
}

/**
  * @brief SPIドライバー入力処理
  * @param None
  * @retval None
  */
void taskSpiDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief SPIドライバー出力処理
  * @param None
  * @retval None
  */
void taskSpiDriverOutput(void)
{
#if SPI_ACTIVATE == ON
	/* SPI有効フラグがONの場合 */
	if (u8s_SpiEnable == ON) {
		/* SPI状態の更新処理 */
		updateSpiState();
	}
#endif
}

/**
  * @brief SPI通信を有効化
  * @param None
  * @retval None
  */
void spiComEnable(void)
{
	u8s_SpiEnable = ON;
}

/**
  * @brief SPI通信を無効化
  * @param None
  * @retval None
  */
void spiComDisable(void)
{
	u8s_SpiEnable = OFF;
}

/**
  * @brief SPIデータを取得する
  * @param u16_Register デバイスのレジスタ
  * @param pu8_Data データのアドレス
  * @retval OK/NG
  */
uint8_t spiGetData(uint16_t u16_Register, uint8_t *pu8_Data)
{
	/* SPIのレジスタが指定レジスタでない場合 */
	if (u16_Register >= SPI_REGISTER_MAX) {
		return NG;
	}

	*pu8_Data = u8s_SpiData[u16_Register] & 0x0F;
	return OK;
}

/**
  * @brief SPIデータを登録する
  * @param u16_Register デバイスのレジスタ
  * @param u8_Data データ
  * @retval OK/NG
  */
uint8_t spiSetData(uint16_t u16_Register, uint8_t u8_Data)
{
	/* SPIのレジスタが指定レジスタでない場合 */
	if (u16_Register >= SPI_REGISTER_MAX) {
		return NG;
	}

	u8s_SpiData[u16_Register] = u8_Data & 0x0F;
	return OK;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief SPIデータを送信する(同期)
  * @param u8_RegAddr レジスタアドレス
  * @param u8_Data データ
  * @retval OK/NG
  */
static uint8_t sendSpiData(uint8_t u8_RegAddr, uint8_t u8_Data)
{
	uint8_t u8_RetCode = NG;

	u8s_SpiIrqTxEnd = OFF;
	u8s_SpiIrqError = OFF;
	/* SPI送信の準備 */

	u8s_SpiTxBuffer[0] = MCP23S17_ADDR;
	u8s_SpiTxBuffer[1] = u8_RegAddr;
	u8s_SpiTxBuffer[2] = u8_Data;
	/* SPI1_NSSをアクティブに設定する */
	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET);
	/* SPI送信(同期)を開始 */
//	if (HAL_SPI_Transmit_IT(&hspi1, (uint8_t *)&u8s_SpiTxBuffer[0], TX_BUFFER_SIZE) != HAL_OK) {
	if (HAL_SPI_Transmit(&hspi1, (uint8_t *)&u8s_SpiTxBuffer[0], TX_BUFFER_SIZE, 10) != HAL_OK) {
		/* Transmitting Error */
		Error_Handler();
	}
	/* SPI1_NSSを非アクティブに設定する */
	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);

//	while ((u8s_SpiIrqTxEnd == OFF) && (u8s_SpiIrqError == OFF)) {
//		/* 何もしない */
//	}
//	if (u8s_SpiIrqTxEnd == ON) {
//		u8_RetCode = OK;
//	}
//	u8s_SpiIrqTxEnd = OFF;
//	u8s_SpiIrqError = OFF;
	u8_RetCode = OK;

	return u8_RetCode;
}

/**
  * @brief SPIデータを送信する(非同期)
  * @param pu8_Data データのアドレス
  * @param u16_Size データのサイズ
  * @retval OK/NG
  */
static uint8_t sendSpiDataReq(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	/* SPI1_NSSをアクティブに設定する */
	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET);
	/* SPI送信(同期)を開始 */
//	if (HAL_SPI_Transmit_IT(&hspi1, pu8_Data, u16_Size) != HAL_OK) {
	if (HAL_SPI_Transmit(&hspi1, pu8_Data, u16_Size, 10) != HAL_OK) {
		/* Transmitting Error */
		Error_Handler();
	}
	/* SPI1_NSSを非アクティブに設定する */
	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);
	return OK;
}

/**
  * @brief SPIデータを受信する(非同期)
  * @param pu8_TxData 送信データのアドレス
  * @param pu8_RxData 受信データのアドレス
  * @param u16_Size データのサイズ
  * @retval OK/NG
  */
static uint8_t recvSpiDataReq(const uint8_t *pu8_TxData, uint8_t *pu8_RxData, uint16_t u16_Size)
{
	/* SPI1_NSSをアクティブに設定する */
	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET);
	/* SPI送信受信(同期)を開始 */
//	if (HAL_SPI_TransmitReceive_IT(&hspi1, pu8_TxData, pu8_RxData, u16_Size) != HAL_OK) {
	if (HAL_SPI_TransmitReceive(&hspi1, pu8_TxData, pu8_RxData, u16_Size, 10) != HAL_OK) {
		/* Receiving Error */
		Error_Handler();
	}
	/* SPI1_NSSを非アクティブに設定する */
	HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);
	return OK;
}

/**
  * @brief SPI状態の更新処理
  * @param None
  * @retval None
  */
static void updateSpiState(void)
{
	switch (u8s_SpiState) {
	/* 動作なし */
	case SPI_STATE_IDLE:
		u8s_SpiIrqTxEnd = OFF;
		u8s_SpiIrqRxEnd = OFF;
		u8s_SpiIrqError = OFF;
		/* SPI受信の準備 */
		u8s_SpiTxBuffer[0] = MCP23S17_ADDR | 1;
		u8s_SpiTxBuffer[1] = SpiRegAddrTable[SPI_REGISTER_PORTA];
		u8s_SpiTxBuffer[2] = 0;
		mem_set08((uint8_t *)&u8s_SpiRxBuffer[0], 0x00, RX_BUFFER_SIZE);
		/* SPIデータを受信する(非同期) */
		recvSpiDataReq((uint8_t *)&u8s_SpiTxBuffer[0], (uint8_t *)&u8s_SpiRxBuffer[0], RX_BUFFER_SIZE);
		u8s_SpiState = SPI_STATE_READ;
		break;
	/* 読み込み中 */
	case SPI_STATE_READ:
//		if (u8s_SpiIrqRxEnd == ON) {
//			u8s_SpiIrqRxEnd = OFF;
			/* 受信したSPIデータを格納する */
			u8s_SpiData[SPI_REGISTER_PORTA] = u8s_SpiRxBuffer[2];
			/* SPI送信の準備 */
			u8s_SpiTxBuffer[0] = MCP23S17_ADDR;
			u8s_SpiTxBuffer[1] = SpiRegAddrTable[SPI_REGISTER_PORTB];
			u8s_SpiTxBuffer[2] = u8s_SpiData[SPI_REGISTER_PORTB];
			/* SPIデータを送信する(非同期) */
			sendSpiDataReq((uint8_t *)&u8s_SpiTxBuffer[0], TX_BUFFER_SIZE);
			u8s_SpiState = SPI_STATE_WRITE;
//		}
//		else if (u8s_SpiIrqError == ON) {
//			u8s_SpiIrqError = OFF;
//			/* 動作なしに遷移する */
//			u8s_SpiState = SPI_STATE_IDLE;
//		}
		break;
	/* 書き込み中 */
	case SPI_STATE_WRITE:
//		if (u8s_SpiIrqTxEnd == ON) {
//			u8s_SpiIrqTxEnd = OFF;
			/* 動作なしに遷移する */
			u8s_SpiState = SPI_STATE_IDLE;
//		}
//		else if (u8s_SpiIrqError == ON) {
//			u8s_SpiIrqError = OFF;
//			/* 動作なしに遷移する */
//			u8s_SpiState = SPI_STATE_IDLE;
//		}
		break;
	/* 上記以外 */
	default:
		/* SPIドライバー初期化処理 */
		taskSpiDriverInit();
		break;
	}
}
