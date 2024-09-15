.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

.include "stm32g0b1.inc"

TimerValue=6000
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
	bl EnableClockGPIOA
	bl EnableClockTIM1
	bl ConfigurePA5

	ldr r1, =GPIOx_BSRR_BS5
	ldr r0, =GPIOA
	str r1, [r0, #GPIOx_BSRR]			@ Store address of GPIOA_BSRR

	ldr r0, =TIM1_UP_IRQn
	bl EnableIRQ
	bl StartTIM1
	1:
		wfi
		b 1b
	.ltorg

.type TIM1_BRK_UP_TRG_COM_IRQHandler, %function
.global TIM1_BRK_UP_TRG_COM_IRQHandler
TIM1_BRK_UP_TRG_COM_IRQHandler:
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

//	itt ne										@ Cortex-M4 code
//	strbne r3, [r0]								@ Cortex-M4 code
//	bne 2f										@ Cortex-M4 code
	beq 3f									@ for Cortex-M0+
	strb r3, [r0]							@ for Cortex-M0+
	b 2f									@ for Cortex-M0+
3:											@ for Cortex-M0+
//	add r1, #1									@ Cortex-M4 code
	movs r2, #1								@ for Cortex-M0+
	adds r1, r2								@ for Cortex-M0+
	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs 1f

	ldr r2, =BlinkTable
	ldrb r3, [r2, r1]
	strb r3, [r0]

1:
	ldr r0, =BlinkStep
	strb r1, [r0]

//	ands r1, #1									@ Cortex-M4 code
	movs r2, #1								@ for Cortex-M0+
	ands r1, r2								@ for Cortex-M0+
//	ite eq										@ Cortex-M4 code
//	ldreq r1, =GPIOx_BSRR_BS5					@ Cortex-M4 code
//	ldrne r1, =GPIOx_BSRR_BR5					@ Cortex-M4 code
	bne 4f									@ for Cortex-M0+
	ldr r1, =GPIOx_BSRR_BS5					@ for Cortex-M0+
	b 5f									@ for Cortex-M0+
4:											@ for Cortex-M0+
	ldr r1, =GPIOx_BSRR_BR5					@ for Cortex-M0+
5:											@ for Cortex-M0+
	ldr r0, =GPIOA
	str r1, [r0, #GPIOx_BSRR]			@ Store address of GPIOA_BSRR

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

.type EnableClockGPIOA, %function
EnableClockGPIOA:
	ldr r1, =RCC
	ldr r0, [r1, #RCC_IOPENR]
//	orr r0, r0, #(1 << RCC_IOPENR_GPIOAEN)		@ Cortex-M4 code
	movs r2, #(1 << RCC_IOPENR_GPIOAEN)		@ for Cortex-M0+
	orrs r0, r2								@ for Cortex-M0+
	str r0, [r1, #RCC_IOPENR]			@ Set GPIOAEN bit in RCC_IOPENR to 1 to enable GPIOA
	bx lr								@ Return to caller

.type EnableClockTIM1, %function
EnableClockTIM1:
	ldr r1, =RCC
	ldr r0, [r1, #RCC_APBENR2]
//	orr r0, r0, #(1 << RCC_APBENR2_TIM1EN)		@ Cortex-M4 code
	ldr r2, =(1 << RCC_APBENR2_TIM1EN)		@ for Cortex-M0+
	orrs r0, r2								@ for Cortex-M0+
	str r0, [r1, #RCC_APBENR2]			@ Set TIM1EN bit in RCC_APBENR2 to 1 to enable TIM1
	bx lr								@ Return to caller
	.ltorg

.type ConfigurePA5, %function
ConfigurePA5:
	ldr r1, =GPIOA
	ldr r0, [r1, #GPIOx_MODER]
//	and r0, #0xfffff3ff							@ Cortex-M4 code
	ldr r2, =0xfffff3ff						@ for Cortex-M0+
	ands r0, r2								@ for Cortex-M0+
//	orr r0, #GPIOx_MODER_MODE5_OUT				@ Cortex-M4 code
	ldr r2, =GPIOx_MODER_MODE5_OUT			@ for Cortex-M0+
	orrs r0, r2								@ for Cortex-M0+
	str r0, [r1, #GPIOx_MODER]			@ Set MODE5 in GPIOA_MODER to 01b
	ldr r0, [r1, #GPIOx_OSPEEDR]
//	and r0, #0xfffff3ff							@ Cortex-M4 code
	ldr r2, =0xfffff3ff						@ for Cortex-M0+
	ands r0, r2								@ for Cortex-M0+
//	orr r0, #GPIOx_OSPEEDR_OSPEED5_LOW			@ Cortex-M4 code
	ldr r2, =#GPIOx_OSPEEDR_OSPEED5_LOW		@ for Cortex-M0+
	orrs r0, r2								@ for Cortex-M0+
	str r0, [r1, #GPIOx_OSPEEDR]		@ Set OSPEED5 in GPIOA_OSPEEDR to 00b
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
	ldr r1, =NVIC_ISER

	movs r2, #1
//	and r3, r0, #0x1F							@ Cortex-M4 code
	movs r3, #0x1F							@ for Cortex-M0+
	ands r3, r0								@ for Cortex-M0+
	lsls r2, r2, r3

	lsrs r3, r0, #5
	lsls r3, r3, #2

	str r2, [r1, r3]

	bx lr
	.ltorg

