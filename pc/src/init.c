#include "functions.h"
#include "tim.h"

#define Clut4Bit 0
#define Clut8Bit 1
#define CLUT4 0
#define CLUT8 1
#define ClutType(tim) ((tim)->flags & 7)
#define TextureHOffset(x) ((x) & 0xffc0)
#define TextureVOffset(y) ((y) & 0x100)
#define TPAGE(x, y) ((((y) & 0x100) >> 4) + ((x) >> 6))
#define TSB(c, a, t) (((c) << 7) | ((a) << 5) | (t))
#define CBA(x, y) ((x) | ((y) << 6))

typedef struct CompressedTextureSequence {
    long textureCount;
    long textureSize[1];
} CompressedTextureSequence;

#pragma intrinsic(strcpy, strcat)

extern short TextureTableCount;
extern Texture *textureTable[];
extern short numTex;
#include "heap_state.h"
extern char errorString[];
extern char scenePath[];
extern char trackDirectory[];
extern int skyPos[];
extern Object *sharedPrimTable[128][40];
extern int loadingSky;
extern int smallVram;
extern short clutX, clutY, textureX, textureY;
extern TIMlist timList;
extern TIMlist wadTimList;

/* Loads track objects and initializes the camera scene. */
short InitScene(Object **prm, short *prmCount, Skeleton *camPos, ConfigData *gameData)
{
#ifdef NETPLAY
    short sceneTextures;
    short sky;
    Object *prim;
    short j, i;
    short skyFirstTex;
    TIMlist *texPtr;
    TIMlist *skyTimPtr;

    sceneTextures = TextureTableCount;
    getPath(scenePath, gameData->actTrackNum, "\\scene.cmp");
    texPtr = LoadCompressedTextureSequence(scenePath);
    LoadCmpFiles(texPtr);
    skyFirstTex = TextureTableCount;
    getPath(scenePath, gameData->actTrackNum, "\\sky.cmp");
    skyTimPtr = LoadCompressedTextureSequence(scenePath);
    LoadCmpFiles(skyTimPtr);
    for (i = 0; i < 128; i++) {
        for (j = 0; j < 40; j++) {
            sharedPrimTable[i][j] = 0;
        }
    }
    getPath(scenePath, gameData->actTrackNum, "\\scene.prm");
    FreePrm(prm[*prmCount]);
    prm[(*prmCount)++] = LoadPrm(scenePath, sceneTextures);
    for (i = 0; i < *prmCount; i++) {
        prim = prm[i];
        while (prim) {
            prim->skeleton->super = camPos;
            prim = prim->next;
        }
    }
    sky = *prmCount;
    getPath(scenePath, gameData->actTrackNum, "\\sky.prm");
    FreePrm(prm[*prmCount]);
    prm[(*prmCount)++] = LoadPrm(scenePath, skyFirstTex);
    prim = prm[sky];
    prim->skeleton->super = camPos->super;
    SetSkeletonPosition(prm[sky]->skeleton, 0, skyPos[gameData->trackNum], 0);
    if (gameData->actTrackNum == 20) {
        SetSkeletonDirection(prm[sky]->skeleton, 0, 100, 0);
    }
    SceneRadiusCheck(prm[0]);
    return sky;
#else
    TIMlist *timPtr, *skyTimPtr;
    Object *obj;
    short sky;
    short sceneTextures;
    short skyTextures;
    short i, j;

    sceneTextures = TextureTableCount;
    getPath(scenePath, gameData->actTrackNum, "\\scene.cmp");
    timPtr = LoadCompressedTextureSequence(scenePath);
    LoadCmpFiles(timPtr);
    skyTextures = TextureTableCount;
    getPath(scenePath, gameData->actTrackNum, "\\sky.cmp");
    skyTimPtr = LoadCompressedTextureSequence(scenePath);
    LoadCmpFiles(skyTimPtr);
    for (i = 0; i < 128; i++) {
        for (j = 0; j < 40; j++) {
            sharedPrimTable[i][j] = 0;
        }
    }
    getPath(scenePath, gameData->actTrackNum, "\\scene.prm");
    FreePrm(prm[*prmCount]);
    prm[(*prmCount)++] = LoadPrm(scenePath, sceneTextures);
    for (i = 0; i < *prmCount; i++) {
        obj = prm[i];
        while (obj) {
            obj->skeleton->super = camPos;
            obj = obj->next;
        }
    }
    sky = *prmCount;
    getPath(scenePath, gameData->actTrackNum, "\\sky.prm");
    FreePrm(prm[*prmCount]);
    prm[(*prmCount)++] = LoadPrm(scenePath, skyTextures);
    obj = prm[sky];
    obj->skeleton->super = camPos->super;
    SetSkeletonPosition(prm[sky]->skeleton, 0, skyPos[gameData->trackNum], 0);
    if (gameData->actTrackNum == 20) {
        SetSkeletonDirection(prm[sky]->skeleton, 0, 100, 0);
    }
    SceneRadiusCheck(prm[0]);
    return sky;
#endif
}

