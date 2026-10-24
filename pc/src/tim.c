#include "functions.h"

#include "heap_state.h"
extern char errorString[];
extern short clutX, clutY, textureX, textureY;
#define TextureHOffset(x) ((x) & 0xffc0)
#define TextureVOffset(y) ((y) & 0x100)
#define TPAGE(x,y) (((x) >> 6) + (((y) & 0x100) >> 4))
#define TSB(c,a,t) (((c) << 7) | ((a) << 5) | (t))
#ifdef NETPLAY
/* NET build: the colour look-up Y is scaled with a multiply (MSVC /Od orders the operands as the NET object does). */
#define CBA(x,y) ((x) | ((y) * 64))
#else
#define CBA(x,y) (((y) << 6) | (x))
#endif

/* name and length are assigned to EBX and EDI in the reverse order. */
#ifdef EQUIV
/* Loads a TIM image and creates a texture descriptor for it. */
#ifdef NETPLAY
/* NET build: the frame is laid out from these names (MSVC /Od). */
Texture *LoadTexture(char *name, short translucent)
{
    long fileLen;
    int xPos;
    Tim *tim;
    TimClut4 *tim4;
    RECT rect;
    TimClut8 *tc8;
    int y;
    Texture *texture;

    fileLen = FileLength(name);
    if (fileLen <= 0) {
        FileAccessError(name);
        wsprintf(errorString, "Tim.c:LoadTexture: File %s not found\n", name);
        Error(errorString, 0);
        return 0;
    }
    tim = DAlloc(reserveHeap, fileLen + 32);
    if (!tim) {
        wsprintf(errorString, "Tim.c:LoadTexture(): Failed to allocate memory for file %s\n", name);
        Error(errorString, Fatal);
    }
    if (LoadFile(name, (char *)tim) != fileLen) {
        wsprintf(errorString, "Tim.c:LoadTexture(): Failed to load file %s\n", name);
        Error(errorString, Fatal);
    }
    if ((tim->flags & 7) == 0) {
        tim4 = (TimClut4 *)tim;
        texture = DAlloc(heap, sizeof(Texture));
        texture->type = 0;
        if (!tim4->textureX && !tim4->textureY) {
            tim4->clutX = clutX;
            tim4->clutY = clutY;
            tim4->clutW = 16;
            tim4->clutH = 1;
            tim4->textureX = textureX;
            tim4->textureY = textureY;
            clutX += 16;
            if (clutX >= 384) {
                clutX = 320;
                clutY += 1;
            }
            textureY += 32;
            if (textureY >= 256) {
                textureX += 8;
                textureY = 0;
            }
        }
        texture->clutX = tim4->clutX;
        texture->clutY = tim4->clutY;
        texture->clutW = tim4->clutW;
        texture->clutH = tim4->clutH;
        texture->textureX = tim4->textureX;
        texture->textureY = tim4->textureY;
        texture->textureW = tim4->textureW;
        texture->textureH = tim4->textureH;
        xPos = tim4->textureX - TextureHOffset(tim4->textureX);
        y = tim4->textureY - TextureVOffset(tim4->textureY);
        texture->u0 = xPos << 2;
        texture->v0 = y;
        texture->u1 = ((xPos + tim4->textureW) << 2) - 1;
        texture->v1 = y;
        texture->u2 = xPos << 2;
        texture->v2 = (y + tim4->textureH) - 1;
        texture->u3 = ((xPos + tim4->textureW) << 2) - 1;
        texture->v3 = (y + tim4->textureH) - 1;
        texture->tsb = TSB(0, translucent, TPAGE(texture->textureX, texture->textureY));
        texture->cba = CBA(texture->clutX >> 4, texture->clutY);
        rect.x = tim4->clutX;
        rect.y = tim4->clutY;
        rect.w = tim4->clutW;
        rect.h = tim4->clutH;
        LoadImageStub(&rect, (unsigned long *)&tim4->clut);
        DrawSync(0);
        rect.x = tim4->textureX;
        rect.y = tim4->textureY;
        rect.w = tim4->textureW;
        rect.h = tim4->textureH;
        LoadImageStub(&rect, (unsigned long *)(tim4 + 1));
        DrawSync(0);
    }
    else if ((tim->flags & 7) == 1) {
        tc8 = (TimClut8 *)tim;
        texture = DAlloc(heap, sizeof(Texture));
        texture->type = 1;
        if (!tc8->textureX && !tc8->textureY)
            Error("tim.c:LoadTexture(): Cannot auto place 8 bit textures", Fatal);
        texture->clutX = tc8->clutX;
        texture->clutY = tc8->clutY;
        texture->clutW = tc8->clutW;
        texture->clutH = tc8->clutH;
        texture->textureX = tc8->textureX;
        texture->textureY = tc8->textureY;
        texture->textureW = tc8->textureW;
        texture->textureH = tc8->textureH;
        xPos = tc8->textureX - TextureHOffset(tc8->textureX);
        y = tc8->textureY - TextureVOffset(tc8->textureY);
        texture->u0 = xPos << 1;
        texture->v0 = y;
        texture->u1 = ((xPos + tc8->textureW) << 1) - 1;
        texture->v1 = y;
        texture->u2 = xPos << 1;
        texture->v2 = (y + tc8->textureH) - 1;
        texture->u3 = ((xPos + tc8->textureW) << 1) - 1;
        texture->v3 = (y + tc8->textureH) - 1;
        texture->tsb = TSB(1, translucent, TPAGE(texture->textureX, texture->textureY));
        texture->cba = CBA(texture->clutX >> 4, texture->clutY);
        rect.x = tc8->clutX;
        rect.y = tc8->clutY;
        rect.w = tc8->clutW;
        rect.h = tc8->clutH;
        LoadImageStub(&rect, (unsigned long *)&tc8->clut);
        DrawSync(0);
        rect.x = tc8->textureX;
        rect.y = tc8->textureY;
        rect.w = tc8->textureW;
        rect.h = tc8->textureH;
        LoadImageStub(&rect, (unsigned long *)(tc8 + 1));
        DrawSync(0);
    }
    else {
        texture = 0;
    }
    DFree(reserveHeap, (char *)tim);
    tim = 0;
    return texture;
}
#else
Texture *LoadTexture(char *name, short translucent)
{
    Tim *tim;
    TimClut4 *tc4;
    TimClut8 *tc8;
    Texture *texture;
    long length;
    RECT rect;
    int x, y;

    length = FileLength(name);
    if (length <= 0) {
        FileAccessError(name);
        wsprintf(errorString, "Tim.c:LoadTexture: File %s not found\n", name);
        Error(errorString, 0);
        return 0;
    }
    tim = DAlloc(reserveHeap, length + 32);
    if (!tim) {
        wsprintf(errorString, "Tim.c:LoadTexture(): Failed to allocate memory for file %s\n", name);
        Error(errorString, Fatal);
    }
    if (LoadFile(name, (char *)tim) != length) {
        wsprintf(errorString, "Tim.c:LoadTexture(): Failed to load file %s\n", name);
        Error(errorString, Fatal);
    }
    if ((tim->flags & 7) == 0) {
        tc4 = (TimClut4 *)tim;
        texture = DAlloc(heap, sizeof(Texture));
        texture->type = 0;
        if (!tc4->textureX && !tc4->textureY) {
            tc4->clutW = 16;
            tc4->clutH = 1;
            tc4->clutX = clutX;
            tc4->clutY = clutY;
            tc4->textureX = textureX;
            tc4->textureY = textureY;
            clutX += 16;
            if (clutX >= 384) {
                clutX = 320;
                clutY += 1;
            }
            textureY += 32;
            if (textureY >= 256) {
                textureX += 8;
                textureY = 0;
            }
        }
        texture->clutX = tc4->clutX;
        texture->clutY = tc4->clutY;
        texture->clutW = tc4->clutW;
        texture->clutH = tc4->clutH;
        texture->textureX = tc4->textureX;
        texture->textureY = tc4->textureY;
        texture->textureW = tc4->textureW;
        texture->textureH = tc4->textureH;
        x = tc4->textureX - TextureHOffset(tc4->textureX);
        y = tc4->textureY - TextureVOffset(tc4->textureY);
        texture->u0 = x << 2;
        texture->v0 = y;
        texture->u1 = ((x + tc4->textureW) << 2) - 1;
        texture->v1 = y;
        texture->u2 = x << 2;
        texture->v2 = (y + tc4->textureH) - 1;
        texture->u3 = ((x + tc4->textureW) << 2) - 1;
        texture->v3 = (y + tc4->textureH) - 1;
        texture->tsb = TSB(0, translucent, TPAGE(texture->textureX, texture->textureY));
        texture->cba = CBA(texture->clutX >> 4, texture->clutY);
        rect.x = tc4->clutX;
        rect.y = tc4->clutY;
        rect.w = tc4->clutW;
        rect.h = tc4->clutH;
        LoadImageStub(&rect, (unsigned long *)&tc4->clut);
        DrawSync(0);
        rect.x = tc4->textureX;
        rect.y = tc4->textureY;
        rect.w = tc4->textureW;
        rect.h = tc4->textureH;
        LoadImageStub(&rect, (unsigned long *)(tc4 + 1));
        DrawSync(0);
    }
    else if ((tim->flags & 7) == 1) {
        tc8 = (TimClut8 *)tim;
        texture = DAlloc(heap, sizeof(Texture));
        texture->type = 1;
        if (!tc8->textureX && !tc8->textureY)
            Error("tim.c:LoadTexture(): Cannot auto place 8 bit textures", Fatal);
        texture->clutX = tc8->clutX;
        texture->clutY = tc8->clutY;
        texture->clutW = tc8->clutW;
        texture->clutH = tc8->clutH;
        texture->textureX = tc8->textureX;
        texture->textureY = tc8->textureY;
        texture->textureW = tc8->textureW;
        texture->textureH = tc8->textureH;
        x = tc8->textureX - TextureHOffset(tc8->textureX);
        y = tc8->textureY - TextureVOffset(tc8->textureY);
        texture->u0 = x << 1;
        texture->v0 = y;
        texture->u1 = ((x + tc8->textureW) << 1) - 1;
        texture->v1 = y;
        texture->u2 = x << 1;
        texture->v2 = (y + tc8->textureH) - 1;
        texture->u3 = ((x + tc8->textureW) << 1) - 1;
        texture->v3 = (y + tc8->textureH) - 1;
        texture->tsb = TSB(1, translucent, TPAGE(texture->textureX, texture->textureY));
        texture->cba = CBA(texture->clutX >> 4, texture->clutY);
        rect.x = tc8->clutX;
        rect.y = tc8->clutY;
        rect.w = tc8->clutW;
        rect.h = tc8->clutH;
        LoadImageStub(&rect, (unsigned long *)&tc8->clut);
        DrawSync(0);
        rect.x = tc8->textureX;
        rect.y = tc8->textureY;
        rect.w = tc8->textureW;
        rect.h = tc8->textureH;
        LoadImageStub(&rect, (unsigned long *)(tc8 + 1));
        DrawSync(0);
    }
    else {
        texture = 0;
    }
    DFree(reserveHeap, (char *)tim);
    return texture;
}
#endif
#endif
