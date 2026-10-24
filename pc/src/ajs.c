#include "ajs.h"
#include <string.h>

#define GPU_CODE_LINE_F2 0x40
#define GPU_CODE_LINE_G2 0x50
#define GPU_CODE_POLY_F3 0x20
#define GPU_CODE_POLY_FT3 0x24
#define GPU_CODE_POLY_G3 0x30
#define GPU_CODE_POLY_F4 0x28
#define GPU_CODE_POLY_FT4 0x2c
#define GPU_CODE_POLY_G4 0x38
#define GPU_CODE_POLY_GT4 0x3c
#define GPU_CODE_SPRT_8 0x74
#define GPU_CODE_SPRT_16 0x7c
#define GPU_CODE_SPRT 0x64
#define GPU_CODE_TILE_1 0x68
#define GPU_CODE_TILE_8 0x70
#define GPU_CODE_TILE_16 0x78
#define GPU_CODE_TILE 0x60
#define GPU_CODE_BLOCK_FILL 2
#define GPU_CODE_POLY_GT3 0x34
#define GPU_SEMITRANS_BIT 2
#define GPU_SHADE_TEXTURE_BIT 1
#define VRAM_PAGE_WIDTH 64
#define VRAM_PAGE_HEIGHT 256
#define VRAM_PAGES_PER_BAND 16

int UsesAlternateTexturePageLayout(void);

/* Blends the first two colour bytes of in1 and in2; p0 is the weight of in1 in 4096ths. */
void InterpolByte(u_char *in1, u_char *in2, long p0, u_char *result)
{
    float in2Weight;
    in2Weight = (4096 - (float)p0) / 4096;
    result[0] = (u_char)((in2[0] - in1[0]) * in2Weight) + in1[0];
    result[1] = (u_char)((in2[1] - in1[1]) * in2Weight) + in1[1];
}

/* The PC renderer completes drawing synchronously. */
int DrawSync(int mode) { return 1; }
/* Callback registration is unused by the PC renderer. */
void DrawSyncCallback(void (*callback)(void)) { }
/* The emulated graphics device is always ready. */
int ResetGraph(int mode) { return 1; }
/* Debug output is not provided by the graphics shim. */
int SetGraphDebug(int level) { return 1; }

/* Prepends one primitive to an ordering-table link. */
void AddPrim(u_long *ot, u_long *p)
{
    *p = *ot;
    *ot = (u_long)p;
}

/* Prepends a primitive chain to an ordering-table link. */
void AddPrims(u_long *ot, u_long *first, u_long *last)
{
    *last = *ot;
    *ot = (u_long)first;
}

/* Joins two primitive links. */
void CatPrim(u_long *first, u_long *next) { *first = *next; }
/* CLUT diagnostics are not used on the PC renderer. */
void DumpClut(u_short clut) { }
/* Texture-page diagnostics are not used on the PC renderer. */
void DumpTPage(u_short tpage) { }

/* Encodes the palette coordinates in a CLUT identifier. */
u_short GetClut(int x, int y)
{
    return (((unsigned)x >> 4) & 63) | (y << 6);
}

/* Encodes the texture page for the renderer's selected page layout. */
#ifdef EQUIV
#ifdef NETPLAY
u_short GetTPage(int tp, int abr, int x, int y)
{
    if (UsesAlternateTexturePageLayout())
        return ((y & 0x300) >> 3) | ((x & 0x3c0) >> 6) | ((abr & 3) << 7) | ((tp & 3) << 9);
    return ((x & 0x3c0) >> 6) | ((y & 0x100) >> 4) | ((abr & 3) << 5) | ((tp & 3) << 7);
}
#endif
#endif
#ifndef NETPLAY
u_short GetTPage(int tp, int abr, int x, int y)
{
    if (UsesAlternateTexturePageLayout()) {
        int page;
        page = (y >> 3) & 0x60;
        page |= (x >> 6) & 15;
        page |= (abr & 3) << 7;
        page |= (tp & 3) << 9;
        return page;
    } else {
        int page;
        page = (tp & 3) << 2;
        page |= abr & 3;
        page <<= 5;
        page |= (x >> 6) & 15;
        page |= (y >> 4) & 0x10;
        return page;
    }
}
#endif

/* Follows a primitive's link to the next primitive. */
u_long *NextPrim(u_long *p) { return (u_long *)*p; }

