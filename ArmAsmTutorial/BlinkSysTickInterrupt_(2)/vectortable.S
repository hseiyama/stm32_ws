.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

@ Define a handler ISR macro
.macro defisr name
	.global \name
	.weak \name
	.thumb_set \name, Default_Handler
	.word \name
.endm

@ The minimal vector table for a Cortex-M4
.section .VectorTable, "a"
.type VectorTable, %object
.global VectorTable
VectorTable:
	.word	_StackEnd
	defisr	Reset_Handler
	defisr	NMI_Handler
	defisr	HardFault_Handler
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	defisr	SVC_Handler
	.word	0
	.word	0
	defisr	PendSV_Handler
	defisr	SysTick_Handler
	defisr	WWDG_IRQHandler                   /* Window WatchDog                             */
	defisr	PVD_VDDIO2_IRQHandler             /* PVD through EXTI Line detect                */
	defisr	RTC_IRQHandler                    /* RTC through the EXTI line                   */
	defisr	FLASH_IRQHandler                  /* FLASH                                       */
	defisr	RCC_CRS_IRQHandler                /* RCC, CRS                                    */
	defisr	EXTI0_1_IRQHandler                /* EXTI Line 0 and 1                           */
	defisr	EXTI2_3_IRQHandler                /* EXTI Line 2 and 3                           */
	defisr	EXTI4_15_IRQHandler               /* EXTI Line 4 to 15                           */
	defisr	USB_DRD_FS_IRQHandler             /* USB Dual Role                               */
	defisr	DMA1_Channel1_IRQHandler          /* DMA1 Channel 1                              */
	defisr	DMA1_Channel2_3_IRQHandler        /* DMA1 Channel 2 and Channel 3                */
	defisr	DMAMUX1_DMA1_CH4_5_IRQHandler     /* DMAMUX1, DMA1 Channel 4 and 5               */
	defisr	ADC1_IRQHandler                   /* ADC1                                        */
	defisr	TIM1_BRK_UP_TRG_COM_IRQHandler    /* TIM1 Break, Update, Trigger and Commutation */
	defisr	TIM1_CC_IRQHandler                /* TIM1 Capture Compare                        */
	defisr	TIM2_IRQHandler                   /* TIM2                                        */
	defisr	TIM3_IRQHandler                   /* TIM3                                        */
	.word	0                                 /* reserved                                    */
	.word	0                                 /* reserved                                    */
	defisr	TIM14_IRQHandler                  /* TIM14                                       */
	.word	0                                 /* reserved                                    */
	defisr	TIM16_IRQHandler                  /* TIM16                                       */
	defisr	TIM17_IRQHandler                  /* TIM17                                       */
	defisr	I2C1_IRQHandler                   /* I2C1                                        */
	defisr	I2C2_IRQHandler                   /* I2C2                                        */
	defisr	SPI1_IRQHandler                   /* SPI1                                        */
	defisr	SPI2_IRQHandler                   /* SPI2                                        */
	defisr	USART1_IRQHandler                 /* USART1                                      */
	defisr	USART2_IRQHandler                 /* USART2                                      */
	.word	0                                 /* reserved                                    */
	.word	0                                 /* reserved                                    */
	.word	0                                 /* reserved                                    */

@ Default_Handler
.text
.type Default_Handler, %function
.global Default_Handler
Default_Handler:
	bkpt
	b.n Default_Handler
