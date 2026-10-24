#define PC_DISPLAY_IMPORTS
#include "functions.h"
#include "gpu_stubs.h"
#include "object2.h"
#include "camera_facing.h"

extern DISPENV DispEnv0;
extern DB doubleBuffer[];
extern unsigned long currentFrameTime;
extern unsigned long previousFrameTime;
/* The frame clock retains repeated writes to this value. */
extern volatile long FR60;
#include "heap_state.h"
extern char errorString[];

void SetBlockFill(BLK_FILL *fill);

extern DRAWENV DrawEnv0;
extern short CurrentScreen;
void InitGeom(void);

/* Initializes the PSYQ display state and first draw environment. */
void InitSystem(void)
{
    ResetGraph(0);
    SetGraphDebug(0);
    InitGeom();
    SetDefDrawEnv(&DrawEnv0, 0, 256, 320, 240);
#ifdef NETPLAY
    InitGameDisplay(1);
#else
    SetDefDispEnv(&DispEnv0, 0, 0, 320, 240);
    SetGeomOffset(160, 120);
    SetGeomScreen(160);
    PutDispEnvStub(doubleBuffer[0].disp);
#endif
    doubleBuffer[0].disp = &DispEnv0;
    doubleBuffer[0].draw = &DrawEnv0;
    CurrentScreen = 0;
#ifdef NETPLAY
    PutDrawEnvStub(doubleBuffer[0].draw);
#else
    PutDrawEnvStub(&DrawEnv0);
#endif
    PutDispEnvStub(doubleBuffer[0].disp);
    SetDispMask(1);
}

/* Configures geometry and display dimensions for menu scenes. */
void InitMenuDisplay(int geomY, int unused)
{
    SetDefDispEnv(&DispEnv0, 0, 0, 320, 180);
    SetGeomOffset(160, geomY);
    SetGeomScreen(160);
    PutDispEnvStub(doubleBuffer[0].disp);
}

/* Configures geometry and display dimensions for the game view. */
void InitGameDisplay(int unused)
{
    SetDefDispEnv(&DispEnv0, 0, 0, 320, 240);
    SetGeomOffset(160, 120);
    SetGeomScreen(160);
    PutDispEnvStub(doubleBuffer[0].disp);
}

/* Configures geometry for the 640 by 256 Sony display mode. */
void InitSonyDisplay(void)
{
    SetDefDispEnv(&DispEnv0, 0, 0, 640, 256);
    SetGeomOffset(320, 128);
    SetGeomScreen(320);
    PutDispEnvStub(doubleBuffer[0].disp);
}

extern int debugTextFlag;
extern int menuQuit;
extern int displayStat0, displayStat1;
#include "render.h"
extern char primitiveBuffer[];
void FlushGameRenderQueues(void);

extern int blinkFrame;
extern int blinkFlag;
int PresentRenderFrame(void);
void FlushRenderQueues(void);

/* Updates the frame count and blink state from the system clock. */
static __inline void UpdateFrameClock(void)
{
    unsigned long now;
    unsigned long elapsed;
    unsigned long remainder;
    int frames;
    int i;

    now = timeGetTime();
    if (previousFrameTime > now)
        elapsed = (0xffffffffUL - previousFrameTime) + now;
    else
        elapsed = now - previousFrameTime;
    frames = (elapsed << 12) / 68266UL;
    remainder = ((elapsed << 12) % 68266UL) >> 12;
    if (frames % 2) {
        remainder += (elapsed - remainder) / frames;
        frames--;
    }
    currentFrameTime = now;
    FR60 = frames;
    if (frames < 1) {
        FR60 = 1;
        timeGetTime();
        now = currentFrameTime;
        frames = FR60;
    }
    currentFrameTime = now;
    FR60 = frames;
    previousFrameTime = now - remainder;
    if (frames < 1)
        frames = 1;
    for (i = 0; i < frames; i++) {
        blinkFrame++;
        blinkFlag = 1;
        if (blinkFrame % 30 >= 20)
            blinkFlag = 0;
    }
}