/* Sets the GPU opcode for a flat 2D line. */
void SetLineF2(LINE_F2 *p) { p->code = GPU_CODE_LINE_F2; }
/* Sets the GPU opcode for a Gouraud-shaded 2D line. */
void SetLineG2(LINE_G2 *p) { p->code = GPU_CODE_LINE_G2; }
/* Sets the GPU opcode for a flat untextured triangle. */
void SetPolyF3(POLY_F3 *p) { p->code = GPU_CODE_POLY_F3; }
/* Sets the GPU opcode for a flat textured triangle. */
void SetPolyFT3(POLY_FT3 *p) { p->code = GPU_CODE_POLY_FT3; }
/* Sets the GPU opcode for a Gouraud-shaded triangle. */
void SetPolyG3(POLY_G3 *p) { p->code = GPU_CODE_POLY_G3; }
/* Sets the GPU opcode for a flat untextured quadrilateral. */
void SetPolyF4(POLY_F4 *p) { p->code = GPU_CODE_POLY_F4; }
/* Sets the GPU opcode for a flat textured quadrilateral. */
void SetPolyFT4(POLY_FT4 *p) { p->code = GPU_CODE_POLY_FT4; }
/* Sets the GPU opcode for a Gouraud-shaded quadrilateral. */
void SetPolyG4(POLY_G4 *p) { p->code = GPU_CODE_POLY_G4; }
/* Sets the GPU opcode for a Gouraud-shaded textured quadrilateral. */
void SetPolyGT4(POLY_GT4 *p) { p->code = GPU_CODE_POLY_GT4; }

/* Enables or disables semi-transparency in a primitive's GPU command. */
void SetSemiTrans(void *p, int abe)
{
    if (abe) ((POLY_F3 *)p)->code |= GPU_SEMITRANS_BIT;
    else ((POLY_F3 *)p)->code &= ~GPU_SEMITRANS_BIT;
}

/* Enables or disables texture shading in a primitive's GPU command. */
void SetShadeTex(void *p, int tge)
{
    if (tge) ((POLY_F3 *)p)->code |= GPU_SHADE_TEXTURE_BIT;
    else ((POLY_F3 *)p)->code &= ~GPU_SHADE_TEXTURE_BIT;
}

/* Sets the opcode for an 8-by-8 sprite. */
void SetSprt8(SPRT *p) { p->code = GPU_CODE_SPRT_8; }
/* Sets the opcode for a 16-by-16 sprite. */
void SetSprt16(SPRT *p) { p->code = GPU_CODE_SPRT_16; }
/* Sets the opcode for a variable-size sprite. */
void SetSprt(SPRT *p) { p->code = GPU_CODE_SPRT; }
/* Sets the opcode for a 1-by-1 tile. */
void SetTile1(TILE_1 *p) { p->code = GPU_CODE_TILE_1; }
/* Sets the opcode for an 8-by-8 tile. */
void SetTile8(TILE_1 *p) { p->code = GPU_CODE_TILE_8; }
/* Sets the opcode for a 16-by-16 tile. */
void SetTile16(TILE_1 *p) { p->code = GPU_CODE_TILE_16; }
/* Sets the opcode for a variable-size tile. */
void SetTile(TILE *p) { p->code = GPU_CODE_TILE; }

/* Terminates a primitive link. */
void TermPrim(u_long *p) { *p = 0xffffffff; }

/* Copies the active display environment to the caller's structure. */
DISPENV *GetDispEnv(DISPENV *env)
{
    env->disp.x = g_DispEnv.disp.x;
    env->disp.y = g_DispEnv.disp.y;
    env->disp.w = g_DispEnv.disp.w;
    env->disp.h = g_DispEnv.disp.h;
    env->screen.x = g_DispEnv.screen.x;
    env->screen.y = g_DispEnv.screen.y;
    env->screen.w = g_DispEnv.screen.w;
    env->screen.h = g_DispEnv.screen.h;
    env->isinter = g_DispEnv.isinter;
    env->isrgb24 = g_DispEnv.isrgb24;
    env->pad0 = g_DispEnv.pad0;
    return env;
}

/* Selects the emulated display mask. */
int SetDispMask(int mask)
{
    g_mask = mask;
    return 1;
}

