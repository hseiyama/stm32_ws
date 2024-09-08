.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.text
@ void mem_cpy32(uint32_t *dst, const uint32_t *src, size_t n)
.type mem_cpy32, %function
.global mem_cpy32
mem_cpy32:
	@ uint32_t *dst_end = dst + n;
	add r2, r0, r2, lsl #2
	b 2f
	@ *(dst++) = *(src++);
1:	ldr r3, [r1], #4
	str r3, [r0], #4
	@ while (dst < dst_end)
2:	cmp r0, r2
	blo 1b
	bx	lr

@ void mem_cpy16(uint16_t *dst, const uint16_t *src, size_t n)
.type mem_cpy16, %function
.global mem_cpy16
mem_cpy16:
	@ uint16_t *dst_end = dst + n;
	add r2, r0, r2, lsl #1
	b 2f
	@ *(dst++) = *(src++);
1:	ldrh r3, [r1], #2
	strh r3, [r0], #2
	@ while (dst < dst_end)
2:	cmp r0, r2
	blo 1b
	bx	lr

@ extern void mem_cpy08(uint8_t *dst, const uint8_t *src, size_t n)
.type mem_cpy08, %function
.global mem_cpy08
mem_cpy08:
	@ uint8_t *dst_end = dst + n;
	add r2, r0
	b 2f
	@ *(dst++) = *(src++);
1:	ldrb r3, [r1], #1
	strb r3, [r0], #1
	@ while (dst < dst_end)
2:	cmp r0, r2
	blo 1b
	bx	lr

@ void mem_set32(uint32_t *s, uint32_t c, size_t n)
.type mem_set32, %function
.global mem_set32
mem_set32:
	@ uint32_t *s_end = dst + n;
	add r2, r0, r2, lsl #2
	b 2f
	@ *(s++) = c;
1:	str r1, [r0], #4
	@ while (s < s_end)
2:	cmp r0, r2
	blo 1b
	bx	lr

@ void mem_set16(uint16_t *s, uint16_t c, size_t n)
.type mem_set16, %function
.global mem_set16
mem_set16:
	@ uint16_t *s_end = dst + n;
	add r2, r0, r2, lsl #1
	b 2f
	@ *(s++) = c;
1:	strh r1, [r0], #2
	@ while (s < s_end)
2:	cmp r0, r2
	blo 1b
	bx	lr

@ extern void mem_set08(uint8_t *s, uint8_t c, size_t n)
.type mem_set08, %function
.global mem_set08
mem_set08:
	@ uint8_t *s_end = dst + n;
	add r2, r0
	b 2f
	@ *(s++) = c;
1:	strb r1, [r0], #1
	@ while (s < s_end)
2:	cmp r0, r2
	blo 1b
	bx	lr

@ int mem_cmp32(const uint32_t *s1, const uint32_t *s2, size_t n)
.type mem_cmp32, %function
.global mem_cmp32
mem_cmp32:
	push {r4, lr}
	@ uint32_t *s1_end = s1 + n;
	add r2, r0, r2, lsl #2
	@ int diff;
	mov r4, r0
	b 2f
	@ diff = *(s1++) - *(s2++);
1:	ldr r3, [r4], #4
	ldr r0, [r1], #4
	subs r0, r3, r0
	@ if (diff != 0)
	bne 3f
	@ while (s1 < s1_end)
2:	cmp r4, r2
	blo 1b
	@ diff = 0;
	mov r0, #0
3:	pop {r4, pc}

@ int mem_cmp16(const uint16_t *s1, const uint16_t *s2, size_t n);
.type mem_cmp16, %function
.global mem_cmp16
mem_cmp16:
	push {r4, lr}
	@ uint16_t *s1_end = s1 + n;
	add r2, r0, r2, lsl #1
	@ int diff;
	mov r4, r0
	b 2f
	@ diff = *(s1++) - *(s2++);
1:	ldrh r3, [r4], #2
	ldrh r0, [r1], #2
	subs r0, r3, r0
	@ if (diff != 0)
	bne 3f
	@ while (s1 < s1_end)
2:	cmp r4, r2
	blo 1b
	@ diff = 0;
	mov r0, #0
3:	pop {r4, pc}

@ int mem_cmp08(const uint8_t *s1, const uint8_t *s2, size_t n);
.type mem_cmp08, %function
.global mem_cmp08
mem_cmp08:
	push {r4, lr}
	@ uint8_t *s1_end = s1 + n;
	add r2, r0
	@ int diff;
	mov r4, r0
	b 2f
	@ diff = *(s1++) - *(s2++);
1:	ldrb r3, [r4], #1
	ldrb r0, [r1], #1
	subs r0, r3, r0
	@ if (diff != 0)
	bne 3f
	@ while (s1 < s1_end)
2:	cmp r4, r2
	blo 1b
	@ diff = 0;
	mov r0, #0
3:	pop {r4, pc}
