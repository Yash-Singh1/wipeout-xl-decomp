#ifndef MPEGBITS_H
#define MPEGBITS_H

/* Bit reader macros for the MPEG video stream (MSVC /Od expands these in place).
   The accumulator vs->curBits holds vs->bitOffset valid low bits; bytes are
   shifted in from vs->bufStart as needed. */

extern unsigned long bitMask[];

#define READ_BYTE(vs) \
    do { \
        (vs)->curBits <<= 8; \
        (vs)->curBits += *(vs)->bufStart++; \
    } while (0)

/* Make sure at least n bits are available (n <= 8: three bytes fit, n <= 16: two). */
#define NEED_BITS3(vs, n) \
    if ((vs)->bitOffset < (n)) { \
        READ_BYTE(vs); \
        READ_BYTE(vs); \
        READ_BYTE(vs); \
        (vs)->bitOffset += 24; \
    }

#define NEED_BITS2(vs, n) \
    if ((vs)->bitOffset < (n)) { \
        READ_BYTE(vs); \
        READ_BYTE(vs); \
        (vs)->bitOffset += 16; \
    }

#define NEED_BITS_LOOP(vs, n) \
    while ((vs)->bitOffset < (n)) { \
        READ_BYTE(vs); \
        (vs)->bitOffset += 8; \
    }

/* Variant used by table lookups: loops, two bytes at a time. */
#define NEED_BITS2_LOOP(vs, n) \
    while ((vs)->bitOffset < (n)) { \
        READ_BYTE(vs); \
        READ_BYTE(vs); \
        (vs)->bitOffset += 16; \
    }

#define NEED_BIT1(vs) \
    if ((vs)->bitOffset == 0) { \
        READ_BYTE(vs); \
        READ_BYTE(vs); \
        READ_BYTE(vs); \
        (vs)->bitOffset += 24; \
    }

/* Consume n bits into result. */
#define TAKE_BITS(vs, n, result) \
    (vs)->bitOffset -= (n); \
    (result) = (vs)->curBits >> (vs)->bitOffset; \
    (vs)->curBits &= bitMask[(vs)->bitOffset];

/* Consume n bits and drop them. */
#define SKIP_BITS(vs, n) \
    (vs)->bitOffset -= (n); \
    (vs)->curBits &= bitMask[(vs)->bitOffset];

/* Look at the next n bits (after the caller made them available). */
#define PEEK_BITS(vs, n, result) \
    (result) = (vs)->curBits >> ((vs)->bitOffset - (n));

#define GET_BITS3(vs, n, result) NEED_BITS3(vs, n) TAKE_BITS(vs, n, result)
#define GET_BITS2(vs, n, result) NEED_BITS2(vs, n) TAKE_BITS(vs, n, result)
#define GET_BITS_LOOP(vs, n, result) NEED_BITS_LOOP(vs, n) TAKE_BITS(vs, n, result)
#define GET_BIT1(vs, result) NEED_BIT1(vs) TAKE_BITS(vs, 1, result)
#define SKIP_BIT1(vs) NEED_BIT1(vs) SKIP_BITS(vs, 1)

#endif
