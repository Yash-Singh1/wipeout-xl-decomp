#include "mpeg.h"

#define MPEG_JIT_CODE_BYTES 0x9c40
#define MPEG_JIT_JUMP_TABLE_BYTES 0x400
#define MPEG_JIT_MASK_COUNT 0x100
#define MPEG_JIT_MASK_BITS 8
#define X86_NOP 0x90
#define X86_SHORT_BRANCH_MAX 0x7e
#define X86_JNZ_SHORT 0x75
#define X86_JNZ_NEAR_PREFIX 0x0f
#define X86_JNZ_NEAR 0x85
#define X86_JLE_SHORT 0x7e

/* Writes a 32-bit little-endian immediate. */
#define EMIT32(x) \
    do { \
        *p++ = (x); \
        *p++ = (x) >> 8; \
        *p++ = (x) >> 16; \
        *p++ = (x) >> 24; \
    } while (0)

/* Generates 256 small machine-code routines, one per bit mask of a byte. Each one
   walks the set bits, copying the matching entries of the table at `base` (the
   scale table, mpegScaleTable) into the block structure and looping until all bits
   are handled. The routines are reached through mpegJumpTable[mask]. */
short mpegBuildDecodeJumpTable(VidStream *vs, unsigned long base)
{
    unsigned char *p;
    int base0;
    int next;
    int ii;
    int length;
    int loopStart;
    int nn;
    unsigned long addr;
    int opcode;
    int shift;

    mpegCodeBase = mpegHeapAlloc(&vs->heap, MPEG_JIT_CODE_BYTES);
    mpegJumpTable = mpegHeapAlloc(&vs->heap, MPEG_JIT_JUMP_TABLE_BYTES);
    p = mpegCodeBase;
    for (ii = 0; ii < MPEG_JIT_MASK_COUNT; ii++) {
        nn = 0;
        for (shift = 0; shift < MPEG_JIT_MASK_BITS; shift++) {
            if (ii & (1 << shift)) {
                nn = shift;
                break;
            }
        }
        base0 = nn;
        if (shift != MPEG_JIT_MASK_BITS) {
            length = nn ? 0x10 : 0xf;
            while ((((int)(p + length)) & 3) != 0)
                *p++ = X86_NOP;
        }
        mpegJumpTable[ii] = p;
        if (shift != MPEG_JIT_MASK_BITS) {
            *p++ = 0x66;
            *p++ = 0x8b;
            if (nn) {
                *p++ = 0x53;
                *p++ = nn << 4;
            } else {
                *p++ = 0x13;
            }
            *p++ = 0x81;
            *p++ = 0xe2;
            *p++ = 0xf0;
            *p++ = 0x7f;
            *p++ = 0;
            *p++ = 0;
            addr = (nn << 15) + base;
            *p++ = 0x8d;
            *p++ = 0xb2;
            EMIT32(addr);
            loopStart = (int)p;
            do {
                next = 0;
                for (shift++; shift < MPEG_JIT_MASK_BITS; shift++) {
                    if (ii & (1 << shift)) {
                        next = shift;
                        break;
                    }
                }
                if (shift == MPEG_JIT_MASK_BITS)
                    next = base0;
                opcode = nn == base0 ? 0x8b : 3;
                addr = (base0 == next ? 2 : 0) + (next << 4);
                *p++ = 0x66;
                *p++ = 0x8b;
                if (addr) {
                    *p++ = 0x53;
                    *p++ = addr;
                } else {
                    *p++ = 0x13;
                }
                *p++ = opcode;
                *p++ = 6;
                *p++ = opcode;
                *p++ = 0x4e;
                *p++ = 4;
                *p++ = 0x81;
                *p++ = 0xe2;
                *p++ = 0xf0;
                *p++ = 0x7f;
                *p++ = 0;
                *p++ = 0;
                *p++ = opcode;
                *p++ = X86_JLE_SHORT;
                *p++ = 8;
                *p++ = opcode;
                *p++ = 0x6e;
                *p++ = 0xc;
                addr = (next << 15) + base;
                *p++ = 0x8d;
                *p++ = 0xb2;
                EMIT32(addr);
                nn = next;
            } while (shift != MPEG_JIT_MASK_BITS);
            addr = (unsigned long)vs->decodeBlock - ((unsigned long)vs->codeBuf << 3);
            *p++ = 0x8d;
            *p++ = 0x14;
            *p++ = 0xdd;
            EMIT32(addr);
            *p++ = 0x83;
            *p++ = 0xc3;
            *p++ = 2;
            *p++ = 0x89;
            *p++ = 2;
            *p++ = 0x89;
            *p++ = 0x4a;
            *p++ = 4;
            *p++ = 0x89;
            *p++ = 0x7a;
            *p++ = 8;
            *p++ = 0x89;
            *p++ = 0x6a;
            *p++ = 0xc;
            *p++ = 0xf6;
            *p++ = 0xc3;
            *p++ = 0xf;
            length = (int)p - loopStart;
            if (length <= X86_SHORT_BRANCH_MAX) {
                *p++ = X86_JNZ_SHORT;
                *p++ = -2 - length;
            } else {
                *p++ = X86_JNZ_NEAR_PREFIX;
                *p++ = X86_JNZ_NEAR;
                length = -6 - length;
                *p++ = length;
                if (length >= -256)
                    *p = 0xff;
                else
                    *p = 0xfe;
                p++;
                *p++ = 0xff;
                *p++ = 0xff;
            }
        }
        *p++ = 0xc3;
    }
    return 0;
}