/* Copies the active drawing environment to the caller's structure. */
DRAWENV *GetDrawEnv(DRAWENV *env)
{
    u_char count;
    env->clip.x = g_DrawEnv.clip.x;
    env->clip.y = g_DrawEnv.clip.y;
    env->clip.w = g_DrawEnv.clip.w;
    env->clip.h = g_DrawEnv.clip.h;
    env->ofs[0] = g_DrawEnv.ofs[0];
    env->ofs[1] = g_DrawEnv.ofs[1];
    env->tw.x = g_DrawEnv.tw.x;
    env->tw.y = g_DrawEnv.tw.y;
    env->tw.w = g_DrawEnv.tw.w;
    env->tw.h = g_DrawEnv.tw.h;
    env->tpage = g_DrawEnv.tpage;
    env->dtd = g_DrawEnv.dtd;
    env->dfe = g_DrawEnv.dfe;
    env->isbg = g_DrawEnv.isbg;
    env->r0 = g_DrawEnv.r0;
    env->g0 = g_DrawEnv.g0;
    env->b0 = g_DrawEnv.b0;
    env->dr_env.tag = g_DrawEnv.dr_env.tag;
    for (count = 0; count < 15; count++)
        env->dr_env.code[count] = g_DrawEnv.dr_env.code[count];
    return env;
}

/* Initializes a display environment for the given drawing rectangle. */
DISPENV *SetDefDispEnv(DISPENV *disp, int x, int y, int w, int h)
{
    disp->disp.x = x;
    disp->disp.y = y;
    disp->disp.w = w;
    disp->disp.h = h;
    disp->screen.x = 0;
    disp->screen.y = 0;
    disp->screen.w = 0;
    disp->screen.h = 0;
    disp->isinter = 0;
    disp->isrgb24 = 0;
    return disp;
}

/* Initializes a drawing environment with the default texture page and flags. */
DRAWENV *SetDefDrawEnv(DRAWENV *draw, int x, int y, int w, int h)
{
    draw->clip.x = x;
    draw->clip.y = y;
    draw->clip.w = w;
    draw->clip.h = h;
    draw->ofs[0] = x;
    draw->ofs[1] = y;
    draw->tw.x = 0;
    draw->tw.y = 0;
    draw->tw.w = 0;
    draw->tw.h = 0;
    draw->tpage = 10;
    draw->dtd = 1;
    draw->dfe = 0;
    draw->isbg = 0;
    draw->r0 = 0;
    draw->g0 = 0;
    draw->b0 = 0;
    return draw;
}

/* Encodes drawing-environment fields into the GPU command words. */
void SetDrawEnv(DR_ENV *dr_env, DRAWENV *env)
{
    dr_env->code[1] = env->clip.x << 16;
    dr_env->code[1] += env->clip.y;
    dr_env->code[2] = env->clip.w << 16;
    dr_env->code[2] += env->clip.h;
    dr_env->code[3] = env->ofs[0] << 16;
    dr_env->code[3] += env->ofs[1];
    dr_env->code[4] = env->tw.x << 16;
    dr_env->code[4] += env->tw.y;
    dr_env->code[5] = env->tw.w << 16;
    /* Unlike the other fields this one is assigned, not added: the height replaces the width word. */
    dr_env->code[5] = env->tw.h;
    dr_env->code[6] = env->tpage << 16;
    dr_env->code[6] += env->dtd << 8;
    dr_env->code[7] = env->isbg << 24;
}

/* Encodes the drawing-mode fields into the GPU command. The texture-window argument is not used. */
void SetDrawMode(DR_MODE *dr_mode, int dfe, int dtd, int tpage, RECT *tw)
{
    dr_mode->code[1] = (u_char)dfe << 24;
    dr_mode->code[1] += (u_char)dtd << 16;
    dr_mode->code[1] += tpage;
    dr_mode->code[0] = 0x90;
    dr_mode->code[0] = dr_mode->code[0] << 24;
}

/* Colour byte for one 15-bit pixel, using the original's PC colour masks. */
#define PC_IMAGE_COLOUR(pixel) ((((pixel).red << 3) & 0xC0) >> 5 | (((pixel).green << 3) & 0xE0) >> 3 | (((pixel).blue << 3) & 0xE0))

/* Copies a rectangle of 15-bit pixels into the emulated VRAM. VRAM is kept as 64x256 pages,
   16 per 256-line band; a rectangle that crosses line 256 is written as two bands. Each pixel
   passes through the PC colour masks and is stored four times. */
