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
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crs.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

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

/* size_t型 */
typedef long unsigned int size_t;

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* 割り込み用コールバック関数 */
extern void SYSTICK_PeriodElapsed_Callback(void);			/* SysTickタイマ経過コールバック関数	*/
extern void USART_CharReception_Callback(void);				/* USART受信コールバック関数			*/
extern void USART_TXEmpty_Callback(void);					/* USART送信Enptyコールバック関数		*/
extern void USART_CharTransmitComplete_Callback(void);		/* USART送信完了コールバック関数		*/

/* main_app.c */
extern void setup(void);									/* 初期化関数							*/
extern void loop(void);										/* 周期処理関数							*/

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define T_SWDIO_Pin LL_GPIO_PIN_13
#define T_SWDIO_GPIO_Port GPIOA
#define T_SWCLK_Pin LL_GPIO_PIN_14
#define T_SWCLK_GPIO_Port GPIOA
#define T_SWO_Pin LL_GPIO_PIN_3
#define T_SWO_GPIO_Port GPIOB
#define LD2_Pin LL_GPIO_PIN_8
#define LD2_GPIO_Port GPIOB
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
#endif

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
