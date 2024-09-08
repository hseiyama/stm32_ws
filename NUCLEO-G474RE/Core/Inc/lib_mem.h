#ifndef __LIB_MEM_H
#define __LIB_MEM_H

extern void mem_cpy32(uint32_t *dst, const uint32_t *src, size_t n);
extern void mem_cpy16(uint16_t *dst, const uint16_t *src, size_t n);
extern void mem_cpy08(uint8_t *dst, const uint8_t *src, size_t n);
extern void mem_set32(uint32_t *s, uint32_t c, size_t n);
extern void mem_set16(uint16_t *s, uint16_t c, size_t n);
extern void mem_set08(uint8_t *s, uint8_t c, size_t n);
extern int mem_cmp32(const uint32_t *s1, const uint32_t *s2, size_t n);
extern int mem_cmp16(const uint16_t *s1, const uint16_t *s2, size_t n);
extern int mem_cmp08(const uint8_t *s1, const uint8_t *s2, size_t n);

#endif /* __LIB_MEM_H */