#ifndef NETPLAY
int LoadImage(RECT *rect, u_long *pixels)
{
    short sourceX;
    short x, y, width, height;
    short startX, endX, row;
    signed char firstPage, lastPage, page, split;
    u_short *source, *word;
    u_char *dest;
    struct ImagePixel { u_short blue:5, green:5, red:5, transparent:1; } *pixel;

    x = rect->x;
    y = rect->y;
    width = rect->w;
    height = rect->h;
    if (y > 255) split = 0;
    else if (y + height > VRAM_PAGE_HEIGHT) split = 1;
    else split = 0;
    if (!split) {
        firstPage = x / VRAM_PAGE_WIDTH;
        lastPage = (x + width - 1) / VRAM_PAGE_WIDTH;
        if (y > 255) {
            firstPage += VRAM_PAGES_PER_BAND;
            lastPage += VRAM_PAGES_PER_BAND;
            y -= VRAM_PAGE_HEIGHT;
        }
        sourceX = 0;
        for (page = firstPage; page <= lastPage; page++) {
            if (page == firstPage) startX = x % VRAM_PAGE_WIDTH;
            else startX = 0;
            if (page == lastPage) endX = (x + width - 1) % VRAM_PAGE_WIDTH + 1;
            else endX = VRAM_PAGE_WIDTH;
            for (row = y; row < y + height; row++) {
                /* The PC routine uses absolute page offsets for this legacy destination. */
                dest = (u_char *)((((page << 8) + row) << 8) + startX);
                source = (u_short *)pixels + (row - y) * width + sourceX;
                for (word = source; word < source + (endX - startX); word++) {
                    pixel = (struct ImagePixel *)word;
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                }
            }
            sourceX += endX - startX;
        }
    } else {
        firstPage = x / VRAM_PAGE_WIDTH;
        lastPage = (x + width - 1) / VRAM_PAGE_WIDTH;
        sourceX = 0;
        for (page = firstPage; page <= lastPage; page++) {
            if (page == firstPage) startX = x % VRAM_PAGE_WIDTH;
            else startX = 0;
            if (page == lastPage) endX = (x + width - 1) % VRAM_PAGE_WIDTH + 1;
            else endX = VRAM_PAGE_WIDTH;
            for (row = y; row < VRAM_PAGE_HEIGHT; row++) {
                dest = (u_char *)((((page << 8) + row) << 8) + startX);
                source = (u_short *)pixels + (row - y) * width + sourceX;
                for (word = source; word < source + (endX - startX); word++) {
                    pixel = (struct ImagePixel *)word;
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                }
            }
            sourceX += endX - startX;
        }
        firstPage += VRAM_PAGES_PER_BAND;
        lastPage += VRAM_PAGES_PER_BAND;
        sourceX = 0;
        for (page = firstPage; page <= lastPage; page++) {
            if (page == firstPage) startX = x % VRAM_PAGE_WIDTH;
            else startX = 0;
            if (page == lastPage) endX = (x + width - 1) % VRAM_PAGE_WIDTH + 1;
            else endX = VRAM_PAGE_WIDTH;
            for (row = 0; row < y + height - VRAM_PAGE_HEIGHT; row++) {
                dest = (u_char *)((((page << 8) + row) << 8) + startX);
                source = (u_short *)pixels + (row - y + VRAM_PAGE_HEIGHT) * width + sourceX;
                for (word = source; word < source + (endX - startX); word++) {
                    pixel = (struct ImagePixel *)word;
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                }
            }
            sourceX += endX - startX;
        }
    }
    return 1;
}
#endif
#ifdef EQUIV
#ifdef NETPLAY
int LoadImage(RECT *rect, u_long *pixels)
{
    short sourceX;
    short x, y, width, height;
    short startX, endX, row;
    signed char firstPage, lastPage, page, split;
    u_short *source, *word;
    u_char *dest;
    struct ImagePixel { u_short blue:5, green:5, red:5, transparent:1; } *pixel;

    x = rect->x;
    y = rect->y;
    width = rect->w;
    height = rect->h;
    if (y > 255) split = 0;
    else if (y + height > VRAM_PAGE_HEIGHT) split = 1;
    else split = 0;
    if (!split) {
        firstPage = x / VRAM_PAGE_WIDTH;
        lastPage = (x + width - 1) / VRAM_PAGE_WIDTH;
        if (y > 255) {
            firstPage += VRAM_PAGES_PER_BAND;
            lastPage += VRAM_PAGES_PER_BAND;
            y -= VRAM_PAGE_HEIGHT;
        }
        sourceX = 0;
        for (page = firstPage; page <= lastPage; page++) {
            if (page == firstPage) startX = x % VRAM_PAGE_WIDTH;
            else startX = 0;
            if (page == lastPage) endX = (x + width - 1) % VRAM_PAGE_WIDTH + 1;
            else endX = VRAM_PAGE_WIDTH;
            for (row = y; row < y + height; row++) {
                /* The PC routine uses absolute page offsets for this legacy destination. */
                dest = (u_char *)((((page << 8) + row) << 8) + startX);
                source = (u_short *)pixels + (row - y) * width + sourceX;
                for (word = source; word < source + (endX - startX); word++) {
                    pixel = (struct ImagePixel *)word;
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                }
            }
            sourceX += endX - startX;
        }
    } else {
        firstPage = x / VRAM_PAGE_WIDTH;
        lastPage = (x + width - 1) / VRAM_PAGE_WIDTH;
        sourceX = 0;
        for (page = firstPage; page <= lastPage; page++) {
            if (page == firstPage) startX = x % VRAM_PAGE_WIDTH;
            else startX = 0;
            if (page == lastPage) endX = (x + width - 1) % VRAM_PAGE_WIDTH + 1;
            else endX = VRAM_PAGE_WIDTH;
            for (row = y; row < VRAM_PAGE_HEIGHT; row++) {
                dest = (u_char *)((((page << 8) + row) << 8) + startX);
                source = (u_short *)pixels + (row - y) * width + sourceX;
                for (word = source; word < source + (endX - startX); word++) {
                    pixel = (struct ImagePixel *)word;
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                }
            }
            sourceX += endX - startX;
        }
        firstPage += VRAM_PAGES_PER_BAND;
        lastPage += VRAM_PAGES_PER_BAND;
        sourceX = 0;
        for (page = firstPage; page <= lastPage; page++) {
            if (page == firstPage) startX = x % VRAM_PAGE_WIDTH;
            else startX = 0;
            if (page == lastPage) endX = (x + width - 1) % VRAM_PAGE_WIDTH + 1;
            else endX = VRAM_PAGE_WIDTH;
            for (row = 0; row < y + height - VRAM_PAGE_HEIGHT; row++) {
                dest = (u_char *)((((page << 8) + row) << 8) + startX);
                source = (u_short *)pixels + (row - y + VRAM_PAGE_HEIGHT) * width + sourceX;
                for (word = source; word < source + (endX - startX); word++) {
                    pixel = (struct ImagePixel *)word;
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                    *dest++ = PC_IMAGE_COLOUR(*pixel);
                }
            }
            sourceX += endX - startX;
        }
    }
    return 1;
}
#endif
#endif
#undef PC_IMAGE_COLOUR


