.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

.include "stm32c071.inc"

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
	adds r2, r0, #(TimerEvents-Variables)
	strb r1, [r2]

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
	ldr r0, [r0, #STK_CSR]
	ldr r2, =0x10000
	tst r0, r2
	beq Return

	ldr r0, =Variables

	ldrb r1, [r0, #(BlinkStep-Variables)]

	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs Return

	ldrb r3, [r0, #(TimerEvents-Variables)]
	subs r3, #1

	beq 1f
	strb r3, [r0, #(TimerEvents-Variables)]
	b Return
1:	movs r2, #1
	adds r1, r2
	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs SkipRestart

	ldr r2, =BlinkTable
	ldrb r3, [r2, r1]
	strb r3, [r0, #(TimerEvents-Variables)]

SkipRestart:
	strb r1, [r0, #(BlinkStep-Variables)]

	movs r2, #1
	ands r1, r2
	bne 1f
	ldr r1, =GPIOx_BSRR_BS5
	b 2f
1:	ldr r1, =GPIOx_BSRR_BR5
2:	ldr r0, =GPIOA
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
	ldr r0, [r1, #RCC_IOPENR]
	movs r2, #(1 << RCC_IOPENR_GPIOAEN)
	orrs r0, r2
	str r0, [r1, #RCC_IOPENR]			@ Set GPIOAEN bit in RCC_IOPENR to 1 to enable GPIOA
	bx lr								@ Return to caller

.type ConfigurePA5, %function
ConfigurePA5:
	ldr r1, =GPIOA
	ldr r0, [r1, #GPIOx_MODER]
	ldr r2, =0xfffff3ff
	ands r0, r2
	ldr r2, =GPIOx_MODER_MODE5_OUT
	orrs r0, r2
	str r0, [r1, #GPIOx_MODER]			@ Set MODE5 in GPIOA_MODER to 01b
	ldr r0, [r1, #GPIOx_OSPEEDR]
	ldr r2, =0xfffff3ff
	ands r0, r2
	ldr r2, =#GPIOx_OSPEEDR_OSPEED5_LOW
	orrs r0, r2
	str r0, [r1, #GPIOx_OSPEEDR]		@ Set OSPEED5 in GPIOA_OSPEEDR to 00b
	bx lr
	.ltorg

@ r0 = Count-Down value for timer
.type StartSysTick, %function
StartSysTick:
	ldr r1, =STK

	str r0, [r1, #STK_RVR]
	ldr r0, =0
	str r0, [r1, #STK_CVR]

	ldr r0, =7
	str r0, [r1, #STK_CSR]

	bx lr
