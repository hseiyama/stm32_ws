.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.include "stm32l4a6.inc"

TimerValue=1500
TimerPrescaler=1000

.data
TimerEvents:
	.byte 2

.bss
BlinkStep:
	.space 1

.text
.type main, %function
.global main
main:
	ldr r0, =_DataStart
	ldr r1, =_DataEnd
	ldr r2, =_DataLoad

	b 2f
1:	ldr r3, [r2], #4
	str r3, [r0], #4
2:	cmp r0, r1
	blo 1b

	ldr r0, =_BssStart
	ldr r1, =_BssEnd
	ldr r2, =0

	b 2f
1:	str r2, [r0], #4
2:	cmp r0, r1
	blo 1b

	bl EnableClockGPIOB
	bl EnableClockTIM1
	bl ConfigurePB14

	ldr r1, =GPIOx_BSRR_BS14
	ldr r0, =GPIOB
	str r1, [r0, #GPIOx_BSRR]			@ Store address of GPIOB_BSRR

	ldr r0, =TIM1_UP_IRQn
	bl EnableIRQ
	bl StartTIM1
	1:
		wfi
		b 1b
	.ltorg

.type TIM1_UP_TIM16_IRQHandler, %function
.global TIM1_UP_TIM16_IRQHandler
TIM1_UP_TIM16_IRQHandler:
	ldr r0, =TIM1
	ldr r2, =(~(1 << TIMx_SR_UIF))

	ldr r1, [r0, #TIMx_SR]
	bics r1, r2
	beq 2f

	str r2, [r0, #TIMx_SR]

	ldr r0, =BlinkStep

	ldrb r1, [r0]

	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs 2f

	ldr r0, =TimerEvents
	ldrb r3, [r0]
	subs r3, #1

	itt ne
	strbne r3, [r0]
	bne 2f

	add r1, #1
	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs 1f

	ldr r2, =BlinkTable
	ldrb r3, [r2, r1]
	strb r3, [r0]

1:
	ldr r0, =BlinkStep
	strb r1, [r0]

	ands r1, #1
	ite eq
	ldreq r1, =GPIOx_BSRR_BS14
	ldrne r1, =GPIOx_BSRR_BR14

	ldr r0, =GPIOB
	str r1, [r0, #GPIOx_BSRR]			@ Store address of GPIOB_BSRR

2:
	bx lr

.align 2
.type BlinkTable,%object	
BlinkTable:
	.byte	2, 2, 2, 2, 2, 2
	.byte	5, 2, 5, 2, 5, 2
	.byte	2, 2, 2, 2, 2
BlinkTableEnd:

.align 2

.type EnableClockGPIOB, %function
EnableClockGPIOB:
	ldr r1, =RCC
	ldr r0, [r1, #RCC_AHB2ENR]
	orr r0, r0, #(1 << RCC_AHB2ENR_GPIOBEN)
	str r0, [r1, #RCC_AHB2ENR]			@ Set GPIOBEN bit in RCC_AHB2ENR to 1 to enable GPIOB
	bx lr								@ Return to caller

.type EnableClockTIM1, %function
EnableClockTIM1:
	ldr r1, =RCC
	ldr r0, [r1, #RCC_APB2ENR]
	orr r0, r0, #(1 << RCC_APB2ENR_TIM1EN)
	str r0, [r1, #RCC_APB2ENR]			@ Set TIM1EN bit in RCC_APB2ENR to 1 to enable TIM1
	bx lr								@ Return to caller
	.ltorg

.type ConfigurePB14, %function
ConfigurePB14:
	ldr r1, =GPIOB
	ldr r0, [r1, #GPIOx_MODER]
	and r0, #0xcfffffff
	orr r0, #GPIOx_MODER_MODE14_OUT
	str r0, [r1, #GPIOx_MODER]			@ Set MODE14 in GPIOB_MODER to 01b
	ldr r0, [r1, #GPIOx_OSPEEDR]
	and r0, #0xcfffffff
	orr r0, #GPIOx_OSPEEDR_OSPEED14_LOW
	str r0, [r1, #GPIOx_OSPEEDR]		@ Set OSPEED14 in GPIOB_OSPEEDR to 00b
	bx lr
	.ltorg

@ r0 = Count-Down value for timer
.type InitializeSysTick, %function
StartTIM1:
	ldr r0, =TIM1
	ldr r1, =(1 << TIMx_CR1_URS)
	str r1, [r0, #TIMx_CR1]

	ldr r1, =TimerPrescaler
	str r1, [r0, #TIMx_PSC]

	ldr r1, =TimerValue
	str r1, [r0, #TIMx_ARR]

	ldr r1, =(1 << TIMx_DIER_UIE)
	str r1, [r0, #TIMx_DIER]

	ldr r1, =(1 << TIMx_EGR_UG)
	str r1, [r0, #TIMx_EGR]

	dsb

	ldr r1, =(1 << TIMx_CR1_CEN)
	str r1, [r0, #TIMx_CR1]

	bx lr
	.ltorg

@ r0 = IRQ Number
.type EnableIRQ, %function
EnableIRQ:
	ldr r1, =NVIC_ISER0

	movs r2, #1
	and r3, r0, #0x1F
	lsls r2, r2, r3

	lsrs r3, r0, #5
	lsls r3, r3, #2

	str r2, [r1, r3]

	bx lr
	.ltorg