/* Prepares the next display buffer and updates frame timing. */
int RaceFrameStart(void)
{
    if (debugTextFlag && !menuQuit) {
        AddNumber(displayStat0, 150, 0, 8, 0);
        AddNumber(displayStat1, 250, 0, 8, 0);
    }
    CurrentScreen = (CurrentScreen + 1) & 1;
    PutDrawEnvStub(doubleBuffer[0].draw);
    PutDispEnvStub(doubleBuffer[0].disp);
    FlushRenderQueues();
#ifdef NETPLAY
    {
        unsigned long elapsed;
        unsigned long remainder;
        unsigned long frameRate;
        unsigned long now;
        int frames;
        short frameAdjusted;

        frameRate = 68266;
        frameAdjusted = 0;
        currentFrameTime = timeGetTime();
        if (previousFrameTime > currentFrameTime)
            elapsed = (0xffffffffUL - previousFrameTime) + currentFrameTime;
        else
            elapsed = currentFrameTime - previousFrameTime;
        FR60 = (elapsed << 12) / frameRate;
        remainder = ((elapsed << 12) % frameRate) >> 12;
        if (FR60 % 2) {
            remainder += (elapsed - remainder) / FR60;
            FR60--;
            frameAdjusted = 1;
        }
        if (FR60 < 1) {
            FR60 = 1;
            now = timeGetTime();
        }
        frameAdjusted = 0;
        previousFrameTime = currentFrameTime - remainder;
        frames = FR60;
        if (frames < 1)
            frames = 1;
        while (frames > 0) {
            blinkFrame++;
            if (blinkFrame % 30 >= 20)
                blinkFlag = 0;
            else
                blinkFlag = 1;
            frames--;
        }
    }
#else
    UpdateFrameClock();
#endif
    trackVertexCursor = primitiveBuffer;
#ifdef NETPLAY
    if (PresentRenderFrame())
        return 1;
    return 0;
#else
    return PresentRenderFrame() != 0;
#endif
}

#ifdef NETPLAY
#include "lockstep_stats.h"
void FlushTimedRenderQueues(void);
#endif

/* Presents the current buffer and resets the track vertex cursor. */
void CallbackSwap(void)
{
    if (debugTextFlag && !menuQuit) {
        AddNumber(displayStat0, 150, 0, 8, 0);
        AddNumber(displayStat1, 250, 0, 8, 0);
#ifdef NETPLAY
        if (networkGame) {
            AddNumber(lockstepStatDelta, 10, 0, 8, 0);
            AddNumber(lockstepStatAverage, 50, 0, 8, 0);
        }
#endif
    }
    CurrentScreen = (CurrentScreen + 1) & 1;
    PutDrawEnvStub(doubleBuffer[0].draw);
    PutDispEnvStub(doubleBuffer[0].disp);
#ifdef NETPLAY
    FlushTimedRenderQueues();
#else
    FlushGameRenderQueues();
#endif
    trackVertexCursor = primitiveBuffer;
}

