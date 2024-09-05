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

DelayLoopIterations = 1000000

.text
.type Reset_Handler, %function
.global Reset_Handler
Reset_Handler:
	bl EnableClockGPIOB
	bl ConfigurePB14
	ldr r0, =5
	bl Blink
	b .

.type Blink, %function
Blink:
	push { r4-r8, lr }
	ldr r4, =GPIOB_BSRR			@ Load address of GPIOB_BSRR
	ldr r5, =GPIOx_BSRR_BS14	@ Register value to set pin to high
	ldr r6, =GPIOx_BSRR_BR14	@ Register value to set pin to low
	ldr r7, =BlinkTable			@ Move address of "BlinkTable" into r7
	ldr r8, =BlinkTableEnd		@ Move address of "BlinkTableEnd" into r8

	BlinkLoop:
		str r5, [r4]			@ Set BS14 in GPIOB_BSRR to 1 to set PB14 high

		ldr r0, [r7], #4		@ Load delay iterations from table and increment address
		bl Delay

		str r6, [r4]			@ Set BR14 in GPIOB_BSRR to 1 to set PB14 low

		ldr r0, [r7], #4		@ Load delay iterations from table and increment address
		bl Delay

		cmp r7, r8
		blo BlinkLoop
	
	pop { r4-r8, pc }

.align 2
.type BlinkTable, %object
BlinkTable:
	.word	1000000, 1000000, 1000000, 1000000, 1000000, 1000000
	.word	2500000, 1000000, 2500000, 1000000, 2500000, 1000000
	.word	1000000, 1000000, 1000000, 1000000, 1000000, 1000000
BlinkTableEnd:

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

@ Parameters: r0 = Number of iterations
.type Delay, %function
Delay:
	DelayLoop:
	subs r0, #1
	bne DelayLoop			@ Iterate delay loop
	bx lr
