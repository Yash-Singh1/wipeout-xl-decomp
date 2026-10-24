#include "functions.h"
#include "ship_state.h"
#include "race_mode.h"
#include "menu_rotation.h"
#include "cheats.h"
#include "camera.h"
#include "pc.h"
#include "pause.h"
#include "hiscores.h"
#include "raceinit.h"
#include "effects.h"
#include "pc_mesh.h"
#include "track.h"
#include "mathinline.h"
#include "sound_samples.h"
#ifdef NETPLAY
#include "race_globals.h"
#include "gst.h"
#include "race_packets.h"
#include "netmenu.h"
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>

extern ShipData *shipIndex;
extern ConfigData *gameData;

#include "heap_state.h"
extern HeapBlock *mainheapcopy;
extern short TextureTableCount;
extern short savedTextureCount;
extern short startTextureBase;
extern TIMlist *residentTextureSeq;
extern WeaponGrid *weaponGrids;
extern WeaponData *weaponIndex;
extern WeaponShape *weaponBars;
extern Object *startDroidObjects[];
extern CheckCell checkCells[][7];
extern char startPath[];


extern Object *teamShip;
extern Object *redBull[];
extern short redBullCount;
extern short redBullNumber;
extern Object *cameraDroids[];
extern short cameraCount;
extern int bobCount;
extern Object *fans[];
extern short fanCount;
extern Object *smokeSlowObjects[];
extern short smokeSlowCount;
extern short smokeNumber;
extern CVECTOR smokeColour;
extern int fastCount;
extern int fastTexCount;
extern Object *smokeFastObjects[];
extern short smokeFastCount;
extern int slowCount;
extern int slowTexCount;
extern Object *advert[];
extern short advertCount;
extern Object *radar[];
extern short dishCount;
extern Object *neonObject[];
extern short zepCount;
extern short beamCollide;
extern int loopNumber;


extern char wobbleDir3[];
extern char wobbleDir4[];
extern char g_cdMessage[];
extern char scenePath[];

extern int raceContext;


extern int pauseRequested;
extern int raceEndState;
extern int linkMyState;
extern int linkSendFlag;
extern u_char linkRaceDone;
extern u_short menuKeys;
extern int oneMoreTime;
extern int fillEnabled;
extern int loadPercent;

extern int smallVram;
extern short textureX;
extern short textureY;
extern Texture *textureTable[];
extern int trackNameScroll;
extern short raceFinished;
extern int fadeAmount;
extern int shipDeathCount[];
extern int fracdamage[];
extern int shipDamage[];
extern SParticle *particleSlots;

extern int attractDelayFrames;
extern int quadIndex;
extern short CurrentScreen;
extern int texture16Bit;
extern TrackData *track;
extern ConfigData *tempGameData;
extern RescueData rescueData;
extern void (*CameraUpdate)();
extern void (*prevCameraUpdate)();
extern Object *prmObjects[];
extern Object *girderObjects[];
extern Object *torchLight[];
extern Object *pylons[];

extern Object *cdState[];

/* Race state. */
#include "sound_state.h"
extern int skycyc;
extern int lightDroid;
extern int savedUpperY;
extern int spareMax;
extern int firstTimeIn;
extern int attractModeFlag;
extern int checkTimer;
extern int dangerFlag;
extern int shipLostFlag;

extern int raceEndFlag;
extern int hudEnabled;
extern volatile long FR60;
extern int raceElapsed;
extern int raceStartVcount;
#ifdef NETPLAY
int InitNetworkRace(void);
#endif
extern short trackCycleTimer;
extern short raceFlag;
extern short neonCount;
extern short girderCount;
extern short pylonCount;
extern short torchCount;
extern short torchNumber;
extern short prmCount;
extern short tempTextureCount;
extern int fpsSampleCount;
extern int fpsSampleTotal;
extern int displayStat1;

extern int blendByBucket;
extern short menuButtonCount;

/* Frame timings, in milliseconds. */
extern double frameStart;
extern double profileMark;
extern double frameTime;
extern double updateTime;
extern double setupTime;
extern double drawTime;
extern double flipTime;

void SetSpeedUpFaceTexture(char texture);
void SetBoostFaceTexture(short texture);
void UpdateStartDroid(Object **shapes, ShipData *ships, DroidData *droids, short textureBase, ConfigData *game, int mode);
#include "start_lights.h"
void DrawStartDroid(Object **objects, ShipData *ships, DroidData *droids);
void LogMemoryStatus(void);
double GetProfileTime(void);
#include "texture_pages.h"
void ResetFrameClock(void);
int UpdateFrameAndPresent(int unused);
void resetMiscFx(void);
void PlayCredits(int context);
void CallbackSwap(void);
void QuitGame(void);
void DrawEffects(WeaponData *weapons, ShipData *ships);
void DrawSectionObjects(void *unused, TrackCamera *camera);
void UpdateRaceComplete(void *screen, void *unused, ConfigData *config, short *complete, ScoreData *score, ShipData *ships);

/* Same computation as the original; only register allocation differs, so it is kept in EQUIV. */
#ifdef EQUIV
#ifndef NETPLAY
/* Runs a full race session until it ends or the player quits. */
int race(ConfigData *gameData)
{
    InGameMenuPolys inGameMenu;
    TrackData trackData;
    HiScoreData scoreTab;
    char text[64];
    ResetData resetData;
    int crowdCount;
    int noWeaponGrids;
    int thunderStart;
    int steps;
    int radius;
    int i;
    int thunderCycle;
    short tileCount;
    short skyObjectIndex;
    short raceComplete;
    short step;
    short noSpecialGrids;
    TexTemplate *tiles;
    WADFILE *wad;
    Object *zeppelinObject;
    Texture *texture;
    u_char selectedWeapon;
    ShipData *playerShip;
    WeaponGrid *raceWeaponGrids;

    tileCount = 0;
    noSpecialGrids = 0;
    noWeaponGrids = 0;
    raceComplete = 0;
    thunderCycle = 0;
    thunderStart = 0;
    InitDynamicMem(reserveHeap, 0x1C2000);
    advertCount = 0;
    cameraCount = 0;
    heap = reserveHeap;
    neonCount = 0;
    girderCount = 0;
    zepCount = 0;
    dishCount = 0;
    pylonCount = 0;
    fanCount = 0;
    smokeSlowCount = 0;
    smokeFastCount = 0;
    smokeNumber = 0;
    fastCount = 0;
    fastTexCount = 0;
    slowCount = 0;
    slowTexCount = 0;
    bobCount = 0;
    torchCount = 0;
    torchNumber = 0;
    fpsSampleCount = 0;
    fpsSampleTotal = 0;
    prmCount = 0;
    displayStat1 = 0;
    redBullCount = 0;
    attractModeFlag = 0;
    redBullNumber = 0;
    firstTimeIn = 1;
    checkTimer = 0;
    savedTextureCount = TextureTableCount;
    LogMemoryStatus();
    if (raceContext == 0)
        raceFlag = 1;
    BlockFill(0, 0, 320, 240, 0, 0, 0);
    BlockFill(0, 256, 320, 240, 0, 0, 0);
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    cameraPtr = &residentSkeleton;
    residentSkeleton.camPos->super = upperSkeleton;
    track = &trackData;
    tempGameData = gameData;
    getPath(scenePath, gameData->actTrackNum, "\\track.wad");
    wad = openwadfile(scenePath);
    WADInitVertices(wad);
    WADInitFaces(wad);
    WADInitSections(wad);
    WADInitViewList(wad);
    tempTextureCount = TextureTableCount;
    TextureTableCount = 0;
    OutputDebugString("Track load start\n");
    smallVram = 1;
    LoadCmpFiles(WADLoadCompressedTextureSequence(wad));
    textureY = 0;
    TextureTableCount = 0;
    smallVram = 0;
    textureX = 320;
    getPath(scenePath, gameData->actTrackNum, "\\tracktex.cmp");
    LoadCmpFiles(LoadCompressedTextureSequence(scenePath));
    OutputDebugString("Track load finish\n");
    tiles = WADLoadTtfFile(wad, &tileCount, 0);
    TextureTableCount = tempTextureCount;
    skyObjectIndex = WADInitScene(prmObjects, &prmCount, residentSkeleton.camPos, gameData, wad);
    closewadfile(wad);
    for (i = 0; i < 6; i++) {
        checkCells[gameData->trackNum * 4 + gameData->shipClass][i].bonusSeconds = 0;
        checkCells[gameData->trackNum * 4 + gameData->shipClass][i].section = -1;
        checkCells[gameData->trackNum * 4 + gameData->shipClass][i].done = -1;
    }
    InitTrackTextures(gameData);
    loadPercent = 0x60;
    FlushMessages();
    VsyncFunc();
    RaceFrameStart();
    InitWeaponGrids(weaponGrids, track, &noWeaponGrids, &noSpecialGrids, checkCells[gameData->trackNum * 4 + gameData->shipClass]);
    InitCheckPoints(checkCells[gameData->trackNum * 4 + gameData->shipClass], gameData);
    loadPercent++;
    FlushMessages();
    VsyncFunc();
    RaceFrameStart();
    trackCycleTimer = 0;
    InitTrackCycle();
    InitDroidThings(startDroidObjects, "sroid", residentSkeleton.camPos, checkCells[gameData->trackNum * 4 + gameData->shipClass], gameData, shipIndex);
    InitRaceLine(track, gameData);
    InitInGameMenu(&inGameMenu);
    crowdCount = InitCrowdSound(prmObjects, prmCount, cdState, 0);
    if (gameData->actTrackNum == 1) {
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
        InitObject(prmObjects, prmCount, fans, "fan", 3, &fanCount);
        InitObject(prmObjects, prmCount, smokeSlowObjects, "smokes", 6, &smokeSlowCount);
        InitObject(prmObjects, prmCount, smokeFastObjects, "smokef", 6, &smokeFastCount);
        InitObject(prmObjects, prmCount, redBull, "redb", 4, &redBullCount);
        redBullNumber = TextureTableCount;
        strcpy(startPath, wobbleDir3);
        strcat(startPath, "th.tim");
        texture = LoadTexture(startPath, 1);
        textureTable[TextureTableCount++] = texture;
        smokeNumber = TextureTableCount;
        strcpy(startPath, wobbleDir3);
        strcat(startPath, "smoke.cmp");
        LoadCmpFiles(LoadCompressedTextureSequence(startPath));
        smokeColour.r = 0x40;
        smokeColour.g = 0x40;
        smokeColour.b = 0x40;
    }
    if (gameData->actTrackNum == 6) {
        InitObject(prmObjects, prmCount, advert, "AGunit", 6, &advertCount);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
    }
    if (gameData->actTrackNum == 8) {
        InitObject(prmObjects, prmCount, advert, "AGunit", 6, &advertCount);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
    }
    if (gameData->actTrackNum == 20) {
        InitObject(prmObjects, prmCount, radar, "dish", 4, &dishCount);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
        InitObject(prmObjects, prmCount, neonObject, "grid1", 5, &neonCount);
    }
    if (gameData->actTrackNum == 17)
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
    if (gameData->actTrackNum == 7)
        InitObject(prmObjects, prmCount, advert, "AGunit", 6, &advertCount);
    if (gameData->actTrackNum == 13) {
        InitWaterFall();
        InitObject(prmObjects, prmCount, torchLight, "torch", 5, &torchCount);
    }
    if (gameData->actTrackNum == 2) {
        LoadTrain(monoRailObjs, "train", residentSkeleton.camPos, posMonoRail);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
        InitObject(prmObjects, prmCount, zeppelinObj, "zeppelin", 8, &zepCount);
        InitObject(prmObjects, prmCount, girderObjects, "stewy", 5, &girderCount);
        InitTrainLights(prmObjects, prmCount, monoRailObjs, pause.selection);
        monoRail.index = 0;
        monoRail.clearedX = 0;
        monoRail.clearedY = 0;
        monoRail.clearedZ = 0;
        zeppelinObj[0]->skeleton->relative.t[2] -= 8000;
        zeppelinObject = zeppelinObj[0];
        zeppelinBase.vx = zeppelinObject->skeleton->relative.t[0];
        zeppelinBase.vy = zeppelinObject->skeleton->relative.t[1];
        zeppelinBase.vz = zeppelinObject->skeleton->relative.t[2];
        InitObject(prmObjects, prmCount, pylons, "pylon", 5, &pylonCount);
        InitMonoData(pylons, pylonCount, &monoRail);
    }
    loadPercent++;
    FlushMessages();
    VsyncFunc();
    RaceFrameStart();
    PollKeyboard();
    initShipData(track->sections, shipIndex, gameData);
    InitDroidThings(lightGroups, "light", residentSkeleton.camPos, checkCells[gameData->trackNum * 4 + gameData->shipClass], gameData, shipIndex);
    if (gameData->actTrackNum == 13)
        LoadBirdGFX();
    InitRescueDroid(&rescueData, shipShapeTable, &shipIndex[ownShip]);
    CameraUpdate = UpdateCameraRaceStart;
    InitCamera(&residentSkeleton, shipIndex[ownShip].nearTrkSect);
    spareMax = 0;
    InitTrails();
    AsmGenerateTiles(tiles, tileCount);
    if (gameData->trackNum >= 3 && gameData->trackNum <= 6) {
        if (!rgbTextures)
            SetSpeedUpFaceTexture(tileCount);
    }
    if (!rgbTextures)
        SetBoostFaceTexture(tileCount);
    InitRaceResetData(&resetData, shipIndex, gameData, weaponIndex, weaponBars, &rescueData, shipShapeTable, checkCells[gameData->trackNum * 4 + gameData->shipClass]);
    memset(fracdamage, 0, 15 * sizeof(int));
    memset(shipDamage, 0, 15 * sizeof(int));
    particleSlots = InitParticle();
    InitExplode();
    InitRing();
    InitScoreTabData(&scoreTab);
    InitialiseBumps();
    if (gameType == 0) {
        switch (gameData->shipClass) {
        case 0:
            LoadModel(&teamShip, "vecto");
            break;
        case 1:
            LoadModel(&teamShip, "venom");
            break;
        case 2:
            LoadModel(&teamShip, "rapie");
            break;
        case 3:
            LoadModel(&teamShip, "phant");
            break;
        }
    } else if (gameType == 2 && gameData->venomComplete) {
        LoadModel(&teamShip, "rapie");
    } else if (gameType == 2 && gameData->rapierComplete) {
        LoadModel(&teamShip, "phant");
    }
    loadPercent++;
    FlushMessages();
    VsyncFunc();
    RaceFrameStart();
    trackNameScroll = 480;
    gameData->raceCount = 0;
    if (!blendByBucket)
        CreateTexturePages();
    if (!texture16Bit) {
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "nrace0");
        wsprintf(text, "%d", gameData->trackNum + 1);
        strcat(startPath, text);
        strcat(startPath, ".bmp");
        LoadBitmapTextures(0, startPath, 0, 11);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "sky0");
        wsprintf(text, "%d", gameData->trackNum + 1);
        strcat(startPath, text);
        strcat(startPath, ".bmp");
        LoadBitmapTextures(0, startPath, 17, 1);
    } else {
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "nrace0");
        wsprintf(text, "%d", gameData->trackNum + 1);
        strcat(startPath, text);
        strcat(startPath, ".tga");
        Load16BitBitmapTextures(0, startPath, 0, 11);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "sky0");
        wsprintf(text, "%d", gameData->trackNum + 1);
        strcat(startPath, text);
        strcat(startPath, ".tga");
        Load16BitBitmapTextures(0, startPath, 17, 1);
    }
    loadPercent++;
    FlushMessages();
    VsyncFunc();
    RaceFrameStart();
    droidPaletteColour[0] = droidBlinkColour[0];
    droidPaletteColour[1] = droidBlinkColour[1];
    droidPaletteColour[2] = droidBlinkColour[2];
    wsprintf(g_cdMessage, "Heap free before main loop: %d\n", heap->free);
    wsprintf(g_cdMessage, "Temp Heap free before main loop: %d\n", reserveHeap->free);
    raceFinished = 1;
    SfxSetVolume(gameData->effectVolume * 255 / 100);
    SfxSetCdVolume(gameData->musicVolume * 255 / 100);

