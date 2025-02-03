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
#define TX_QUEUE_SIZE		(128)			/* UART送信Queueサイズ			*/
#define RX_QUEUE_SIZE		(128)			/* UART受信Queueサイズ			*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile static uint8_t u8s_UartTxBuffer[TX_QUEUE_SIZE];	/* UART送信Queueデータ			*/
volatile static uint8_t u8s_UartRxBuffer[RX_QUEUE_SIZE];	/* UART受信Queueデータ			*/
volatile static QueueControl sts_UartTxQueue;				/* UART送信Queue情報			*/
volatile static QueueControl sts_UartRxQueue;				/* UART受信Queue情報			*/

/* Private function prototypes -----------------------------------------------*/
static uint8_t setUartTxQueue(const uint8_t u8_Data);		/* UART送信Queueに登録する				*/
static uint8_t getUartTxQueue(uint8_t *pu8_Data);			/* UART送信Queueから取得する			*/
static uint8_t setUartRxQueue(const uint8_t u8_Data);		/* UART受信Queueに登録する				*/
static uint8_t getUartRxQueue(uint8_t *pu8_Data);			/* UART受信Queueから取得する			*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  USART受信コールバック関数
  * @param  None
  * @retval None
  */
void USART_CharReception_Callback(void)
{
	uint8_t u8_RxData;

	/* Read Received character. RXNE flag is cleared by reading of RDR register */
	u8_RxData = LL_USART_ReceiveData8(USART3);
	/* UART受信Queueに登録する */
	setUartRxQueue(u8_RxData);
}

/**
  * @brief  USART送信Enptyコールバック関数
  * @param  None
  * @retval None
  */
void USART_TXEmpty_Callback(void)
{
	uint8_t u8_TxData = 0;

	/* UART送信Queueから取得する */
	if (getUartTxQueue(&u8_TxData)) {
		/* Fill TDR with a new char */
		LL_USART_TransmitData8(USART3, u8_TxData);
	}
	else {
		/* Disable TXE interrupt */
		LL_USART_DisableIT_TXE_TXFNF(USART3);
		/* Enable TC interrupt */
		LL_USART_EnableIT_TC(USART3);
	}
}

/**
  * @brief  USART送信完了コールバック関数
  * @param  None
  * @retval None
  */
void USART_CharTransmitComplete_Callback(void)
{
	/* Disable TC interrupt */
	LL_USART_DisableIT_TC(USART3);
}

/**
  * @brief  UARTドライバー初期化処理
  * @param  None
  * @retval None
  */
void taskUartDriverInit(void)
{
	mem_set08((uint8_t *)&u8s_UartTxBuffer[0], 0x00, TX_QUEUE_SIZE);
	mem_set08((uint8_t *)&u8s_UartRxBuffer[0], 0x00, RX_QUEUE_SIZE);
	mem_set08((uint8_t *)&sts_UartTxQueue, 0x00, sizeof(sts_UartTxQueue));
	mem_set08((uint8_t *)&sts_UartRxQueue, 0x00, sizeof(sts_UartRxQueue));

	/* Enable RXNE and Error interrupts */
	LL_USART_EnableIT_RXNE_RXFNE(USART3);
	LL_USART_EnableIT_ERROR(USART3);
}

/**
  * @brief  UARTドライバー入力処理
  * @param  None
  * @retval None
  */
void taskUartDriverInput(void)
{
	/* 処理なし */
}

/**
  * @brief  UARTドライバー出力処理
  * @param  None
  * @retval None
  */
void taskUartDriverOutput(void)
{
	/* UART送信Queueデータが存在し、かつUSART送信Enpty割り込みが無効な場合 */
	if ((sts_UartTxQueue.u16_count > 0) && !LL_USART_IsEnabledIT_TXE_TXFNF(USART3)) {
		/* Enable TXE interrupt */
		LL_USART_EnableIT_TXE_TXFNF(USART3);
	}
}

/**
  * @brief  UART送信データを登録する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 登録した数
  */
uint16_t uartSetTxData(const uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t RetValue = 0;

	while (u16_Size > 0) {
		/* UART送信Queueに登録する */
		if (setUartTxQueue(pu8_Data[RetValue]) != OK) {
			break;
		}
		RetValue++;
		u16_Size--;
	}
	return RetValue;
}

/**
  * @brief  UART受信データを取得する
  * @param  pu8_Data: データのポインタ
  * @param  u16_Size: データのサイズ
  * @retval 取得した数
  */
uint16_t uartGetRxData(uint8_t *pu8_Data, uint16_t u16_Size)
{
	uint16_t RetValue = 0;

	while (u16_Size > 0) {
		/* UART受信Queueから取得する */
		if (getUartRxQueue(&pu8_Data[RetValue]) != OK) {
			break;
		}
		RetValue++;
		u16_Size--;
	}
	return RetValue;
}

