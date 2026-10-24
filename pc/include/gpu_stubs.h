#ifndef GPU_STUBS_H
#define GPU_STUBS_H

#include "wipeout.h"

/* PC compatibility stubs retain the environment pointer passed by game code. */
DISPENV *PutDispEnvStub(DISPENV *env);
DRAWENV *PutDrawEnvStub(DRAWENV *env);
void DrawPrim(void *primitive);

#endif