/* Restart race setup before entering another frame sequence. */
restart:
    Restart(&resetData);
    InitRaceData(&pause);
    if (raceContext != 1)
        attractDelayFrames = 0;
    if (cheatWeapon)
        GetNewWeapon(shipIndex, ownShip);
    InitAttractMode(raceContext, shipIndex);
    gameData->finished = 0;
    SetUpFirstCheckPoint();
    ResetFrameClock();
    raceInProgress = 1;
    messageTimer = 0;
    ResetKeyboardState();
    goto frame;

    for (;; frameTime = GetProfileTime() - frameStart) {
/* Setup enters at the first frame; later frames use the loop tail. */
frame:
        fillEnabled = 0;
        frameStart = GetProfileTime();
        profileMark = frameStart;
        menuButtonCount = 0;
        dangerFlag = 0;
        shipLostFlag = 0;
        extendedTimeFlag = 0;
        quadIndex = CurrentScreen * 300;
        steps = FR60 >> 1;
        if (steps == 0)
            steps = 1;
        if (oneMoreTime) {
            oneMoreTime = 0;
            steps = 1;
        }
        for (step = 0; step < steps; step++) {
            PollKeyboard();
            DoDetailKeys();
            shipClassIndex = gameData->shipClass;
            clipLevel = gameData->trackNum;
            CheckShipDestroyed(shipIndex, gameData, &raceContext, 0xD2F0);
            if ((shipIndex[ownShip].attr & RACING) && (keysPressed & PadTriangle0)) {
                PlayNote(8, 255, 15, 15, 256);
                pauseRequested ^= 1;
            }
            CheckGamePause(shipIndex, gameData);
            PerformPauseOperations(&inGameMenu, gameData);
            if (pause.selection == -1) {
                CheckForButtonPress();
                UpdateShips(shipIndex, weaponBars, shipShapeTable, weaponIndex, 0);
                rescueData.droidUpdate(&rescueData, &shipIndex[ownShip], shipShapeTable);
                CameraUpdate(&residentSkeleton, &shipIndex[ownShip], &rescueData);
                {
                    playerShip = &shipIndex[ownShip];
                    if (playerShip->attr & RACING) {
                        selectedWeapon = playerShip->weapon;
                        if (selectedWeapon == 2 || selectedWeapon == 5) {
                            MissileLock(ownShip, shipIndex);
                            if (shipIndex[ownShip].weapon == 2)
                                RearMissileLock(ownShip, shipIndex);
                        }
                        ShakeScreen();
                    }
                }
                UpdateWeapons(shipIndex, weaponBars, weaponIndex);
                CheckPitShips();
                if (pause.selection == -1) {
                    AdjustEngineSound(&shipIndex[ownShip], &residentSkeleton);
                    AdjustCrowdSound(cdState, crowdCount);
                }
                TrackSpecificFunctions();
                ColourDroidLights(lightDroid);
                AddEffects(weaponIndex, shipIndex);
                raceElapsed = -raceStartVcount;
                raceWeaponGrids = weaponGrids;
                if (gameType != 1 && gameData->weaponsOn)
                    CycleWeaponGrids(shipIndex, raceWeaponGrids, noWeaponGrids);
                else if (!gameData->weaponsOn && gameType == 2)
                    CycleWeaponGrids(shipIndex, raceWeaponGrids, noWeaponGrids);
                UpdateShipTrails(shipIndex, attractDelayFrames);
                if (shipDeathCount[ownShip] == 100)
                    shipIndex[ownShip].attr &= ~VIEW_IN;
                if ((shipDeathCount[ownShip] & 7) < 4 && shipDeathCount[ownShip] > 15 && !(shipIndex[ownShip].combatAttr & 4)) {
                    dangerFlag = 1;
                    if (!(shipDeathCount[ownShip] & 7))
                        PlayNote(SAMPLE_WARNING, 255, 15, 15, 256);
                }
                for (i = 0; i < 15; i++) {
                    if (!(shipIndex[i].combatAttr & DESTROYED) && shipDeathCount[i] > 0) {
                        if (i == ownShip && shipDeathCount[ownShip] > 15) {
                            shipIndex[i].ppivot.vx += shipIndex[i].vpivot.vx >> 6;
                            shipIndex[i].ppivot.vy += shipIndex[i].vpivot.vy >> 6;
                            shipIndex[i].ppivot.vz += shipIndex[i].vpivot.vz >> 6;
                            shipIndex[i].vpivot.vx -= shipIndex[i].vpivot.vx >> 3;
                            shipIndex[i].vpivot.vy -= shipIndex[i].vpivot.vy >> 3;
                            shipIndex[i].vpivot.vz -= shipIndex[i].vpivot.vz >> 3;
                            shipIndex[i].vhdg = 0;
                            shipIndex[i].roll += (shipIndex[i].vpivot.vz + shipIndex[i].vpivot.vx) >> 5;
                        }
                        if ((shipDeathCount[i] & 3) == 1 && shipDeathCount[i] < 15) {
                            radius = (15 - shipDeathCount[i]) * 64;
                            AddExplode(shipIndex[i].ppivot.vx + newRand() % radius - (radius >> 1),
                                       shipIndex[i].ppivot.vy - newRand() % 127,
                                       shipIndex[i].ppivot.vz + newRand() % radius - (radius >> 1), -1);
                            AddSmokeExplode(shipIndex[i].ppivot.vx + newRand() % radius - (radius >> 1),
                                            shipIndex[i].ppivot.vy - newRand() % 127,
                                            shipIndex[i].ppivot.vz + newRand() % radius - (radius >> 1), -1);
                            radius = (20 - shipDeathCount[i]) * 64;
                            AddSpurtExplode(shipIndex[i].ppivot.vx + newRand() % radius - (radius >> 1),
                                            shipIndex[i].ppivot.vy - newRand() % 127,
                                            shipIndex[i].ppivot.vz + newRand() % radius - (radius >> 1), -1);
                        }
                        if (shipDeathCount[i] == 14) {
                            PlayDirectionalNote(5, 0, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote(5, 0, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote(5, -64, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote(5, -128, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote(5, -128, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote(5, -192, 40, shipIndex[i].ppivot);
                        }
                        if ((newRand() & 3) == 1 && shipDeathCount[i] > 15)
                            AddSmallSmokeExplode(shipIndex[i].ppivot.vx - (shipIndex[i].unitVecNose.vx >> 4),
                                                 shipIndex[i].ppivot.vy - (shipIndex[i].unitVecNose.vy >> 4),
                                                 shipIndex[i].ppivot.vz - (shipIndex[i].unitVecNose.vz >> 4), -1);
                        if ((shipDeathCount[i] & 3) == 1 && shipDeathCount[i] > 15)
                            AddSmallSpurtExplode((newRand() & 511) - (shipIndex[i].unitVecNose.vx >> 5) - 256,
                                                 128 - (shipIndex[i].unitVecNose.vy >> 5),
                                                 (newRand() & 511) - (shipIndex[i].unitVecNose.vz >> 5) - 256, i);
                        shipDeathCount[i]--;
                        if (shipDeathCount[i] == 0) {
                            if (i != ownShip) {
                                shipLostFlag = 1;
                                shipIndex[i].attr &= ~RACING;
                            }
                            shipIndex[i].combatAttr |= DESTROYED;
                        }
                    }
                    if (i == ownShip && (shipIndex[ownShip].combatAttr & DESTROYED)) {
                        if ((newRand() & 31) < 3)
                            AddSmallSmokeExplode(shipIndex[i].ppivot.vx, shipIndex[i].nearTrkSect->centre.vy + 128,
                                                 shipIndex[i].ppivot.vz, -1);
                    }
                }
                if (gameData->actTrackNum == 2 || gameData->actTrackNum == 7 || cheatRain) {
                    thunderCycle = (thunderCycle + 1) & 255;
                    AddRain(particleSlots, 1, &cameraPtr->camPos->relative.t[0]);
                    if (!undercover && !(newRand() & 3) && thunderCycle < 32 && !(thunderCycle & 3)) {
                        if (fadeAmount < 228)
                            fadeAmount = 228;
                        thunderStart = thunderCycle;
                    }
                    if (fadeAmount == 220)
                        fadeAmount = 118;
                    if (fadeAmount == 110)
                        fadeAmount = 0;
                    if (thunderStart - thunderCycle == -32)
                        PlayNote(SAMPLE_THUNDER, 380, 15, 15, 128);
                    if (thunderStart + 32 < thunderCycle && thunderCycle < 64 && !(newRand() & 3) && !(thunderCycle & 3))
                        PlayNote(SAMPLE_THUNDER, (short)(412 - thunderCycle), 15, 15, 320 - thunderCycle);
                }
                if (gameData->actTrackNum == 8)
                    AddSnow(particleSlots, 20, &cameraPtr->camPos->relative.t[0]);
                prmObjects[skyObjectIndex]->skeleton->update = 1;
                savedUpperY = residentSkeleton.camPos->super->relative.t[1];
                if (gameData->actTrackNum == 17) {
                    if (residentSkeleton.camPos->relative.t[1] > 0)
                        residentSkeleton.camPos->super->relative.t[1] += 1024 + (((residentSkeleton.camPos->relative.t[1] >> 3) * rsin(skycyc)) >> 12);
                    if (pause.selection == -1 && (residentSkeleton.section->secNo <= 302 || residentSkeleton.section->secNo >= 329))
                        skycyc = (skycyc + 96) & 4095;
                }
                residentSkeleton.camPos->super->relative.t[1] = savedUpperY;
                DrawShips(shipIndex, shipShapeTable, ownShip, residentSkeleton, pause.selection);
                UpdateStartDroid(startDroidObjects, shipIndex, checkCells[gameData->trackNum * 4 + gameData->shipClass], startTextureBase, gameData, pause.selection);
                UpdateLightStart(lightGroups, shipIndex, checkCells[gameData->trackNum * 4 + gameData->shipClass], startTextureBase, gameData, pause.selection);
                if (gameData->actTrackNum == 2 && shipIndex[ownShip].nearTrkSect->secNo >= 200 && shipIndex[ownShip].nearTrkSect->secNo <= 275) {
                    MoveMonoRail(monoRailObjs, 6, &monoRail, shipIndex, pause.selection);
                    InitTrainLights(prmObjects, prmCount, monoRailObjs, pause.selection);
                }
                if (sillyShips)
                    UpdateSharkModel();
                UpdateParticles(particleSlots, residentSkeleton.camPos, pause.selection + 1);
                UpdateExplode();
                if (gameData->actTrackNum == 13)
                    UpdateBirds();
                UpdateRing();
            }
            if (!linkSendFlag) {
                if (raceContext == 1 && attractDelayFrames > 0) {
                    attractDelayFrames--;
                    if (attractDelayFrames == 0) {
                        raceEndState = 1;
                        linkMyState = 2;
                        raceInProgress = 0;
                    } else if (menuKeys != 0 || pauseRequested) {
                        if (!(shipIndex[ownShip].combatAttr & DESTROYED)) {
                            pauseRequested = 0;
                            raceEndState = 1;
                            linkMyState = 3;
                            raceInProgress = 0;
                        }
                    }
                }
            } else {
                UpdateRaceComplete(&inGameMenu, &resetData, gameData, &raceComplete, &scoreTab, shipIndex, &pause);
            }
            if (raceEndState > 0) {
                raceEndFlag = 1;
                raceEndState--;
                if (raceEndState == 0)
                    /* Leave the frame before the remaining updates and drawing. */
                    goto quit;
            }
            if (scoreTab.highScore != 1 && raceEndState == 0) {
                UpdateRaceHud(shipIndex, pause.selection, gameData, &scoreTab);
                if (shipIndex[ownShip].update == UpdatePlayerShip || shipIndex[ownShip].update == UpdateRescueDroid
                    || (shipIndex[ownShip].update == UpdateDroneShip && shipIndex[ownShip].autopilotCount > 0)) {
                    if (!(shipIndex[ownShip].combatAttr & 4) || !(shipIndex[ownShip].combatAttr & DESTROYED))
                        AlterCheckPoint(checkCells[gameData->trackNum * 4 + gameData->shipClass], shipIndex, pause.selection, gameData);
                }
            }
        }
        updateTime = GetProfileTime() - profileMark;
        profileMark = GetProfileTime();
        if (!firstTimeIn)
            UpdateFrameAndPresent(0);
        setupTime = GetProfileTime() - profileMark;
        profileMark = GetProfileTime();
        resetMiscFx();
        PlayCredits(raceContext);
        DrawPauseMenu(&inGameMenu, gameData);
        if (gameData->actTrackNum == 2 && shipIndex[ownShip].nearTrkSect->secNo >= 200 && shipIndex[ownShip].nearTrkSect->secNo <= 275)
            DrawMonoRail(monoRailObjs, 6, &monoRail, shipIndex, pause.selection);
        if ((shipIndex[ownShip].attr & RACING) && shipDeathCount[ownShip] <= -1) {
            if (!(shipIndex[ownShip].attr & DIRECTION) && gameData->trackNum != 1 && gameData->trackNum != 6)
                DrawAllDirections(&residentSkeleton);
            else
                DrawNewTrack(&residentSkeleton);
        } else {
            DrawAllTrack(&residentSkeleton);
        }
        DrawShadow(shipShapeTable, shipIndex);
        AddShipRenderObjects(shipIndex, shipShapeTable, ownShip, residentSkeleton, pause.selection);
        DrawWeapons(shipIndex, weaponBars, weaponIndex);
        if (rescueData.droidUpdate == UpdateDroidStart || rescueData.droidUpdate == UpdateDroidCircle || rescueData.droidUpdate == UpdateDroidRescue)
            DrawRescueDroid(&rescueData, shipShapeTable);
        DrawEffects(weaponIndex, shipIndex);
        if (hudEnabled) {
            if (gameData->weaponsOn || gameType == 1 || gameType == 2) {
                if ((shipIndex[ownShip].attr & RACING) && ownShip < 12)
                    DrawWeaponIcons(&shipIndex[ownShip]);
            }
        }
        if (linkSendFlag)
            RaceComplete(&inGameMenu, &resetData, gameData, &raceComplete, &scoreTab, shipIndex, &pause);
        if (hudEnabled && scoreTab.highScore != 1 && raceEndState == 0)
            DrawRaceHud(shipIndex, pause.selection, gameData, &scoreTab);
        DrawSectionObjects(&shipIndex[ownShip], &residentSkeleton);
        if (gameData->skyEnabled && normalSetNo != 16)
            AddRenderObject(prmObjects[skyObjectIndex], 0, 0);
        if (prevCameraUpdate != CameraUpdate) {
            UnsetParticles(particleSlots);
            prevCameraUpdate = CameraUpdate;
        }
        if (sillyShips && pause.selection == -1)
            UpdateSharkFeet();
        AsmDrawParticles(particleSlots, residentSkeleton.camPos, pause.selection + 1);
        DrawExplode();
        if (gameData->actTrackNum == 13)
            DrawBirds();
        DrawRing();
        resetMiscFx();
        DrawShipTrails(cameraPtr);
        if (dangerFlag) {
            CentreText("DANGER", 15, 64, 8, 0);
            CentreText("CRITICAL SHIELD FAILURE", 45, 128, 8, 1);
        }
        if (extendedTimeFlag)
            CentreText("EXTENDED TIME", 230, 75, 8, 1);
        if (shipLostFlag)
            PlayNote(SAMPLE_CONTINUE_ELIMINATED, 480, 15, 15, 256);
        DrawStartDroid(startDroidObjects, shipIndex, checkCells[gameData->trackNum * 4 + gameData->shipClass], startTextureBase, gameData, pause.selection);
        DrawLightStart(lightGroups, shipIndex, checkCells[gameData->trackNum * 4 + gameData->shipClass], startTextureBase, gameData, pause.selection);
        cardNum.accumulator -= FR60 * 30;
        menuHeading.accumulator -= FR60 * 15;
        drawTime = GetProfileTime() - profileMark;
        profileMark = GetProfileTime();
        CallbackSwap();
        flipTime = GetProfileTime() - profileMark;
        profileMark = GetProfileTime();
        firstTimeIn = 0;
        UpdateNotes(0);
        if (linkRaceDone) {
            raceContext = 0;
            /* A completed link race returns to race setup. */
            goto restart;
        }
    }
/* Race termination converges here for shared cleanup. */
quit:
    fillEnabled = 1;
    QuitGame();
    return 0;
}
#endif
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* Unused argument bits and identical constant copies differ. */
#pragma intrinsic(strcpy, strcat, memcmp, sin)
/* Runs a full race session until it ends or the player quits. */
int race(ConfigData *gameData)
{
    short raceNumTiles;
    TrackData trackData;
    int currentNoWeaponGrids;
    HiScoreData initialScoreData;
    TexTemplate * tileData;
    TrackSection *vcountStart;
    int savedRadius;
    Texture * currentNewTexture;
    WADFILE * localWad;
    int loadOnce;
    ResetData currentResetData;
    BLK_FILL * screenFill1;
    BLK_FILL * raceLowerFill;
    int shipNumber;
    short initialSkyNumber;
    short savedComplete;
    short localTileStart;
    int localDestroyedTime;
    char savedText[64];
    int localFirstPass;
    int localCrowdCount;
    int localStormCycle;
    int thunderStartCycle;
    TIMlist * textureSequence;
    short initialNoSpecialGrids;
    int initialSteps;
    int shipCounter;
    int localFrameCounter;
    short frameStep;
    InGameMenuPolys raceMenu;
    raceNumTiles = 0;
    localFrameCounter = 0; /* The original initializes this counter but never reads it. */
    localDestroyedTime = 0xD2F0;
    localCrowdCount = 0;
    currentNoWeaponGrids = 0;
    loadOnce = 1; /* The original never reads this flag. */
    localFirstPass = 1; /* The original never reads this flag. */
    initialNoSpecialGrids = 0;
    savedComplete = 0;
    frameStep = 0;
    initialSteps = 0;
    localStormCycle = 0;
    thunderStartCycle = 0;
    SetLocalRandom(0);
    if (networkGame) {
        if (!InitNetworkRace())
            return 0;
    }
    InitDynamicMem(reserveHeap, 0x1C2000);
    heap = reserveHeap;
    advertCount = 0;
    cameraCount = 0;
    neonCount = 0;
    girderCount = 0;
    zepCount = 0;
    dishCount = 0;
    pylonCount = 0;
    fanCount = 0;
    smokeSlowCount = 0;
    smokeFastCount = 0;
    smokeNumber = 0;
    fastCount = 0;
    fastTexCount = 0;
    slowCount = 0;
    slowTexCount = 0;
    torchCount = 0;
    torchNumber = 0;
    bobCount = 0;
    prmCount = 0;
    redBullCount = 0;
    redBullNumber = 0;
    fpsSampleCount = 0;
    fpsSampleTotal = 0;
    displayStat1 = 0;
    attractModeFlag = 0;
    firstTimeIn = 1;
    checkTimer = 0;
    savedCheckTimer = 0;
    savedTextureCount = TextureTableCount;
    LogMemoryStatus();
    if (raceContext == 0)
        raceFlag = 1;
    screenFill1 = BlockFill(0, 0, 320, 240, 0, 0, 0);
    raceLowerFill = BlockFill(0, 256, 320, 240, 0, 0, 0);
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    residentSkeleton.camPos->super = upperSkeleton;
    cameraPtr = &residentSkeleton;
    track = &trackData;
    tempGameData = gameData;
    getPath(scenePath, gameData->actTrackNum, "\\track.wad");
    localWad = openwadfile(scenePath);
    WADInitVertices(localWad);
    WADInitFaces(localWad);
    WADInitSections(localWad);
    WADInitViewList(localWad);
    tempTextureCount = TextureTableCount;
    localTileStart = 0;
    TextureTableCount = 0;
    OutputDebugString("Track load start\n");
    smallVram = 1;
    textureSequence = WADLoadCompressedTextureSequence(localWad);
    LoadCmpFiles(textureSequence);
    textureX = 320;
    textureY = 0;
    TextureTableCount = 0;
    smallVram = 0;
    getPath(scenePath, gameData->actTrackNum, "\\tracktex.cmp");
    textureSequence = LoadCompressedTextureSequence(scenePath);
    LoadCmpFiles(textureSequence);
    OutputDebugString("Track load finish\n");
    tileData = WADLoadTtfFile(localWad, &raceNumTiles, localTileStart);
    TextureTableCount = tempTextureCount;
    initialSkyNumber = WADInitScene(prmObjects, &prmCount, residentSkeleton.camPos, gameData, localWad);
    closewadfile(localWad);
    for (shipNumber = 0; shipNumber < 6; shipNumber++) {
        ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass][shipNumber].bonusSeconds = 0;
        ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass][shipNumber].section = -1;
        ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass][shipNumber].done = -1;
    }
    InitTrackTextures(gameData);
    loadPercent = 0x60;
    RaceFrameEnd();
    InitWeaponGrids(weaponGrids, track, &currentNoWeaponGrids, &initialNoSpecialGrids, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass]);
    InitCheckPoints(((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], gameData);
    loadPercent++;
    RaceFrameEnd();
    trackCycleTimer = 0;
    InitTrackCycle();
    InitDroidThings(startDroidObjects, "sroid", residentSkeleton.camPos, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], gameData, shipIndex);
    InitRaceLine(track, gameData);
    InitInGameMenu(&raceMenu);
    localCrowdCount = InitCrowdSound(prmObjects, prmCount, cdState, localCrowdCount);
    if (gameData->actTrackNum == 1) {
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
        InitObject(prmObjects, prmCount, fans, "fan", 3, &fanCount);
        InitObject(prmObjects, prmCount, smokeSlowObjects, "smokes", 6, &smokeSlowCount);
        InitObject(prmObjects, prmCount, smokeFastObjects, "smokef", 6, &smokeFastCount);
        InitObject(prmObjects, prmCount, redBull, "redb", 4, &redBullCount);
        redBullNumber = TextureTableCount;
        strcpy(startPath, wobbleDir3);
        strcat(startPath, "th.tim");
        currentNewTexture = LoadTexture(startPath, 1);
        textureTable[TextureTableCount++] = currentNewTexture;
        smokeNumber = TextureTableCount;
        strcpy(startPath, wobbleDir3);
        strcat(startPath, "smoke.cmp");
        textureSequence = LoadCompressedTextureSequence(startPath);
        LoadCmpFiles(textureSequence);
        smokeColour.r = 0x40;
        smokeColour.g = 0x40;
        smokeColour.b = 0x40;
    }
    if (gameData->actTrackNum == 6) {
        InitObject(prmObjects, prmCount, advert, "AGunit", 6, &advertCount);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
    }
    if (gameData->actTrackNum == 8) {
        InitObject(prmObjects, prmCount, advert, "AGunit", 6, &advertCount);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
    }
    if (gameData->actTrackNum == 20) {
        InitObject(prmObjects, prmCount, radar, "dish", 4, &dishCount);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
        InitObject(prmObjects, prmCount, neonObject, "grid1", 5, &neonCount);
    }
    if (gameData->actTrackNum == 17)
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
    if (gameData->actTrackNum == 7)
        InitObject(prmObjects, prmCount, advert, "AGunit", 6, &advertCount);
    if (gameData->actTrackNum == 13) {
        InitWaterFall();
        InitObject(prmObjects, prmCount, torchLight, "torch", 5, &torchCount);
    }
    if (gameData->actTrackNum == 2) {
        LoadTrain(monoRailObjs, "train", residentSkeleton.camPos, posMonoRail);
        InitObject(prmObjects, prmCount, cameraDroids, "camera", 6, &cameraCount);
        InitObject(prmObjects, prmCount, zeppelinObj, "zeppelin", 8, &zepCount);
        InitObject(prmObjects, prmCount, girderObjects, "stewy", 5, &girderCount);
        InitTrainLights(prmObjects, prmCount, monoRailObjs, pause.selection);
        monoRail.index = 0;
        monoRail.clearedX = 0;
        monoRail.clearedY = 0;
        monoRail.clearedZ = 0;
        zeppelinObj[0]->skeleton->relative.t[2] -= 8000;
        zeppelinBase.vx = zeppelinObj[0]->skeleton->relative.t[0];
        zeppelinBase.vy = zeppelinObj[0]->skeleton->relative.t[1];
        zeppelinBase.vz = zeppelinObj[0]->skeleton->relative.t[2];
        InitObject(prmObjects, prmCount, pylons, "pylon", 5, &pylonCount);
        InitMonoData(pylons, pylonCount, &monoRail);
    }
    loadPercent++;
    RaceFrameEnd();
    PollKeyboard();
    initShipData(track->sections, shipIndex, gameData);
    InitDroidThings(lightGroups, "light", residentSkeleton.camPos, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], gameData, shipIndex);
    if (gameData->actTrackNum == 13)
        LoadBirdGFX();
    InitRescueDroid(&rescueData, shipShapeTable, &shipIndex[ownShip]);
    CameraUpdate = UpdateCameraRaceStart;
    InitCamera(&residentSkeleton, shipIndex[ownShip].nearTrkSect);
    spareMax = 0;
    InitTrails();
    AsmGenerateTiles(tileData, raceNumTiles);
    if (gameData->trackNum >= 3 && gameData->trackNum <= 6) {
        if (!rgbTextures)
            SetSpeedUpFaceTexture(raceNumTiles);
    }
    if (!rgbTextures)
        SetBoostFaceTexture(raceNumTiles);
    InitRaceResetData(&currentResetData, shipIndex, gameData, weaponIndex, weaponBars, &rescueData, shipShapeTable, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass]);
    for (shipNumber = 0; shipNumber < 15; shipNumber++) {
        shipDamage[shipNumber] = 0;
        fracdamage[shipNumber] = 0;
    }
    particleSlots = InitParticle();
    InitExplode();
    InitRing();
    InitScoreTabData(&initialScoreData);
    InitialiseBumps();
    if (gameType == 0) {
        switch (gameData->shipClass) {
        case 0:
            LoadModel(&teamShip, "vecto");
            break;
        case 1:
            LoadModel(&teamShip, "venom");
            break;
        case 2:
            LoadModel(&teamShip, "rapie");
            break;
        case 3:
            LoadModel(&teamShip, "phant");
            break;
        }
    } else if (gameType == 2 && gameData->phantomChallenge) {
        LoadModel(&teamShip, "rapie");
    } else if (gameType == 2 && gameData->piranhaChallenge) {
        LoadModel(&teamShip, "phant");
    }
    loadPercent++;
    RaceFrameEnd();
    trackNameScroll = 480;
    gameData->raceCount = 0;
    if (!blendByBucket)
        CreateTexturePages();
    if (!texture16Bit) {
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "nrace0");
        wsprintf(savedText, "%d", gameData->trackNum + 1);
        strcat(startPath, savedText);
        strcat(startPath, ".bmp");
        LoadBitmapTextures(0, startPath, 0, 11);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "sky0");
        wsprintf(savedText, "%d", gameData->trackNum + 1);
        strcat(startPath, savedText);
        strcat(startPath, ".bmp");
        LoadBitmapTextures(0, startPath, 17, 1);
    } else {
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "nrace0");
        wsprintf(savedText, "%d", gameData->trackNum + 1);
        strcat(startPath, savedText);
        strcat(startPath, ".tga");
        Load16BitBitmapTextures(0, startPath, 0, 11);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "sky0");
        wsprintf(savedText, "%d", gameData->trackNum + 1);
        strcat(startPath, savedText);
        strcat(startPath, ".tga");
        Load16BitBitmapTextures(0, startPath, 17, 1);
    }
    loadPercent++;
    RaceFrameEnd();
    droidPaletteColour[0] = droidBlinkColour[0];
    droidPaletteColour[1] = droidBlinkColour[1];
    droidPaletteColour[2] = droidBlinkColour[2];
    wsprintf(g_cdMessage, "Heap free before main loop: %d\n", heap->free);
    wsprintf(g_cdMessage, "Temp Heap free before main loop: %d\n", reserveHeap->free);
    raceFinished = 1;
    SfxSetVolume(gameData->effectVolume * 255 / 100);
    SfxSetCdVolume(gameData->musicVolume * 255 / 100);

    for (;;) {
        Restart(&currentResetData);
        InitRaceData(&pause);
        if (raceContext != 1)
            attractDelayFrames = 0;
        if (cheatWeapon)
            GetNewWeapon(shipIndex, ownShip);
        InitAttractMode(raceContext, shipIndex);
        vcountStart = raceStartSection;
        gameData->finished = 0;
        SetUpFirstCheckPoint();
        ResetFrameClock();
        raceInProgress = 1;
        messageTimer = 0;
        func_43dd04();
        if (networkGame) {
            SetLocalRandom(1);
            if (!gstEnumLockstepPlayers(gstSessionId)) {
                gstDebugPrintf("race: gstEnumLockstepPlayers Failed, 2nd while()");
                fillEnabled = 1;
                QuitGame();
                gstShutdownNetworkRace(1);
                return 0;
            }
            gstPostLockstepEvent(0, 0, 0, gstLockstepChannel, 1);
            gstDumpSync(-1);
            savedCheckTimer = checkTimer;
        }
        raceFrameReady = 1;
        for (;;) {
            fillEnabled = 0;
            frameStart = GetProfileTime();
            profileMark = frameStart;
            quadIndex = CurrentScreen * 300;
            menuButtonCount = 0;
            dangerFlag = 0;
            shipLostFlag = 0;
            killMessageFlash = 0;
            extendedTimeFlag = 0;
            initialSteps = FR60 >> 1;
            if (initialSteps == 0)
                initialSteps = 1;
            if (oneMoreTime) {
                oneMoreTime = 0;
                initialSteps = 1;
            }
            if (networkGame) {
                initialSteps = gstEventGetCount();
                if (raceControlError) {
                    gstDebugPrintf("quit_flag %d, Exiting Race", raceControlError);
                    fillEnabled = 1;
                    QuitGame();
                    if (raceControlError == 2) {
                        gstDebugPrintf("calling ShutdownNetworkRace(0): EXITRACE_NEXTRACE...");
                        gstShutdownNetworkRace(0);
                        linkMyState = 4;
                        Sleep(2000);
                    } else {
                        gstDebugPrintf("calling ShutdownNetworkRace(1): EXITRACE_TOMENUS");
                        gstShutdownNetworkRace(1);
                        linkMyState = 3;
                    }
                    return 0;
                }
            }
            for (frameStep = 0; frameStep < initialSteps; frameStep++) {
                if (networkGame) {
                    GstEvent *event;
                    event = 0;
                    if (gstRestartPending == 1) {
                        gstDebugPrintf("1: restart_pending %d, oldLockstepChannel %d", gstRestartPending, gstOldLockstepChannel);
                        event = gstEventDequeue(gstOldLockstepChannel);
                    } else {
                        event = gstEventDequeue(gstLockstepChannel);
                    }
                    if (!event) {
                        if (gstRestartPending == 1) {
                            gstRestartPending = 2;
                            frameStep = initialSteps;
                            gstDebugPrintf("2: restart_pending %d, passCounter %d, numTimes %d", gstRestartPending, frameStep, initialSteps);
                        }
                        continue;
                    }
                    ++localRandomSeed;
                    if (gstDebugOutputEnabled) {
                        if ((unsigned)localRandomSeed % 100 == 0) {
                            for (shipNumber = 0; shipNumber < 15; shipNumber++) {
                                if (shipIndex[shipNumber].netActive == 1) {
                                    gstSyncLocal[shipNumber].valid = 1;
                                    gstSyncLocal[shipNumber].x = shipIndex[shipNumber].ppivot.vx;
                                    gstSyncLocal[shipNumber].y = shipIndex[shipNumber].ppivot.vy;
                                    gstSyncLocal[shipNumber].z = shipIndex[shipNumber].ppivot.vz;
                                }
                            }
                        }
                        if ((unsigned)localRandomSeed % 110 == 0)
                            gstPostSessionAsynch(gstSessionId, &gstSyncLocal[ownShip], 20, 1);
                    }
                    for (shipNumber = 0; shipNumber < 15; shipNumber++) {
                        if ((unsigned char)gstCheckFail[shipNumber].set == 1 && gstCheckFail[shipNumber].when == localRandomSeed) {
                            gstDebugPrintf("CheckPointHack shipNo %d failed to pass CheckPoints", shipNumber);
                            shipIndex[shipNumber].combatAttr |= 8;
                            gstCheckFail[shipNumber].set = 0;
                            gstCheckFail[shipNumber].when = 0;
                        }
                    }
                    for (shipNumber = 0; shipNumber < 15; shipNumber++) {
                        if ((unsigned char)gstRestartHack[shipNumber].set == 1 && gstRestartHack[shipNumber].when == localRandomSeed) {
                            int foundShip;
                            int sortIndex;
                            if (lockstepPosted[shipNumber] == 1) {
                                gstDebugPrintf("SentCheckSlowDown: fixing CheckPoint data for shipNo %d", shipNumber);
                                lockstepPosted[shipNumber] = 0;
                                shipIndex[ownShip].combatAttr &= ~8;
                                if (!(shipIndex[ownShip].combatAttr & 0x40)) {
                                    shipIndex[ownShip].combatAttr |= 0x40;
                                    gstDebugPrintf("SentCheckSlowDown: not racing! fixed: racing now");
                                }
                                if (ownShip == shipNumber) {
                                    checkTimer = savedCheckTimer;
                                    gstDebugPrintf("SentCheckSlowDown: restored checkTime to %d", checkTimer);
                                }
                            }
                            gstDebugPrintf("RestartHack: shipNo %d: Undestroying Ship", shipNumber);
                            gstRestartHack[shipNumber].set = 0;
                            gstRestartHack[shipNumber].when = 0;
                            shipIndex[shipNumber].combatAttr &= ~DESTROYED;
                            shipIndex[shipNumber].attr |= RACING;
                            shipDeathCount[shipNumber] = -1;
                            fracdamage[shipNumber] = 0;
                            shipDamage[shipNumber] = 0;
                            if (ownShip == shipNumber) {
                                if (playerDestroyed)
                                    playerDestroyed = 0;
                                CameraUpdate = UpdateCameraRaceExternal;
                            }
                            if (gstLevelLives > 0)
                                livesRemaining[shipNumber]--;
                            foundShip = 0;
                            for (sortIndex = 0; activeShipCount > sortIndex; sortIndex++) {
                                if (shipRank[sortIndex].shipNo == shipNumber)
                                    foundShip = 1;
                            }
                            if (!foundShip) {
                                gstDebugPrintf("RestartHack: adding shipNo %d to sort table", shipNumber);
                                shipRank[activeShipCount].position = raceLines[shipIndex[shipNumber].nearSecNo].distance;
                                shipRank[activeShipCount].shipNo = shipNumber;
                                gstDebugPrintf("RestartHack: shipSortTable[ %d ].shipNumber %d", activeShipCount, shipRank[activeShipCount].shipNo);
                                activeShipCount++;
                            } else {
                                gstDebugPrintf("RestartHack: shipNo %d already in sort table", shipNumber);
                            }
                        }
                    }
                    if (gstDebugOutputEnabled)
                        printf("%d: ", event->kind);
                    fflush(stdout);
                    for (shipNumber = 0; shipNumber < 15; shipNumber++) {
                        if (shipIndex[shipNumber].netActive == 1) {
                            unsigned short padInput;
                            unsigned short auxInput;
                            unsigned short steering;
                            unsigned char missingPacket[6];
                            unsigned char inputSlot;
                            inputSlot = shipIndex[shipNumber].netSlot;
                            missingPacket[0] = 0xff;
                            missingPacket[1] = 0xff;
                            missingPacket[2] = 0xff;
                            missingPacket[3] = 0xff;
                            missingPacket[4] = 0xff;
                            missingPacket[5] = 0xff;
                            padInput = event->packet.input[inputSlot].pad;
                            steering = event->packet.input[inputSlot].steer;
                            auxInput = event->packet.input[inputSlot].aux;
                            if (memcmp(event->packet.player[inputSlot], missingPacket, 6) == 0) {
                                char missingText[100];
                                sprintf(missingText, "Got Lie for Ship %d", inputSlot);
                                gstDebugWriteText(0, 1, missingText);
                                padInput = 0;
                                steering = 128;
                                auxInput = 0;
                            }
                            shipIndex[shipNumber].net3FA = padInput;
                            shipIndex[shipNumber].net3FE = steering;
                            shipIndex[shipNumber].net3FC = auxInput;
                            if (gstDebugOutputEnabled)
                                printf("%2d %4x %4x %4x ", shipNumber, padInput & 0xffff, steering & 0xffff, auxInput & 0xffff);
                            fflush(stdout);
                        } else {
                            shipIndex[shipNumber].net3FA = 0;
                            shipIndex[shipNumber].net3FE = 128;
                            shipIndex[shipNumber].net3FC = 0;
                        }
                    }
                } else {
                    PollKeyboard();
                    DoDetailKeys();
                }
                if (gstDebugOutputEnabled)
                    printf(" %d\n", localRandomSeed);
                fflush(stdout);
                if (networkGame) {
                    short lapNumber;
                    int shipNo;
                    for (shipNo = 0; shipNo < 15; shipNo++) {
                        if ((shipIndex[shipNo].netActive & 1) && (shipIndex[shipNo].combatAttr & 0x4000)
                            && classLaps[gameData->shipClass] + 1 > shipIndex[shipNo].lap) {
                            lapNumber = shipIndex[shipNo].lap;
                            if (lapNumber == 0)
                                lapNumber = 1;
                            shipIndex[shipNo].lapTimes[lapNumber - 1] += 2;
                        }
                    }
                }
                shipClassIndex = gameData->shipClass;
                clipLevel = gameData->trackNum;
                CheckShipDestroyed(shipIndex, gameData, &raceContext, localDestroyedTime);
                if ((shipIndex[ownShip].attr & RACING) && !networkGame && (keysPressed & PadTriangle0)) {
                    PlayNote(8, 255, 15, 15, 256);
                    pauseRequested ^= 1;
                }
                CheckGamePause(shipIndex, gameData);
                PerformPauseOperations(&raceMenu, gameData);
                if (pause.selection == -1) {
                    CheckForButtonPress();
                    UpdateShips(shipIndex, weaponBars, shipShapeTable, weaponIndex, 0);
                }
                rescueData.droidUpdate(&rescueData, &shipIndex[ownShip], shipShapeTable);
                CameraUpdate(&residentSkeleton, &shipIndex[ownShip], &rescueData);
                if (shipIndex[ownShip].attr & RACING) {
                    if (shipIndex[ownShip].weapon == 2 || shipIndex[ownShip].weapon == 5) {
                        MissileLock(ownShip, shipIndex);
                        if (shipIndex[ownShip].weapon == 2)
                            RearMissileLock(ownShip, shipIndex);
                    }
                    ShakeScreen();
                }
                UpdateWeapons(shipIndex, weaponBars, weaponIndex);
                CheckPitShips();
                if (pause.selection == -1) {
                    AdjustEngineSound(&shipIndex[ownShip], &residentSkeleton);
                    AdjustCrowdSound(cdState, localCrowdCount);
                }
                TrackSpecificFunctions();
                ColourDroidLights(lightDroid);
                AddEffects(weaponIndex, shipIndex);
                raceElapsed = 0 - raceStartVcount;
                if (gameType != 1 && gameData->weaponsOnSigned)
                    CycleWeaponGrids(shipIndex, weaponGrids, currentNoWeaponGrids);
                else if (!gameData->weaponsOnSigned) {
                    /* The empty arm preserves the original /Od branches. */
                    if (gameType != 2) {
                    } else {
                        CycleWeaponGrids(shipIndex, weaponGrids, currentNoWeaponGrids);
                    }
                }
                UpdateShipTrails(shipIndex, attractDelayFrames);
                if (shipDeathCount[ownShip] == 100)
                    shipIndex[ownShip].attr &= ~VIEW_IN;
                if ((shipDeathCount[ownShip] & 7) < 4 && shipDeathCount[ownShip] > 15 && !(shipIndex[ownShip].combatAttr & 4)) {
                    dangerFlag = 1;
                    if (!(shipDeathCount[ownShip] & 7))
                        PlayNote(SAMPLE_WARNING, 255, 15, 15, 256);
                }
                for (shipCounter = 0; shipCounter < 15; shipCounter++) {
                    if (!(shipIndex[shipCounter].combatAttr & DESTROYED) && shipDeathCount[shipCounter] > 0) {
                        if (shipCounter == ownShip && shipDeathCount[ownShip] > 15 && !networkGame) {
                            shipIndex[shipCounter].ppivot.vx += shipIndex[shipCounter].vpivot.vx >> 6;
                            shipIndex[shipCounter].ppivot.vy += shipIndex[shipCounter].vpivot.vy >> 6;
                            shipIndex[shipCounter].ppivot.vz += shipIndex[shipCounter].vpivot.vz >> 6;
                            shipIndex[shipCounter].vpivot.vx -= shipIndex[shipCounter].vpivot.vx >> 3;
                            shipIndex[shipCounter].vpivot.vy -= shipIndex[shipCounter].vpivot.vy >> 3;
                            shipIndex[shipCounter].vpivot.vz -= shipIndex[shipCounter].vpivot.vz >> 3;
                            shipIndex[shipCounter].vhdg = 0;
                            shipIndex[shipCounter].roll += (short)((shipIndex[shipCounter].vpivot.vz + shipIndex[shipCounter].vpivot.vx) >> 5);
                        }
                        if ((shipDeathCount[shipCounter] & 3) == 1 && shipDeathCount[shipCounter] < 15) {
                            savedRadius = (15 - shipDeathCount[shipCounter]) * 64;
                            AddExplode(shipIndex[shipCounter].ppivot.vx + newRand() % savedRadius - (savedRadius >> 1),
                                       shipIndex[shipCounter].ppivot.vy - newRand() % 127,
                                       shipIndex[shipCounter].ppivot.vz + newRand() % savedRadius - (savedRadius >> 1), -1);
                            AddSmokeExplode(shipIndex[shipCounter].ppivot.vx + newRand() % savedRadius - (savedRadius >> 1),
                                            shipIndex[shipCounter].ppivot.vy - newRand() % 127,
                                            shipIndex[shipCounter].ppivot.vz + newRand() % savedRadius - (savedRadius >> 1), -1);
                            savedRadius = (20 - shipDeathCount[shipCounter]) * 64;
                            AddSpurtExplode(shipIndex[shipCounter].ppivot.vx + newRand() % savedRadius - (savedRadius >> 1),
                                            shipIndex[shipCounter].ppivot.vy - newRand() % 127,
                                            shipIndex[shipCounter].ppivot.vz + newRand() % savedRadius - (savedRadius >> 1), -1);
                        }
                        if (shipDeathCount[shipCounter] == 14) {
                            PlayDirectionalNote(5, 0, 40, shipIndex[shipCounter].ppivot);
                            PlayDirectionalNote(5, 0, 40, shipIndex[shipCounter].ppivot);
                            PlayDirectionalNote(5, -64, 40, shipIndex[shipCounter].ppivot);
                            PlayDirectionalNote(5, -128, 40, shipIndex[shipCounter].ppivot);
                            PlayDirectionalNote(5, -128, 40, shipIndex[shipCounter].ppivot);
                            PlayDirectionalNote(5, -192, 40, shipIndex[shipCounter].ppivot);
                        }
                        if ((newRand() & 3) == 1 && shipDeathCount[shipCounter] > 15)
                            AddSmallSmokeExplode(shipIndex[shipCounter].ppivot.vx - (shipIndex[shipCounter].unitVecNose.vx >> 4),
                                                 shipIndex[shipCounter].ppivot.vy - (shipIndex[shipCounter].unitVecNose.vy >> 4),
                                                 shipIndex[shipCounter].ppivot.vz - (shipIndex[shipCounter].unitVecNose.vz >> 4), -1);
                        if ((shipDeathCount[shipCounter] & 3) == 1 && shipDeathCount[shipCounter] > 15)
                            AddSmallSpurtExplode(-(shipIndex[shipCounter].unitVecNose.vx >> 5) + (newRand() & 511) - 256,
                                                 -(shipIndex[shipCounter].unitVecNose.vy >> 5) + 128,
                                                 -(shipIndex[shipCounter].unitVecNose.vz >> 5) + (newRand() & 511) - 256, shipCounter);
                        shipDeathCount[shipCounter]--;
                        if (shipDeathCount[shipCounter] == 0) {
                            if (ownShip != shipCounter) {
                                shipIndex[shipCounter].attr &= ~RACING;
                                if (networkGame)
                                    shipIndex[shipCounter].combatAttr |= DESTROYED;
                                shipLostFlag = 1;
                                gstDebugPrintf("shipNo %d: destroyed, contenderElim %d", shipCounter, shipLostFlag);
                            } else {
                                if (networkGame) {
                                    shipIndex[shipCounter].attr &= ~RACING;
                                    shipIndex[shipCounter].combatAttr |= DESTROYED;
                                    playerDestroyed = 1;
                                    gstDebugPrintf("ownShip %d: destroyed");
                                } else {
                                    shipIndex[shipCounter].combatAttr |= DESTROYED;
                                }
                            }
                        }
                    }
                    if (shipCounter == ownShip && (shipIndex[ownShip].combatAttr & DESTROYED)) {
                        if ((newRand() & 31) < 3)
                            AddSmallSmokeExplode(shipIndex[shipCounter].ppivot.vx, shipIndex[shipCounter].nearTrkSect->centre.vy + 128,
                                                 shipIndex[shipCounter].ppivot.vz, -1);
                    }
                }
                if (gameData->actTrackNum == 2 || gameData->actTrackNum == 7 || cheatRain) {
                    localStormCycle = (localStormCycle + 1) & 255;
                    AddRain(particleSlots, 1, &cameraPtr->camPos->relative.t[0]);
                    if (!undercover && !(newRand() & 3) && localStormCycle < 32 && !(localStormCycle & 3)) {
                        if (fadeAmount < 228)
                            fadeAmount = 228;
                        thunderStartCycle = localStormCycle;
                    }
                    if (fadeAmount == 220)
                        fadeAmount = 118;
                    if (fadeAmount == 110)
                        fadeAmount = 0;
                    if (thunderStartCycle + 32 == localStormCycle)
                        PlayNote(SAMPLE_THUNDER, 380, 15, 15, 128);
                    if (thunderStartCycle + 32 < localStormCycle && localStormCycle < 64 && !(newRand() & 3) && !(localStormCycle & 3))
                        PlayNote(SAMPLE_THUNDER, (short)(380 - (localStormCycle - 32)), 15, 15, 256 + (32 - (localStormCycle - 32)));
                }
                if (gameData->actTrackNum == 8)
                    AddSnow(particleSlots, 20, &cameraPtr->camPos->relative.t[0]);
                prmObjects[initialSkyNumber]->skeleton->update = 1;
                savedUpperY = residentSkeleton.camPos->super->relative.t[1];
                if (gameData->actTrackNum == 17) {
                    if (residentSkeleton.camPos->relative.t[1] > 0) {
                        /* The original keeps this jump over an empty alternative. */
                        if (NETPLAY) {
                        } else {
                        }
                        residentSkeleton.camPos->super->relative.t[1] += 1024 + (((residentSkeleton.camPos->relative.t[1] >> 3) * rsin(skycyc)) >> 12);
                    }
                    if (pause.selection == -1 && (residentSkeleton.section->secNo <= 302 || residentSkeleton.section->secNo >= 329))
                        skycyc = (skycyc + 96) & 4095;
                }
                residentSkeleton.camPos->super->relative.t[1] = savedUpperY;
                DrawShips(shipIndex, shipShapeTable, ownShip, residentSkeleton, pause.selection);
                UpdateStartDroid(startDroidObjects, shipIndex, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], startTextureBase, gameData, pause.selection);
                UpdateLightStart(lightGroups, shipIndex, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], startTextureBase, gameData, pause.selection);
                if (gameData->actTrackNum == 2 && shipIndex[ownShip].nearTrkSect->secNo >= 200 && shipIndex[ownShip].nearTrkSect->secNo <= 275) {
                    MoveMonoRail(monoRailObjs, 6, &monoRail, shipIndex, pause.selection);
                    InitTrainLights(prmObjects, prmCount, monoRailObjs, pause.selection);
                }
                if (sillyShips)
                    UpdateSharkModel();
                UpdateParticles(particleSlots, residentSkeleton.camPos, pause.selection + 1);
                UpdateExplode();
                if (gameData->actTrackNum == 13)
                    UpdateBirds();
                UpdateRing();
            }
            if (networkGame) {
                while (!raceFrameReady) { }
            }
            raceFrameReady = 0;
            if (!linkSendFlag) {
                if (raceContext == 1 && attractDelayFrames > 0) {
                    attractDelayFrames--;
                    if (attractDelayFrames == 0) {
                        raceEndState = 1;
                        linkMyState = 2;
                        raceInProgress = 0;
                    } else if (menuKeys != 0 || pauseRequested) {
                        if (!(shipIndex[ownShip].combatAttr & DESTROYED)) {
                            pauseRequested = 0;
                            raceEndState = 1;
                            linkMyState = 3;
                            raceInProgress = 0;
                        }
                    }
                }
            } else if (!networkGame) {
                UpdateRaceComplete(&raceMenu, &currentResetData, gameData, &savedComplete, &initialScoreData, shipIndex, &pause);
            }
            if (raceEndState > 0) {
                raceEndState--;
                raceEndFlag = 1;
                if (raceEndState == 0) {
                    fillEnabled = 1;
                    QuitGame();
                    return 0;
                }
            }
            if ((signed char)initialScoreData.highScore != 1 && raceEndState == 0) {
                UpdateRaceHud(shipIndex, pause.selection, gameData, &initialScoreData);
                if (shipIndex[ownShip].update == UpdatePlayerShip || shipIndex[ownShip].update == UpdateRescueDroid
                    || shipIndex[ownShip].update == UpdateRacingShip
                    || (shipIndex[ownShip].update == UpdateDroneShip && shipIndex[ownShip].autopilotCount > 0)) {
                    if (!(shipIndex[ownShip].combatAttr & 4) || !(shipIndex[ownShip].combatAttr & DESTROYED))
                        AlterCheckPoint(((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], shipIndex, pause.selection, gameData);
                }
            }
            if (gstRestartPending == 2) {
                gstDebugPrintf("%d: restart_pending %d: calling gstEnumLockstepPlayers", gstRestartPending);
                gstEnumLockstepPlayers(gstSessionId);
                if (!gstRestartReason && (signed char)gstRestartShip != -1) {
                    shipIndex[(signed char)gstRestartShip].netActive = 3;
                    gstDebugPrintf("race: STUPID HACK: fucking HUMAN_CONTROLLED_QUIT for ship %d", (signed char)gstRestartShip);
                }
                gstRestartPending = 0;
            }
            updateTime = GetProfileTime() - profileMark;
            profileMark = GetProfileTime();
            if (!firstTimeIn)
                UpdateFrameAndPresent(0);
            setupTime = GetProfileTime() - profileMark;
            profileMark = GetProfileTime();
            resetMiscFx();
            PlayCredits(raceContext);
            DrawPauseMenu(&raceMenu, gameData);
            if (gameData->actTrackNum == 2 && shipIndex[ownShip].nearTrkSect->secNo >= 200 && shipIndex[ownShip].nearTrkSect->secNo <= 275)
                DrawMonoRail(monoRailObjs, 6, &monoRail, shipIndex, pause.selection);
            if (!(shipIndex[ownShip].attr & RACING) || shipDeathCount[ownShip] > -1) {
                DrawAllTrack(&residentSkeleton);
            } else {
                if (!(shipIndex[ownShip].attr & DIRECTION) && gameData->trackNum != 1 && gameData->trackNum != 6)
                    DrawAllDirections(&residentSkeleton);
                else
                    DrawNewTrack(&residentSkeleton);
            }
            DrawShadow(shipShapeTable, shipIndex);
            AddShipRenderObjects(shipIndex, shipShapeTable, ownShip, residentSkeleton, pause.selection);
            DrawWeapons(shipIndex, weaponBars, weaponIndex);
            if (rescueData.droidUpdate == UpdateDroidStart || rescueData.droidUpdate == UpdateDroidCircle || rescueData.droidUpdate == UpdateDroidRescue)
                DrawRescueDroid(&rescueData, shipShapeTable);
            DrawEffects(weaponIndex, shipIndex);
            if (hudEnabled) {
                /* The original keeps this empty arm of the HUD condition. */
                if (!gameData->weaponsOnSigned && gameType != 1 && gameType != 2) {
                } else {
                    if ((shipIndex[ownShip].attr & RACING) && ownShip < 12)
                        DrawWeaponIcons(&shipIndex[ownShip]);
                }
            }
            if (linkSendFlag)
                RaceComplete(&raceMenu, &currentResetData, gameData, &savedComplete, &initialScoreData, shipIndex, &pause);
            if (hudEnabled && (signed char)initialScoreData.highScore != 1 && raceEndState == 0)
                DrawRaceHud(shipIndex, pause.selection, gameData, &initialScoreData);
            DrawSectionObjects(&shipIndex[ownShip], &residentSkeleton);
            if (gameData->skyEnabledSigned && normalSetNo != 16)
                AddRenderObject(prmObjects[initialSkyNumber], 0, 0);
            if (prevCameraUpdate != CameraUpdate) {
                UnsetParticles(particleSlots);
                prevCameraUpdate = CameraUpdate;
            }
            if (sillyShips && pause.selection == -1)
                UpdateSharkFeet();
            AsmDrawParticles(particleSlots, residentSkeleton.camPos, pause.selection + 1);
            DrawExplode();
            if (gameData->actTrackNum == 13)
                DrawBirds();
            DrawRing();
            resetMiscFx();
            DrawShipTrails(cameraPtr);
            if (dangerFlag) {
                CentreText("DANGER", 15, 64, 8, 0);
                CentreText("CRITICAL SHIELD FAILURE", 45, 128, 8, 1);
            }
            if (extendedTimeFlag)
                CentreText("EXTENDED TIME", 230, 75, 8, 1);
            if (killMessageFlash) {
                char killText[100];
                sprintf(killText, "%s DESTROYED", shipIndex[lastKiller].netName);
                CentreText(killText, 0, 140, 8, 1);
                if (lastVictim == ownShip)
                    sprintf(killText, "YOU");
                else
                    sprintf(killText, "%s", shipIndex[lastVictim].netName);
                CentreText(killText, 0, 155, 8, 1);
            }
            if (shipLostFlag) {
                PlayNote(SAMPLE_CONTINUE_ELIMINATED, 480, 15, 15, 256);
                gstDebugPrintf("contenderElim %d: playing voice", shipLostFlag);
            }
            DrawStartDroid(startDroidObjects, shipIndex, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], startTextureBase, gameData, pause.selection);
            DrawLightStart(lightGroups, shipIndex, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], startTextureBase, gameData, pause.selection);
            cardNum.accumulator -= FR60 * 30;
            menuHeading.accumulator -= FR60 * 15;
            drawTime = GetProfileTime() - profileMark;
            profileMark = GetProfileTime();
            CallbackSwap();
            flipTime = GetProfileTime() - profileMark;
            profileMark = GetProfileTime();
            firstTimeIn = 0;
            UpdateNotes(0);
            if ((signed char)linkRaceDone) {
                raceContext = 0;
                break;
            }
            if (networkGame) {
                int destroyedCount;
                int resultShipIndex;
                int finishedCount;
                int numRacing;
                finishedCount = 0;
                numRacing = 0;
                destroyedCount = 0;
                for (resultShipIndex = 0; resultShipIndex < 15; resultShipIndex++) {
                    if (shipIndex[resultShipIndex].netActive & 1) {
                        numRacing++;
                        if (classLaps[gameData->shipClass] < shipIndex[resultShipIndex].lap) {
                            if (shipIndex[resultShipIndex].netPlace == -1) {
                                int position;
                                int rankIndex;
                                position = 1;
                                for (rankIndex = 0; rankIndex < 15; rankIndex++) {
                                    if (shipIndex[rankIndex].netPlace != -1)
                                        position++;
                                }
                                shipIndex[resultShipIndex].netPlace = position;
                                if (position == 1) {
                                    playerPoints[resultShipIndex] += 10;
                                    if (gstDebugOutputEnabled)
                                        printf("1st Place: PlayerPoints[ %d ] == %d\n", resultShipIndex, playerPoints[resultShipIndex]);
                                } else if (position == 2) {
                                    playerPoints[resultShipIndex] += 5;
                                    if (gstDebugOutputEnabled)
                                        printf("2nd Place: PlayerPoints[ %d ] == %d\n", resultShipIndex, playerPoints[resultShipIndex]);
                                } else if (position == 3) {
                                    playerPoints[resultShipIndex] += 3;
                                    if (gstDebugOutputEnabled)
                                        printf("3rd PlacePlayerPoints[ %d ] == %d\n", resultShipIndex, playerPoints[resultShipIndex]);
                                }
                                gstDebugPrintf("Player Finished: shipNo %d, finish_pos %d", resultShipIndex, shipIndex[resultShipIndex].netPlace);
                            }
                            if (ownShip == resultShipIndex && !(signed char)playerFinishedRace) {
                                playerFinishedRace = 1;
                                shipIndex[resultShipIndex].attr &= ~RACING;
                                deathCameraActive = 1;
                                gstDebugPrintf("ownShip %d: FinishedRace %d, dispMultiScores %d", resultShipIndex, (signed char)playerFinishedRace, deathCameraActive);
                            }
                            finishedCount++;
                        } else {
                            if (shipIndex[resultShipIndex].combatAttr & DESTROYED)
                                destroyedCount++;
                        }
                    }
                }
                if (finishedCount >= numRacing) {
                    /* The original leaves this completion branch empty. */
                }
            }
            frameTime = GetProfileTime() - frameStart;
        }
    }
    return 0;
}
#pragma function(strcpy, strcat, memcmp, sin)
#endif
#endif

