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
	defisr	WWDG_IRQHandler                   /* Window WatchDog              */
	defisr	PVD_VDDIO2_IRQHandler             /* PVD through EXTI Line detect */
	defisr	RTC_TAMP_IRQHandler               /* RTC through the EXTI line    */
	defisr	FLASH_IRQHandler                  /* FLASH                        */
	defisr	RCC_CRS_IRQHandler                /* RCC & CRS                    */
	defisr	EXTI0_1_IRQHandler                /* EXTI Line 0 and 1            */
	defisr	EXTI2_3_IRQHandler                /* EXTI Line 2 and 3            */
	defisr	EXTI4_15_IRQHandler               /* EXTI Line 4 to 15            */
	defisr	USB_UCPD1_2_IRQHandler            /* USB, UCPD1, UCPD2            */
	defisr	DMA1_Channel1_IRQHandler          /* DMA1 Channel 1               */
	defisr	DMA1_Channel2_3_IRQHandler        /* DMA1 Channel 2 and Channel 3 */
	defisr	DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler /* DMA1 Ch4 to Ch7, DMA2 Ch1 to Ch5, DMAMUX1 overrun */
	defisr	ADC1_COMP_IRQHandler              /* ADC1, COMP1 and COMP2         */
	defisr	TIM1_BRK_UP_TRG_COM_IRQHandler    /* TIM1 Break, Update, Trigger and Commutation */
	defisr	TIM1_CC_IRQHandler                /* TIM1 Capture Compare         */
	defisr	TIM2_IRQHandler                   /* TIM2                         */
	defisr	TIM3_TIM4_IRQHandler              /* TIM3, TIM4                   */
	defisr	TIM6_DAC_LPTIM1_IRQHandler        /* TIM6, DAC and LPTIM1         */
	defisr	TIM7_LPTIM2_IRQHandler            /* TIM7 and LPTIM2              */
	defisr	TIM14_IRQHandler                  /* TIM14                        */
	defisr	TIM15_IRQHandler                  /* TIM15                        */
	defisr	TIM16_FDCAN_IT0_IRQHandler        /* TIM16 & FDCAN1_IT0 & FDCAN2_IT0 */
	defisr	TIM17_FDCAN_IT1_IRQHandler        /* TIM17 & FDCAN1_IT1 & FDCAN2_IT1 */
	defisr	I2C1_IRQHandler                   /* I2C1                         */
	defisr	I2C2_3_IRQHandler                 /* I2C2, I2C3                   */
	defisr	SPI1_IRQHandler                   /* SPI1                         */
	defisr	SPI2_3_IRQHandler                 /* SPI2, SPI3                   */
	defisr	USART1_IRQHandler                 /* USART1                       */
	defisr	USART2_LPUART2_IRQHandler         /* USART2 & LPUART2             */
	defisr	USART3_4_5_6_LPUART1_IRQHandler   /* USART3, USART4, USART5, USART6, LPUART1   */
	defisr	CEC_IRQHandler                    /* CEC                          */

@ Default_Handler
.text
.type Default_Handler, %function
.global Default_Handler
Default_Handler:
	bkpt
	b.n Default_Handler
