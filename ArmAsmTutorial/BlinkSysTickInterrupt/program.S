.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

RCC_AHB2ENR = 0x4002104C
RCC_AHB2ENR_GPIOBEN = 2

GPIOB_MODER = 0x48000400
GPIOx_MODER_MODE14_OUT = 0x10000000

GPIOB_OSPEEDR = 0x48000408
GPIOx_OSPEEDR_OSPEED14_LOW = 0x00000000

GPIOB_BSRR = 0x48000418
GPIOx_BSRR_BS14 = 0x00004000
GPIOx_BSRR_BR14 = 0x40000000

STK = 0xE000E000
STK_CTRL = 0x10
STK_LOAD = 0x14
STK_VAL = 0x18
TimerValue=1500000

.data
Variables:
BlinkStep:
	.space 1
TimerEvents:
	.space 1

.text
.type Reset_Handler, %function
.global Reset_Handler
Reset_Handler:
	ldr r0, =Variables
	ldr r1, =0
	str r1, [r0, #(BlinkStep-Variables)]
	ldr r1, BlinkTable
	str r1, [r0, #(TimerEvents-Variables)]
	
	bl EnableClockGPIOB
	bl ConfigurePB14
	
	ldr r1, =GPIOx_BSRR_BS14
	ldr r0, =GPIOB_BSRR			@ Load address of GPIOB_BSRR
	str r1, [r0]

	
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
	ldreq r1, =GPIOx_BSRR_BS14
	ldrne r1, =GPIOx_BSRR_BR14
	
	ldr r0, =GPIOB_BSRR			@ Load address of GPIOB_BSRR
	str r1, [r0]

Return:
	bx lr

.align 2
BlinkTable:
	.byte	2, 2, 2, 2, 2, 2
	.byte	5, 2, 5, 2, 5, 2
	.byte	2, 2, 2, 2, 2
BlinkTableEnd:

.align 2

.type EnableClockGPIOB, %function
EnableClockGPIOB:
	ldr r1, =RCC_AHB2ENR
	ldr r0, [r1]
	orr r0, r0, #RCC_AHB2ENR_GPIOBEN
	str r0, [r1]	@ Set GPIOBEN bit in RCC_AHB2ENR to 1 to enable GPIOB
	bx lr			@ Return to caller

.type ConfigurePB14, %function
ConfigurePB14:
	ldr r1, =GPIOB_MODER
	ldr r0, [r1]
	and r0, #0xcfffffff
	orr r0, #GPIOx_MODER_MODE14_OUT
	str r0, [r1]			@ Set MODE14 in GPIOB_MODER to 01b
	ldr r1, =GPIOB_OSPEEDR
	ldr r0, [r1]
	and r0, #0xcfffffff
	orr r0, #GPIOx_OSPEEDR_OSPEED14_LOW
	str r0, [r1]			@ Set OSPEED14 in GPIOB_OSPEEDR to 00b
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
