/**
  ******************************************************************************
  * @file           : drv_uart.c
  * @brief          : UARTドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"
#include "lib.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TX_QUEUE_SIZE		(64)			/* UART送信Queueサイズ			*/
#define RX_QUEUE_SIZE		(64)			/* UART受信バッファサイズ		*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile static uint8_t u8s_UartTxBuffer[TX_QUEUE_SIZE];		/* UART送信Queueデータ			*/
volatile static uint8_t u8s_UartRxBuffer[RX_QUEUE_SIZE];		/* UART受信Queueデータ			*/
volatile static QueueControl sts_UartTxQueue;					/* UART送信Queue情報			*/
volatile static QueueControl sts_UartRxQueue;					/* UART受信Queue情報			*/
volatile static uint8_t u8s_HalTxBuffer[TX_QUEUE_SIZE];			/* HAL用UART送信バッファ		*/
volatile static uint8_t u8s_HalRxBuffer[RX_QUEUE_SIZE];			/* HAL用UART受信バッファ		*/
volatile static uint8_t u8s_TxActiveState;						/* UART送信アクティブ状態		*/
volatile static uint8_t u8s_RxActiveState;						/* UART受信アクティブ状態		*/

/* Private function prototypes -----------------------------------------------*/
static uint16_t setUartTxQueue(const uint8_t *pu8_Data, uint16_t u16_Size);	/* UART送信Queueに登録する				*/
static uint16_t getUartTxQueue(uint8_t *pu8_Data, uint16_t u16_Size);		/* UART送信Queueから取得する			*/
static uint16_t setUartRxQueue(const uint8_t *pu8_Data, uint16_t u16_Size);	/* UART受信Queueに登録する				*/
static uint16_t getUartRxQueue(uint8_t *pu8_Data, uint16_t u16_Size);		/* UART受信Queueから取得する			*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief Rx Transfer completed callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		/* UART受信Queueに登録する */
		setUartRxQueue((uint8_t *)&u8s_HalRxBuffer[0], UART_RX_BLOCK_SIZE);
		u8s_RxActiveState = OFF;
	}
}

/**
  * @brief Tx Transfer completed callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2) {
		u8s_TxActiveState = OFF;
	}
}

/**
  * @brief UART error callback.
  * @param huart UART handle.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	/* Error */
	Error_Handler();
}

/**
  * @brief UARTドライバー初期化処理
  * @param None
  * @retval None
  */
void taskUartDriverInit(void)
{
	mem_set08((uint8_t *)&u8s_UartTxBuffer[0], 0x00, TX_QUEUE_SIZE);
	mem_set08((uint8_t *)&u8s_UartRxBuffer[0], 0x00, RX_QUEUE_SIZE);
	mem_set08((uint8_t *)&sts_UartTxQueue, 0x00, sizeof(sts_UartTxQueue));
	mem_set08((uint8_t *)&sts_UartRxQueue, 0x00, sizeof(sts_UartRxQueue));
	mem_set08((uint8_t *)&u8s_HalTxBuffer[0], 0x00, TX_QUEUE_SIZE);
	mem_set08((uint8_t *)&u8s_HalRxBuffer[0], 0x00, TX_QUEUE_SIZE);
	u8s_TxActiveState = OFF;
	u8s_RxActiveState = OFF;
}

/**
  * @brief UARTドライバー入力処理
  * @param None
  * @retval None
  */
void taskUartDriverInput(void)
{
	/* UART受信アクティブ状態がOFFの場合 */
	if (u8s_RxActiveState == OFF) {
		/* UART受信(割り込み)を開始 */
		if (HAL_UART_Receive_IT(&huart2, (uint8_t *)&u8s_HalRxBuffer[0], UART_RX_BLOCK_SIZE) != HAL_OK) {
			/* Reception Error */
			Error_Handler();
		}
		u8s_RxActiveState = ON;
	}
}

/**
  * @brief UARTドライバー出力処理
  * @param None
  * @retval None
  */
void taskUartDriverOutput(void)
{
	uint16_t u16_TxSize;

	/* UART送信アクティブ状態がOFFの場合 */
	if (u8s_TxActiveState == OFF) {
		/* UART送信Queueから取得する */
		u16_TxSize = getUartTxQueue((uint8_t *)&u8s_HalTxBuffer[0], TX_QUEUE_SIZE);
		if (u16_TxSize > 0) {
			/* UART送信(割り込み)を開始 */
			if (HAL_UART_Transmit_IT(&huart2, (uint8_t *)&u8s_HalTxBuffer[0], u16_TxSize) != HAL_OK) {
				/* Transmission Error */
				Error_Handler();
			}
			u8s_TxActiveState = ON;
		}
	}
}

