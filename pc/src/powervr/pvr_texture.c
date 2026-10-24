#include "functions.h"

extern DISPENV DispEnv0;
extern void *g_menuDisplayEnv;
DISPENV *PutDispEnvStub(DISPENV *env);

void func_425ad0(void)
{
}

void ResetRenderEnvironment2(void)
{
    SetDefDispEnv(&DispEnv0, 0, 0, 0x140, 0xf0);
    SetGeomOffset(0xa0, 0x78);
    SetGeomScreen(0xa0);
    PutDispEnvStub(g_menuDisplayEnv);
}
