#include "include_asm_g8.h"
#include "functions.h"
#include "hud.h"
#include "hiscores.h"
#include "weapon.h"
#include "raceinit.h"

/* $gp-relative globals are tentative definitions; the rest are extern. */
u_short          startTextureBase;
int              weaponGrids;
void            *upperSkeleton;
TIMlist         *residentTextureSeq;
int              shadowData;
int              weaponIndex;
int              weaponBars;
Texture         *continuesTexture;
ShipData        *shipIndex;
u_short          savedTextureCount;
short            beamCollide;
short            advertCount;
short            cameraCount;
short            zepCount;
short            dishCount;
short            fanCount;
short            smokeSlowCount;
short            smokeFastCount;
short            smokeNumber;
short            torchCount;
short            torchNumber;
short            redBullCount;
short            redBullNumber;
int              linkFlashFrame;

extern char          startWadPath[];
extern char          startDroidNames[];
extern char          lightGroupNames[];
extern Object       *startDroidObjects[];
extern ConfigData   *gameData;
extern WADFILE      *textureWad;
extern CheckCell     checkCells[][4][6];
extern POLY_FT4      weaponIcons[];
extern Object       *shipShapeTable[];
extern Skeleton     *residentSkeletons[];
extern Speedo        speedoHud;
extern u_char        weaponInitArg[];
extern Object       *lightGroups[];
extern Texture      *weaponIconTextureTable[];
extern int           heap;
extern TrackData    *track;
extern int           mainheapcopy;
extern int           reserveHeap;
extern int           fastCount;
extern int           fastTexCount;
extern int           slowCount;
extern int           slowTexCount;
extern int           bobCount;
extern CVECTOR       smokeColour;
extern Object       *zeppelinObj[];
extern Object       *radar[];
extern Object       *neonObject[];
extern Object       *torchLight[];
extern Object       *advert[];
extern Object       *redBull[];
extern Object       *cameraDroids[];
extern Object       *fans[];
extern Object       *smokeFastObjects[];
extern Object       *smokeSlowObjects[];
extern u_char        msgCombatCable[];
extern u_char        msgLinkBroken[];
extern u_char        msgReturningToMenus[];
extern u_char        msgRemoved[];
extern short         linkTimer;
extern int           raceEndTimer;

typedef struct {
    unsigned char pad0[0x9000];
    short count, draw, pad9004, mode, active;
    unsigned char tail[34];
} TrackCache;
typedef struct {
    short value;
    unsigned char pad[4];
} CheckPointShort;
typedef struct {
    signed char value;
    unsigned char pad[5];
} CheckPointByte;

typedef struct RaceLocals {
    TrackData trackData;
    void *clearPolys[2];
    u_char pad58[16];
    Object *crowdObjects[20];
    u_char cdLocations[336];
    union {
        ResetData data;
        u_char storage[32];
    } reset;
    union {
        InGameMenu polys;
        u_char storage[3312];
    } pauseMenu;
    short tileCount;
    u_char padF1A[2];
    int weaponGridCount;
    short specialGridCount;
    short cdTrack;
    int cdMode;
    signed char viewMode;
    u_char padF29;
    short raceComplete;
    u_char padF2C[4];
    int idleLimit;
    u_char padF34[4];
    int crowdCount;
    u_char padF3C[4];
} RaceLocals;
void *BlockFill__eq1() __asm__("BlockFill");
void *DAlloc__eq1() __asm__("DAlloc");
int InitCheckPoints__eq1() __asm__("InitCheckPoints");
int AsmDrawObjectFixed();
int AsmDrawParticles();
int SetSkeletonDirection__eq1() __asm__("SetSkeletonDirection");
void *WADLoadTtfFile__eq1() __asm__("WADLoadTtfFile");
int RaceComplete__eq1() __asm__("RaceComplete");
int Check2PlayerPause__eq1() __asm__("Check2PlayerPause");
int PerformPauseOperations__eq1() __asm__("PerformPauseOperations");
int InitiateTwoPlayerPause__eq1() __asm__("InitiateTwoPlayerPause");
int Perform2PlayerPauseCommand__eq1() __asm__("Perform2PlayerPauseCommand");
int getPath__eq1() __asm__("getPath");
int PlayNote__eq1() __asm__("PlayNote");
int PlayDirectionalNote__eq1() __asm__("PlayDirectionalNote");
int InitCrowdSound__eq1() __asm__("InitCrowdSound");
int InitTrainLights__eq1() __asm__("InitTrainLights");
int InitObject__eq1() __asm__("InitObject");
int InitMonoData__eq1() __asm__("InitMonoData");
int UpdateStartDroid__eq1() __asm__("UpdateStartDroid");
int MoveMonoRail__eq1() __asm__("MoveMonoRail");
int LoadModel__eq1() __asm__("LoadModel");
int AsmGenerateTiles__eq1() __asm__("AsmGenerateTiles");
int AddSpurtExplode__eq1() __asm__("AddSpurtExplode");
int AddExplode__eq1() __asm__("AddExplode");
int AddSmallSpurtExplode__eq1() __asm__("AddSmallSpurtExplode");
int AddSmallSmokeExplode__eq1() __asm__("AddSmallSmokeExplode");
extern u_char trackPathTable[];
extern u_char track1SmokeTextureSequence[];
extern u_char sharedTrackTextureSequence[];
extern u_char track6TextureSequence[];
extern u_char track20TextureSequence[];
extern u_char track17TextureSequence[];
extern u_char track7TextureSequence[];
extern u_char track13TorchTextureSequence[];
extern u_char track13TextureSequence[];
extern u_char track2ZeppelinNames[];
extern u_char track2TextureSequence[];
extern u_char msgOvertake[];
extern short linked;
extern short linkDropped;
extern short linkMyState;
extern short linkRemoteState;
extern u_char sillyShips;
extern u_short TextureTableCount;
extern int spareMax;
extern u_char medalDisplay;
extern short fadeAmount;
extern int trackNameScroll;
int attractModeFlag;
int firstTimeIn;
int playCredits;
short trackCycleTimer;
int skycyc;
short raceFlag;
int oneMoreTime;
int masterWait;
short advertCount;
short cameraCount;
short neonCount;
short girderCount;
short videoCount;
short zepCount;
short dishCount;
short pylonCount;
short fanCount;
short smokeSlowCount;
short smokeFastCount;

