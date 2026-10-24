#ifndef TIM_H
#define TIM_H

#include "wipeout.h"

/* TIM file header: id, flags (low 3 bits: CLUT type). */
typedef struct Tim {
    long id;
    long flags;
} Tim;

/* TIM with a 16-entry CLUT (4-bit pixels). */
typedef struct TimClut4 {
    long   id;
    long   flags;
    long   clutBytes;
    short  clutX, clutY, clutW, clutH;
    short  clut[16];
    long   textureBytes;
    short  textureX, textureY, textureW, textureH;
} TimClut4;

/* TIM with a 256-entry CLUT (8-bit pixels). */
typedef struct TimClut8 {
    long   id;
    long   flags;
    long   clutBytes;
    short  clutX, clutY, clutW, clutH;
    short  clut[256];
    long   textureBytes;
    short  textureX, textureY, textureW, textureH;
} TimClut8;

#endif
