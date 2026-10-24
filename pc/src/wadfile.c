#include "functions.h"
#include "camera.h"
#include "d3dpoly.h"
#include <stdio.h>
#include "pc.h"
#include <math.h>
#pragma intrinsic(sin)

extern int wadSize;
extern int wadRemaining;
extern int loadPercent;
extern WADFILE *currentWad;
extern int *particleX;
extern int *particleY;
extern int *particleVelX;
extern int *particleVelY;
extern int *particleLife;
extern POLY_F3 *particlePolys;
extern int loadParticleIndex;
extern int loadLineIndex;
extern LINE_F2 *linePrims;
extern short loadFrame;
extern u_char vsyncBusy;
extern signed char barHeight[];
extern int waveIndex;

extern ConfigData *gameData;
extern short ownShip;
extern short CurrentScreen;
extern int quadIndex;
extern char numberText[];
void DrawLineDepth(int x0, int y0, int x1, int y1, u_char r, u_char g, u_char b);

extern int scanPhase;
extern BLK_FILL *screenFillTop;
extern BLK_FILL *screenFill1;
#include "heap_state.h"
int newRand(void);
void Init2DParticle(void);
void DrawLine(int x0, int y0, int x1, int y1, u_char r, u_char g, u_char b);

void FileAccessError(char *fileName);
long ReadFileBytes(int handle, void *buffer, long size);
int iso_close(int handle);

/* Adds a particle at the given screen position. */
void Insert2DParticle(int x, int y)
{
    particleX[loadParticleIndex] = x << 7;
    particleY[loadParticleIndex] = y << 7;
    particleVelX[loadParticleIndex] = newRand() % 512 - 256;
    particleVelY[loadParticleIndex] = -(newRand() % 128 - 32);
    particleLife[loadParticleIndex] = newRand() % 128;
    loadParticleIndex = (loadParticleIndex + 1) % 128;
}

/* Updates the position and lifetime of each screen particle. */
void Update2DParticle(void)
{
    int i;

    for (i = 0; i < 128; i++) {
        if (particleLife[i] > 0) {
            particleLife[i] -= 3;
            particlePolys[i].r0 = (u_char)particleLife[i] * 2;
            particlePolys[i].g0 = (u_char)particleLife[i] * 2;
            particlePolys[i].b0 = 0;
            particleX[i] += particleVelX[i];
            particleY[i] += particleVelY[i];
            particleVelY[i] += 1;
            particlePolys[i].x0 = particleX[i] >> 7;
            particlePolys[i].y0 = particleY[i] >> 7;
            particlePolys[i].x1 = (particleX[i] >> 7) + 1;
            particlePolys[i].y1 = particleY[i] >> 7;
            particlePolys[i].x2 = particleX[i] >> 7;
            particlePolys[i].y2 = (particleY[i] >> 7) + 1;
            AddPolyPrim(0, &particlePolys[i]);
        }
    }
}

/* Allocates and initializes the screen particle arrays. */
void Init2DParticle(void)
{
    int i;

    particlePolys = DAlloc(reserveHeap, 0xa00);
    particleX = DAlloc(reserveHeap, 0x200);
    particleY = DAlloc(reserveHeap, 0x200);
    particleVelX = DAlloc(reserveHeap, 0x200);
    particleVelY = DAlloc(reserveHeap, 0x200);
    particleLife = DAlloc(reserveHeap, 0x200);
    for (i = 0; i < 128; i++) {
        SetPolyF3(&particlePolys[i]);
        particleLife[i] = 0;
        particleX[i] = (newRand() % 320) << 7;
        particleY[i] = (newRand() % 240) << 7;
        particleVelX[i] = (newRand() % 9) - 4;
        particleVelX[i] = -(newRand() % 4);
    }
}