/* Loads track objects and initializes a scene from a WAD file. */
short WADInitScene(Object **prm, short *prmCount, Skeleton *camPos, ConfigData *gameData, WADFILE *wad)
{
#ifdef NETPLAY
    short sceneTextures;
    short sky;
    Object *prim;
    short j, i;
    short skyFirstTex;
    TIMlist *texPtr;
    TIMlist *skyTimPtr;

    sceneTextures = TextureTableCount;
    texPtr = WADLoadCompressedTextureSequence(wad);
    LoadCmpFiles(texPtr);
    skyFirstTex = TextureTableCount;
    skyTimPtr = WADLoadCompressedTextureSequence(wad);
    LoadCmpFiles(skyTimPtr);
    for (i = 0; i < 128; i++) {
        for (j = 0; j < 40; j++) {
            sharedPrimTable[i][j] = 0;
        }
    }
    FreePrm(prm[*prmCount]);
    prm[(*prmCount)++] = WADLoadPrm(wad, sceneTextures);
    for (i = 0; i < *prmCount; i++) {
        prim = prm[i];
        while (prim) {
            prim->skeleton->super = camPos;
            prim = prim->next;
        }
    }
    sky = *prmCount;
    loadingSky = 1;
    prm[(*prmCount)++] = WADLoadPrm(wad, skyFirstTex);
    loadingSky = 0;
    prim = prm[sky];
    prim->skeleton->super = camPos->super;
    SetSkeletonPosition(prm[sky]->skeleton, 0, skyPos[gameData->trackNum], 0);
    SceneRadiusCheck(prm[0]);
    return sky;
#else
    short sceneTextures;
    short skyTextures;
    short i, j;
    Object *obj;
    short sky;
    TIMlist *timPtr, *skyTimPtr;

    sceneTextures = TextureTableCount;
    timPtr = WADLoadCompressedTextureSequence(wad);
    LoadCmpFiles(timPtr);
    skyTextures = TextureTableCount;
    skyTimPtr = WADLoadCompressedTextureSequence(wad);
    LoadCmpFiles(skyTimPtr);
    for (i = 0; i < 128; i++) {
        for (j = 0; j < 40; j++) {
            sharedPrimTable[i][j] = 0;
        }
    }
    FreePrm(prm[*prmCount]);
    prm[(*prmCount)++] = WADLoadPrm(wad, sceneTextures);
    for (i = 0; i < *prmCount; i++) {
        obj = prm[i];
        while (obj) {
            obj->skeleton->super = camPos;
            obj = obj->next;
        }
    }
    sky = *prmCount;
    loadingSky = 1;
    prm[(*prmCount)++] = WADLoadPrm(wad, skyTextures);
    loadingSky = 0;
    obj = prm[sky];
    obj->skeleton->super = camPos->super;
    SetSkeletonPosition(prm[sky]->skeleton, 0, skyPos[gameData->trackNum], 0);
    SceneRadiusCheck(prm[0]);
    return sky;
#endif
}

/* Builds a path for a file stored in the selected track directory. */
void getPath(char *trackPath, char trackNo, char *name)
{
    char No[2];

    strcpy(trackPath, trackDirectory);
    wsprintf(No, "%02d", trackNo);
    strcat(trackPath, No);
    strcat(trackPath, name);
    OutputDebugString(trackPath);
    OutputDebugString("\n");
}

