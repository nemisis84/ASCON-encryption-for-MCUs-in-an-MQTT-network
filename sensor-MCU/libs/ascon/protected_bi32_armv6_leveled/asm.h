#ifndef ASM_H_
#define ASM_H_

#ifndef __GNUC__
#define __asm__ asm
#endif

#define LDR(rd, ptr, offset)  \
  do {                        \
    register uint32_t temp __asm__("r6"); /* Force a low register */  \
    __asm__ volatile(         \
        "ldr %[dest], [%[src], %[off]]\n\t" \
        : [dest] "=r"(rd), [temp] "=r"(temp) \
        : [src] "r"(ptr), [off] "i"(offset)); \
  } while (0)

#define STR(rd, ptr, offset)  \
  do {                        \
    register uint32_t temp __asm__("r6"); /* Force a low register */  \
    __asm__ volatile(         \
        "str %[src], [%[dest], %[off]]\n\t" \
        :                      \
        : [src] "r"(rd), [dest] "r"(ptr), [off] "i"(offset) \
        : "memory");           \
  } while (0)



#define CLEAR()                                            \
  do {                                                     \
    uint32_t r, v = 0;                                     \
    __asm__ volatile("mov %0, %1\n\t" : "=r"(r) : "i"(v)); \
  } while (0)

// ✅ Move immediate value into a register (No change needed)
#define MOV(rd, imm) __asm__ volatile("mov %0, %1\n\t" : "=r"(rd) : "i"(imm))

#define ROR(rd, rn, imm)  \
  do {                    \
    register uint32_t temp __asm__("r6"); /* Ensure low register */ \
    __asm__ volatile(     \
        "lsr %[dest], %[src], %[i1] \n\t" \
        "lsl %[temp], %[src], %[i2] \n\t" \
        "orr %[dest], %[dest], %[temp] \n\t" \
        : [dest] "=r"(rd), [temp] "=r"(temp) \
        : [src] "r"(rn), [i1] "i"(imm), [i2] "i"(32 - (imm))); \
  } while (0)


// ❌ Fix `EOR ROR` (Now passes index explicitly)
#define EOR_ROR(ce, ae, be, imm, tmp, index)  \
  do {                                        \
    uint32_t rotated;                         \
    __asm__ volatile(                         \
        "ror %[rotated], %[b], %[i1]\n\t"     \
        "eor %[c], %[a], %[rotated]\n\t"      \
        : [c] "=r"(ce[index]), [rotated] "=r"(rotated) \
        : [a] "r"(ae[index]), [b] "r"(be[index]), [i1] "i"(imm)); \
  } while (0)

// ❌ Fix `EOR AND ROR` (Pass index explicitly)
#define EOR_AND_ROR(ce, ae, be, imm, tmp, index)  \
  do {                                            \
    uint32_t rotated;                             \
    __asm__ volatile(                             \
        "ror %[rotated], %[b], %[i1]\n\t"         \
        "and %[t], %[a], %[rotated]\n\t"         \
        "eor %[c], %[c], %[t]\n\t"               \
        : [c] "+r"(ce[index]), [t] "=r"(tmp), [rotated] "=r"(rotated) \
        : [a] "r"(ae[index]), [b] "r"(be[index]), [i1] "i"(imm)); \
  } while (0)

// ❌ Fix `EOR BIC ROR`
#define EOR_BIC_ROR(ce, ae, be, imm, tmp, index)  \
  do {                                            \
    uint32_t rotated;                             \
    __asm__ volatile(                             \
        "ror %[rotated], %[b], %[i1]\n\t"         \
        "bic %[t], %[a], %[rotated]\n\t"         \
        "eor %[c], %[c], %[t]\n\t"               \
        : [c] "+r"(ce[index]), [t] "=r"(tmp), [rotated] "=r"(rotated) \
        : [a] "r"(ae[index]), [b] "r"(be[index]), [i1] "i"(imm)); \
  } while (0)

// ❌ Fix `EOR ORR ROR`
#define EOR_ORR_ROR(ce, ae, be, imm, tmp, index)  \
  do {                                            \
    uint32_t rotated;                             \
    __asm__ volatile(                             \
        "ror %[rotated], %[b], %[i1]\n\t"         \
        "orr %[t], %[a], %[rotated]\n\t"         \
        "eor %[c], %[c], %[t]\n\t"               \
        : [c] "+r"(ce[index]), [t] "=r"(tmp), [rotated] "=r"(rotated) \
        : [a] "r"(ae[index]), [b] "r"(be[index]), [i1] "i"(imm)); \
  } while (0)

#endif  // ASM_H_