/* Creates the resident skeletons, ship data, weapons and textures that stay loaded for the whole game. */
#ifdef NETPLAY
/* The network build indexes the check cells by track and class. */
#pragma intrinsic(strcpy, strcat)
#endif
void InitResident(void)
{
#ifndef NETPLAY
    ShipData *shipData;
    TIMlist *textureSequence;
#endif

    residentSkeleton.camPos = NewSkeleton();
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    upperSkeleton = NewSkeleton();
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    residentSkeleton.camPos->super = upperSkeleton;
    startTextureBase = TextureTableCount;
    strcpy(startPath, wobbleDir3);
    strcat(startPath, "cdroid.cmp");
#ifdef NETPLAY
    residentTextureSeq = LoadCompressedTextureSequence(startPath);
    LoadCmpFiles(residentTextureSeq);
    shipIndex = DAlloc(heap, 0x3E1C);
    loadShipData(0, shipIndex, shipShapeTable, residentSkeleton.camPos);
#else
    textureSequence = LoadCompressedTextureSequence(startPath);
    residentTextureSeq = textureSequence;
    LoadCmpFiles(textureSequence);
    shipData = DAlloc(heap, 0xFF0);
    shipIndex = shipData;
    loadShipData(0, shipData, shipShapeTable, residentSkeleton.camPos);
#endif
    weaponGrids = DAlloc(heap, 0x420);
    weaponBars = DAlloc(heap, 0x108);
    weaponIndex = DAlloc(heap, 0x610);
    initWeapons(weaponBars, residentSkeleton.camPos, weaponIndex);
    LoadExplodeGFX();
#ifdef NETPLAY
    LoadStartDroid(startDroidObjects, "sroid", residentSkeleton.camPos, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], gameData);
    LoadStartDroid(lightGroups, "light", residentSkeleton.camPos, ((CheckCellClasses *)checkCells)[gameData->trackNum][gameData->shipClass], gameData);