/* Frees the screen particle arrays. */
void Close2DParticle(void)
{
    DFree(reserveHeap, (char *)particlePolys);
    particlePolys = 0;
    DFree(reserveHeap, (char *)particleX);
    particleX = 0;
    DFree(reserveHeap, (char *)particleY);
    particleY = 0;
    DFree(reserveHeap, (char *)particleVelX);
    particleVelX = 0;
    DFree(reserveHeap, (char *)particleVelY);
    particleVelY = 0;
    DFree(reserveHeap, (char *)particleLife);
    particleLife = 0;
}

/* Initializes the loading screen and its ship models. */
void InitPop(void)
{
    int i;

    loadFrame = 0;
    linePrims = DAlloc(reserveHeap, 0x2000);
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    vsyncBusy = 0;
    for (i = 0; i < 15; i++) {
        InitMatrix(&shipShapeTable[i]->skeleton->relative);
        InitMatrix(&shipShapeTable[i]->skeleton->absolute);
        shipShapeTable[i]->skeleton->update = 1;
    }
    Init2DParticle();
    for (i = 0; i < 128; i++) {
        while (0)
            ;
        barHeight[i] = ((int)(sin((double)(i * 16) / 4096.0 * 6.28318530718) * 4096.0) * 12 >> 12) + 20;
    }
    scanPhase = 0;
    loadPercent = 0;
    screenFillTop = BlockFill(0, 80, 320, 100, 0, 0, 46);
    screenFill1 = BlockFill(0, 336, 320, 100, 0, 0, 46);
}

/* Adds a colored line to the loading screen. */
void DrawLine(int x0, int y0, int x1, int y1, u_char r, u_char g, u_char b)
{
    if (loadLineIndex > 256)
        return;
    SetLineF2(&linePrims[loadLineIndex]);
    linePrims[loadLineIndex].x0 = x0;
    linePrims[loadLineIndex].y0 = y0;
    linePrims[loadLineIndex].x1 = x1;
    linePrims[loadLineIndex].y1 = y1;
    linePrims[loadLineIndex].r0 = r;
    linePrims[loadLineIndex].g0 = g;
    linePrims[loadLineIndex].b0 = b;
    AddLinePrim(0, &linePrims[loadLineIndex], 0);
    loadLineIndex++;
    if (loadLineIndex >= 256)
        loadLineIndex = 0;
}

/* Adds a colored line at the loading screen's depth layer. */
void DrawLineDepth(int x0, int y0, int x1, int y1, u_char r, u_char g, u_char b)
{
    if (loadLineIndex > 256)
        return;
    SetLineF2(&linePrims[loadLineIndex]);
    linePrims[loadLineIndex].x0 = x0;
    linePrims[loadLineIndex].y0 = y0;
    linePrims[loadLineIndex].x1 = x1;
    linePrims[loadLineIndex].y1 = y1;
    linePrims[loadLineIndex].r0 = r;
    linePrims[loadLineIndex].g0 = g;
    linePrims[loadLineIndex].b0 = b;
    AddLinePrim(0.1f, &linePrims[loadLineIndex], 0);
    loadLineIndex++;
    if (loadLineIndex >= 256)
        loadLineIndex = 0;
}

/* Draws a rectangle from four loading screen lines. */
void DrawBox(int x, int y, int w, int h, u_char r, u_char g, u_char b)
{
    DrawLine(x, y, x + w, y, r, g, b);
    DrawLine(x + w, y, x + w, y + h, r, g, b);
    DrawLine(x + w, y + h, x, y + h, r, g, b);
    DrawLine(x, y + h, x, y, r, g, b);
}

