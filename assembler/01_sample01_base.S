.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.equ RCC_BASE,    0x40021000			@ RCC base address
.equ GPIOB_BASE,  0x48000400			@ GPIOB base address
.equ GPIOC_BASE,  0x48000800			@ GPIOC base address
.equ RCC_AHB2ENR, (RCC_BASE + 0x4C)		@ AHB2 peripheral clock enable register
.equ GPIOB_MODER, (GPIOB_BASE + 0x00)	@ GPIO port mode register
.equ GPIOB_ODR,   (GPIOB_BASE + 0x14)	@ GPIO port output data register
.equ GPIOB_BSRR,  (GPIOB_BASE + 0x18)	@ GPIO port bit set/reset register
.equ GPIOC_MODER, (GPIOC_BASE + 0x00)	@ GPIO port mode register
.equ GPIOC_IDR,   (GPIOC_BASE + 0x10)	@ GPIO port input data register

.equ RCC_AHB2ENR_GPIOBEN,    0x00000002		@ GPIOBEN=1
.equ RCC_AHB2ENR_GPIOCEN,    0x00000004		@ GPIOCEN=1
.equ GPIOx_MODER_MODE7_OUT,  0x00004000		@ MODE7=01b
.equ GPIOx_MODER_MODE14_OUT, 0x10000000		@ MODE14=01b
.equ GPIOx_MODER_MODE13_IN,  0x00000000		@ MODE13=00b
.equ GPIOx_MODER_MODE7_MSK,  0x0000C000		@ MODE7=11b
.equ GPIOx_MODER_MODE14_MSK, 0x30000000		@ MODE14=11b
.equ GPIOx_MODER_MODE13_MSK, 0x0C000000		@ MODE13=11b
.equ GPIOx_IDR_ID7,          0x2000			@ ID13=1
.equ GPIOx_ODR_OD7,          0x0080			@ OD7=1
.equ GPIOx_ODR_OD14,         0x4000			@ OD14=1
.equ GPIOx_BSRR_BR14,        0x40000000		@ BR14=1
.equ GPIOx_BSRR_BS14,        0x00004000		@ BS14=1

.data
data_sw:
	.space 2						@ Port input data
	.align 2
data_led:
	.space 4						@ Port set/reset data

@ VectorTable
.section .VectorTable, "a"
.word _StackEnd
.word Reset_Handler
.space 0x1A4

@ Reset_Handler
.text
.type Reset_Handler, %function
Reset_Handler:
	bl port_init					@ Call port_init
loop:
	bl port_read					@ Call port_read
	bl port_update					@ Call port_update
	b loop

@ port_init
.type port_init, %function
port_init:
	push {r0-r2}
	ldr r0, =(RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN)
	ldr r1, =RCC_AHB2ENR
	str r0, [r1]					@ Set GPIOBEN,GPIOCEN bit in RCC_AHB2ENR to 1 to enable GPIOB
	ldr r1, =GPIOB_MODER
	ldr r0, [r1]
	ldr r2, =~(GPIOx_MODER_MODE7_MSK | GPIOx_MODER_MODE14_MSK)
	and r0, r2
	ldr r2, =(GPIOx_MODER_MODE7_OUT | GPIOx_MODER_MODE14_OUT)
	orr r0, r2
	str r0, [r1]					@ Set MODE7,MODE14 in GPIOB_MODER to 1 (default:0xFFFFFEBF)
	mov r0, #(GPIOx_ODR_OD7 | GPIOx_ODR_OD14)
	ldr r1, =GPIOB_ODR
	str r0, [r1]					@ Set OD7,OD14 in GPIOB_ODR to 1 to set PB7,PB14 high
	ldr r1, =GPIOC_MODER
	ldr r0, [r1]
	ldr r2, =(~GPIOx_MODER_MODE13_MSK | GPIOx_MODER_MODE13_IN)
	and r0, r2
	str r0, [r1]					@ Set MODE13 in GPIOC_MODER to 0 (default:0xFFFFFFFF)
	pop {r0-r2}
	bx lr

@ port_read
.type port_read, %function
port_read:
	push {r0-r1}
	ldr r0, =GPIOC_IDR
	ldr r1, [r0]
	ldr r0, =data_sw
	strh r1, [r0]
	pop {r0-r1}
	bx lr

@ port_write
.type port_write, %function
port_write:
	push {r0-r1}
	ldr r0, =data_led
	ldr r1, [r0]
	ldr r0, =GPIOB_BSRR
	str r1, [r0]					@ Set GPIOB_BSRR
	pop {r0-r1}
	bx lr

@ port_update
.type port_update, %function
port_update:
	push {r0-r1,lr}
	ldr r0, =data_sw
	ldrh r1, [r0]
	ands r1, #GPIOx_IDR_ID7
	bne sw_on
	ldr r0, =data_led
	ldr r1, =GPIOx_BSRR_BR14
	str r1, [r0]					@ Set BR14 in GPIOB_BSRR(data_led) to 1 to set PB14 low
	b next
sw_on:
	ldr r0, =data_led
	ldr r1, =GPIOx_BSRR_BS14
	str r1, [r0]					@ Set BS14 in GPIOB_BSRR(data_led) to 1 to set PB14 high
next:
	bl port_write					@ Call port_write
	pop {r0-r1,pc}
