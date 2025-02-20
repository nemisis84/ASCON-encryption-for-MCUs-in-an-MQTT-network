#ifndef ASM_H_
#define ASM_H_

#ifndef __GNUC__
#define __asm__ asm
#endif

#define LDR(rd, ptr, offset) \
  __asm__ volatile("ldr %0, [%1, %2]\n\t" : "=r"(rd) : "r"(ptr), "ri"(offset))

#define STR(rd, ptr, offset)                                                \
  __asm__ volatile("str %0, [%1, %2]\n\t" ::"r"(rd), "r"(ptr), "ri"(offset) \
                   : "memory")

#define CLEAR()                                            \
  do {                                                     \
    uint32_t r, v = 0;                                     \
    __asm__ volatile("mov %0, %1\n\t" : "=r"(r) : "i"(v)); \
  } while (0)

// Not used in the code
#define MOV(rd, imm) __asm__ volatile("mov %0, %1\n\t" : "=r"(rd) : "i"(imm))
// Not used in the code
#define ROR(rd, rn, imm) \
  __asm__ volatile("ror %0, %1, #%c2\n\t" : "=r"(rd) : "r"(rn), "i"(imm))
// Not used in the code
#define EOR_ROR(rd, rn, rm, imm)                  \
  __asm__ volatile("eor %0, %1, %2, ror #%c3\n\t" \
                   : "=r"(rd)                     \
                   : "r"(rn), "r"(rm), "i"(imm))


                 

#define SAFE_ROR(dest, src, imm) \
do { \
  uint32_t shift = (imm) & 31; /* Ensure shift is within 0-31 range */ \
  if (shift > 0) { \
    __asm__ volatile ( \
      "lsr %0, %1, %2\n\t"  /* tmp1 = src >> shift */ \
      "lsl %1, %1, %3\n\t"  /* tmp2 = src << (32 - shift) */ \
      "orr %0, %0, %1\n\t"  /* dest = tmp1 | tmp2 */ \
      : "=r"(dest) \
      : "r"(src), "I"(shift), "I"(32 - shift)); \
  } else { \
    dest = src; /* No rotation if imm <= 0 */ \
  } \
} while (0)
                 
                 
                 

#define EOR_AND_ROR(ce, ae, be, imm, tmp) \
  do { \
    SAFE_ROR(tmp, be, imm); \
    __asm__ volatile ( \
        "and %0, %0, %1\n\t" \
        : "+r"(tmp) : "r"(ae)); \
    __asm__ volatile ( \
        "eor %0, %0, %1\n\t" \
        : "+r"(ce) : "r"(tmp)); \
  } while (0)


#define EOR_BIC_ROR(ce, ae, be, imm, tmp) \
  do { \
    SAFE_ROR(tmp, be, imm); \
    __asm__ volatile ( \
      "bic %0, %0, %1\n\t" \
      : "+r"(tmp) : "r"(ae)); \
    __asm__ volatile ( \
      "eor %0, %0, %1\n\t" \
      : "+r"(ce) : "r"(tmp)); \
  } while (0)



#define EOR_ORR_ROR(ce, ae, be, imm, tmp) \
  do { \
    SAFE_ROR(tmp, be, imm); \
    __asm__ volatile ( \
        "orr %0, %0, %1\n\t" \
        : "+r"(tmp) : "r"(ae)); \
    __asm__ volatile ( \
        "eor %0, %0, %1\n\t" \
        : "+r"(ce) : "r"(tmp)); \
  } while (0)



#endif  // ASM_H_