/* Draws and updates the loading screen once per frame. */
#ifndef NETPLAY
void VsyncFunc(void)
{
    short gr;
    unsigned char lineGrey;
    short wave;
    short x;
    WADFILE *wad;
    short tintR;
    short tintB;
    short dir;
    unsigned char lineWhite;

    lineWhite = 0xff;
    lineGrey = 0xc0;
    loadFrame++;
    wad = currentWad;
    DrawSync(0);
    quadIndex = CurrentScreen * 300;
    AddText("LOADING", 0xa0, 0x56, 8, 0);
    AddJustifyNumber((loadPercent / 100) % 10, 0x30, 0x62, 8, 0);
    AddJustifyNumber((loadPercent / 10) % 10, 0x3e, 0x62, 8, 0);
    AddJustifyNumber(loadPercent % 10, 0x4c, 0x62, 8, 0);
    Print("  LOAD          33    33    33", 0x20, 0x70, 0);
    if (loadFrame > 0x46)
        Print("VENUE", 0x20, 0x78, 0);
    if (loadFrame > 0x1e)
        Print("LOCATION", 0x20, 0x7f, 0);
    if (loadFrame > 0x32)
        Print("LENGTH", 0x20, 0x86, 0);
    if (loadFrame > 0x14)
        Print("HEIGHT", 0x20, 0x8d, 0);
    if (loadFrame > 0x28)
        Print("CLASS", 0x20, 0x94, 0);
    if (loadFrame > 0x3c)
        Print("TEAM", 0x20, 0x9b, 0);
    if (gameData->shipClass == 0)
        Print("VECTOR", 0x50, 0x94, 0);
    if (gameData->shipClass == 1)
        Print("VENOM", 0x50, 0x94, 0);
    if (gameData->shipClass == 2)
        Print("RAPIER", 0x50, 0x94, 0);
    if (gameData->shipClass == 3)
        Print("PHANTOM", 0x50, 0x94, 0);
    if (ownShip >= 0 && ownShip <= 2)
        Print("AG SYSTEMS", 0x50, 0x9b, 0);
    else if (ownShip >= 3 && ownShip <= 5)
        Print("AURICOM", 0x50, 0x9b, 0);
    else if (ownShip >= 6 && ownShip <= 8)
        Print("QIREX", 0x50, 0x9b, 0);
    else if (ownShip >= 9 && ownShip <= 11)
        Print("FEISAR", 0x50, 0x9b, 0);
    else if (ownShip >= 12 && ownShip <= 14)
        Print("PIRANHA", 0x50, 0x9b, 0);
    Print(GetTrackName(gameData->actTrackNum, 0, 0, 0), 0x50, 0x78, 0);
    switch (gameData->actTrackNum) {
    case 1:
        Print("CANADA", 0x50, 0x7f, 0);
        Print("3.2 KM", 0x50, 0x86, 0);
        Print("74 M", 0x50, 0x8d, 0);
        tintR = 255;
        gr = 0;
        tintB = 0;
        break;
    case 2:
        Print("FRANCE", 0x50, 0x7f, 0);
        Print("3.5 KM", 0x50, 0x86, 0);
        Print("179 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 255;
        tintB = 0;
        break;
    case 6:
        Print("SOUTH PACIFIC", 0x50, 0x7f, 0);
        Print("5.1 KM", 0x50, 0x86, 0);
        Print("97 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 0;
        tintB = 255;
        break;
    case 7:
        Print("NORTH AMERICA", 0x50, 0x7f, 0);
        Print("4.0 KM", 0x50, 0x86, 0);
        Print("82 M", 0x50, 0x8d, 0);
        tintR = 255;
        gr = 255;
        tintB = 0;
        break;
    case 8:
        Print("NEPAL", 0x50, 0x7f, 0);
        Print("4.3 KM", 0x50, 0x86, 0);
        Print("158 M", 0x50, 0x8d, 0);
        tintR = 255;
        gr = 0;
        tintB = 255;
        break;
    case 13:
        Print("CHILE", 0x50, 0x7f, 0);
        Print("3.9 KM", 0x50, 0x86, 0);
        Print("220 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 255;
        tintB = 255;
        break;
    case 17:
        Print("BLACK SEA", 0x50, 0x7f, 0);
        Print("4.4 KM", 0x50, 0x86, 0);
        Print("121 M", 0x50, 0x8d, 0);
        tintR = 128;
        gr = 32;
        tintB = 255;
        break;
    case 20:
        Print("GERMANY", 0x50, 0x7f, 0);
        Print("3.8 KM", 0x50, 0x86, 0);
        Print("137 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 64;
        tintB = 0;
        break;
    }
    for (x = 0xa0; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x62, 0);
    }
    for (x = 0xa0; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x68, 0);
    }
    for (x = 0xa0; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x70, 0);
    }
    for (x = 0xa2; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x78, 0);
    }
    for (x = 0x102; x < ((newRand() & 5) + 5) * 6 + 0x102; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0xa8, 0);
    }
    Print("TM", 0xee, 0x56, 0);
    if (rgbTextures == 0) {
        lineWhite = 0x63;
        lineGrey = 0xc;
    }
    DrawBox(0x9e, 0x54, 0x60, 0xb, lineWhite, 0xff, 0xff);
    DrawBox(0xa0, 0x76, 0x80, 0x30, lineWhite, 0xff, 0xff);
    DrawBox(0x100, 0xa6, 0x40, 8, lineWhite, 0xff, 0xff);
    DrawBox(0xa0, 0xa2, 0xa0, 4, lineWhite, 0xff, 0xff);
    DrawLine(0xa0, 0x82, 0x120, 0x82, lineGrey, 0xc0, 0xc0);
    DrawLine(0xa0, 0x7e, 0x120, 0x7e, lineGrey, 0xc0, 0xc0);
    DrawLine(0xa0, 0x9e, 0x120, 0x9e, lineGrey, 0xc0, 0xc0);
    for (x = 0x86; x < 0x9e; x += 8)
        DrawLine(0x100, x, 0x120, x, lineGrey, 0xc0, 0xc0);
    for (x = 0xb0; x < 0x120; x += 0x10)
        DrawLine(x, 0xa6, x, 0x7e, lineGrey, 0xc0, 0xc0);
    for (x = 0xa8; x < 0x120; x += 0x10)
        DrawLine(x, 0xa2, x, 0xa6, lineGrey, 0xc0, 0xc0);
    DrawLine(0x20, 0x6e, 0x110, 0x6e, lineWhite, 0xff, 0xff);
    DrawLine(0x110, 0x76, 0x110, 0x7e, lineWhite, 0xff, 0xff);
    DrawLine(0x20, 0x76, 0xa0, 0x76, lineWhite, 0xff, 0xff);
    DrawLine(0x20, 0x70, 0x20, 0x70, lineWhite, 0xff, 0xff);
    DrawLine(0x24, 0x70, 0x20, 0x74, lineWhite, 0xff, 0xff);
    DrawLine(0x24, 0x74, 0x24, 0x74, lineWhite, 0xff, 0xff);
    DrawLine(0xe0, 0x86, 0x100, 0x86, lineGrey, 0xc0, 0xc0);
    for (x = 0xe8; x < 0x120; x += 0x10)
        DrawLine(x, 0x86, x, 0x82, lineGrey, 0xc0, 0xc0);
    waveIndex = 0;
    wave = scanPhase;
    scanPhase = (scanPhase - 10) & 0xff;
    for (x = 0xa2; x < 0x100; x += 2) {
        wave = (wave + 5) & 0xff;
        if (rgbTextures)
            DrawLineDepth(x, 0xa1, x, barHeight[waveIndex] + 0x78, tintR * wave >> 8, wave * gr >> 8, tintB * wave >> 8);
        else
            DrawLineDepth(x, 0xa1, x, barHeight[waveIndex] + 0x78, 0x36, wave * gr >> 8, tintB * wave >> 8);
        dir = newRand() & 3;
        switch (dir) {
        case 1:
            if (barHeight[waveIndex] > 8)
                barHeight[waveIndex] -= 2;
            break;
        case 2:
            if (barHeight[waveIndex] < 0x28)
                barHeight[waveIndex] += 2;
            break;
        }
        waveIndex++;
    }
}
#endif
#ifdef NETPLAY
/* The network build's /Od frame needs these local names (the D3D MATCH* depends on the names above). */
void VsyncFunc(void)
{
    unsigned char lineGrey;
    unsigned char lineIntensity;
    short inc;
    WADFILE *wad;
    short swing;
    short gr;
    short x;
    short tintR;
    short tintB;

    lineIntensity = 0xff;
    lineGrey = 0xc0;
    loadFrame++;
    wad = currentWad;
    DrawSync(0);
    quadIndex = CurrentScreen * 300;
    AddText("LOADING", 0xa0, 0x56, 8, 0);
    AddJustifyNumber((loadPercent / 100) % 10, 0x30, 0x62, 8, 0);
    AddJustifyNumber((loadPercent / 10) % 10, 0x3e, 0x62, 8, 0);
    AddJustifyNumber(loadPercent % 10, 0x4c, 0x62, 8, 0);
    Print("  LOAD          33    33    33", 0x20, 0x70, 0);
    if (loadFrame > 0x46)
        Print("VENUE", 0x20, 0x78, 0);
    if (loadFrame > 0x1e)
        Print("LOCATION", 0x20, 0x7f, 0);
    if (loadFrame > 0x32)
        Print("LENGTH", 0x20, 0x86, 0);
    if (loadFrame > 0x14)
        Print("HEIGHT", 0x20, 0x8d, 0);
    if (loadFrame > 0x28)
        Print("CLASS", 0x20, 0x94, 0);
    if (loadFrame > 0x3c)
        Print("TEAM", 0x20, 0x9b, 0);
    if (gameData->shipClass == 0)
        Print("VECTOR", 0x50, 0x94, 0);
    if (gameData->shipClass == 1)
        Print("VENOM", 0x50, 0x94, 0);
    if (gameData->shipClass == 2)
        Print("RAPIER", 0x50, 0x94, 0);
    if (gameData->shipClass == 3)
        Print("PHANTOM", 0x50, 0x94, 0);
    if (ownShip >= 0 && ownShip <= 2)
        Print("AG SYSTEMS", 0x50, 0x9b, 0);
    else if (ownShip >= 3 && ownShip <= 5)
        Print("AURICOM", 0x50, 0x9b, 0);
    else if (ownShip >= 6 && ownShip <= 8)
        Print("QIREX", 0x50, 0x9b, 0);
    else if (ownShip >= 9 && ownShip <= 11)
        Print("FEISAR", 0x50, 0x9b, 0);
    else if (ownShip >= 12 && ownShip <= 14)
        Print("PIRANHA", 0x50, 0x9b, 0);
    Print(GetTrackName(gameData->actTrackNum, 0, 0, 0), 0x50, 0x78, 0);
    switch (gameData->actTrackNum) {
    case 1:
        Print("CANADA", 0x50, 0x7f, 0);
        Print("3.2 KM", 0x50, 0x86, 0);
        Print("74 M", 0x50, 0x8d, 0);
        tintR = 255;
        gr = 0;
        tintB = 0;
        break;
    case 2:
        Print("FRANCE", 0x50, 0x7f, 0);
        Print("3.5 KM", 0x50, 0x86, 0);
        Print("179 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 255;
        tintB = 0;
        break;
    case 6:
        Print("SOUTH PACIFIC", 0x50, 0x7f, 0);
        Print("5.1 KM", 0x50, 0x86, 0);
        Print("97 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 0;
        tintB = 255;
        break;
    case 7:
        Print("NORTH AMERICA", 0x50, 0x7f, 0);
        Print("4.0 KM", 0x50, 0x86, 0);
        Print("82 M", 0x50, 0x8d, 0);
        tintR = 255;
        gr = 255;
        tintB = 0;
        break;
    case 8:
        Print("NEPAL", 0x50, 0x7f, 0);
        Print("4.3 KM", 0x50, 0x86, 0);
        Print("158 M", 0x50, 0x8d, 0);
        tintR = 255;
        gr = 0;
        tintB = 255;
        break;
    case 13:
        Print("CHILE", 0x50, 0x7f, 0);
        Print("3.9 KM", 0x50, 0x86, 0);
        Print("220 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 255;
        tintB = 255;
        break;
    case 17:
        Print("BLACK SEA", 0x50, 0x7f, 0);
        Print("4.4 KM", 0x50, 0x86, 0);
        Print("121 M", 0x50, 0x8d, 0);
        tintR = 128;
        gr = 32;
        tintB = 255;
        break;
    case 20:
        Print("GERMANY", 0x50, 0x7f, 0);
        Print("3.8 KM", 0x50, 0x86, 0);
        Print("137 M", 0x50, 0x8d, 0);
        tintR = 0;
        gr = 64;
        tintB = 0;
        break;
    }
    for (x = 0xa0; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x62, 0);
    }
    for (x = 0xa0; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x68, 0);
    }
    for (x = 0xa0; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x70, 0);
    }
    for (x = 0xa2; x < (newRand() & 0xf) * 6 + 0xb4; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0x78, 0);
    }
    for (x = 0x102; x < ((newRand() & 5) + 5) * 6 + 0x102; x += 6) {
        wsprintfA(numberText, "%d", newRand() % 10);
        Print(numberText, x, 0xa8, 0);
    }
    Print("TM", 0xee, 0x56, 0);
    if (rgbTextures == 0) {
        lineIntensity = 0x63;
        lineGrey = 0xc;
    }
    DrawBox(0x9e, 0x54, 0x60, 0xb, lineIntensity, 0xff, 0xff);
    DrawBox(0xa0, 0x76, 0x80, 0x30, lineIntensity, 0xff, 0xff);
    DrawBox(0x100, 0xa6, 0x40, 8, lineIntensity, 0xff, 0xff);
    DrawBox(0xa0, 0xa2, 0xa0, 4, lineIntensity, 0xff, 0xff);
    DrawLine(0xa0, 0x82, 0x120, 0x82, lineGrey, 0xc0, 0xc0);
    DrawLine(0xa0, 0x7e, 0x120, 0x7e, lineGrey, 0xc0, 0xc0);
    DrawLine(0xa0, 0x9e, 0x120, 0x9e, lineGrey, 0xc0, 0xc0);
    for (x = 0x86; x < 0x9e; x += 8)
        DrawLine(0x100, x, 0x120, x, lineGrey, 0xc0, 0xc0);
    for (x = 0xb0; x < 0x120; x += 0x10)
        DrawLine(x, 0xa6, x, 0x7e, lineGrey, 0xc0, 0xc0);
    for (x = 0xa8; x < 0x120; x += 0x10)
        DrawLine(x, 0xa2, x, 0xa6, lineGrey, 0xc0, 0xc0);
    DrawLine(0x20, 0x6e, 0x110, 0x6e, lineIntensity, 0xff, 0xff);
    DrawLine(0x110, 0x76, 0x110, 0x7e, lineIntensity, 0xff, 0xff);
    DrawLine(0x20, 0x76, 0xa0, 0x76, lineIntensity, 0xff, 0xff);
    DrawLine(0x20, 0x70, 0x20, 0x70, lineIntensity, 0xff, 0xff);
    DrawLine(0x24, 0x70, 0x20, 0x74, lineIntensity, 0xff, 0xff);
    DrawLine(0x24, 0x74, 0x24, 0x74, lineIntensity, 0xff, 0xff);
    DrawLine(0xe0, 0x86, 0x100, 0x86, lineGrey, 0xc0, 0xc0);
    for (x = 0xe8; x < 0x120; x += 0x10)
        DrawLine(x, 0x86, x, 0x82, lineGrey, 0xc0, 0xc0);
    waveIndex = 0;
    swing = scanPhase;
    scanPhase = (scanPhase - 10) & 0xff;
    for (x = 0xa2; x < 0x100; x += 2) {
        swing = (swing + 5) & 0xff;
        if (rgbTextures)
            DrawLineDepth(x, 0xa1, x, barHeight[waveIndex] + 0x78, tintR * swing >> 8, swing * gr >> 8, tintB * swing >> 8);
        else
            DrawLineDepth(x, 0xa1, x, barHeight[waveIndex] + 0x78, 0x36, swing * gr >> 8, tintB * swing >> 8);
        inc = newRand() & 3;
        switch (inc) {
        case 1:
            if (barHeight[waveIndex] > 8)
                barHeight[waveIndex] -= 2;
            break;
        case 2:
            if (barHeight[waveIndex] < 0x28)
                barHeight[waveIndex] += 2;
            break;
        }
        waveIndex++;
    }
}
#endif