int fastCount;
int fastTexCount;
int slowCount;
int slowTexCount;
short torchCount;

short redBullCount;

int bobCount;
short prmCount;
short startTextureBase__v1 __asm__("startTextureBase");
extern u_char cameraDroidNames[];
extern u_char fanNames[];
extern u_char smokeSlowNames[];
extern u_char smokeFastNames[];
extern u_char redBullNames[];
extern u_char advertNames[];
extern u_char videoNames[];
extern u_char radarNames[];
extern u_char neonNames[];
extern u_char torchNames[];
extern u_char monoRailNames[];
extern u_char girderNames[];
extern u_char pylonNames[];
extern u_char vectorShipName[];
extern u_char venomShipName[];
extern u_char rapierShipName[];
extern u_char phantomShipName[];
extern u_char overtakeMessage[];
extern u_char undercover;
extern int writeFlag;
extern short buttonCount;
extern int cardNum;
extern int heading;
extern u_char cheatRain;
extern u_char menuSendFlags;
extern u_char menuRecvFlags;
int weaponGrids;
extern int remoteShip;
int raceElapsed;
extern u_char remoteRaceDone;
void *upperSkeleton;
extern void *girderObjects[];
int shadowData;
int weaponIndex;
extern unsigned long *OT[];
void *tempGameData;
extern int (*CameraUpdate)();
extern short shipClassIndex;
extern unsigned long *OT2[];
int (*prevCameraUpdate)();
u_short sceneTextureCount;
extern int attractDelayFrames;
extern short clipLevel;
extern int ownShip;
int savedUpperY;
int weaponBars;
extern u_char teamShip[];
extern int linkSendFlag;
extern u_char gameType;
signed char smokeColour__v1 __asm__("smokeColour");
extern signed char smokeColourGreen;
extern signed char smokeColourBlue;
extern int raceType;
TrackCache *trackCache;
extern short CurrentScreen;
extern int reserveHeap;
int raceStartVcount;
extern int quadIndex;
extern int raceEndTimer;
Object *volatile zeppelinObj__v1 __asm__("zeppelinObj");
extern u_char padBuf1[8];
u_short tempTextureCount;
int checkTimer;
extern TrackCamera *cameraPtr;
extern u_char keysPressed[];
extern u_char linkRaceDone;
int particleSlots;
int savedHeap;
extern int fracdamage[];
extern int lightDroid;
extern HiScoreData scoreTabData;
extern int shipDamage[];
extern TrackCamera raceCamera __asm__("residentSkeletons");
extern u_char posMonoRail[];
extern RescueData rescueData;
extern Vector zeppelinBase __asm__("zeppelinBaseX");
extern void *pylons[];
extern void *videoObjects[];
extern int shipTargetIndex[];
extern signed char twoPlayerMode;
extern signed char twoPlayerFlag;
extern u_char playerControl[];
extern u_char monoRailObjs[];
extern Object *prmObjects[];
extern POLY_FT4 weaponIconPrims[];
extern u_char scenePath[];
extern volatile short monoRailFlag[];
extern Vector trainMuzzle __asm__("trainMuzzleX");
extern int heap;
/* value / FRAMES_PER_TENTH, multiplying by the reciprocal as the compiler would (magic is the reciprocal). */
static inline int framesToTenths(int value, int magic)
{
    union {
        long long product;
        struct {
            int hi;
            u_int lo;
        } words;
    } result;
    result.product = (long long)value * magic;
    return (result.words.hi >> REGION(0, 1)) - (value >> 31);
}
extern CheckPointShort checkCells__v1[][4][6] __asm__("checkCells");
extern CheckPointShort raceCheckpointResults[][4][6];
extern CheckPointByte raceCheckpointFlags[][4][6];

