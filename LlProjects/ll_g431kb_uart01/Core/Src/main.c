/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* キュー制御情報 */
typedef struct _QueueControl {
	uint16_t u16_head;				/* 先頭データのインデックス				*/
	uint16_t u16_tail;				/* 末尾データのインデックス				*/
	uint16_t u16_count;				/* データの登録数						*/
} QueueControl;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* OK/NG定義 */
#define NG					(0)
#define OK					(1)

#define TX_QUEUE_SIZE		(64)			/* UART送信Queueサイズ			*/
#define RX_QUEUE_SIZE		(64)			/* UART受信バッファサイズ		*/

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile static uint8_t u8s_UartTxBuffer[TX_QUEUE_SIZE];	/* UART送信Queueデータ			*/
volatile static uint8_t u8s_UartRxBuffer[RX_QUEUE_SIZE];	/* UART受信Queueデータ			*/
volatile static QueueControl sts_UartTxQueue;				/* UART送信Queue情報			*/
volatile static QueueControl sts_UartRxQueue;				/* UART受信Queue情報			*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static uint8_t setUartTxQueue(const uint8_t u8_Data);		/* UART送信Queueに登録する				*/
static uint8_t getUartTxQueue(uint8_t *pu8_Data);			/* UART送信Queueから取得する			*/
static uint8_t setUartRxQueue(const uint8_t u8_Data);		/* UART受信Queueに登録する				*/
static uint8_t getUartRxQueue(uint8_t *pu8_Data);			/* UART受信Queueから取得する			*/
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  SysTick割り込みコールバック関数
  * @param  None
  * @retval None
  */
void SysTick_Init_Callback(void)
{
	uint8_t u8_RxData;

	/* UART受信Queueから取得する */
	while (getUartRxQueue(&u8_RxData)) {
		setUartTxQueue(u8_RxData);
	}
	/* UART送信Queueデータが存在し、かつUSART送信Enpty割り込みが無効な場合 */
	if ((sts_UartTxQueue.u16_count > 0) && !LL_USART_IsEnabledIT_TXE_TXFNF(USART2)) {
		/* Enable TXE interrupt */
		LL_USART_EnableIT_TXE_TXFNF(USART2);
	}
}

/**
  * @brief  USART受信コールバック関数
  * @param  None
  * @retval None
  */
void USART_CharReception_Callback(void)
{
	uint8_t u8_RxData;

	/* Read Received character. RXNE flag is cleared by reading of RDR register */
	u8_RxData = LL_USART_ReceiveData8(USART2);
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
		LL_USART_TransmitData8(USART2, u8_TxData);
	}
	else {
		/* Disable TXE interrupt */
		LL_USART_DisableIT_TXE_TXFNF(USART2);
		/* Enable TC interrupt */
		LL_USART_EnableIT_TC(USART2);
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
	LL_USART_DisableIT_TC(USART2);
	/* LD2を反転出力する */
	LL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

/**
  * @brief  Hex1Byte表示処理
  * @param  u8_Data データ
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
  * @param  u16_Data データ
  * @retval None
  */
void uartEchoHex16(uint16_t u16_Data) {
	uartEchoHex8((u16_Data >> 8) & 0xFF);
	uartEchoHex8(u16_Data & 0xFF);
}

/**
  * @brief  Hex4Byte表示処理
  * @param  u32_Data データ
  * @retval None
  */
void uartEchoHex32(uint32_t u32_Data) {
	uartEchoHex16((u32_Data >> 16) & 0xFFFF);
	uartEchoHex16(u32_Data & 0xFFFF);
}

/**
  * @brief  文字列表示処理
  * @param  pu8_Data データのポインタ
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
  * @param  pu8_Data データのポインタ
  * @retval None
  */
void uartEchoStrln(const char *ps8_Data) {
	uartEchoStr(ps8_Data);
	uartEchoStr("\r\n");
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /** Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
  */
  LL_PWR_DisableUCPDDeadBattery();

  /* USER CODE BEGIN Init */
	sts_UartTxQueue.u16_head = 0;
	sts_UartTxQueue.u16_tail = 0;
	sts_UartTxQueue.u16_count = 0;
	sts_UartRxQueue.u16_head = 0;
	sts_UartRxQueue.u16_tail = 0;
	sts_UartRxQueue.u16_count = 0;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	LL_SYSTICK_EnableIT();
	/* Enable RXNE and Error interrupts */
	LL_USART_EnableIT_RXNE_RXFNE(USART2);
	LL_USART_EnableIT_ERROR(USART2);
	/* 開始メッセージ */
	uartEchoStrln("Start ll_g431kb_uart01 !!");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		/* LD2を反転出力する */
		LL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		/* 1秒ウェイト */
		LL_mDelay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4)
  {
  }
  LL_PWR_EnableRange1BoostMode();
  LL_RCC_HSI_Enable();
   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_4, 85, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_EnableDomain_SYS();
  LL_RCC_PLL_Enable();
   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);
   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Insure 1us transition state at intermediate medium speed clock*/
  for (__IO uint32_t i = (170 >> 1); i !=0; i--);

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  LL_Init1msTick(170000000);

  LL_SetSystemCoreClock(170000000);
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  /**USART2 GPIO Configuration
  PA2   ------> USART2_TX
  PA3   ------> USART2_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_SetTXFIFOThreshold(USART2, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_SetRXFIFOThreshold(USART2, LL_USART_FIFOTHRESHOLD_1_8);
  LL_USART_EnableFIFO(USART2);
  LL_USART_ConfigAsyncMode(USART2);

  /* USER CODE BEGIN WKUPType USART2 */

  /* USER CODE END WKUPType USART2 */

  LL_USART_Enable(USART2);

  /* Polling USART2 initialisation */
  while((!(LL_USART_IsActiveFlag_TEACK(USART2))) || (!(LL_USART_IsActiveFlag_REACK(USART2))))
  {
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_ResetOutputPin(LD2_GPIO_Port, LD2_Pin);

  /**/
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  UART送信Queueに登録する
  * @param  u8_Data データ
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
		sts_UartTxQueue.u16_count ++;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  UART送信Queueから取得する
  * @param  pu8_Data データのポインタ
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
		sts_UartTxQueue.u16_count --;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/**
  * @brief  UART受信Queueに登録する
  * @param  u8_Data データ
  * @retval OK/NG
  */
static uint8_t setUartRxQueue(const uint8_t u8_Data)
{
	/* Disable Interrupts */
	__disable_irq();
	/* 上限を超えるQueueデータの登録は上書きする */
	u8s_UartRxBuffer[sts_UartRxQueue.u16_head] = u8_Data;
	sts_UartRxQueue.u16_head = (sts_UartRxQueue.u16_head + 1) % RX_QUEUE_SIZE;
	sts_UartRxQueue.u16_count ++;
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
  * @param  pu8_Data データのポインタ
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
		sts_UartRxQueue.u16_count --;
		/* Enable Interrupts */
		__enable_irq();
		u8_RetCode = OK;
	}
	return u8_RetCode;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
