.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.include "stm32g474.inc"

TimerValue=6000000

.data
Variables:
BlinkStep:
	.space 1
TimerEvents:
	.space 1

.text
.type main, %function
.global main
main:
	ldr r0, =Variables
	ldr r1, =0
	strb r1, [r0, #(BlinkStep-Variables)]
	ldr r1, BlinkTable
	strb r1, [r0, #(TimerEvents-Variables)]

	bl EnableClockGPIOA
	bl ConfigurePA5

	ldr r1, =GPIOx_BSRR_BS5
	ldr r0, =GPIOA
	str r1, [r0, #GPIOx_BSRR]			@ Store address of GPIOA_BSRR

	ldr r0, =TimerValue
	bl StartSysTick
	SleepLoop:
		wfi
		b SleepLoop

.type SysTick_Handler, %function
.global SysTick_Handler
SysTick_Handler:
	ldr r0, =STK
	ldr r0, [r0, #STK_CTRL]
	tst r0, #0x10000
	beq Return

	ldr r0, =Variables

	ldrb r1, [r0, #(BlinkStep-Variables)]

	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs Return

	ldrb r3, [r0, #(TimerEvents-Variables)]
	subs r3, #1

	itt ne
	strbne r3, [r0, #(TimerEvents-Variables)]
	bne Return

	add r1, #1
	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs SkipRestart

	ldr r2, =BlinkTable
	ldrb r3, [r2, r1]
	strb r3, [r0, #(TimerEvents-Variables)]

SkipRestart:
	strb r1, [r0, #(BlinkStep-Variables)]

	ands r1, #1
	ite eq
	ldreq r1, =GPIOx_BSRR_BS5
	ldrne r1, =GPIOx_BSRR_BR5

	ldr r0, =GPIOA
	str r1, [r0, #GPIOx_BSRR]			@ Store address of GPIOA_BSRR

Return:
	bx lr

.align 2
BlinkTable:
	.byte	2, 2, 2, 2, 2, 2
	.byte	5, 2, 5, 2, 5, 2
	.byte	2, 2, 2, 2, 2
BlinkTableEnd:

.align 2

.type EnableClockGPIOA, %function
EnableClockGPIOA:
	ldr r1, =RCC
	ldr r0, [r1, #RCC_AHB2ENR]
	orr r0, r0, #(1 << RCC_AHB2ENR_GPIOAEN)
	str r0, [r1, #RCC_AHB2ENR]			@ Set GPIOAEN bit in RCC_AHB2ENR to 1 to enable GPIOA
	bx lr								@ Return to caller

.type ConfigurePA5, %function
ConfigurePA5:
	ldr r1, =GPIOA
	ldr r0, [r1, #GPIOx_MODER]
	and r0, #0xfffff3ff
	orr r0, #GPIOx_MODER_MODE5_OUT
	str r0, [r1, #GPIOx_MODER]			@ Set MODE5 in GPIOA_MODER to 01b
	ldr r0, [r1, #GPIOx_OSPEEDR]
	and r0, #0xfffff3ff
	orr r0, #GPIOx_OSPEEDR_OSPEED5_LOW
	str r0, [r1, #GPIOx_OSPEEDR]		@ Set OSPEED5 in GPIOA_OSPEEDR to 00b
	bx lr
	.ltorg

@ r0 = Count-Down value for timer
.type StartSysTick, %function
StartSysTick:
	ldr r1, =STK

	str r0, [r1, #STK_LOAD]
	ldr r0, =0
	str r0, [r1, #STK_VAL]

	ldr r0, =7
	str r0, [r1, #STK_CTRL]

	bx lr
