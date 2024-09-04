.syntax unified
.cpu cortex-m4
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
	defisr	MemManage_Handler
	defisr	BusFault_Handler
	defisr	UsageFault_Handler
	.word	0
	.word	0
	.word	0
	.word	0
	defisr	SVC_Handler
	defisr	DebugMon_Handler
	.word	0
	defisr	PendSV_Handler
	defisr	SysTick_Handler
	defisr	WWDG_IRQHandler
	defisr	PVD_PVM_IRQHandler
	defisr	TAMP_STAMP_IRQHandler
	defisr	RTC_WKUP_IRQHandler
	defisr	FLASH_IRQHandler
	defisr	RCC_IRQHandler
	defisr	EXTI0_IRQHandler
	defisr	EXTI1_IRQHandler
	defisr	EXTI2_IRQHandler
	defisr	EXTI3_IRQHandler
	defisr	EXTI4_IRQHandler
	defisr	DMA1_Channel1_IRQHandler
	defisr	DMA1_Channel2_IRQHandler
	defisr	DMA1_Channel3_IRQHandler
	defisr	DMA1_Channel4_IRQHandler
	defisr	DMA1_Channel5_IRQHandler
	defisr	DMA1_Channel6_IRQHandler
	defisr	DMA1_Channel7_IRQHandler
	defisr	ADC1_2_IRQHandler
	defisr	CAN1_TX_IRQHandler
	defisr	CAN1_RX0_IRQHandler
	defisr	CAN1_RX1_IRQHandler
	defisr	CAN1_SCE_IRQHandler
	defisr	EXTI9_5_IRQHandler
	defisr	TIM1_BRK_TIM15_IRQHandler
	defisr	TIM1_UP_TIM16_IRQHandler
	defisr	TIM1_TRG_COM_TIM17_IRQHandler
	defisr	TIM1_CC_IRQHandler
	defisr	TIM2_IRQHandler
	defisr	TIM3_IRQHandler
	defisr	TIM4_IRQHandler
	defisr	I2C1_EV_IRQHandler
	defisr	I2C1_ER_IRQHandler
	defisr	I2C2_EV_IRQHandler
	defisr	I2C2_ER_IRQHandler
	defisr	SPI1_IRQHandler
	defisr	SPI2_IRQHandler
	defisr	USART1_IRQHandler
	defisr	USART2_IRQHandler
	defisr	USART3_IRQHandler
	defisr	EXTI15_10_IRQHandler
	defisr	RTC_Alarm_IRQHandler
	defisr	DFSDM1_FLT3_IRQHandler
	defisr	TIM8_BRK_IRQHandler
	defisr	TIM8_UP_IRQHandler
	defisr	TIM8_TRG_COM_IRQHandler
	defisr	TIM8_CC_IRQHandler
	defisr	ADC3_IRQHandler
	defisr	FMC_IRQHandler
	defisr	SDMMC1_IRQHandler
	defisr	TIM5_IRQHandler
	defisr	SPI3_IRQHandler
	defisr	UART4_IRQHandler
	defisr	UART5_IRQHandler
	defisr	TIM6_DAC_IRQHandler
	defisr	TIM7_IRQHandler
	defisr	DMA2_Channel1_IRQHandler
	defisr	DMA2_Channel2_IRQHandler
	defisr	DMA2_Channel3_IRQHandler
	defisr	DMA2_Channel4_IRQHandler
	defisr	DMA2_Channel5_IRQHandler
	defisr	DFSDM1_FLT0_IRQHandler
	defisr	DFSDM1_FLT1_IRQHandler
	defisr	DFSDM1_FLT2_IRQHandler
	defisr	COMP_IRQHandler
	defisr	LPTIM1_IRQHandler
	defisr	LPTIM2_IRQHandler
	defisr	OTG_FS_IRQHandler
	defisr	DMA2_Channel6_IRQHandler
	defisr	DMA2_Channel7_IRQHandler
	defisr	LPUART1_IRQHandler
	defisr	QUADSPI_IRQHandler
	defisr	I2C3_EV_IRQHandler
	defisr	I2C3_ER_IRQHandler
	defisr	SAI1_IRQHandler
	defisr	SAI2_IRQHandler
	defisr	SWPMI1_IRQHandler
	defisr	TSC_IRQHandler
	defisr	LCD_IRQHandler
	defisr	AES_IRQHandler
	defisr	HASH_RNG_IRQHandler
	defisr	FPU_IRQHandler
	defisr	CRS_IRQHandler
	defisr	I2C4_EV_IRQHandler
	defisr	I2C4_ER_IRQHandler
	defisr	DCMI_IRQHandler
	defisr	CAN2_TX_IRQHandler
	defisr	CAN2_RX0_IRQHandler
	defisr	CAN2_RX1_IRQHandler
	defisr	CAN2_SCE_IRQHandler
	defisr	DMA2D_IRQHandler

@ Default_Handler
.text
.type Default_Handler, %function
.global Default_Handler
Default_Handler:
	bkpt
	b.n Default_Handler