#else
    LoadStartDroid(startDroidObjects, "sroid", residentSkeleton.camPos, checkCells[gameData->trackNum * 4 + gameData->shipClass], gameData);
    LoadStartDroid(lightGroups, "light", residentSkeleton.camPos, checkCells[gameData->trackNum * 4 + gameData->shipClass], gameData);
#endif
}
#ifdef NETPLAY
#pragma function(strcpy, strcat)
#endif

/* Releases the race heap and restores the shared game heap. */
void racetidy(void)
{
    heap = mainheapcopy;
    InitDynamicMem(reserveHeap, 0x1C2000);
    TextureTableCount = savedTextureCount;
}

/* Runs the setup and animation routines for the active track. */
#ifdef NETPLAY
/* The network build tests the track number in the config directly. */
void TrackSpecificFunctions(void)
{
    if (gameData->actTrackNum == 1) {
        RedBullAnimate(redBull, redBullCount, 0x14, redBullNumber, 2);
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveFan(fans, fanCount);
        AnimateTexture(smokeSlowObjects, smokeSlowCount, 1, smokeNumber, 25, smokeColour, &fastCount, &fastTexCount, 1);
        AnimateTexture(smokeFastObjects, smokeFastCount, 1, smokeNumber, 25, smokeColour, &slowCount, &slowTexCount, 2);
    } else if (gameData->actTrackNum == 8) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveAdvert(advert, advertCount);
    } else if (gameData->actTrackNum == 6) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveAdvert(advert, advertCount);
    } else if (gameData->actTrackNum == 20) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveRadar(radar, dishCount, shipIndex);
        DoNeonSign(neonObject, 3);
    } else if (gameData->actTrackNum == 7) {
        MoveAdvert(advert, advertCount);
    } else if (gameData->actTrackNum == 17) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
    } else if (gameData->actTrackNum == 13) {
        WaterFall();
    } else if (gameData->actTrackNum == 2) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveZeppelin(zeppelinObj, zepCount, shipIndex);
        beamCollide = 0;
    }
}
#else
void TrackSpecificFunctions(void)
{
    u_char trackNumber;

    trackNumber = gameData->actTrackNum;
    if (trackNumber == 1) {
        RedBullAnimate(redBull, redBullCount, 0x14, redBullNumber, 2);
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveFan(fans, fanCount);
        AnimateTexture(smokeSlowObjects, smokeSlowCount, 1, smokeNumber, 25, smokeColour, &fastCount, &fastTexCount, 1);
        AnimateTexture(smokeFastObjects, smokeFastCount, 1, smokeNumber, 25, smokeColour, &slowCount, &slowTexCount, 2);
    } else if (trackNumber == 8) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveAdvert(advert, advertCount);
    } else if (trackNumber == 6) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveAdvert(advert, advertCount);
    } else if (trackNumber == 20) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveRadar(radar, dishCount, shipIndex);
        DoNeonSign(neonObject, 3);
    } else if (trackNumber == 7) {
        MoveAdvert(advert, advertCount);
    } else if (trackNumber == 17) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
    } else if (trackNumber == 13) {
        WaterFall();
    } else if (trackNumber == 2) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveZeppelin(zeppelinObj, zepCount, shipIndex);
        beamCollide = 0;
    }
}
#endif