#define CHECKPOINTS (checkCells__v1[gameData->trackNum][gameData->shipClass])
void race(ConfigData *gameData, int attractMode, int idleLimit)
{
    extern PauseState pauseState;
    RaceLocals work;
    unsigned short skyObject;
    int lightningFrame;
    int thunderFrame;
    u_char updateTrails;
    int divisionMagic;
    Object **updateShapes;
    Object **renderShapes;

    void *wad, *tiles;
    int i, j, radius, x, z, bob;
    int advance;
    int setupOne = 1;
    int one;
    int setupTwo = 2;
    work.tileCount = 0;
    work.weaponGridCount = 0;
    work.cdMode = setupOne;
    work.viewMode = setupTwo;
    work.specialGridCount = 0;
    work.raceComplete = 0;
    work.idleLimit = idleLimit;
    InitDynamicMem(reserveHeap, 0xE1000);
    work.crowdCount = 0;
    advertCount = 0;
    cameraCount = 0;
    neonCount = 0;
    girderCount = 0;
    videoCount = 0;
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
    redBullCount = 0;
    redBullNumber = 0;
    bobCount = 0;
    prmCount = 0;
    attractModeFlag = 0;
    firstTimeIn = setupOne;
    twoPlayerMode = setupTwo;
    twoPlayerFlag = 0;
    checkTimer = 0;
    savedHeap = heap;
    heap = reserveHeap;
    savedTextureCount = TextureTableCount;
    masterWait = 0;
    writeFlag = 0x3938700;
    lightningFrame = 0;
    thunderFrame = 0;
    updateTrails = 1;
    if (attractMode == 0)
        raceFlag = 1;
    trackCache = DAlloc__eq1(reserveHeap, 0x902C);
    trackCache->draw = 0;
    trackCache->count = 0;
    trackCache->mode = 0;
    trackCache->active = 1;
    work.clearPolys[0] = BlockFill__eq1(0, 0, 320, SCREEN_HEIGHT, 0, 0, 0);
    work.clearPolys[1] = BlockFill__eq1(0, 256, 320, SCREEN_HEIGHT, 0, 0, 0);
    SetSkeletonPosition(raceCamera.camPos, 0, 0, 0);
    SetSkeletonDirection__eq1(raceCamera.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection__eq1(upperSkeleton, 0, 0, 0);
    cameraPtr = &raceCamera;
    track = &work.trackData;
    tempGameData = gameData;
    raceCamera.camPos->super = upperSkeleton;
    getPath__eq1(scenePath, gameData->actTrackNum, trackPathTable);
    wad = openwadfile(scenePath);
    WADInitVertices(wad);
    WADInitFaces(wad);
    WADInitSections(wad);
    WADInitViewList(wad);
    tempTextureCount = TextureTableCount;
    TextureTableCount = 0;
    LoadCmpFiles(WADLoadCompressedTextureSequence(wad));
    tiles = WADLoadTtfFile__eq1(wad, &work.tileCount, 0);
    TextureTableCount = tempTextureCount;
    skyObject = WADInitScene(prmObjects, &prmCount, raceCamera.camPos, gameData, wad);
    sceneTextureCount = TextureTableCount;
    closewadfile(wad);
    for (j = 0; j < 6; j++) {
        raceCheckpointResults[gameData->trackNum][gameData->shipClass][j].value = 0;
        checkCells__v1[gameData->trackNum][gameData->shipClass][j].value = -1;
        raceCheckpointFlags[gameData->trackNum][gameData->shipClass][j].value = -1;
    }
    InitTrackTextures(gameData);
    InitWeaponGrids(weaponGrids, track, &work.weaponGridCount, &work.specialGridCount, CHECKPOINTS);
    InitCheckPoints__eq1(CHECKPOINTS, gameData, gameData->shipClass);
    trackCycleTimer = 0;
    InitTrackCycle();
    InitDroidThings(startDroidObjects, startDroidNames, raceCamera.camPos, CHECKPOINTS,
                    gameData);
    InitDroidThings(lightGroups, lightGroupNames, raceCamera.camPos, CHECKPOINTS, gameData);
    InitRaceLine(track, gameData);
    InitInGameMenu(&work.pauseMenu.polys);
    {
        int crowd = InitCrowdSound__eq1(prmObjects, prmCount, work.crowdObjects, work.crowdCount);
        int currentTrack = (unsigned char)gameData->actTrackNum;
        work.crowdCount = crowd;
        if (currentTrack == 1) {
            InitObject__eq1(prmObjects, prmCount, cameraDroids, cameraDroidNames, 6, &cameraCount);
            InitObject__eq1(prmObjects, prmCount, fans, fanNames, 3, &fanCount);
            InitObject__eq1(prmObjects, prmCount, smokeSlowObjects, smokeSlowNames, 6,
                            &smokeSlowCount);
            InitObject__eq1(prmObjects, prmCount, smokeFastObjects, smokeFastNames, 6,
                            &smokeFastCount);
            smokeNumber = TextureTableCount;
            LoadCmpFiles(LoadCompressedTextureSequence(track1SmokeTextureSequence));
            InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
            redBullNumber = TextureTableCount;
            LoadCmpFiles(LoadCompressedTextureSequence(sharedTrackTextureSequence));
            smokeColour__v1 = 64;
            smokeColourGreen = 64;
            smokeColourBlue = 64;
        }
    }
    if (gameData->actTrackNum == 6) {
        InitObject__eq1(prmObjects, prmCount, advert, advertNames, 6, &advertCount);
        InitObject__eq1(prmObjects, prmCount, cameraDroids, cameraDroidNames, 6, &cameraCount);
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        InitObject__eq1(prmObjects, prmCount, videoObjects, videoNames, 6, &videoCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track6TextureSequence));
    }
    if (gameData->actTrackNum == 8) {
        InitObject__eq1(prmObjects, prmCount, advert, advertNames, 6, &advertCount);
        InitObject__eq1(prmObjects, prmCount, cameraDroids, cameraDroidNames, 6, &cameraCount);
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(sharedTrackTextureSequence));
    }
    if (gameData->actTrackNum == 20) {
        InitObject__eq1(prmObjects, prmCount, radar, radarNames, 4, &dishCount);
        InitObject__eq1(prmObjects, prmCount, cameraDroids, cameraDroidNames, 6, &cameraCount);
        InitObject__eq1(prmObjects, prmCount, neonObject, neonNames, 5, &neonCount);
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track20TextureSequence));
    }
    if (gameData->actTrackNum == 17) {
        InitObject__eq1(prmObjects, prmCount, cameraDroids, cameraDroidNames, 6, &cameraCount);
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track17TextureSequence));
    }
    if (gameData->actTrackNum == 7) {
        InitObject__eq1(prmObjects, prmCount, videoObjects, videoNames, 6, &videoCount);
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        InitObject__eq1(prmObjects, prmCount, advert, advertNames, 6, &advertCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track7TextureSequence));
    }
    if (gameData->actTrackNum == 13) {
        InitWaterFall();
        InitObject__eq1(prmObjects, prmCount, torchLight, torchNames, 5, &torchCount);
        torchNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track13TorchTextureSequence));
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track13TextureSequence));
    }
    if (gameData->actTrackNum == 2) {
        LoadTrain(monoRailObjs, monoRailNames, raceCamera.camPos, posMonoRail);
        InitObject__eq1(prmObjects, prmCount, cameraDroids, cameraDroidNames, 6, &cameraCount);
        InitObject__eq1(prmObjects, prmCount, &zeppelinObj__v1, track2ZeppelinNames, 8, &zepCount);
        InitObject__eq1(prmObjects, prmCount, girderObjects, girderNames, 5, &girderCount);
        InitTrainLights__eq1(prmObjects, prmCount, monoRailObjs, pauseState.selection);
        InitObject__eq1(prmObjects, prmCount, redBull, redBullNames, 4, &redBullCount);
        redBullNumber = TextureTableCount;
        LoadCmpFiles(LoadCompressedTextureSequence(track2TextureSequence));
        {
            Object *trainObject = zeppelinObj__v1;
            monoRailFlag[0] = 0;
            trainMuzzle.vx = 0;
            trainMuzzle.vy = 0;
            trainMuzzle.vz = 0;
            {
                /* Keep the skeleton and the following object read in v1 for PsyQ scheduling. */
                register Skeleton *trainSkeleton asm("$3") = trainObject->skeleton;
                trainSkeleton->relative.t[2] -= 8000;
            }
        }
        {
            /* Reuse v1 after the skeleton store to preserve the retail load order. */
            register Object *trainPosition asm("$3") = zeppelinObj__v1;
            zeppelinBase.vx = trainPosition->skeleton->relative.t[0];
            zeppelinBase.vy = trainPosition->skeleton->relative.t[1];
            zeppelinBase.vz = trainPosition->skeleton->relative.t[2];
        }
        InitObject__eq1(prmObjects, prmCount, pylons, pylonNames, 5, &pylonCount);
        InitMonoData__eq1(pylons, pylonCount, &monoRailFlag[0]);
    }
    buildGameKeyPad(padBuf1, playerControl);
    initShipData(track->sections, shipIndex, gameData);
    if (gameData->actTrackNum == 13)
        LoadBirdGFX();
    InitRescueDroid(&rescueData, shipShapeTable, &shipIndex[ownShip]);
    CameraUpdate = UpdateCameraRaceStart;
    InitCamera(&raceCamera, shipIndex[ownShip].nearTrkSect);
    spareMax = 0;
    InitTrails();
    AsmGenerateTiles__eq1(tiles, work.tileCount);
    InitRaceResetData(&work.reset.data, shipIndex, gameData, weaponIndex, weaponBars, &rescueData,
                      shipShapeTable, CHECKPOINTS);
    for (j = 0; j < 15; j++) {
        shipDamage[j] = 0;
        fracdamage[j] = 0;
    }
    particleSlots = InitParticle();
    InitExplode();
    InitRing();
    InitScoreTabData(&scoreTabData);
    InitialiseBumps();
    if (raceType == RACE_ARCADE) {
        switch (gameData->shipClass) {
        case 0:
            LoadModel__eq1(teamShip, vectorShipName);
            break;
        case 1:
            LoadModel__eq1(teamShip, venomShipName);
            break;
        case 2:
            LoadModel__eq1(teamShip, rapierShipName);
            break;
        case 3:
            LoadModel__eq1(teamShip, phantomShipName);
            break;
        }
    } else if (raceType == RACE_CHALLENGE) {
        if (gameData->venomComplete)
            LoadModel__eq1(teamShip, rapierShipName);
        else if (gameData->rapierComplete)
            LoadModel__eq1(teamShip, phantomShipName);
    }
    one = 1;
