.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.include "stm32l4a6.inc"

TimerValue=1500000

.data
TimerEvents:
	.byte 2

.bss
BlinkStep:
	.space 1
	.space 100

.text
.type Reset_Handler, %function
.global Reset_Handler
Reset_Handler:
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
	bl ConfigurePB14

	ldr r1, =GPIOx_BSRR_BS14
	ldr r0, =GPIOB					@ Load address of GPIOB_BSRR
	str r1, [r0, #GPIOx_BSRR]

	ldr r0, =TimerValue
	bl StartSysTick
	1:
		wfi
		b 1b
	.ltorg

.type SysTick_Handler, %function
.global SysTick_Handler
SysTick_Handler:
	ldr r0, =STK
	ldr r0, [r0, #STK_CTRL]
	tst r0, #0x10000
	beq 2f
	
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
	str r1, [r0, #GPIOx_BSRR]

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
	ldr r0, [r1, RCC_AHB2ENR]
	orr r0, r0, #(1 << RCC_AHB2ENR_GPIOBEN)
	str r0, [r1, RCC_AHB2ENR]			@ Set GPIOBEN bit in RCC_AHB2ENR to 1 to enable GPIOB
	bx lr								@ Return to caller

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
.type StartSysTick, %function
StartSysTick:
	ldr r1, =STK

	str r0, [r1, #STK_LOAD]
	ldr r0, =0
	str r0, [r1, #STK_VAL]

	ldr r0, =7
	str r0, [r1, #STK_CTRL]

	bx lr
