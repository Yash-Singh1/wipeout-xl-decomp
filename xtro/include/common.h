#ifndef COMMON_H
#define COMMON_H

/* Shared types for the intro movie player. */

/* Under -G8, GCC emits a file's functions after its top-level asm blocks, so in the
 * default build a C function must come after every INCLUDE_ASM of its file.
 * The NON_MATCHING build wraps each INCLUDE_ASM in a function to keep file order;
 * its string constants then sit ahead of the asm's (check with fdiff.py --strings). */
#ifdef NON_MATCHING
#include "include_asm_g8.h"
#else
#include "include_asm.h"
#endif

typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;

#endif /* COMMON_H */