/**
  * @brief  UART受信データの数を取得する
  * @param  None
  * @retval データの数
  */
uint16_t uartGetRxCount(void)
{
	/* UART受信Queueデータの登録数 */
	return sts_UartRxQueue.u16_count;
}

/**
  * @brief  Hex1Byte表示処理
  * @param  u8_Data: データ
  * @retval None
  */
void uartEchoHex8(uint8_t u8_Data) {
	const uint8_t HexTable[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};
	setUartTxQueue(HexTable[(u8_Data >> 4) & 0x0F]);
	setUartTxQueue(HexTable[u8_Data & 0x0F]);
}

/**
  * @brief  Hex2Byte表示処理
  * @param  u16_Data: データ
  * @retval None
  */
void uartEchoHex16(uint16_t u16_Data) {
	uartEchoHex8((u16_Data >> 8) & 0xFF);
	uartEchoHex8(u16_Data & 0xFF);
}

/**
  * @brief  Hex4Byte表示処理
  * @param  u32_Data: データ
  * @retval None
  */
void uartEchoHex32(uint32_t u32_Data) {
	uartEchoHex16((u32_Data >> 16) & 0xFFFF);
	uartEchoHex16(u32_Data & 0xFFFF);
}

/**
  * @brief  文字列表示処理
  * @param  pu8_Data: データのポインタ
  * @retval None
  */
void uartEchoStr(const char *ps8_Data) {
	while (*ps8_Data != 0x00) {
		setUartTxQueue(*ps8_Data);
		ps8_Data++;
	}
}

/**
  * @brief  文字列表示処理(改行付き)
  * @param  pu8_Data: データのポインタ
  * @retval None
  */
void uartEchoStrln(const char *ps8_Data) {
	uartEchoStr(ps8_Data);
	uartEchoStr("\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  UART送信Queueに登録する
  * @param  u8_Data: データ
  * @retval OK/NG
  */
static uint8_t setUartTxQueue(const uint8_t u8_Data)
{
	uint8_t u8_RetCode = NG;

	/* 上限を超えるQueueデータの登録は破棄する */
	if (sts_UartTxQueue.u16_count < TX_QUEUE_SIZE) {
		/* Disable Interrupts */
		__disable_irq();
		u8s_UartTxBuffer[sts_UartTxQueue.u16_head] = u8_Data;
		sts_UartTxQueue.u16_head = (sts_UartTxQueue.u16_head + 1) % TX_QUEUE_SIZE;
		sts_UartTxQueue.u16_count++;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  UART送信Queueから取得する
  * @param  pu8_Data: データのポインタ
  * @retval OK/NG
  */
static uint8_t getUartTxQueue(uint8_t *pu8_Data)
{
	uint8_t u8_RetCode = NG;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_UartTxQueue.u16_count > 0) {
		/* Disable Interrupts */
		__disable_irq();
		*pu8_Data = u8s_UartTxBuffer[sts_UartTxQueue.u16_tail];
		sts_UartTxQueue.u16_tail = (sts_UartTxQueue.u16_tail + 1) % TX_QUEUE_SIZE;
		sts_UartTxQueue.u16_count--;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  UART受信Queueに登録する
  * @param  u8_Data: データ
  * @retval OK/NG
  */
static uint8_t setUartRxQueue(const uint8_t u8_Data)
{
	/* Disable Interrupts */
	__disable_irq();
	/* 上限を超えるQueueデータの登録は上書きする */
	u8s_UartRxBuffer[sts_UartRxQueue.u16_head] = u8_Data;
	sts_UartRxQueue.u16_head = (sts_UartRxQueue.u16_head + 1) % RX_QUEUE_SIZE;
	sts_UartRxQueue.u16_count++;
	/* Queueデータの上書きが起きる場合 */
	if (sts_UartRxQueue.u16_count > RX_QUEUE_SIZE) {
		sts_UartRxQueue.u16_count = RX_QUEUE_SIZE;
		sts_UartRxQueue.u16_tail = (sts_UartRxQueue.u16_tail + 1) % RX_QUEUE_SIZE;
	}
	/* Enable Interrupts */
	__enable_irq();

	return OK;
}

/**
  * @brief  UART受信Queueから取得する
  * @param  pu8_Data: データのポインタ
  * @retval OK/NG
  */
static uint8_t getUartRxQueue(uint8_t *pu8_Data)
{
	uint8_t u8_RetCode = NG;

	/* 登録済のQueueデータが存在する場合 */
	if (sts_UartRxQueue.u16_count > 0) {
		/* Disable Interrupts */
		__disable_irq();
		*pu8_Data = u8s_UartRxBuffer[sts_UartRxQueue.u16_tail];
		sts_UartRxQueue.u16_tail = (sts_UartRxQueue.u16_tail + 1) % RX_QUEUE_SIZE;
		sts_UartRxQueue.u16_count--;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}