/* Writes the available heap sizes to the debug log. */
void LogMemoryStatus(void)
{
    MEMORYSTATUS status;
    char text[64];

    status.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&status);
    wsprintf(text, "-Loop number-  %ld\n", loopNumber);
    OutputDebugString(text);
    wsprintf(text, "Mem load       %ld\n", status.dwMemoryLoad);
    OutputDebugString(text);
    wsprintf(text, "Total phys     %ld\n", status.dwTotalPhys);
    OutputDebugString(text);
    wsprintf(text, "Avail phys     %ld\n", status.dwAvailPhys);
    OutputDebugString(text);
    wsprintf(text, "Total pagefile %ld\n", status.dwTotalPageFile);
    OutputDebugString(text);
    wsprintf(text, "Avail pagefile %ld\n", status.dwAvailPageFile);
    OutputDebugString(text);
    wsprintf(text, "Total virtual  %ld\n", status.dwTotalVirtual);
    OutputDebugString(text);
    wsprintf(text, "Avail virtual  %ld\n", status.dwAvailVirtual);
    OutputDebugString(text);
    loopNumber++;
}

/* Finishes rendering and timing for the current race frame. */
void RaceFrameEnd(void)
{
    FlushMessages();
    VsyncFunc();
    RaceFrameStart();
}