/**
  * @brief UART送信データを登録する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval 登録した数
  */
uint16_t uartSetTxData(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	/* UART送信Queueに登録する */
	return setUartTxQueue(pu8_Data, u16_Size);
}

/**
  * @brief UART受信データを取得する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval 取得した数
  */
uint16_t uartGetRxData(uint8_t *pu8_Data, uint16_t u16_Size)
{
	/* UART受信Queueから取得する */
	return getUartRxQueue(pu8_Data, u16_Size);
}

/**
  * @brief UART受信データの数を取得する
  * @param None
  * @retval データの数
  */
uint16_t uartGetRxCount(void)
{
	/* UART受信Queueデータの登録数 */
	return sts_UartRxQueue.u16_count;
}

/**
  * @brief Hex1Byte表示処理
  * @param u8_Data データ
  * @retval None
  */
void uartEchoHex8(uint8_t u8_Data) {
	const uint8_t HexTable[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
	setUartTxQueue(&HexTable[(u8_Data >> 4) & 0x0F], 1);
	setUartTxQueue(&HexTable[u8_Data & 0x0F], 1);
}

/**
  * @brief Hex2Byte表示処理
  * @param u16_Data データ
  * @retval None
  */
void uartEchoHex16(uint16_t u16_Data) {
	uartEchoHex8((u16_Data >> 8) & 0xFF);
	uartEchoHex8(u16_Data & 0xFF);
}

/**
  * @brief Hex4Byte表示処理
  * @param u32_Data データ
  * @retval None
  */
void uartEchoHex32(uint32_t u32_Data) {
	uartEchoHex16((u32_Data >> 16) & 0xFFFF);
	uartEchoHex16(u32_Data & 0xFFFF);
}

/**
  * @brief 文字列表示処理
  * @param pu8_Data データのポインタ
  * @retval None
  */
void uartEchoStr(const char *ps8_Data) {
	while (*ps8_Data != 0x00) {
		setUartTxQueue((uint8_t *)ps8_Data, 1);
		ps8_Data++;
	}
}

/**
  * @brief 文字列表示処理(改行付き)
  * @param pu8_Data データのポインタ
  * @retval None
  */
void uartEchoStrln(const char *ps8_Data) {
	uartEchoStr(ps8_Data);
	uartEchoStr("\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief UART送信Queueに登録する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval 登録した数
  */
static uint16_t setUartTxQueue(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_RemainSize;
	uint16_t u16_RetValue = 0;

	/* 上限を超えるQueueデータの登録は破棄する */
	if ((sts_UartTxQueue.u16_count + u16_Size) <= TX_QUEUE_SIZE) {
		/* Disable Interrupts */
		__disable_irq();
		u16_RemainSize = TX_QUEUE_SIZE - sts_UartTxQueue.u16_head;
		/* バッファが循環する場合 */
		if (u16_Size > u16_RemainSize) {
			mem_cpy08((uint8_t *)&u8s_UartTxBuffer[sts_UartTxQueue.u16_head], &pu8_Data[0], u16_RemainSize);
			mem_cpy08((uint8_t *)&u8s_UartTxBuffer[0], &pu8_Data[u16_RemainSize], u16_Size - u16_RemainSize);
		}
		/* 上記以外 */
		else {
			mem_cpy08((uint8_t *)&u8s_UartTxBuffer[sts_UartTxQueue.u16_head], &pu8_Data[0], u16_Size);
		}
		sts_UartTxQueue.u16_head = (sts_UartTxQueue.u16_head + u16_Size) % TX_QUEUE_SIZE;
		sts_UartTxQueue.u16_count += u16_Size;
		/* Enable Interrupts */
		__enable_irq();
		u16_RetValue = u16_Size;
	}
	return u16_RetValue;
}

/**
  * @brief UART送信Queueから取得する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval 取得した数
  */
static uint16_t getUartTxQueue(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_RemainSize;
	uint16_t u16_GetDataSize = 0;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_UartTxQueue.u16_count > 0) {
		u16_GetDataSize = (u16_Size <= sts_UartTxQueue.u16_count) ? u16_Size : sts_UartTxQueue.u16_count;
		u16_RemainSize = TX_QUEUE_SIZE - sts_UartTxQueue.u16_tail;
		/* バッファが循環する場合 */
		if (u16_GetDataSize > u16_RemainSize) {
			mem_cpy08(&pu8_Data[0], (uint8_t *)&u8s_UartTxBuffer[sts_UartTxQueue.u16_tail], u16_RemainSize);
			mem_cpy08(&pu8_Data[u16_RemainSize], (uint8_t *)&u8s_UartTxBuffer[0], u16_GetDataSize - u16_RemainSize);
		}
		/* 上記以外 */
		else {
			mem_cpy08(&pu8_Data[0], (uint8_t *)&u8s_UartTxBuffer[sts_UartTxQueue.u16_tail], u16_GetDataSize);
		}
		sts_UartTxQueue.u16_tail = (sts_UartTxQueue.u16_tail + u16_GetDataSize) % TX_QUEUE_SIZE;
		sts_UartTxQueue.u16_count -= u16_GetDataSize;
	}
	return u16_GetDataSize;
}

/**
  * @brief UART受信Queueに登録する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval 登録した数
  */
static uint16_t setUartRxQueue(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_RemainSize;
	uint16_t u16_OverWriteSize;

	/* 上限を超えるQueueデータの登録は上書きする */
	u16_RemainSize = RX_QUEUE_SIZE - sts_UartRxQueue.u16_head;
	/* バッファが循環する場合 */
	if (u16_Size > u16_RemainSize) {
		mem_cpy08((uint8_t *)&u8s_UartRxBuffer[sts_UartRxQueue.u16_head], &pu8_Data[0], u16_RemainSize);
		mem_cpy08((uint8_t *)&u8s_UartRxBuffer[0], &pu8_Data[u16_RemainSize], u16_Size - u16_RemainSize);
	}
	/* 上記以外 */
	else {
		mem_cpy08((uint8_t *)&u8s_UartRxBuffer[sts_UartRxQueue.u16_head], &pu8_Data[0], u16_Size);
	}
	sts_UartRxQueue.u16_head = (sts_UartRxQueue.u16_head + u16_Size) % RX_QUEUE_SIZE;
	/* Queueデータの上書きが起きる場合 */
	if ((sts_UartRxQueue.u16_count + u16_Size) > RX_QUEUE_SIZE) {
		sts_UartRxQueue.u16_count = RX_QUEUE_SIZE;
		u16_OverWriteSize = (sts_UartRxQueue.u16_count + u16_Size) - RX_QUEUE_SIZE;
		sts_UartRxQueue.u16_tail = (sts_UartRxQueue.u16_tail + u16_OverWriteSize) % RX_QUEUE_SIZE;
	}
	/* 上記以外 */
	else {
		sts_UartRxQueue.u16_count += u16_Size;
	}
	return u16_Size;
}

/**
  * @brief UART受信Queueから取得する
  * @param pu8_Data データのポインタ
  * @param u16_Size データのサイズ
  * @retval 取得した数
  */
static uint16_t getUartRxQueue(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t u16_RemainSize;
	uint16_t u16_GetDataSize = 0;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_UartRxQueue.u16_count > 0) {
		/* Disable Interrupts */
		__disable_irq();
		u16_GetDataSize = (u16_Size <= sts_UartRxQueue.u16_count) ? u16_Size : sts_UartRxQueue.u16_count;
		u16_RemainSize = RX_QUEUE_SIZE - sts_UartRxQueue.u16_tail;
		/* バッファが循環する場合 */
		if (u16_GetDataSize > u16_RemainSize) {
			mem_cpy08(&pu8_Data[0], (uint8_t *)&u8s_UartRxBuffer[sts_UartRxQueue.u16_tail], u16_RemainSize);
			mem_cpy08(&pu8_Data[u16_RemainSize], (uint8_t *)&u8s_UartRxBuffer[0], u16_GetDataSize - u16_RemainSize);
		}
		/* 上記以外 */
		else {
			mem_cpy08(&pu8_Data[0], (uint8_t *)&u8s_UartRxBuffer[sts_UartRxQueue.u16_tail], u16_GetDataSize);
		}
		sts_UartRxQueue.u16_tail = (sts_UartRxQueue.u16_tail + u16_GetDataSize) % RX_QUEUE_SIZE;
		sts_UartRxQueue.u16_count -= u16_GetDataSize;
		/* Enable Interrupts */
		__enable_irq();
	}
	return u16_GetDataSize;
}
