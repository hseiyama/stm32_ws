/**
  ******************************************************************************
  * @file           : drv_i2c.c
  * @brief          : I2Cドライバー
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
  * @brief  Master Tx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	/* 処理なし */
}

/**
  * @brief  Master Rx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	/* 処理なし */
}

/**
  * @brief  I2C error callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	/* Error */
	Error_Handler();
}

/**
  * @brief I2Cドライバー初期化処理
  * @param None
  * @retval None
  */
void taskI2cDriverInit(void)
{
	/* 処理なし */
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
	/* 処理なし */
}

/* Private functions ---------------------------------------------------------*/