/* Image movement is handled by the PC renderer's separate blitter. */
int MoveImage(RECT *rect, int x, int y) { return 1; }
/* Image storage is not needed by the PC renderer. */
int StoreImage(RECT *rect, u_long *pixels) { return 1; }
/* Callback reset is a no-op in the graphics shim. */
void ResetCallback(void) { }
/* Sets the opcode for a block-fill primitive. */
void SetBlockFill(BLK_FILL *p) { p->code = GPU_CODE_BLOCK_FILL; }
/* Sets the opcode for a Gouraud-shaded textured triangle. */
void SetPolyGT3(POLY_GT3 *p) { p->code = GPU_CODE_POLY_GT3; }

/* Copies a 4-bit CLUT into the renderer's palette storage. */
void LoadClut4(RECT *rect, u_short *source, u_short *cluts)
{
#ifdef NETPLAY
    /* The network build copies the CLUT with an element loop. */
    u_short *dst;
    int i;

    dst = cluts;
    for (i = 0; i < 16; i++) {
        *dst = *source;
        source++;
        dst++;
    }
#else
    memcpy(cluts, source, 32);
#endif
}

/* Copies an 8-bit CLUT into the renderer's palette storage. */
void LoadClut8(RECT *rect, u_short *source, u_short *cluts)
{
#ifdef NETPLAY
    /* The network build copies the CLUT with an element loop. */
    u_short *dst;
    int i;

    dst = cluts;
    for (i = 0; i < 256; i++) {
        *dst = *source;
        source++;
        dst++;
    }
#else
    memcpy(cluts, source, 512);
#endif
}

/* The 4-bit image loader is an unused successful compatibility stub. */
int LoadImage4(RECT *rect, u_long *pixels, u_short *cluts) { return 1; }
/* The 8-bit image loader is an unused successful compatibility stub. */
int LoadImage8(RECT *rect, u_long *pixels, u_short *cluts) { return 1; }

/* These compatibility entry points leave the return register untouched. */
DISPENV *PutDispEnv(DISPENV *env) { }
/* This compatibility entry point also leaves the return register untouched. */
DRAWENV *PutDrawEnv(DRAWENV *env) { }

#ifdef POWERVR
extern DISPENV DispEnv0;
extern void *g_menuDisplayEnv;
extern int PutDispEnvStub(int value);

void ResetRenderEnvironment(void)
{
    SetDefDispEnv(&DispEnv0, 0, 0, 0x140, 0xf0);
    SetGeomOffset(0xa0, 0x78);
    SetGeomScreen(0xa0);
    PutDispEnvStub((int)g_menuDisplayEnv);
}

#endif