/* Updates frame timing and presents the rendered buffer. */
int UpdateFrameAndPresent(int unused)
{
#ifdef NETPLAY
    unsigned long elapsed;
    unsigned long remainder;
    unsigned long frameRate;
    unsigned long now;
    int frames;
    short frameAdjusted;

    frameRate = 68266;
    frameAdjusted = 0;
    currentFrameTime = timeGetTime();
    if (previousFrameTime > currentFrameTime)
        elapsed = (0xffffffffUL - previousFrameTime) + currentFrameTime;
    else
        elapsed = currentFrameTime - previousFrameTime;
    FR60 = (elapsed << 12) / frameRate;
    remainder = ((elapsed << 12) % frameRate) >> 12;
    if (FR60 % 2) {
        remainder += (elapsed - remainder) / FR60;
        FR60--;
        frameAdjusted = 1;
    }
    if (FR60 < 1) {
        FR60 = 1;
        now = timeGetTime();
    }
    frameAdjusted = 0;
    previousFrameTime = currentFrameTime - remainder;
    frames = FR60;
    if (frames < 1)
        frames = 1;
    while (frames > 0) {
        blinkFrame++;
        if (blinkFrame % 30 >= 20)
            blinkFlag = 0;
        else
            blinkFlag = 1;
        frames--;
    }
    if (PresentRenderFrame())
        return 1;
    return 0;
#else
    unsigned long elapsed;
    unsigned long now;
    unsigned long remainder;
    int frames;
    int i;

    now = timeGetTime();
    if (previousFrameTime > now)
        elapsed = (0xffffffffUL - previousFrameTime) + now;
    else
        elapsed = now - previousFrameTime;
    frames = (elapsed << 12) / 68266UL;
    remainder = ((elapsed << 12) % 68266UL) >> 12;
    if (frames % 2) {
        remainder += (elapsed - remainder) / frames;
        frames--;
    }
    currentFrameTime = now;
    FR60 = frames;
    if (frames < 1) {
        FR60 = 1;
        timeGetTime();
        now = currentFrameTime;
        frames = FR60;
    }
    currentFrameTime = now;
    FR60 = frames;
    previousFrameTime = now - remainder;
    if (frames < 1)
        frames = 1;
    for (i = 0; i < frames; i++) {
        blinkFrame++;
        blinkFlag = 1;
        if (blinkFrame % 30 >= 20)
            blinkFlag = 0;
    }
    return PresentRenderFrame() != 0;
#endif
}

/* Resets the frame clock before the next race frame. */
void ResetFrameClock(void)
{
    currentFrameTime = timeGetTime();
    previousFrameTime = currentFrameTime;
    FR60 = 1;
}

/* Clears all four blocks of video memory. */
void ClearVram(void)
{
    BLK_FILL *fill;

    fill = BlockFill(0, 0, 512, 256, 0, 0, 0);
    DrawPrim(fill);
    DrawSync(0);
    DFree(reserveHeap, (char *)fill);
    fill = 0;
    fill = BlockFill(0, 256, 512, 256, 0, 0, 0);
    DrawPrim(fill);
    DrawSync(0);
    DFree(reserveHeap, (char *)fill);
    fill = 0;
    fill = BlockFill(512, 0, 512, 256, 0, 0, 0);
    DrawPrim(fill);
    DrawSync(0);
    DFree(reserveHeap, (char *)fill);
    fill = 0;
    fill = BlockFill(512, 256, 512, 256, 0, 0, 0);
    DrawPrim(fill);
    DrawSync(0);
    DFree(reserveHeap, (char *)fill);
    fill = 0;
}

BLK_FILL *BlockFill(short x, short y, short w, short h, short red, short green, short blue)
{
    BLK_FILL *fill;

    fill = DAlloc(reserveHeap, sizeof(BLK_FILL));
    if (!fill)
        wsprintfA(errorString, "Main.c:BlockFill(): Failed to allocate BlkFill\n");
    SetBlockFill(fill);
    fill->x0 = x;
    fill->y0 = y;
    fill->w = w;
    fill->h = h;
    fill->r0 = red;
    fill->g0 = green;
    fill->b0 = blue;
    return fill;
}

/* Returns the camera-facing direction of its current track section. */
char GetCameraFacing(TrackCamera *camera)
{
#ifdef NETPLAY
    TrackSection *nextSection;
    SVECTOR direction;
    int headingDifference;

    nextSection = camera->section->nextSection.ptr;
    direction.vx = nextSection->centre.vx - camera->section->centre.vx;
    direction.vy = nextSection->centre.vy - camera->section->centre.vy;
    direction.vz = nextSection->centre.vz - camera->section->centre.vz;
    headingDifference = -ratan2(direction.vx, direction.vz) + camera->hdg;
    headingDifference = ang(headingDifference);
    if (headingDifference >= -512 && headingDifference < 512)
        return 1;
    else if (headingDifference >= 512 && headingDifference < 1536)
        return 4;
    else if (headingDifference >= 1536 || headingDifference < -1536)
        return 2;
    else
        return 8;
#else
    short dx;
    short dz;
    int angle;

    dx = camera->section->nextSection.ptr->centre.vx - camera->section->centre.vx;
    dz = camera->section->nextSection.ptr->centre.vz - camera->section->centre.vz;
    angle = ang(camera->hdg - ratan2(dx, dz));
    if (angle >= -512 && angle < 512)
        return 1;
    else if (angle >= 512 && angle < 1536)
        return 4;
    else if (angle < 1536 && angle >= -1536)
        return 8;
    return 2;
#endif
}

