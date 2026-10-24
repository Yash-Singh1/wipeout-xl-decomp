#ifndef PSYQ_H
#define PSYQ_H

/* PlayStation runtime library types and prototypes used by the intro movie player. */

#include "common.h"

/* libgpu */

typedef struct {
    short x, y;
    short w, h;
} RECT;

typedef struct {
    u_long tag;
    u_long code[15];
} DR_ENV;

typedef struct {
    RECT    clip;
    short   ofs[2];
    RECT    tw;
    u_short tpage;
    u_char  dtd;
    u_char  dfe;
    u_char  isbg;
    u_char  r0, g0, b0;
    DR_ENV  dr_env;
} DRAWENV;

typedef struct {
    RECT   disp;
    RECT   screen;
    u_char isinter;
    u_char isrgb24;
    u_char pad0, pad1;
} DISPENV;

/* libcd (CdControl and CdlLOC are declared in iso9660.h / avplay.h) */

int  CdRead2(int mode);
void CdDataCallback(void (*func)(void));

/* libds (streaming) */

void StSetRing(u_long *ring, int sectors);
void StClearRing(void);
void StUnSetRing(void);
void StSetStream(int isMode, long start, long end, long chan, long unused);

/* libgpu */

int  ResetGraph(int mode);
void SetGraphDebug(int level);
void SetDispMask(int mask);
void DrawSync(int mode);
void VSync(int mode);
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h);
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h);
DISPENV *PutDispEnv(DISPENV *env);
DRAWENV *PutDrawEnv(DRAWENV *env);

#endif /* PSYQ_H */
