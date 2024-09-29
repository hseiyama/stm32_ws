.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

.include "stm32g0b1.inc"

TimerValue=6000000						@ HSISYS is used (HSI16 16 MHz frequency)

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
//	str r1, [r0, #(TimerEvents-Variables)]		@ Cortex-M4 code
												@ Note. 1 is not a multiple of 4.
	adds r2, r0, #(TimerEvents-Variables)	@ for Cortex-M0+
	strb r1, [r2]							@ for Cortex-M0+

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
//	tst r0, #0x10000							@ Cortex-M4 code
	ldr r2, =0x10000						@ for Cortex-M0+
	tst r0, r2								@ for Cortex-M0+
	beq Return

	ldr r0, =Variables

	ldrb r1, [r0, #(BlinkStep-Variables)]

	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs Return

	ldrb r3, [r0, #(TimerEvents-Variables)]
	subs r3, #1

//	itt ne										@ Cortex-M4 code
//	strbne r3, [r0, #(TimerEvents-Variables)]	@ Cortex-M4 code
//	bne Return									@ Cortex-M4 code
	beq 1f									@ for Cortex-M0+
	strb r3, [r0, #(TimerEvents-Variables)]	@ for Cortex-M0+
	b Return								@ for Cortex-M0+
1:											@ for Cortex-M0+
//	add r1, #1									@ Cortex-M4 code
	movs r2, #1								@ for Cortex-M0+
	adds r1, r2								@ for Cortex-M0+
	cmp r1, #(BlinkTableEnd-BlinkTable)
	bhs SkipRestart

	ldr r2, =BlinkTable
	ldrb r3, [r2, r1]
	strb r3, [r0, #(TimerEvents-Variables)]

SkipRestart:
	strb r1, [r0, #(BlinkStep-Variables)]

//	ands r1, #1									@ Cortex-M4 code
	movs r2, #1								@ for Cortex-M0+
	ands r1, r2								@ for Cortex-M0+
//	ite eq										@ Cortex-M4 code
//	ldreq r1, =GPIOx_BSRR_BS5					@ Cortex-M4 code
//	ldrne r1, =GPIOx_BSRR_BR5					@ Cortex-M4 code
	bne 1f									@ for Cortex-M0+
	ldr r1, =GPIOx_BSRR_BS5					@ for Cortex-M0+
	b 2f									@ for Cortex-M0+
1:											@ for Cortex-M0+
	ldr r1, =GPIOx_BSRR_BR5					@ for Cortex-M0+
2:											@ for Cortex-M0+
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
	ldr r0, [r1, #RCC_IOPENR]
//	orr r0, r0, #(1 << RCC_IOPENR_GPIOAEN)		@ Cortex-M4 code
	movs r2, #(1 << RCC_IOPENR_GPIOAEN)		@ for Cortex-M0+
	orrs r0, r2								@ for Cortex-M0+
	str r0, [r1, #RCC_IOPENR]			@ Set GPIOAEN bit in RCC_IOPENR to 1 to enable GPIOA
	bx lr								@ Return to caller

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
.type StartSysTick, %function
StartSysTick:
	ldr r1, =STK

	str r0, [r1, #STK_RVR]
	ldr r0, =0
	str r0, [r1, #STK_CVR]

	ldr r0, =7
	str r0, [r1, #STK_CSR]

	bx lr