#ifndef VERSION_EU /* NTSC: reciprocal of 6 frames per tenth, set once */
    divisionMagic = 0x2AAAAAAB;
#endif
    trackNameScroll = 480;
    gameData->raceInfo = 0;
    medalDisplay = 0;
    for (;;) {
        Restart(&work.reset.data);
        InitRaceData(&pauseState);
        if (attractMode != one)
            attractDelayFrames = 0;
#ifdef VERSION_EU /* PAL: reciprocal of 5 frames per tenth, set at the start of every race */
        divisionMagic = 0x66666667;
#endif
        InitAttractMode(attractMode, shipIndex);
        gameData->complete = 0;
        SetUpFirstCheckPoint();
        do {
#ifdef VERSION_EU /* PAL polls the pad packet once per vsync */
            GetJoyPadPacketOnVsync(pauseState.stage);
#endif
            buildGameKeyPad(padBuf1, playerControl);
            buildFixedPad(padBuf1);
            if (raceType == one)
                RecordKeypresses(shipIndex);
            UpdateCheatSequence(0);
            ClearOTagR(OT[CurrentScreen], 8192);
            ClearOTagR(OT2[CurrentScreen], 1600);
            buttonCount = 0;
            quadIndex = CurrentScreen * 300;
            shipClassIndex = gameData->shipClass;
            clipLevel = gameData->trackNum;
            SetInGameCDTrack(&work.cdTrack, work.cdLocations, gameData, &work.cdMode);
            if (gameType != GAME_SINGLE_PLAYER)
                doSerialGame(gameData, shipIndex);
            ShowLinkBroken();
            writeFlag = 0;
            CheckShipDestroyed(shipIndex, gameData, &attractMode, work.idleLimit);
            PlayCredits(attractMode);
            CheckGamePause(shipIndex, &pauseState, gameData, keysPressed, &pauseState.musicPaused);
            Check2PlayerPause__eq1(pauseState.stage, &work.viewMode, &pauseState.musicPaused);
            PerformPauseOperations__eq1(&pauseState, &work.pauseMenu.polys, gameData, &work.reset.data);
            InitiateTwoPlayerPause__eq1(&pauseState, shipIndex);
            if (!raceEndTimer)
                Perform2PlayerPauseCommand__eq1(&pauseState, gameData, &work.reset.data);
            if ((pauseState.selection == -1 || oneMoreTime) && !masterWait && !linkDropped) {
                oneMoreTime = 0;
                CheckForButtonPress();
                updateShapes = shipShapeTable;
                UpdateShips(shipIndex, weaponBars, updateShapes, weaponIndex);
                {
                    /* Retail also passes the ship array as an unused fourth argument. */
                    /* Keep the callback slot in v0 until its final load. */
                    register void (**updateSlot)(RescueData *, ShipData *, Object **) asm("$2") =
                        &rescueData.droidUpdate;
                    /* Reusing v1 avoids PsyQ's priority boost for a register set only once. */
                    register int player asm("$3") = ownShip;
                    ShipData *ships = shipIndex;
                    void (*update)() = (void (*)())*updateSlot;
                    update(&rescueData, &ships[player], updateShapes, ships);
                }
                CameraUpdate(&raceCamera, &shipIndex[ownShip], &rescueData);
                if (shipIndex[ownShip].attr & RACING) {
                    if (shipIndex[ownShip].weapon == WEAPON_MISSILE ||
                        shipIndex[ownShip].weapon == WEAPON_ELECTROBOLT) {
                        MissileLock(ownShip, shipIndex);
                        if (shipIndex[ownShip].weapon == WEAPON_MISSILE)
                            RearMissileLock(ownShip, shipIndex);
                    }
                    ShakeScreen();
                }
                if (gameType != GAME_SINGLE_PLAYER && (shipIndex[remoteShip].attr & RACING)) {
                    if (shipIndex[remoteShip].weapon == WEAPON_MISSILE ||
                        shipIndex[remoteShip].weapon == WEAPON_ELECTROBOLT) {
                        MissileLock(remoteShip, shipIndex);
                        if (shipIndex[remoteShip].weapon == WEAPON_MISSILE)
                            RearMissileLock(remoteShip, shipIndex);
                    }
                }
                UpdateWeapons(shipIndex, weaponBars, weaponIndex);
                CheckPitShips();
                if (pauseState.selection == -1 && !linkDropped) {
                    AdjustEngineSound(&shipIndex[ownShip], &raceCamera);
                    AdjustCrowdSound(work.crowdObjects, work.crowdCount);
                }
                TrackSpecificFunctions();
                ColourDroidLights(lightDroid);
                AddEffects(weaponIndex, shipIndex);
                raceElapsed = GsGetVcount() - raceStartVcount;
                if (raceType != one && gameData->weaponsOn)
                    CycleWeaponGrids(shipIndex, weaponGrids, work.weaponGridCount);
                else if (!gameData->weaponsOn && raceType == RACE_CHALLENGE)
                    CycleWeaponGrids(shipIndex, weaponGrids, work.weaponGridCount);
            }
            masterWait = 0;
            if (!linkSendFlag) {
                if (attractDelayFrames > 0) {
                    attractDelayFrames--;
                    if ((attractMode == one && CheckAttractOver(gameData)) || !attractDelayFrames)
                        raceEndTimer = 2;
                }
            } else
                RaceComplete__eq1(&work.pauseMenu.polys, &work.reset.data, gameData, &work.raceComplete,
                                  &scoreTabData, shipIndex, &pauseState);
            if (raceEndTimer > 0) {
                if (linked) {
                    if (linkRemoteState == 4)
                        raceEndTimer = 0;
                    else if (linkRemoteState == 3) {
                        linkMyState = 4;
                        raceEndTimer = one;
                        playCredits = one;
                    }
                } else {
                    raceEndTimer--;
                    playCredits = one;
                }
                if (!raceEndTimer) {
                    QuitGame();
                    return;
                }
            }
            if (scoreTabData.nameEntryActive != one && !raceEndTimer) {
                UpdateRaceHud(shipIndex, &speedoHud, pauseState.selection, gameData, &scoreTabData);
                if (shipIndex[ownShip].update == UpdatePlayerShip ||
                    shipIndex[ownShip].update == UpdateRescueDroid ||
                    (shipIndex[ownShip].update == UpdateDroneShip &&
                     shipIndex[ownShip].autopilotCount > 0)) {
                    if (((*(u_long *)&shipIndex[ownShip].combatAttr) & 0x8004) != 0x8004)
                        AlterCheckPoint(CHECKPOINTS, shipIndex, pauseState.selection, gameData);
                }
            }
            if (gameData->weaponsOn || raceType == one || raceType == RACE_CHALLENGE) {
                if ((shipIndex[ownShip].attr & RACING) && ownShip < 12) {
                    DrawWeaponIcons(weaponIcons, weaponIconTextureTable, &shipIndex[ownShip]);
                    AddPrim(OT[CurrentScreen], &weaponIconPrims[CurrentScreen]);
                }
            }
            if (gameData->actTrackNum == 2 && raceType != 2 &&
                (unsigned int)((u_short)shipIndex[ownShip].nearTrkSect->secNo - 200) < 76) {
                MoveMonoRail__eq1(monoRailObjs, 6, &monoRailFlag[0], shipIndex, pauseState.selection);
                InitTrainLights__eq1(prmObjects, prmCount, monoRailObjs, pauseState.selection);
            }
            if (gameData->actTrackNum == 6 || gameData->actTrackNum == 7)
                Video(videoObjects, videoCount);
            if (!(shipIndex[ownShip].attr & RACING) || shipTargetIndex[ownShip] >= 0)
                DrawAllTrack(&raceCamera, trackCache);
            else
                DrawNewTrack(&raceCamera, trackCache);
            /* Sample the table for each call so PsyQ keeps the render address local. */
            DrawShadow(renderShapes = shipShapeTable, shipIndex, shadowData);
            {
                ShipData *ships = shipIndex;
                int mode = pauseState.selection;
                renderShapes = shipShapeTable;
                DrawShips(ships, renderShapes, ownShip, raceCamera, mode);
            }
            if (!medalDisplay)
                DrawLandscape(&shipIndex[ownShip], &raceCamera);
            (*(Object **)((char *)prmObjects +
                          ((int)((u_int) (u_short)skyObject << 16) >> 14)))
                ->skeleton->update = one;
            savedUpperY = ((Skeleton *)raceCamera.camPos->super)->relative.t[1];
            if (gameData->actTrackNum == 17) {
                if (raceCamera.camPos->relative.t[1] > 0) {
                    bob = rsin(skycyc);
                    ((Skeleton *)raceCamera.camPos->super)->relative.t[1] +=
                        1024 + (((raceCamera.camPos->relative.t[1] >> 3) * bob) >> 12);
                }
                if (pauseState.selection == -1 &&
                    (unsigned int)((u_short)raceCamera.section->secNo - 303) >= 26)
                    skycyc = (skycyc + 96) & 4095;
            }
            AsmDrawObjectFixed(
                (*(Object **)((char *)prmObjects +
                              ((int)((u_int) (u_short)skyObject << 16) >> 14))),
                raceCamera.camPos, 8191);
            ((Skeleton *)raceCamera.camPos->super)->relative.t[1] = savedUpperY;
            if (prevCameraUpdate != CameraUpdate) {
                UnsetParticles(particleSlots);
                prevCameraUpdate = CameraUpdate;
            }
            AsmDrawParticles(particleSlots, raceCamera.camPos, pauseState.selection + 1);
            DrawExplode();
            if (gameData->actTrackNum == 13)
                DrawBirds();
            DrawRing();
            if (sillyShips)
                UpdateSharkModel();
            if (gameType != GAME_SINGLE_PLAYER &&
                !(shipIndex[remoteShip].combatAttr & DESTROYED))
                DrawLinkSign(shipIndex);
            if (pauseState.selection < 0) {
                UpdateShipTrails(shipIndex, attractDelayFrames);
                updateTrails = 1;
                if (shipTargetIndex[ownShip] == 100)
                    shipIndex[ownShip].attr &= ~VIEW_IN;
                if ((shipTargetIndex[ownShip] & 7) < 4 && shipTargetIndex[ownShip] >= 16 &&
                    !(shipIndex[ownShip].combatAttr & 4)) {
                    CentreText(overtakeMessage, 15, 64, 12, one);
                    CentreText(msgOvertake, 45, 128, 8, 0);
                    if (!(shipTargetIndex[ownShip] & 7))
                        PlayNote__eq1(48, 255, 15, 15, 256);
                }
                for (i = 0; i < 15; i++) {
                    if (!(shipIndex[i].combatAttr & DESTROYED) &&
                        shipTargetIndex[i] > 0) {
                        if ((i == ownShip && shipTargetIndex[ownShip] >= 16) ||
                            (i == remoteShip && shipTargetIndex[remoteShip] >= 16)) {
                            ShipData *ship = shipIndex;
                            ship[i].ppivot.vx += ship[i].vpivot.vx >> 6;
                            ship[i].ppivot.vy += ship[i].vpivot.vy >> 6;
                            ship[i].ppivot.vz += ship[i].vpivot.vz >> 6;
                            ship[i].vpivot.vx -= ship[i].vpivot.vx >> 3;
                            ship[i].vpivot.vy -= ship[i].vpivot.vy >> 3;
                            ship[i].vpivot.vz -= ship[i].vpivot.vz >> 3;
                            ship[i].vhdg = 0;
                            ship[i].roll += (ship[i].vpivot.vx + ship[i].vpivot.vz) >> 5;
                        }
                        if (shipTargetIndex[i] < 15) {
                            radius = (15 - shipTargetIndex[i]) * 64;
#ifdef VERSION_EU /* wreck explosions: every 5th frame on PAL, every 8th on NTSC */
                            if (shipTargetIndex[i] -
                                    framesToTenths(shipTargetIndex[i], divisionMagic) * FRAMES_PER_TENTH == one) {
#else
                            if ((shipTargetIndex[i] & 7) == 7) {
#endif
                                int x, y, z;
                                x = shipIndex[i].ppivot.vx + newRand() % radius - (radius >> 1);
                                y = shipIndex[i].ppivot.vy - newRand() % 127;
                                z = shipIndex[i].ppivot.vz + newRand() % radius - (radius >> 1);
                                AddExplode__eq1(x, y, z, -1);
                            }
#ifdef VERSION_EU /* PAL divides by a literal 6 here, not by the frames per tenth */
                            if (shipTargetIndex[i] / 6 * 6 ==
#else
                            if ((framesToTenths(shipTargetIndex[i], divisionMagic)) * FRAMES_PER_TENTH ==
#endif
                                shipTargetIndex[i] - 4) {
                                int x, y, z;
                                x = shipIndex[i].ppivot.vx + newRand() % radius - (radius >> 1);
                                y = shipIndex[i].ppivot.vy - newRand() % 127;
                                z = shipIndex[i].ppivot.vz + newRand() % radius - (radius >> 1);
                                AddSmokeExplode(x, y, z, -1);
                            }
                            radius = (20 - shipTargetIndex[i]) * 64;
                            if ((shipTargetIndex[i] -
                                 framesToTenths(shipTargetIndex[i], divisionMagic) * FRAMES_PER_TENTH) == one) {
                                int x, y, z;
                                x = shipIndex[i].ppivot.vx + newRand() % radius - (radius >> 1);
                                y = shipIndex[i].ppivot.vy - newRand() % 127;
                                z = shipIndex[i].ppivot.vz + newRand() % radius - (radius >> 1);
                                AddSpurtExplode__eq1(x, y, z, -1);
                            }
                        }
                        if (shipTargetIndex[i] == 14) {
                            PlayDirectionalNote__eq1(5, 0, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote__eq1(5, 0, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote__eq1(5, -64, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote__eq1(5, -128, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote__eq1(5, -128, 40, shipIndex[i].ppivot);
                            PlayDirectionalNote__eq1(5, -192, 40, shipIndex[i].ppivot);
                        }
                        if ((newRand() & 3) == one && shipTargetIndex[i] >= 16)
                            AddSmallSmokeExplode__eq1(
                                shipIndex[i].ppivot.vx - (shipIndex[i].unitVecNose.vx >> 4),
                                shipIndex[i].ppivot.vy - (shipIndex[i].unitVecNose.vy >> 4),
                                shipIndex[i].ppivot.vz - (shipIndex[i].unitVecNose.vz >> 4), -1);
                        if ((shipTargetIndex[i] & 3) == one && shipTargetIndex[i] >= 16) {
                            x = (newRand() & 511) - (shipIndex[i].unitVecNose.vx >> 5) - 256;
                            z = (newRand() & 511) - (shipIndex[i].unitVecNose.vz >> 5) - 256;
                            AddSmallSpurtExplode__eq1(x, -(shipIndex[i].unitVecNose.vy >> 5) + 128, z, i);
                        }
                        shipTargetIndex[i]--;
                        if (!shipTargetIndex[i]) {
                            if (i != ownShip && i != remoteShip) {
                                shipIndex[i].attr &= ~RACING;
                                PlayNote__eq1(51, 480, 15, 15, 256);
                            }
                            shipIndex[i].combatAttr |= DESTROYED;
                        }
                    }
                    if ((i == ownShip && (shipIndex[ownShip].combatAttr & DESTROYED)) ||
                        (i == remoteShip &&
                         (shipIndex[remoteShip].combatAttr & DESTROYED))) {
                        if ((newRand() & 31) < 3)
                            AddSmallSmokeExplode__eq1(shipIndex[i].ppivot.vx,
                                                      shipIndex[i].nearTrkSect->centre.vy + 128,
                                                      shipIndex[i].ppivot.vz, -1);
                    }
                }
                if (gameData->actTrackNum == 2 || gameData->actTrackNum == 7 || cheatRain) {
                    AddRain(particleSlots, 1, &cameraPtr->camPos->relative.t[0]);
                    advance = lightningFrame + 1;
                    lightningFrame = advance & 255;
                    if (!undercover && !(newRand() & 3) && lightningFrame < 32 && !(advance & 3)) {
                        if (fadeAmount < 228)
                            fadeAmount = 228;
                        thunderFrame = lightningFrame;
                    }
                    if (fadeAmount == 220)
                        fadeAmount = 118;
                    if (fadeAmount == 110)
                        fadeAmount = 0;
                    advance = thunderFrame + 32;
                    if (lightningFrame == advance)
                        PlayNote__eq1(35, 380, 15, 15, 128);
                    if (advance < lightningFrame && lightningFrame < 64 && !(newRand() & 3) &&
                        !(lightningFrame & 3))
                        PlayNote__eq1(35, (short)(412 - lightningFrame), 15, 15,
                                      320 - lightningFrame);
                }
                if (gameData->actTrackNum == 8)
                    AddSnow(particleSlots, 20, &cameraPtr->camPos->relative.t[0]);
            } else {
                if (updateTrails)
                    UpdateShipTrails(shipIndex, attractDelayFrames);
                updateTrails = 0;
            }
            resetMiscFx();
            DrawShipTrails(cameraPtr);
            if (!linkDropped) {
                UpdateStartDroid__eq1(startDroidObjects, shipIndex, CHECKPOINTS, startTextureBase__v1,
                                      gameData, pauseState.selection);
                UpdateLightStart(lightGroups, shipIndex, CHECKPOINTS, startTextureBase__v1, gameData,
                                 pauseState.selection);
            }
            if (playCredits) {
                ClearOTagR(OT[CurrentScreen], 8192);
                ClearOTagR(OT2[CurrentScreen], 1600);
            }
            AddPrim(OT[CurrentScreen] + 8191, work.clearPolys[CurrentScreen]);
#ifdef VERSION_EU /* PAL linked play waits for the vsync here */
            if (!pauseState.stage && gameType == one)
                VSync(0);
#endif
            {
                int menuDirection;
                if (gameType != one)
                    menuDirection = menuSendFlags;
                else
                    menuDirection = menuRecvFlags;
                cardNum -= menuDirection * 30;
                heading -= menuDirection * 15;
            }
            Swap();
            firstTimeIn = 0;
            UpdateNotes(0);
        } while (!linkRaceDone || (gameType != GAME_SINGLE_PLAYER && !remoteRaceDone));
        attractMode = 0;
    }
}
#undef CHECKPOINTS

/* Creates the resident skeletons, ship data, weapons and textures that stay loaded for the whole game. */
void InitResident(void)
{
    ShipData     *shipData;
    TrackSection *sections;
    TIMlist      *textureSequence;
    void         *weaponPool;
    Skeleton     *rootSkeleton;
    Skeleton     *subSkeleton;

    rootSkeleton = NewSkeleton();
    residentSkeletons[0] = rootSkeleton;
    SetSkeletonPosition(rootSkeleton, 0, 0, 0);
    SetSkeletonDirection(residentSkeletons[0], 0, 0, 0);
    subSkeleton = NewSkeleton();
    upperSkeleton = subSkeleton;
    SetSkeletonPosition(subSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    residentSkeletons[0]->super = upperSkeleton;
    shadowData = InitShadow(residentSkeletons[0]);
    shipData = DAlloc(heap, 0xFF0);
    sections = track->sections;
    shipIndex = shipData;
    loadShipData(sections, shipData, shipShapeTable, residentSkeletons[0]);
    textureWad = openwadfilenvc(startWadPath);
    InitSpeedo(&speedoHud);
    InitLinkUpSign();
    continuesTexture = WADLoadTexture(textureWad, 1);
    startTextureBase = TextureTableCount;
    textureSequence = WADLoadCompressedTextureSequence(textureWad);
    residentTextureSeq = textureSequence;
    LoadCmpFiles(textureSequence);
    InitWeaponIcons(weaponIcons, weaponIconTextureTable);
    weaponGrids = (int) DAlloc(heap, 0x420);
    weaponBars = (int) DAlloc(heap, 0x118);
    weaponPool = DAlloc(heap, 0x890);
    weaponIndex = (int) weaponPool;
    initWeapons(weaponBars, residentSkeletons[0], weaponPool, weaponInitArg);
    LoadExplodeGFX();
    LoadStartDroid(startDroidObjects, startDroidNames, residentSkeletons[0], checkCells[gameData->trackNum][gameData->shipClass], gameData);
    LoadStartDroid(lightGroups, lightGroupNames, residentSkeletons[0], checkCells[gameData->trackNum][gameData->shipClass], gameData);
}


void racetidy(void)
{
    heap = mainheapcopy;
    InitDynamicMem(reserveHeap, 0xE1000);
    StartControlledSound();
    TextureTableCount = savedTextureCount;
}


/* Runs the animation and moving parts that belong to the current track. */
void TrackSpecificFunctions(void)
{
    u_char trackNumber;

    RedBullAnimate(redBull, redBullCount, 0x14, redBullNumber, 2);
    trackNumber = gameData->actTrackNum;
    if (trackNumber == 1) {
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
        /* Called with a third argument that MoveRadar ignores. */
        ((void (*)())MoveRadar)(radar, dishCount, shipIndex);
        DoNeonSign(neonObject, 3);
    } else if (trackNumber == 7) {
        MoveAdvert(advert, advertCount);
    } else if (trackNumber == 17) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
    } else if (trackNumber == 13) {
        WaterFall();
        AnimateSprite(torchLight, torchCount, 1, torchNumber, 12);
    } else if (trackNumber == 2) {
        MoveCameraDroid(cameraDroids, cameraCount, shipIndex, &bobCount);
        MoveZeppelin(zeppelinObj, zepCount, shipIndex);
        beamCollide = 0;
    }
}


/* Shows the "link broken" screen for a short time, flashing the cable message. */
void ShowLinkBroken(void)
{
    if (linkTimer > 0) {
        linkTimer -= 1;
        if ((linkFlashFrame % 30) < 0x12) {
            CentreText(msgCombatCable, 0xA0, 0x46, 0xC, 1);
            CentreText(msgRemoved, 0xA0, 0x5A, 0xC, 1);
        }
        CentreText(msgLinkBroken, 0xA0, 0x78, 8, 0);
        CentreText(msgReturningToMenus, 0xA0, 0x84, 8, 0);
        if (linkTimer == 1) {
            raceEndTimer = linkTimer;
        }
        linkFlashFrame += 1;
    }
}