/* Opens a WAD file and prepares its loading screen. */
WADFILE *openwadfile(int fileNo)
{
    int total = 0;
    WADFILE *wad;
    int i;
    char tag;
    char name[16];
    int bytes;

    wad = DAlloc(heap, 0x108);
    wad->handle = iso_open(fileNo, 0, 0);
    if (wad->handle == -1) {
        FileAccessError((char *)fileNo);
        return 0;
    }
    ReadFileBytes(wad->handle, &wad->count, 2);
    for (i = 0; i < wad->count; i++) {
        ReadFileBytes(wad->handle, name, 16);
        ReadFileBytes(wad->handle, &bytes, 4);
        total += bytes;
        ReadFileBytes(wad->handle, &wad->sizes[i], 4);
        ReadFileBytes(wad->handle, &tag, 1);
    }
    wadSize = total;
    wadRemaining = wadSize;
    wad->current = 0;
    currentWad = wad;
    InitPop();
    SetDispMask(1);
    return wad;
}

/* Opens a WAD file without the loading screen setup. */
WADFILE *openwadfilenvc(int fileNo)
{
    WADFILE *wad;
    int i;
    char tag;
    char name[16];
    int bytes;

    wad = DAlloc(heap, 0x108);
    wad->handle = iso_open(fileNo, 0, 0);
    if (wad->handle == -1) {
        FileAccessError((char *)fileNo);
        return 0;
    }
    ReadFileBytes(wad->handle, &wad->count, 2);
    for (i = 0; i < wad->count; i++) {
        ReadFileBytes(wad->handle, name, 16);
        ReadFileBytes(wad->handle, &bytes, 4);
        ReadFileBytes(wad->handle, &wad->sizes[i], 4);
        ReadFileBytes(wad->handle, &tag, 1);
    }
    wad->current = 0;
    currentWad = wad;
    SetDispMask(1);
    return wad;
}

