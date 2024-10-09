/**
  ******************************************************************************
  * @file           : drv_flash.c
  * @brief          : FLASHドライバー
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  FLASH end of operation interrupt callback.
  * @param  ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                  Mass Erase: 0
  *                  Page Erase: Page which has been erased
  *                  Program: Address which was selected for data program
  * @retval None
  */
void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
	/* 処理なし */
}

/**
  * @brief  FLASH operation error interrupt callback.
  * @param  ReturnValue The value saved in this parameter depends on the ongoing procedure
  *                 Mass Erase: 0
  *                 Page Erase: Page number which returned an error
  *                 Program: Address which was selected for data program
  * @retval None
  */
void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue)
{
	/* 処理なし */
}

/**
  * @brief FLASHドライバー初期化処理
  * @param None
  * @retval None
  */
void taskFlashDriverInit(void)
{
	/* 処理なし */
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
	/* 処理なし */
}

/* Private functions ---------------------------------------------------------*/

