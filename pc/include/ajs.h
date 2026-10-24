#ifndef AJS_H
#define AJS_H

#include "psyq.h"

extern DISPENV g_DispEnv;
extern DRAWENV g_DrawEnv;
extern int g_mask;
void InterpolByte(u_char *in1, u_char *in2, long position, u_char *result);
void DrawSyncCallback(void (*callback)(void));
int SetGraphDebug(int level);
void AddPrims(u_long *ot, u_long *first, u_long *last);
void CatPrim(u_long *first, u_long *next);
void DumpClut(u_short clut);
void DumpTPage(u_short tpage);
u_long *NextPrim(u_long *p);
void SetSprt8(SPRT *p);
void SetSprt16(SPRT *p);
void SetSprt(SPRT *p);
void SetTile1(TILE_1 *p);
void SetTile8(TILE_1 *p);
void SetTile16(TILE_1 *p);
void TermPrim(u_long *p);
DISPENV *GetDispEnv(DISPENV *env);
DRAWENV *GetDrawEnv(DRAWENV *env);
void SetDrawEnv(DR_ENV *primitive, DRAWENV *env);
void ResetCallback(void);
void SetBlockFill(BLK_FILL *p);
void LoadClut4(RECT *rect, u_short *source, u_short *cluts);
void LoadClut8(RECT *rect, u_short *source, u_short *cluts);
int LoadImage4(RECT *rect, u_long *pixels, u_short *cluts);
int LoadImage8(RECT *rect, u_long *pixels, u_short *cluts);

#endif