#include "pc_game.h"
extern Object **trackObjects;
extern PCGameConfig *gameData;
extern u_short objectCullDistances[8];
extern int waveIndex;
void AddRenderObject(Object *object, int mode, u_char *rgb);

/* Adds visible objects from the current section to the render queues. */
void DrawSectionObjects(void *unused, TrackCamera *camera)
{
#ifdef NETPLAY
    TrackSection *section;
    SVECTOR origin;
    long nearDepth;
    long extentForRender;
    long flagPoint;
    short indexRadius;
    int unusedValueCoordinates;
    VECTOR projectedData;
    Object *object;

    unusedValueCoordinates = 0;
    section = camera->section;
    if (section->objectCount > 0) {
        origin.vx = 0;
        origin.vy = 0;
        origin.vz = 0;
        if (((ShipData *)unused)->attr & VIEW_IN)
            unusedValueCoordinates = ((ShipData *)unused)->roll;
        for (indexRadius = section->objectCount - 1; indexRadius > -1; indexRadius--) {
            object = trackObjects[section->objects[indexRadius]];
            object->skeleton->update = 1;
            SetSkeletonTransform(object->skeleton);
            extentForRender = object->extent;
            RotTrans(&origin, &projectedData, &flagPoint);
            nearDepth = projectedData.vz - extentForRender;
            projectedData.vz += extentForRender;
            if (projectedData.vz > 2048 && objectCullDistances[gameData->detailLevel] > nearDepth) {
                projectedData.vx = projectedData.vx * 160 / projectedData.vz;
                projectedData.vy = projectedData.vy * 160 / projectedData.vz;
                extentForRender = extentForRender * 160 / projectedData.vz;
                if (-(extentForRender + 160) <= projectedData.vx) {
                    if (projectedData.vx <= 160 + extentForRender) {
                        if (-(extentForRender + 120) <= projectedData.vy) {
                            if (projectedData.vy <= 120 + extentForRender)
                                AddRenderObject(object, 0, 0);
                        }
                    }
                }
            }
        }
    }
    waveIndex++;
#else
    TrackSection *section;
    Object *object;
    SVECTOR origin;
    VECTOR projected;
    long flag;
    long extent;
    long nearZ;
    long screenRadius;
    short i;

    section = camera->section;
    if (section->objectCount > 0) {
        origin.vx = 0;
        origin.vy = 0;
        origin.vz = 0;
        for (i = section->objectCount - 1; i > -1; i--) {
            object = trackObjects[section->objects[i]];
            object->skeleton->update = 1;
            SetSkeletonTransform(object->skeleton);
            extent = object->extent;
            RotTrans(&origin, &projected, &flag);
            nearZ = projected.vz - extent;
            projected.vz += extent;
            if (projected.vz > 2048 && objectCullDistances[gameData->detailLevel] > nearZ) {
                projected.vx = projected.vx * 160 / projected.vz;
                projected.vy = projected.vy * 160 / projected.vz;
                screenRadius = extent * 160 / projected.vz;
                if (projected.vx >= -160 - screenRadius && projected.vx <= 160 + screenRadius &&
                    projected.vy >= -120 - screenRadius && projected.vy <= 120 + screenRadius)
                    AddRenderObject(object, 0, 0);
            }
        }
    }
    waveIndex++;
#endif
}

#ifdef POWERVR
void Stub_45DDB0(void)
{
}

void Stub_45DDC0(void)
{
}
#endif
