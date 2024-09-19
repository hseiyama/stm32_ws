/**
  ******************************************************************************
  * @file      startup_stm32l4a6xx.s
  * @author    MCD Application Team
  * @brief     STM32L4A6xx devices vector table GCC toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address,
  *                - Configure the clock system  
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M4 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

  .syntax unified
	.cpu cortex-m4
	.fpu softvfp
	.thumb

.global	g_pfnVectors
.global	Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word	_sidata
/* start address for the .data section. defined in linker script */
.word	_sdata
/* end address for the .data section. defined in linker script */
.word	_edata
/* start address for the .bss section. defined in linker script */
.word	_sbss
/* end address for the .bss section. defined in linker script */
.word	_ebss

.equ  BootRAM,        0xF1E0F85F
/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

/* アセンブラ記述開始(定義) */
	.equ	DATA_INIT, 0x12345678	// データ初期値
	.equ	RCC_AHB2ENR, (0x40021000 + 0x4C)	// AHB2 ペリフェラルクロック有効レジスタ
	.equ	GPIOB_MODER, (0x48000400 + 0x00)	// GPIOB ポートモードレジスタ
	.equ	GPIOB_ODR,   (0x48000400 + 0x14)	// GPIOB ポート出力データレジスタ
	.equ	GPIOB_BSRR,  (0x48000400 + 0x18)	// GPIOB ポートビットセット/リセットレジスタ
	.equ	GPIOC_MODER, (0x48000800 + 0x00)	// GPIOC ポートモードレジスタ
	.equ	GPIOC_IDR,   (0x48000800 + 0x10)	// GPIOC ポート入力データレジスタ
/* アセンブラ記述終了(定義) */

/* アセンブラ記述開始(RAM領域) */
    .section	.data
data_3rd:							// データ増分値
	.word	7

    .section	.bss
data_2nd:							// データの転送先
	.space	4
/* アセンブラ記述終了(RAM領域) */

    .section	.text.Reset_Handler
	.weak	Reset_Handler
	.type	Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack    /* Set stack pointer */

/* アセンブラ記述開始(Reset_Handler) */
setup:
	// ポート操作
	ldr r0, =0x00000006
	ldr r1, =RCC_AHB2ENR
	str r0, [r1]					@ Set GPIOBEN,GPIOCEN bit in RCC_AHB2ENR to 1 to enable GPIOB
	ldr r1, =GPIOB_MODER
	ldr r0, [r1]
	ldr r2, =0xCFFF3FFF
	and r0, r2
	ldr r2, =0x10004000
	orr r0, r2
	str r0, [r1]					@ Set MODE7,MODE14 in GPIOB_MODER to 1 (default:0xFFFFFEBF)
	ldr r0, =0x00004080
	ldr r1, =GPIOB_ODR
	str r0, [r1]					@ Set OD7,OD14 in GPIOB_ODR to 1 to set PB7,PB14 high
	ldr r1, =GPIOC_MODER
	ldr r0, [r1]
	ldr r2, =0xF3FFFFFF
	and r0, r2
	str r0, [r1]					@ Set MODE13 in GPIOC_MODER to 1 (default:0xFFFFFFFF)
	// データ操作
	adr r0, data_1st				// 初期値のアドレスをロード
	ldr r1, [r0]					// データ初期値を取得
	ldr r2, =data_2nd				// 転送先のアドレスをロード
	mov r5, #7						// 増分値の初期値を設定
	ldr r0, =data_3rd				// 増分値のアドレスをロード
	str r5, [r0]					// データ増分値を取得
loop:
	// ポート操作
	ldr r3, =GPIOC_IDR
	ldr r4, [r3]
	ands r4, #0x00002000
	bne sw_on
	ldr r3, =0x40000000
	ldr r4, =GPIOB_BSRR
	str r3, [r4]					@ Set BR14 in GPIOB_BSRR to 1 to set PB14 low
	b next
sw_on:
	ldr r3, =0x00004000
	ldr r4, =GPIOB_BSRR
	str r3, [r4]					@ Set BS14 in GPIOB_BSRR to 1 to set PB14 high
next:
	// データ操作
	ldr r0, =data_3rd				// 増分値のアドレスをロード
	ldr r5, [r0]					// データ増分値を取得
	add r5, #1						// データ増分値を更新
	str r5, [r0]					// データをストア
	add r1, r5						// データを更新
	str r1, [r2]					// データをストア
	b loop							// loopへ分岐

data_1st:
	.word DATA_INIT					// データ初期値
/* アセンブラ記述終了(Reset_Handler) */

.size	Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 *
 * @param  None
 * @retval : None
*/
    .section	.text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
	b	Infinite_Loop
	.size	Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex-M4.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/
 	.section	.isr_vector,"a",%progbits
	.type	g_pfnVectors, %object
	.size	g_pfnVectors, .-g_pfnVectors


