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

.text
.type Reset_Handler, %function
.global Reset_Handler
Reset_Handler:
	bl EnableClockGPIOB
	bl ConfigurePB14
	ldr r0, =TimerValue
	bl StartSysTick
	bl Blink
	b .

.type Blink, %function
Blink:
	push { r4-r8, lr }
	ldr r4, =GPIOB_BSRR				@ Load address of GPIOB_BSRR
	ldr r5, =GPIOx_BSRR_BS14		@ Register value to set pin to high
	ldr r6, =GPIOx_BSRR_BR14		@ Register value to set pin to low
	ldr r7, =BlinkTable				@ Move address of "BlinkTable" into r7
	ldr r8, =BlinkTableEnd			@ Move address of "BlinkTableEnd" into r8

	BlinkLoop:
		str r5, [r4]				@ Set BS8 in GPIOA_BSRR to 1 to set PA8 high

		ldrb r0, [r7], #1			@ Load delay iterations from table and increment address
		bl WaitSysTick

		str r6, [r4]				@ Set BR8 in GPIOA_BSRR to 1 to set PA8 low

		ldrb r0, [r7], #1			@ Load delay iterations from table and increment address
		bl WaitSysTick

		cmp r7, r8
		blo BlinkLoop

	pop { r4-r8, pc }

.align 2
.type BlinkTable, %object
BlinkTable:
	.byte	2, 2, 2, 2, 2, 2
	.byte	5, 2, 5, 2, 5, 2
	.byte	2, 2, 2, 2, 2, 2
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

	ldr r0, =5
	str r0, [r1, #STK_CTRL]

	bx lr

@ r0 = Number of timer events to wait for
.type WaitSysTick, %function
WaitSysTick:
	ldr r1, =STK
	
WaitSysTickLoop:
	ldr r2, [r1, #STK_CTRL]
	tst r2, #0x10000
	beq WaitSysTickLoop

	subs r0, #1
	bne WaitSysTickLoop

	bx lr