extern float textDepth;

void DrawScaledTexturedQuad(float depth, POLY_FT4 *poly, int flags);

/* Draws the two message boxes at the top of the race screen, one at each side. */
void FlushMessages(void)
{
    POLY_FT4 poly;

    SetTextQuad(&poly, 25, 38, 0, 0, 0xFE, 0x40, 0x23, 0);
    DrawScaledTexturedQuad(textDepth, &poly, hudTexturePage);
    SetTextQuad(&poly, 151, 38, 0, 64, 0xD2, 0x40, 0x23, 0);
    DrawScaledTexturedQuad(textDepth, &poly, hudTexturePage);
}

#ifdef NETPLAY
extern int gstEventHead;

/* The nested loop computes the same queue clears with different index arithmetic. */
#endif
#ifdef EQUIV
#ifdef NETPLAY
void gstEventInitQueue(void)
{
    int i;
    int j;
    int k;

    gstEventHead = 0;
    gstEventTail = -1;
    gstEventCount = 0;
    for (i = 0; i < GST_MAX_QUEUE_LEN; i++) {
        gstEventQueue[i].kind = 0;
        for (j = 0; j < 20; j++) {
            for (k = 0; k < 6; k++) {
                gstEventQueue[i].packet.player[j][k] = 0;
            }
        }
    }
}
#endif
#endif
#ifdef NETPLAY

int gstEventGetCount(void)
{
    return gstEventCount;
}

int gstEventEnqueue(GstPacket *packet, unsigned char kind)
{
    if (gstEventCount >= GST_MAX_QUEUE_LEN) {
        gstDebugPrintf("gstEventEnqueue: EventCount %d, MAX_QUEUE_LEN %d, Queue Full!", gstEventCount, GST_MAX_QUEUE_LEN);
        return 0;
    } else {
        gstEventCount++;
        gstEventTail = (gstEventTail + 1) % GST_MAX_QUEUE_LEN;
        gstEventQueue[gstEventTail].packet = *packet;
        gstEventQueue[gstEventTail].kind = kind;
    }
    return 1;
}

/* The empty and kind-mismatch exits take different branch paths. */
#endif
#ifdef NETPLAY
GstEvent *gstEventDequeue(int kind)
{
    int index = 0;

#ifdef NETPLAY
    if (gstEventCount <= 0) {
        return NULL;
    } else {
        index = gstEventHead;
        if (kind != -1) {
            if (gstEventQueue[index].kind != kind) {
                return NULL;
            }
        }
        gstEventCount--;
        gstEventHead = (gstEventHead + 1) % GST_MAX_QUEUE_LEN;
    }
    return &gstEventQueue[index];
#else
    if (gstEventCount <= 0) {
        return NULL;
    }
    index = gstEventHead;
    if (kind != -1) {
        if (gstEventQueue[index].kind != kind) {
            return NULL;
        }
    }
    gstEventCount--;
    gstEventHead = (gstEventHead + 1) % GST_MAX_QUEUE_LEN;
    return &gstEventQueue[index];
#endif
}
#endif
#ifdef NETPLAY


#pragma intrinsic(strcat)
/* On-screen table of whether each ship's position agrees with the other machines'
   (shipNo -1 draws the empty table). */
void gstDumpSync(char shipNo)
{
    char allSame;
    char reasons[100];
    int index;
    char report[100];

    if (shipNo == -1) {
        gstDebugWriteText(60, 4, "----+-------+");
        for (index = 0; index < 15; index++) {
            sprintf(report, "%2d: |       |", index);
            gstDebugWriteText(60, index + 5, report);
        }
    } else {
        allSame = 1;
        reasons[0] = 0;
        if (gstSyncRemote[shipNo].x != gstSyncLocal[shipNo].x) {
            allSame = 0;
            strcat(reasons, "X ");
        }
        if (gstSyncRemote[shipNo].y != gstSyncLocal[shipNo].y) {
            allSame = 0;
            strcat(reasons, "Y ");
        }
        if (gstSyncRemote[shipNo].z != gstSyncLocal[shipNo].z) {
            allSame = 0;
            strcat(reasons, "Z ");
        }
        if (!allSame) {
            sprintf(report, "%2d: | FALSE | ", shipNo);
            strcat(report, reasons);
            if (gstDebugOutputEnabled) {
                printf("GAME OUT OF SYNCH: LockstepNum %d, ownShip %d, shipNo %d\n", localRandomSeed, ownShip, shipNo);
            }
            fflush(stdout);
            sprintf(gstSyncMessage[shipNo], "%s OUT OF SYNCH", shipIndex[shipNo].netName);
        } else {
            sprintf(report, "%2d: | TRUE  |       ", shipNo);
            sprintf(gstSyncMessage[shipNo], "%s IN SYNCH", shipIndex[shipNo].netName);
        }
        gstDebugWriteText(60, shipNo + 5, report);
    }
    gstDebugWriteText(60, 19, "----+-------+");
}
#pragma function(strcat)


/* Packet 'A': the server's answer to the player-creation request; type 6 carries this player's id. */
void HandleMakePlayer(GstCodec *packet)
{
    unsigned char cmd;
    unsigned long gstid;

    gstDecodeU8(packet, &cmd);
    gstDecodeU8(packet, &cmd);
    gstDecodePackedU24(packet, &gstid);
    switch (cmd) {
    case 6:
        gstPlayerId = gstid;
        break;
    default:
        gstDebugPrintf("HandleMakePlayer: wtf? unkown type %d %d", cmd, gstid);
        break;
    }
}

/* Packet 'D': the server's answer to the join request; type 3 carries the session id. */
void HandleJoinPkt(GstCodec *packet)
{
    unsigned char cmd;
    unsigned long gstid;

    gstDecodeU8(packet, &cmd);
    gstDecodeU8(packet, &cmd);
    gstDecodePackedU24(packet, &gstid);
    switch (cmd) {
    case 3:
        gstSessionId = gstid;
        gstDebugPrintf("HandleJoinPkt: gstidSession %d, cheese is good dope", gstSessionId);
        break;
    default:
        gstDebugPrintf("HandleJoinPkt: unknown type %d gstid %d", cmd, gstid);
        break;
    }
}

extern unsigned char racePlayerCount;
extern int incomingLockstepEventCount;
extern unsigned char raceControlError;
extern unsigned char raceControlTimerStarted;
extern unsigned char raceControlTimerBusy;
void gstDebugWriteText(int column, int row, const char *text);
extern __declspec(dllimport) unsigned long __stdcall GetTickCount(void);

/* 'S' packet: an asynchronous message from another player; the first byte of the payload says
   which kind (0 flip view, 1 position check, 2 weapon, 3 restart, 4 next race, 5 check fail). */