g_pfnVectors:
	.word	_estack
	.word	Reset_Handler
	.word	Default_Handler		// NMI_Handler
	.word	Default_Handler		// HardFault_Handler
	.word	Default_Handler		// MemManage_Handler
	.word	Default_Handler		// BusFault_Handler
	.word	Default_Handler		// UsageFault_Handler
	.word	0
	.word	0
	.word	0
	.word	0
	.word	Default_Handler		// SVC_Handler
	.word	Default_Handler		// DebugMon_Handler
	.word	0
	.word	Default_Handler		// PendSV_Handler
	.word	Default_Handler		// SysTick_Handler
	.word	Default_Handler		// WWDG_IRQHandler
	.word	Default_Handler		// PVD_PVM_IRQHandler
	.word	Default_Handler		// TAMP_STAMP_IRQHandler
	.word	Default_Handler		// RTC_WKUP_IRQHandler
	.word	Default_Handler		// FLASH_IRQHandler
	.word	Default_Handler		// RCC_IRQHandler
	.word	Default_Handler		// EXTI0_IRQHandler
	.word	Default_Handler		// EXTI1_IRQHandler
	.word	Default_Handler		// EXTI2_IRQHandler
	.word	Default_Handler		// EXTI3_IRQHandler
	.word	Default_Handler		// EXTI4_IRQHandler
	.word	Default_Handler		// DMA1_Channel1_IRQHandler
	.word	Default_Handler		// DMA1_Channel2_IRQHandler
	.word	Default_Handler		// DMA1_Channel3_IRQHandler
	.word	Default_Handler		// DMA1_Channel4_IRQHandler
	.word	Default_Handler		// DMA1_Channel5_IRQHandler
	.word	Default_Handler		// DMA1_Channel6_IRQHandler
	.word	Default_Handler		// DMA1_Channel7_IRQHandler
	.word	Default_Handler		// ADC1_2_IRQHandler
	.word	Default_Handler		// CAN1_TX_IRQHandler
	.word	Default_Handler		// CAN1_RX0_IRQHandler
	.word	Default_Handler		// CAN1_RX1_IRQHandler
	.word	Default_Handler		// CAN1_SCE_IRQHandler
	.word	Default_Handler		// EXTI9_5_IRQHandler
	.word	Default_Handler		// TIM1_BRK_TIM15_IRQHandler
	.word	Default_Handler		// TIM1_UP_TIM16_IRQHandler
	.word	Default_Handler		// TIM1_TRG_COM_TIM17_IRQHandler
	.word	Default_Handler		// TIM1_CC_IRQHandler
	.word	Default_Handler		// TIM2_IRQHandler
	.word	Default_Handler		// TIM3_IRQHandler
	.word	Default_Handler		// TIM4_IRQHandler
	.word	Default_Handler		// I2C1_EV_IRQHandler
	.word	Default_Handler		// I2C1_ER_IRQHandler
	.word	Default_Handler		// I2C2_EV_IRQHandler
	.word	Default_Handler		// I2C2_ER_IRQHandler
	.word	Default_Handler		// SPI1_IRQHandler
	.word	Default_Handler		// SPI2_IRQHandler
	.word	Default_Handler		// USART1_IRQHandler
	.word	Default_Handler		// USART2_IRQHandler
	.word	Default_Handler		// USART3_IRQHandler
	.word	Default_Handler		// EXTI15_10_IRQHandler
	.word	Default_Handler		// RTC_Alarm_IRQHandler
	.word	Default_Handler		// DFSDM1_FLT3_IRQHandler
	.word	Default_Handler		// TIM8_BRK_IRQHandler
	.word	Default_Handler		// TIM8_UP_IRQHandler
	.word	Default_Handler		// TIM8_TRG_COM_IRQHandler
	.word	Default_Handler		// TIM8_CC_IRQHandler
	.word	Default_Handler		// ADC3_IRQHandler
	.word	Default_Handler		// FMC_IRQHandler
	.word	Default_Handler		// SDMMC1_IRQHandler
	.word	Default_Handler		// TIM5_IRQHandler
	.word	Default_Handler		// SPI3_IRQHandler
	.word	Default_Handler		// UART4_IRQHandler
	.word	Default_Handler		// UART5_IRQHandler
	.word	Default_Handler		// TIM6_DAC_IRQHandler
	.word	Default_Handler		// TIM7_IRQHandler
	.word	Default_Handler		// DMA2_Channel1_IRQHandler
	.word	Default_Handler		// DMA2_Channel2_IRQHandler
	.word	Default_Handler		// DMA2_Channel3_IRQHandler
	.word	Default_Handler		// DMA2_Channel4_IRQHandler
	.word	Default_Handler		// DMA2_Channel5_IRQHandler
	.word	Default_Handler		// DFSDM1_FLT0_IRQHandler
	.word	Default_Handler		// DFSDM1_FLT1_IRQHandler
	.word	Default_Handler		// DFSDM1_FLT2_IRQHandler
	.word	Default_Handler		// COMP_IRQHandler
	.word	Default_Handler		// LPTIM1_IRQHandler
	.word	Default_Handler		// LPTIM2_IRQHandler
	.word	Default_Handler		// OTG_FS_IRQHandler
	.word	Default_Handler		// DMA2_Channel6_IRQHandler
	.word	Default_Handler		// DMA2_Channel7_IRQHandler
	.word	Default_Handler		// LPUART1_IRQHandler
	.word	Default_Handler		// QUADSPI_IRQHandler
	.word	Default_Handler		// I2C3_EV_IRQHandler
	.word	Default_Handler		// I2C3_ER_IRQHandler
	.word	Default_Handler		// SAI1_IRQHandler
	.word	Default_Handler		// SAI2_IRQHandler
	.word	Default_Handler		// SWPMI1_IRQHandler
	.word	Default_Handler		// TSC_IRQHandler
	.word	Default_Handler		// LCD_IRQHandler
	.word	Default_Handler		// AES_IRQHandler
	.word	Default_Handler		// HASH_RNG_IRQHandler
	.word	Default_Handler		// FPU_IRQHandler
	.word	Default_Handler		// CRS_IRQHandler
	.word	Default_Handler		// I2C4_EV_IRQHandler
	.word	Default_Handler		// I2C4_ER_IRQHandler
	.word	Default_Handler		// DCMI_IRQHandler
	.word	Default_Handler		// CAN2_TX_IRQHandler
	.word	Default_Handler		// CAN2_RX0_IRQHandler
	.word	Default_Handler		// CAN2_RX1_IRQHandler
	.word	Default_Handler		// CAN2_SCE_IRQHandler
	.word	Default_Handler		// DMA2D_IRQHandler