/* Closes a WAD file opened without loading screen setup. */
void closewadfilenvc(WADFILE *wad)
{
    iso_close(wad->handle);
    DFree(heap, (char *)wad);
    wad = 0;
}

/* Closes the WAD file and releases its descriptor. */
void closewadfile(WADFILE *wad)
{
    iso_close(wad->handle);
    DFree(heap, (char *)wad);
    wad = 0;
}

/* Reads the current WAD entry and advances the entry index. */
void readnextfile(WADFILE *wad, char *dest)
{
    WDread(wad->handle, (u_char *)dest, wad->sizes[wad->current]);
    wad->current = wad->current + 1;
}

/* Reads file data in frame-sized chunks while updating the loading screen. */
int WDread(int handle, u_char *dest, int count)
{
    int size;
    int ret;
    int more = 1;

    while (more) {
        FlushMessages();
        VsyncFunc();
        RaceFrameStart();
        if (count < 8192) {
            size = count;
            more = 0;
        } else {
            size = 8192;
            count -= 8192;
        }
        ret = fread(dest, 1, size, (FILE *)handle);
        dest += size;
        wadRemaining -= -(-size);
        loadPercent = 95 - (wadRemaining * 95) / wadSize;
    }
    return ret;
}

#ifdef POWERVR
int Stub_40A020(void)
{
    return 0;
}

void Stub_40A030(void)
{
}

int Stub_40A040(void)
{
    return 0;
}

int Stub_40A050(void)
{
    return 0;
}

int Stub_40A060(void)
{
    return 0;
}

void Stub_40A070(void)
{
}

#endif
