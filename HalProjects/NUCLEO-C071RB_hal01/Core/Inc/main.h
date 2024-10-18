/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32c0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* boolean型 */
typedef enum _bool {
	false = 0,						/* false(偽)					*/
	true = 1						/* true(真)						*/
} bool;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* ON/OFF定義 */
#define OFF					(0)
#define ON					(1)

/* OK/NG定義 */
#define NG					(0)
#define OK					(1)

/* LOW/HIGH定義 */
#define LOW					(0)
#define HIGH				(1)

#define SYS_CYCLE_TIME		(5)		/* システムの周期時間[ms]		*/

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* main.c */
extern void sleep(void);			/* スリープ移行関数				*/

/* main_app.c */
extern void setup(void);			/* 初期化関数					*/
extern void loop(void);				/* 周期処理関数					*/

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PWM_PERIOD 1000
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define LD1_Pin GPIO_PIN_5
#define LD1_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_9
#define LD2_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