/* Loads the compressed texture groups referenced by a TIM list. */
void LoadCmpFiles(TIMlist *timPtr)
{
    Texture *texture;
    short i;

    for (i = 0; i < numTex; i++) {
        texture = NewLoadTexture((Tim *)timPtr->memOffset[i], 1, 0);
        if (texture) {
            textureTable[TextureTableCount++] = texture;
        }
        if (TextureTableCount >= 800) {
            OutputDebugString("Wtl.c:LoadWtlFiles(): TextureTable is full\n");
        }
    }
    DFree(reserveHeap, timPtr->memBase);
    timPtr->memBase = 0;
}

/* Loads the compressed texture groups used by a ghost replay. */
void GhostLoadCmpFiles(TIMlist *timPtr)
{
    short i;
    Texture *texture;

    for (i = 0; i < numTex; i++) {
        texture = NewLoadTexture((Tim *)timPtr->memOffset[i], 1, 1);
        if (texture) {
            textureTable[TextureTableCount++] = texture;
        }
        if (TextureTableCount >= 800) {
            OutputDebugString("Wtl.c:LoadWtlFiles(): TextureTable is full\n");
        }
    }
    DFree(reserveHeap, timPtr->memBase);
    timPtr->memBase = 0;
}

/* Builds the Texture for one TIM and uploads its CLUT and pixels to VRAM. A 4-bit TIM with no
   stored VRAM position is placed in the next free CLUT and texture slots; an 8-bit TIM must
   come with its position. Returns 0 for other TIM types. */