void gstHandleAsynch(GstCodec *packet)
{
    int pos;
    GstShipOrderMsg ordering;
    unsigned char limit;
    unsigned char inIdx;
    unsigned long netId;
    int shipNo = 0;
    unsigned char type;
    int slot;
    union {
        unsigned char bytes[1000];
        GstShipSync sync;
        NetAsynch asynch;
        GstShipOrderMsg order;
    } data;
    unsigned long size;

    gstDecodeU8(packet, &type);
    gstDecodePackedU24(packet, &netId);
    gstDecodePackedU32(packet, &size);
    gstDecodeBytes(packet, size, &data);
    switch (data.bytes[0]) {
    case 0:
        flipView = 1;
        break;
    case 1:
        for (slot = 0; slot < 15; slot++) {
            if (shipIndex[slot].netId == netId) {
                shipNo = slot;
                break;
            }
        }
        if (slot == 15) {
            gstDebugPrintf("HandleAsynchPkt: Synch Pkt: couldn't find index for gstid %d", netId);
            break;
        }
        gstSyncRemote[shipNo] = data.sync;
        gstDumpSync(shipNo);
        break;
    case 2:
        for (slot = 0; slot < 15; slot++) {
            if (shipIndex[slot].netId == netId) {
                shipNo = slot;
                break;
            }
        }
        if (slot == 15) {
            gstDebugPrintf("HandleAsynchPkt: Asynch Weapon: couldn't find index for gstid %d", netId);
            break;
        }
        inIdx = shipIndex[shipNo].asynchIn;
        if (gstDebugOutputEnabled)
            printf("HandleAsynchPkt: shipNo %d, asynchIn %d, asynchOut %d\n", shipNo, shipIndex[shipNo].asynchIn, shipIndex[shipNo].asynchOut);
        limit = (shipIndex[shipNo].asynchIn + 1) & 0x1f;
        if (shipIndex[shipNo].asynchOut == limit) {
            gstDebugPrintf("HandleAsynchPkt: Asynch Over-fucking-flow!! asynchIn %d, asynchOut %d", shipIndex[shipNo].asynchIn, shipIndex[shipNo].asynchOut);
            raceControlError = 1;
            break;
        }
        shipIndex[shipNo].asynchQueue[inIdx] = data.asynch;
        shipIndex[shipNo].asynchTime[inIdx] = incomingLockstepEventCount + 1;
        shipIndex[shipNo].asynchIn = limit;
        if (gstDebugOutputEnabled)
            printf("HandleAsynchPkt: shipNo %d, asynchIn %d, asynchOut %d LockstepNum %d\n", shipNo, shipIndex[shipNo].asynchIn, shipIndex[shipNo].asynchOut, incomingLockstepEventCount + 1);
        break;
    case 3:
        for (slot = 0; slot < 15; slot++) {
            if (shipIndex[slot].netId == netId) {
                shipNo = slot;
                break;
            }
        }
        if (slot == 15) {
            gstDebugPrintf("HandleAsynchPkt: Asynch Restart: couldn't find index for gstid %d", netId);
            break;
        }
        gstRestartHack[shipNo].set = 1;
        gstRestartHack[shipNo].when = incomingLockstepEventCount + 1;
        gstDebugPrintf("HandleAsynchPkt: RestartHack: player_index %d", shipNo);
        break;
    case 4:
        raceControlTimerStarted = 0;
        ordering = data.order;
        for (pos = 0; pos < 15; pos++)
            gstGlobalShipOrder[pos] = ordering.order[pos];
        if (gstRaceMode == 3) {
            raceControlError = 2;
            gstDebugPrintf("HandleAsynchPkt: Got NEXTRACE Asynch: quit_flag set to %d", raceControlError);
            if (gstDebugOutputEnabled) {
                printf("\nHost Ship Ordering for Next Race:\n");
                for (pos = 0; pos < 15; pos++)
                    printf("GlobalShipOrder[ %2d ] == %2d\n", pos, gstGlobalShipOrder[pos]);
                printf("\n");
            }
        } else {
            raceControlError = 1;
            gstDebugPrintf("HandleAsynchPkt: Got NEXTRACE Asynch: quit_flag set to %d", raceControlError);
        }
        break;
    case 5:
        for (slot = 0; slot < 15; slot++) {
            if (shipIndex[slot].netId == netId) {
                shipNo = slot;
                break;
            }
        }
        if (slot == 15) {
            gstDebugPrintf("HandleAsynchPkt: CheckFail: couldn't find index for gstid %d", netId);
            break;
        }
        gstCheckFail[slot].set = 1;
        gstCheckFail[slot].when = incomingLockstepEventCount + 1;
        break;
    default:
        break;
    }
}

#endif
#ifdef NETPLAY
/* 'F' packet: a player quit or joined, the lockstep game restarts with the new player list. */
void gstHandleRestart(GstCodec *packet)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    unsigned long id;
    unsigned char reasonCode;
    int i;
    char description[100];
    char nameBuffer[100];
    unsigned long playerNameLen;
    unsigned long descriptionLen;

    gstRestartSeen = 1;
    gstDecodeU8(packet, 0);
    gstDecodeU8(packet, 0);
    gstDecodePackedU24(packet, 0);
    gstDecodeU8(packet, &reasonCode);
    gstDecodePackedU24(packet, &id);
    gstRestartReason = reasonCode;
    for (i = 0; i < 15; i++) {
        if (shipIndex[i].netId == id) {
            gstRestartShip = i;
            break;
        }
    }
    if (i == 15) {
        gstDebugPrintf("HandleRestartPkt: couldn't find index for source of restart");
        gstRestartShip = 0xff;
    }
    switch (reasonCode) {
    case 0:
        gstRestartPending = 1;
        gstOldLockstepChannel = gstLockstepChannel;
        gstDebugPrintf("HandleRestartPkt: Player Quit: restart_pending %d, oldLockstepChannel %d", gstRestartPending, gstOldLockstepChannel);
        break;
    case 1:
        gstDebugPrintf("HandleRestartPkt: Player Joined");
        break;
    default:
        gstDebugPrintf("HandleRestartPkt: unknown reason_code %d", reasonCode);
        break;
    }
    gstDebugPrintf("HandleRestartPkt: reason_code %d, player_id %d", reasonCode, id);
    gstDecodePackedU32(packet, &playerNameLen);
    gstDecodeBytes(packet, playerNameLen, nameBuffer);
    gstDecodePackedU32(packet, &descriptionLen);
    gstDecodeBytes(packet, descriptionLen, description);
    gstDecodeU8(packet, 0);
    gstDecodeU8(packet, &gstLockstepChannel);
    gstDecodeU8(packet, &racePlayerCount);
    gstDebugPrintf("HandleRestartPkt: LockstepChannel %d, LockstepNumPlayers %d", gstLockstepChannel, racePlayerCount);
    for (i = 0; i < 15; i++)
        gstidLockstepPlayers[i] = 0;
    for (i = 0; racePlayerCount > i; i++) {
        gstDecodePackedU24(packet, &gstidLockstepPlayers[i]);
        gstDebugPrintf("HandleRestartPkt: gstidLockstepPlayers[%2d] %d", i, gstidLockstepPlayers[i]);
    }
}
#endif
#ifdef NETPLAY

/* Packet 'j': one frame of lockstep input for every player; queues it as an event. */
void HandleEventPkt(GstCodec *packet)
{
    char text[100];
    int detail;
    int packetType;
    int player;
    GstPacket eventData;

    sprintf(text, "HandleEventPkt: Got Pkt, %d ", GetTickCount());
    gstDebugWriteText(0, 2, text);
    if (raceControlTimerBusy) {
        raceControlTimerStarted = 1;
        raceControlTimerBusy = 0;
    }
    for (player = 0; player < 20; player++) {
        eventData.player[player][0] = 0;
        eventData.player[player][1] = 0;
        eventData.player[player][2] = 0;
        eventData.player[player][3] = 0;
        eventData.player[player][4] = 0;
        eventData.player[player][5] = 0;
    }
    gstDecodeU8(packet, (unsigned char *)&packetType);
    gstDecodeU8(packet, (unsigned char *)&detail);
    for (player = 0; player < racePlayerCount; player++)
        gstDecodeBytes(packet, 6, eventData.player[player]);
    if (!gstEventEnqueue(&eventData, detail)) {
        gstDebugPrintf("HandleEventPkt: error enqueueing lockstep event: end");
        raceControlError = 1;
    }
    incomingLockstepEventCount++;
    return;
}

extern int lockstepEventCount;

/* Packet and field locals use different stack slots, with identical calls and stores. */
#endif
#ifdef NETPLAY
void gstPostLockstepEvent(short type, short value, short player, unsigned char data, unsigned char send)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    int status;
    short fields[3];
    GstCodec *upPacket;

    fields[0] = type;
    fields[1] = value;
    fields[2] = player;
    status = gstAllocUpPacket(&upPacket, 0, 0x19);
    if (status < 1) {
        gstDebugPrintf("gstPostLockstepEvent: gstNewUpPacket: Error: %d [%s]", status, gstErrorString(status));
        raceControlError = 1;
    } else {
        gstEncodeU8(upPacket, 0x69);
        gstEncodeU8(upPacket, data);
        gstEncodeBytes(upPacket, 6, fields);
        if (send) {
            status = gstSendEncodedUpPacket(upPacket, 0);
        } else {
            status = gstSendUpPacketGPI(upPacket, 0);
        }
        if (status < 1) {
            gstDebugPrintf("gstPostLockstepEvent: gstSendUpPacket: Error: %d [%s]", status, gstErrorString(status));
            raceControlError = 1;
        } else {
            lockstepEventCount++;
        }
    }
}
#endif
#ifdef NETPLAY

#pragma intrinsic(strcpy)
/* Asks the server which players are in the lockstep game and records them in the ship table: the player's GST id,
   its ship number, its name in capitals and its description. Returns 1 when the list was read. */
/* The local names are chosen so that the frame has the executable's stack layout. */
int gstEnumLockstepPlayers(unsigned long lockstepId)
{
    short shipScore;
    int newCandidate;
    unsigned char lastDesc[100];
    unsigned char playerName[100];
    unsigned char lastValid;
    unsigned char nextSize;
    unsigned long curPlayerId;
    unsigned char uidSize;
    unsigned short latestGameId;
    int newColumn;
    unsigned char localHeader;
    unsigned char nextState;
    int curIndex;
    unsigned char pendingPacketKind;
    unsigned long currentMode;
    unsigned long nextIdent;
    unsigned char curScoreCount;
    short pendingShipNo;
    unsigned char lastUnused;
    unsigned char currentPlayers;
    int pendingShip;
    GstCodec *outAnswer;
    int currentCode;
    GstCodec *lastQuery;

    for (pendingShip = 0; pendingShip < 15; pendingShip++) {
        if (!(shipIndex[pendingShip].netActive & 3))
            shipIndex[pendingShip].netActive = 0;
        shipIndex[pendingShip].netSlot = 0;
        shipIndex[pendingShip].netId = 0;
    }
    currentCode = gstAllocUpPacket(&lastQuery, 1, 500);
    if (currentCode < 1) {
        gstDebugPrintf("EnumPlayers: gstNewUpPacket %d [%s] quitting..", currentCode, gstErrorString(currentCode));
        return 0;
    }
    gstEncodeU8(lastQuery, 0x42);
    gstEncodeU8(lastQuery, 5);
    gstEncodePackedU24(lastQuery, 0x84);
    gstEncodeU8(lastQuery, 6);
    gstEncodeU16(lastQuery, 0x181);
    gstEncodeU8(lastQuery, 3);
    gstEncodePackedU24(lastQuery, lockstepId);
    gstEncodeU8(lastQuery, 1);
    gstEncodeU8(lastQuery, 1);
    gstEncodeU8(lastQuery, 1);
    gstRegisterPolledPackets(1, 0x42, 0);
    currentCode = gstSendEncodedUpPacket(lastQuery, 1);
    if (currentCode < 1) {
        gstDebugPrintf("EnumPlayers: gstSendUpPacket %d [%s] quitting..", currentCode, gstErrorString(currentCode));
        return 0;
    }
    currentCode = gstGetDownPacket(&outAnswer, 1000);
    gstRegisterPolledPackets(0, 0x42, 0);
    if (currentCode < 1) {
        if (gstDebugOutputEnabled)
            printf("EnumPlayers: gstGetdownPacket: Error GET: %d [%s]", currentCode, gstErrorString(currentCode));
        return 0;
    }
    for (curIndex = 0; curIndex < 15; curIndex++)
        shipIndex[curIndex].combatAttr |= DESTROYED;
    gstDecodeU8(outAnswer, &localHeader);
    gstDecodeU8(outAnswer, &pendingPacketKind);
    gstDecodePackedU24(outAnswer, &nextIdent);
    gstDecodeU8(outAnswer, &nextState);
    gstDecodeU16(outAnswer, &latestGameId);
    gstDecodeU8(outAnswer, &lastUnused);
    gstDecodePackedU24(outAnswer, &currentMode);
    gstDecodeU8(outAnswer, &lastUnused);
    gstDecodeU8(outAnswer, &curScoreCount);
    for (curIndex = 0; curScoreCount > curIndex; curIndex++)
        gstDecodeU8(outAnswer, &lastUnused);
    gstDecodeU8(outAnswer, &currentPlayers);
    for (curIndex = 0; currentPlayers > curIndex; curIndex++) {
        gstDecodePackedU24(outAnswer, &curPlayerId);
        gstDecodeU8(outAnswer, &lastUnused);
        gstDecodeU8(outAnswer, &nextSize);
        gstDecodeBytes(outAnswer, nextSize, playerName);
        playerName[nextSize] = 0;
        for (newColumn = 0; nextSize > newColumn; newColumn++) {
            if (isalpha(playerName[newColumn]) && islower(playerName[newColumn]))
                playerName[newColumn] = toupper(playerName[newColumn]);
        }
        gstDecodeU8(outAnswer, &uidSize);
        gstDecodeBytes(outAnswer, uidSize, lastDesc);
        lastDesc[uidSize] = 0;
        gstDecodeU8(outAnswer, &lastValid);
        if (lastValid == 0) {
            gstDebugPrintf("gstEnumLockstepPlayers: No Scores for gstid %d", curPlayerId);
            continue;
        }
        for (newColumn = 0; curScoreCount > newColumn; newColumn++) {
            gstDecodeS16(outAnswer, &shipScore);
            if (newColumn == 0) {
                pendingShipNo = shipScore;
                for (newCandidate = 0; newCandidate < 15; newCandidate++) {
                    if (gstidLockstepPlayers[newCandidate] == curPlayerId) {
                        shipIndex[pendingShipNo].netSlot = newCandidate;
                        break;
                    }
                }
                if (newCandidate == 15) {
                    gstDebugPrintf("gstEnumLockstepPlayers: couldn't find gstid %d", curPlayerId);
                    continue;
                }
                shipIndex[pendingShipNo].netActive = 1;
                shipIndex[pendingShipNo].combatAttr &= ~DESTROYED;
                shipIndex[pendingShipNo].netId = curPlayerId;
                strcpy(shipIndex[pendingShipNo].netName, playerName);
                strcpy(shipIndex[pendingShipNo].netDescription, lastDesc);
            }
        }
        gstDebugPrintf("gstEnumLockstepPlayers: count %d: gstid %d name [%s] uid [%s] ShipNum %d", curIndex, curPlayerId, playerName, lastDesc, pendingShipNo);
    }
    gstFreeDownPacket(outAnswer);
    return 1;
}
#pragma function(strcpy)

