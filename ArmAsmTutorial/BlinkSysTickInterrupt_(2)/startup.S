.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.include "stm32l4a6.inc"

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

	bl main
1:	bkpt
	b 1b
	.ltorg