Texture *NewLoadTexture(Tim *timPtr, short translucent, int scene)
{
    TimClut4 *tc4;
    Tim *tim;
    int y;
    Texture *texture;
    int x;
    RECT rect;
    TimClut8 *tc8;

    tim = timPtr;
    if (ClutType(tim) == Clut4Bit) {
        tc4 = (TimClut4 *)tim;
        texture = DAlloc(heap, sizeof(Texture));
        texture->type = CLUT4;
        if (!tc4->textureX && !tc4->textureY) {
            tc4->clutX = clutX;
            tc4->clutY = clutY;
            tc4->clutW = 16;
            tc4->clutH = 1;
            tc4->textureX = textureX;
            tc4->textureY = textureY;
            clutX += 16;
            if (clutX >= 384) {
                clutX = 320;
                clutY += 1;
            }
            if (smallVram) {
                textureY += 32;
            } else {
                textureY += 128;
            }
            if (textureY >= 256) {
                if (smallVram) {
                    textureX += 8;
                } else {
                    textureX += 32;
                }
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

        texture->u0 = (x << 2);
        texture->v0 = (y);
        texture->u1 = ((x + tc4->textureW) << 2) - 1;
        texture->v1 = (y);
        texture->u2 = (x << 2);
        texture->v2 = (y + tc4->textureH) - 1;
        texture->u3 = ((x + tc4->textureW) << 2) - 1;
        texture->v3 = (y + tc4->textureH) - 1;

        texture->tsb = TSB(Clut4Bit, translucent, TPAGE(texture->textureX, texture->textureY));
        texture->cba = CBA(texture->clutX >> 4, texture->clutY);

        rect.x = tc4->clutX;
        rect.y = tc4->clutY;
        rect.w = tc4->clutW;
        rect.h = tc4->clutH;
        LoadImageStub(&rect, (u_long *)(&tc4->clut));
        DrawSync(0);

        rect.x = tc4->textureX;
        rect.y = tc4->textureY;
        rect.w = tc4->textureW;
        rect.h = tc4->textureH;
        LoadImageStub(&rect, (u_long *)(tc4 + 1));
        DrawSync(0);
    } else if (ClutType(tim) == Clut8Bit) {
        tc8 = (TimClut8 *)tim;
        texture = DAlloc(heap, sizeof(Texture));
        texture->type = CLUT8;
        if (!tc8->textureX && !tc8->textureY) {
            Error("tim.c:LoadTexture(): Cannot auto place 8 bit textures", 2);
        }
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

        texture->u0 = (x << 1);
        texture->v0 = (y);
        texture->u1 = ((x + tc8->textureW) << 1) - 1;
        texture->v1 = (y);
        texture->u2 = (x << 1);
        texture->v2 = (y + tc8->textureH) - 1;
        texture->u3 = ((x + tc8->textureW) << 1) - 1;
        texture->v3 = (y + tc8->textureH) - 1;

        texture->tsb = TSB(Clut8Bit, translucent, TPAGE(texture->textureX, texture->textureY));
        texture->cba = CBA(texture->clutX >> 4, texture->clutY);

        rect.x = tc8->clutX;
        rect.y = tc8->clutY;
        rect.w = tc8->clutW;
        rect.h = tc8->clutH;
        LoadImageStub(&rect, (u_long *)(&tc8->clut));
        DrawSync(0);

        rect.x = tc8->textureX;
        rect.y = tc8->textureY;
        rect.w = tc8->textureW;
        rect.h = tc8->textureH;
        LoadImageStub(&rect, (u_long *)(tc8 + 1));
        DrawSync(0);
    } else {
        texture = 0;
    }
    return texture;
}

/* Reads a compressed texture sequence file into the reserve heap, unpacks its TIMs and returns
   the list of their addresses. */
TIMlist *LoadCompressedTextureSequence(char *fileName)
{
    short texNum;
    char *fileBuffer;
    long fileSize;
    long timSize, timTotalSize;
    int CPURasters;

    fileSize = FileLength(fileName);
    if (fileSize <= 0) {
        FileAccessError(fileName);
        wsprintf(errorString, "Tim decompress: file %s not found\n", fileName);
        Error(errorString, 2);
    }
    OutputDebugString("attempting malloc\n");
    fileBuffer = DAlloc(reserveHeap, fileSize + 32);
    if (!fileBuffer) {
        Error("Decompress Load: Failed to allocate memory for track vertices", 2);
    }
    if (LoadFile(fileName, fileBuffer) != fileSize) {
        wsprintf(errorString, "Decompress(): Failed to load file %s\n", fileName);
        Error(errorString, 2);
    }
    numTex = ((CompressedTextureSequence *)fileBuffer)->textureCount;
    timTotalSize = 0;
    for (texNum = 0; texNum < numTex; texNum++) {
        timList.memOffset[texNum] = timTotalSize;
        timSize = ((CompressedTextureSequence *)fileBuffer)->textureSize[texNum];
        timTotalSize += timSize;
    }
    timList.memBase = DAlloc(reserveHeap, timTotalSize);
    for (texNum = 0; texNum < numTex; texNum++) {
        timList.memOffset[texNum] += (long)timList.memBase;
    }
    CPURasters = 0;
    ExpandData((unsigned char *)&((CompressedTextureSequence *)fileBuffer)->textureSize[numTex], (unsigned char *)timList.memBase);
    CPURasters = 0;
    DFree(reserveHeap, fileBuffer);
    fileBuffer = 0;
    return &timList;
}

/* As LoadCompressedTextureSequence, but reads the next file from the open WAD archive. */
TIMlist *WADLoadCompressedTextureSequence(WADFILE *wad)
{
    short texNum;
    char *fileBuffer;
    long fileSize;
    long timSize, timTotalSize;
    int CPURasters;

    fileSize = wad->sizes[wad->current];
    fileBuffer = DAlloc(reserveHeap, fileSize + 32);
    if (!fileBuffer) {
        Error("Decompress Load: Failed to allocate memory for track vertices", 2);
    }
    readnextfile(wad, fileBuffer);
    numTex = ((CompressedTextureSequence *)fileBuffer)->textureCount;
    timTotalSize = 0;
    for (texNum = 0; texNum < numTex; texNum++) {
        wadTimList.memOffset[texNum] = timTotalSize;
        timSize = ((CompressedTextureSequence *)fileBuffer)->textureSize[texNum];
        timTotalSize += timSize;
    }
    wadTimList.memBase = DAlloc(reserveHeap, timTotalSize + 32);
    for (texNum = 0; texNum < numTex; texNum++) {
        wadTimList.memOffset[texNum] += (long)wadTimList.memBase;
    }
    CPURasters = 0;
    ExpandData((unsigned char *)&((CompressedTextureSequence *)fileBuffer)->textureSize[numTex], (unsigned char *)wadTimList.memBase);
    CPURasters = 0;
    DFree(reserveHeap, fileBuffer);
    fileBuffer = 0;
    return &wadTimList;
}

#ifdef POWERVR
void Stub_447290(void)
{
}
#endif