extern int seedWep;
extern u_char netWeaponUnique;
extern u_char netWeaponChance[];
extern u_char playerFinishedRace;
extern int lockstepStatTick;
extern unsigned lockstepStatLast, lockstepStatSamples;
#include "lockstep_stats.h"
extern u_char lockstepPosted[];

/* Resets the state of a network race: event queue, race control flags, the per-ship network fields, the laps for the
   class, and the weapon pickup chances (an equal share of 100 for every weapon the host allowed). */
int InitNetworkRace(void)
{
    unsigned char pct;
    unsigned char defaultLaps[4];
    int i;

    if (gstDebugOutputEnabled) {
        gstDebugPrintf(0);
        printf("\n");
        gstDebugPrintf("********************** InitNetworkRace: ownShip %d ********************************", ownShip);
        printf("\n");
    }
    gstEventInitQueue();
    killMessageFlash = 0;
    killMessageTimer = 0;
    deathCameraActive = 0;
    resultsShown = 0;
    playerDestroyed = 0;
    playerFinishedRace = 0;
    raceControlTimerEnabled = 0;
    raceControlTimerStarted = 0;
    raceControlTimerBusy = 0;
    raceControlError = 0;
    gstRestartPending = 0;
    localRandomSeed = 0;
    incomingLockstepEventCount = 0;
    lockstepEventCount = 0;
    for (i = 0; i < 15; i++) {
        shipIndex[i].net3FA = 0;
        shipIndex[i].net3FC = 0;
        shipIndex[i].net3FE = 0;
        shipIndex[i].netActive = 0;
        shipIndex[i].netSlot = 0;
        shipIndex[i].netF6 = 0;
        shipIndex[i].netF4 = 0;
        shipIndex[i].netF5 = 0;
        shipIndex[i].netF7 = 0;
        shipIndex[i].rearTarget = 0;
        shipIndex[i].netF9 = 0;
        shipIndex[i].netFA = 0;
        shipIndex[i].netFC = 0;
        shipIndex[i].netPlace = -1;
        shipIndex[i].netEd = -1;
        shipIndex[i].netProximity = 0;
        gstRestartHack[i].set = 0;
        gstRestartHack[i].when = 0;
        gstCheckFail[i].set = 0;
        gstCheckFail[i].when = 0;
        if (gstLevelLives)
            livesRemaining[i] = gstLevelLives;
        killerCount[i] = 0;
        lockstepPosted[i] = 0;
        gstSyncMessage[i][0] = 0;
        shipIndex[i].asynchIn = 0;
        shipIndex[i].asynchOut = 0;
    }
    gstSyncActive = 0;
    StartRaceControlTimer();
    defaultLaps[0] = 2;
    defaultLaps[1] = 3;
    defaultLaps[2] = 4;
    defaultLaps[3] = 5;
    if (gstLevelLaps)
        classLaps[gameData->shipClass] = gstLevelLaps;
    else
        classLaps[gameData->shipClass] = defaultLaps[gameData->shipClass];
    pct = 0;
    seedWep = 0x12345678;
    netWeaponUnique = 0;
    for (i = 0; i < 9; i++)
        netWeaponChance[i] = 0;
    for (i = 0; i < 9; i++) {
        if (gstLevelWeapons[i] == 1)
            netWeaponUnique++;
    }
    if (netWeaponUnique)
        pct = 100 / netWeaponUnique;
    for (i = 0; i < 9; i++) {
        if (gstLevelWeapons[i] == 1) {
            netWeaponChance[i] = pct;
            if (netWeaponUnique)
                pct += 100 / netWeaponUnique;
        }
    }
    lockstepStatTick = 0;
    lockstepStatLast = 0;
    lockstepStatSamples = 0;
    lockstepStatDelta = 0;
    lockstepStatAverage = 0;
    return 1;
}

typedef void (__stdcall *RaceTimerCallback)(unsigned, unsigned, unsigned long, unsigned long, unsigned long);
extern __declspec(dllimport) unsigned __stdcall timeSetEvent(unsigned, unsigned, RaceTimerCallback, unsigned long, unsigned);
extern unsigned char raceControlTimerEnabled;
void __stdcall RaceControlTimer(unsigned, unsigned, unsigned long, unsigned long, unsigned long);

int StartRaceControlTimer(void)
{
    unsigned interval;

    interval = 0x21;
    raceControlTimerId = 0;
    raceControlTimerId = timeSetEvent(interval, 0xA, RaceControlTimer, 0, 1);
    raceControlTimerStarted = 0;
    raceControlTimerBusy = 1;
    raceControlTimerEnabled = 1;
    return 1;
}

extern int raceFrameReady;
extern unsigned short D_ab4e14, D_abbf94, D_ab4e1c;
void PollRaceControlInput(void);

/* Posts the finishing order for the next tournament race: the places of the ships that finished, then those still
   racing, then the ones that are not in the race. */
/* The local names are chosen so that the frame has the executable's stack layout. */
void __stdcall RaceControlTimer(unsigned id, unsigned msg, unsigned long user, unsigned long dw1, unsigned long dw2)
{
    unsigned char curCommand;
    GstShipOrderMsg curHostReply;
    int curHostEntry;
    int hostPlace;
    unsigned char newHostTag;
    unsigned char currentState;
    int curStatus;
    char pendingReport[100];
    unsigned char newPressed;
    char curCharacter;
    unsigned char latestBytes[3];
    GstShipOrderMsg outPacket;
    int index;
    int currentRank;
    unsigned char nextCommand;
    extern void gstPostLockstepEvent(short, short, short, unsigned char, unsigned char);

    if ((unsigned)incomingLockstepEventCount > 30) {
        if (lockstepStatTick % 30 == 0) {
            lockstepStatSamples++;
            lockstepStatDelta = incomingLockstepEventCount - lockstepStatLast;
            lockstepStatAverage = ((lockstepStatSamples >> 1) + incomingLockstepEventCount) / lockstepStatSamples;
            lockstepStatLast = incomingLockstepEventCount;
        }
        lockstepStatTick++;
    }
    PollRaceControlInput();
    DoDetailKeys();
    if (keysPressed & 0x1000) {
        if (deathCameraActive) {
            if (gstIsHost) {
                raceControlError = 1;
            } else {
                gstDebugPrintf("TimeFunc: ESC PRESSED: In Score Screen: Non-Host Player is Quitting Game..");
                raceControlError = 1;
            }
        } else if (resultsShown) {
            if (gstIsHost) {
                nextCommand = 4;
                currentRank = 0;
                outPacket.type = 4;
                for (index = 0; index < 15; index++) {
                    if ((shipIndex[index].netActive & 1) && shipIndex[index].netPlace != -1) {
                        outPacket.order[index] = shipIndex[index].netPlace - 1;
                        if (gstDebugOutputEnabled)
                            printf("nextPkt.ShipOrder[ %d ] == %d\n", index, outPacket.order[index]);
                        currentRank++;
                    }
                }
                for (index = 0; index < 15; index++) {
                    if ((shipIndex[index].netActive & 1) && shipIndex[index].netPlace == -1) {
                        outPacket.order[index] = currentRank;
                        if (gstDebugOutputEnabled)
                            printf("nextPkt.ShipOrder[ %d ] == %d\n", index, outPacket.order[index]);
                        currentRank++;
                    }
                }
                for (index = 0; index < 15; index++) {
                    if (!(shipIndex[index].netActive & 1)) {
                        outPacket.order[index] = currentRank;
                        if (gstDebugOutputEnabled)
                            printf("nextPkt.ShipOrder[ %d ] == %d\n", index, outPacket.order[index]);
                        currentRank++;
                    }
                }
                gstDebugPrintf("TimeFunc: ESC PRESSED: Tournament Screen: Host is Sending Next Race Pkt");
                gstPostSessionAsynch(gstSessionId, &outPacket, 0x20, 0);
            } else {
                gstDebugPrintf("TimeFunc: ESC PRESSED: Tournament Screen: Non-Host is Quitting the Tournament.");
                raceControlError = 1;
            }
        } else {
            gstDebugPrintf("TimeFunc: ESC PRESSED: Local Player is Quitting the Game!");
            raceControlError = 1;
        }
    }
    if (raceControlTimerEnabled) {
        newPressed = 0;
        raceControlTimerEnabled = 0;
        if (lockstepStatTick % 150 == 0) {
            gstVxdReadByte(0x6a, &currentState);
            gstVxdGetLastError(&curStatus);
            sprintf(pendingReport, "Lockstep HandlerMask: %x %d Error: %X", currentState, GetTickCount(), curStatus);
            gstDebugWriteText(0, 0, pendingReport);
        }
        latestBytes[0] = 2;
        latestBytes[1] = 1;
        switch (g_keyboardCode) {
        case 2:
            latestBytes[2] = 2;
            newPressed = 1;
            break;
        case 3:
            latestBytes[2] = 9;
            newPressed = 1;
            break;
        case 4:
            latestBytes[2] = 1;
            newPressed = 1;
            break;
        case 5:
            latestBytes[2] = 5;
            newPressed = 1;
            break;
        case 6:
            latestBytes[2] = 10;
            newPressed = 1;
            break;
        case 7:
            latestBytes[2] = 12;
            newPressed = 1;
            break;
        case 8:
            latestBytes[2] = 3;
            newPressed = 1;
            break;
        case 9:
            latestBytes[2] = 11;
            newPressed = 1;
            break;
        case 10:
            latestBytes[2] = 8;
            newPressed = 1;
            break;
        default:
            break;
        }
        if (g_keyboardCode == 15) {
            if (deathCameraActive) {
                if (gstRaceMode == 3) {
                    deathCameraActive = 0;
                    resultsShown = 1;
                    gstDebugPrintf("TimeFunc: TAB PRESSED: In Score Screen: advancing to Tournament Score Display.");
                } else {
                    gstDebugPrintf("TimeFunc: TAB PRESSED: In Score Screen: Local Player is continuing to RACE.");
                    deathCameraActive = 0;
                    shipIndex[ownShip].attr |= 0x40;
                }
            } else if (resultsShown) {
                resultsShown = 0;
                shipIndex[ownShip].attr |= 0x40;
                gstDebugPrintf("TimeFunc: TAB PRESSED: Tournament Screen: Local Player is continuing to RACE.");
            } else if (classLaps[gameData->shipClass] < shipIndex[ownShip].lap) {
                deathCameraActive = 1;
                shipIndex[ownShip].attr &= ~0x40;
                gstDebugPrintf("TimeFunc: TAB PRESSED: RACING: Got request to re-display Score Screen");
            }
        }
        curCharacter = GetKeyboardChar();
        if (curCharacter) {
            switch (curCharacter) {
            case 'C':
                if (gstDebugOutputEnabled) {
                    if (!gstSyncActive)
                        gstSyncActive = 1;
                    else
                        gstSyncActive = 0;
                }
                break;
            case 'N':
                if (gstIsHost) {
                    newHostTag = 4;
                    hostPlace = 0;
                    curHostReply.type = 4;
                    for (curHostEntry = 0; curHostEntry < 15; curHostEntry++) {
                        if ((shipIndex[curHostEntry].netActive & 1) && shipIndex[curHostEntry].netPlace != -1) {
                            curHostReply.order[curHostEntry] = shipIndex[curHostEntry].netPlace - 1;
                            if (gstDebugOutputEnabled)
                                printf("nextPkt.ShipOrder[ %d ] == %d\n", curHostEntry, curHostReply.order[curHostEntry]);
                            hostPlace++;
                        }
                    }
                    for (curHostEntry = 0; curHostEntry < 15; curHostEntry++) {
                        if ((shipIndex[curHostEntry].netActive & 1) && shipIndex[curHostEntry].netPlace == -1) {
                            curHostReply.order[curHostEntry] = hostPlace;
                            if (gstDebugOutputEnabled)
                                printf("nextPkt.ShipOrder[ %d ] == %d\n", curHostEntry, curHostReply.order[curHostEntry]);
                            hostPlace++;
                        }
                    }
                    for (curHostEntry = 0; curHostEntry < 15; curHostEntry++) {
                        if (!(shipIndex[curHostEntry].netActive & 1)) {
                            curHostReply.order[curHostEntry] = hostPlace;
                            if (gstDebugOutputEnabled)
                                printf("nextPkt.ShipOrder[ %d ] == %d\n", curHostEntry, curHostReply.order[curHostEntry]);
                            hostPlace++;
                        }
                    }
                    gstDebugPrintf("TimeFunc: ESC PRESSED: Tournament Screen: Host is Sending Next Race Pkt");
                    gstPostSessionAsynch(gstSessionId, &curHostReply, 0x20, 0);
                }
                break;
            default:
                break;
            }
        }
        if (GetMenuKey(5) && playerDestroyed) {
            curCommand = 3;
            if (livesRemaining[ownShip] > 0 || !gstLevelLives) {
                gstDebugPrintf("TimeFunc: ENTER PRESSED: Posting request to regenerate this player, ownShip %d", ownShip);
                gstPostSessionAsynch(gstSessionId, &curCommand, 1, 0);
            }
        }
        g_keyboardCode = 0;
        if (raceControlTimerStarted)
            gstPostLockstepEvent(D_ab4e14, D_abbf94, D_ab4e1c, gstLockstepChannel, 0);
        raceControlTimerEnabled = 1;
    }
    raceFrameReady++;
}

extern __declspec(dllimport) unsigned __stdcall timeKillEvent(unsigned);
extern int tournamentTrack;

/* Leaves the network race: when forced (or after the last tournament track) the lockstep object and the session are
   exited, and the race control timer is stopped. */
int gstShutdownNetworkRace(int forced)
{
    unsigned long result;

    gstDebugPrintf("ShutdownNetworkRace:");
    if (tournamentTrack >= 7) {
        if (gstDebugOutputEnabled)
            printf("ShutdownNetworkRace: Last Tournament Track! Overriding disconnect\n");
        forced = 1;
    }
    raceControlTimerStarted = 0;
    if (forced) {
        result = gstExitLockstepGPI();
        if (result < 1)
            gstDebugPrintf("gstExitLockstepGPI: Error: %d [%s]", result, gstErrorString(result));
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
    }
    timeKillEvent(raceControlTimerId);
    if (gstDebugOutputEnabled)
        printf("\n******************************* ShutdownNetworkRace: done Racing *******************************\n\n");
    return 1;
}
#endif
