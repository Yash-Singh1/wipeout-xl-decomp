#include "functions.h"
#include "ship_state.h"
#include "ship_lookup.h"
#include "race_mode.h"
#include "input_state.h"
#include "hud_state.h"
#include "track_results.h"
#include "menu_rotation.h"
#include "cheats.h"
#include "camera.h"
#include "gpu_stubs.h"
#include "hiscores.h"
#include "d3dpoly.h"
#include "pc.h"
#include "pc_blit.h"
#include "sound_samples.h"
#include <stdio.h>
#include <string.h>
#ifdef NETPLAY
#include <ctype.h>
#include "font.h"
#include "netmenu.h"
#endif

/* The file is built with /Ob2, but only the three loaders MenuControl expands inline (LoadMenuLineData,
   LoadModel, LoadMenuModel) are inline candidates; with the pragma off, every other function keeps its
   string literals in its own data section as in the executable. */
#pragma auto_inline(off)

/* The state of the options screens on the PC build lives in one global record. */
struct MenuState {
#ifdef NETPLAY
    signed char weapons;    /* 0x00: weapons on (signed in the network build) */
    signed char internalView; /* 0x01: default view is the cockpit (signed in the network build) */
#else
    u_char weapons;         /* 0x00: weapons on */
    u_char internalView;    /* 0x01: default view is the cockpit */
#endif
    u_char trailDetail;     /* 0x02 */
    u_char skyEnabled;      /* 0x03 */
    u_char viewDetail;      /* 0x04 */
    u_char pad5;
    short musicVolume;      /* 0x06 */
    short effectVolume;     /* 0x08 */
    short otherMusic;       /* 0x0a */
    short otherEffect;      /* 0x0c */
    signed char music;      /* 0x0e: CD track, 1 plays them all */
    signed char actTrackNum;/* 0x0f */
    signed char trackNum;   /* 0x10 */
    signed char shipClass;  /* 0x11 */
    signed char ship;       /* 0x12: ship picked on the team screen */
    u_char pad13;
    short current;          /* 0x14: highlighted row */
    short trackSelection;   /* 0x16 */
#ifdef NETPLAY
    u_char reserved18[0x3c];
    char name[10];                 /* 0x54: the network build copies the player name here (nameBuffer is the same place) */
#else
    u_char reserved18[0x46];
#endif
    short resetWord5E;            /* 0x5e */
    u_char resetBytes60[10];       /* 0x60: cleared when the menus start */
    u_char reserved6A[0x20];
    short resetWord8A, resetWord8C, resetWord8E, resetWord90, resetWord92;
};
extern struct MenuState menuState;
extern int currMenu;
extern MenuEntry *menu;
extern short shieldHumVoice;
extern int deviceType;
extern int savedDeviceType;
extern int deviceChosen;
extern int joystickPresent;
extern int mouseEnabled;
extern int keyboardKeys[11];
extern int joystickButtons[6];
extern int mouseButtons[6];



extern int gameType;
extern short ownShip;
extern short lastClassSelection;
extern int soundChanged;
extern int saveState;
extern int redefineCancelled, redefineStep, redefineKey;
extern int gameKeys[12];
extern int keyBindings[8];
extern int padBindings[8];
extern int mouseSensitivity;
extern u_short turnDivisor;
extern short displayDriverConfigTag;
extern short displayDriverCount;
extern short videoModeIndex;
extern int pixelFormatGroup, pixelFormatIndex, displayBits;
/* Set by the window procedure as keys arrive; the menu code treats it as volatile (EnterName
   keeps every store to it), the other files' declarations are plain. */
extern volatile short g_keyboardCode;
extern char nameBuffer[];

void SetNoteVolume(short voice, unsigned char volume);
void TerminateSound(void);
void StartControlledSound(void);
void InitMenuDisplay(int a, int b);

extern int cdMusicTrack;
extern signed char cdMusicTable[];
extern signed char cdMusicIndex;
extern int cdMusicCount;
extern float textDepth;
extern void *g_menuDisplayEnv;
extern DISPENV DispEnv0;
extern Object *teamShipModels[];
extern Object *teamShip;
extern int blinkFlag;
extern Object *shipModels[];
extern signed char raceTracks[];
extern POLY_F4 menuBars[2];
extern POLY_FT4 menuTiles[];
extern short menuTileCount;
#include "heap_state.h"
extern HeapBlock *mainheapcopy;
extern ConfigData *tempGameData;
extern Object *menuModels[];
extern MenuLayout *menuLayout;
extern char wobbleDir2[], errorString[];
extern short TextureTableCount, raceFlag;
extern short savedTrackSelection;
extern int flipView, trackNo, quitRequested, pauseRequested, raceContext, quadIndex;
extern volatile long FR60;
extern short CurrentScreen;
extern u_short menuKeys;
extern POLY_FT4 menuBackPolys[14];
extern short savedRaceSelection, savedTeamSelection, savedClassSelection, savedMainSelection;
extern int attractSettingsSaved, menuAttractShip, attractTrack, attractTrackNum, attractActTrackNum;
extern int quitMenuSelection, menuDisplayActive, menuSlideActive, menuSlideFrame;
extern short menuEditorActive, menuPrevious, menuButtonCount;
extern short menuSlideX, menuSlideY, menuSlideStep;
#include "shield_hum.h"
int RaceFrameStart(void);
void PollKeyboard(void);
void FileAccessError(char *name);
void SetupUnlocks(void);
void InitMenuState(ConfigData *config);
void NormaliseMenuBoxes(void);
void InitNewDisplay(void);
void ResetFrameClock(void);
void InitMenuBackPolys(void);
void ShowMenu(ConfigData *config, MenuBars *bars);
void SlideScreen(void);
void DrawMenuBox(short x0, short y0, short x1, short y1, float fillDepth, float borderDepth);

void LoadModel(Object **models, char *name);

#ifdef NETPLAY
void RunMenuScreen(PcGameData *gameData, HiScoreData *score);
void InitMenuBackdropPolys(POLY_F4 (*polys)[2]);
void InitMenuBars();
void DrawMenuBackPolys(void);
extern int D_4d8164;
extern short D_4d8208;
extern Texture *textureTable[];
#endif

#ifdef NETPLAY
/* Sets the menu the intro leaves on, clears the input words and polls the keyboard once. Called from the
   intro sequence of the network build. */
void EnterNetworkMenu(void)
{
    if (gstLaunchMode == 0) {
        gstPlayerNameLength = 0;
        gstSessionNameLength = 0;
        gstOptionsSelected = 0;
        currMenu = 0x11;
    } else {
        currMenu = 0;
    }
    menuKeys = 0xffff;
    joyPressed = 0xffff;
    padPressed = 0xffff;
    pauseRequested = 0;
    PollKeyboard();
}
#endif

/* The menu screen loop. The file is built with /Ob2: the model and menu-data loaders defined further down are
   expanded inline here, sharing their string literals. */
#ifdef EQUIV
#ifndef NETPLAY
void MenuControl(ConfigData *config, int timeout)
{
    /* The PC menu calls omit the renderer's unused third parameter. */
    typedef void (*MenuTileSubmit)(float depth, POLY_FT4 *poly);
    HiScoreData score;
    int inputDelay;
    short attractMode;
    int idleCount;
    MenuBars bars;
    short i;
    short j;
    int steps;
    int shipIndex;
    int updateCount;

    inputDelay = 2;
    attractMode = 0;
    idleCount = 0;
    InitDynamicMem(reserveHeap, 0x1C2000);
    SetShieldHum(1);
    cardNum.angle = 0;
    menuHeading.angle = 0;
    flipView = 0;
    tempGameData = config;
    redefineCancelled = 0;
    SfxSetReverbDepth(0);
    KillControlledSound();
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    InitMenuDisplay(150, 0);
    for (shipIndex = 0; shipIndex < 15; shipIndex++) {
        InitMatrix(&shipShapeTable[shipIndex]->skeleton->relative);
        InitMatrix(&shipShapeTable[shipIndex]->skeleton->absolute);
        shipShapeTable[shipIndex]->skeleton->update = 1;
    }
    UpdateNotes(0);
    RaceFrameStart();
    menuDisplayActive = 1;
    menu = DAlloc(reserveHeap, 0x4B6E);
    menuLayout = DAlloc(reserveHeap, 0xBA98);
    InitMenus();
    SetupUnlocks();
    if (!raceFlag) {
        if (attractSettingsSaved) {
            ownShip = menuAttractShip;
            config->actTrackNum = attractActTrackNum;
            config->trackNum = attractTrackNum;
            trackNo = attractTrack;
        } else {
            config->trackNum = 0;
            trackNo = 0;
            gameType = 99;
            ownShip = 9;
            config->actTrackNum = raceTracks[0];
        }
    } else {
        raceFlag = 0;
    }
    menu[1].selection = savedRaceSelection;
    menu[4].selection = savedTeamSelection;
    menu[2].selection = savedClassSelection;
    if (config->shipClass == 0) menu[5].selection = savedTrackSelection;
    if (config->shipClass == 1) menu[6].selection = savedTrackSelection;
    if (config->shipClass == 2) menu[7].selection = savedTrackSelection;
    if (config->shipClass == 3) menu[8].selection = savedTrackSelection;
    menu[0].selection = savedMainSelection;
    switch (savedRaceSelection) {
    case 0: gameType = 0; break;
    case 1: gameType = 1; break;
    case 2:
        /* Both branches store 2 (the exe tests rapierComplete and stores 2 either way; the
           original presumably meant a different value for one of them). */
        if (config->rapierComplete) gameType = 2;
        else gameType = 2;
        break;
    }
    SetShake(0);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 2; j++) {
            SetPolyF4(&bars.options[i][j]);
            bars.options[i][j].r0 = 0;
            bars.options[i][j].g0 = 0;
            bars.options[i][j].b0 = 46;
        }
    }
    InitMenuState(config);
    Init2DParticle();
    InitMenuBackground();
    for (i = 0; i < 2; i++) {
        SetPolyF4(&menuBars[i]);
        if (rgbTextures) menuBars[i].r0 = 255;
        else menuBars[i].r0 = 99;
        menuBars[i].g0 = 255;
        menuBars[i].b0 = 255;
    }
    LoadMenuModel(teamShipModels, "cntrl");
    LoadMenuLineData();
    NormaliseMenuBoxes();
    LoadModel(menuModels, "harry");
    LoadModel(shipModels, "june");
    if (gameType == 2) {
        config->shipClass = config->savedClass;
        config->actTrackNum = config->savedActTrackNum;
        config->trackNum = config->savedTrackNum;
    }
    InitScoreTabData(&score);
    score.nameEndX = 180;
    SfxSetCdVolume(config->musicVolume * 255 / 100);
    if (config->cdTrack == 1) {
        cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
        cdMusicTrack = cdMusicTable[cdMusicIndex];
        CdMusicPlay(cdMusicTrack, 0);
    } else {
        CdMusicPlay(config->cdTrack, 0);
    }
    ResetFrameClock();
    raceInProgress = 2;
    InitMenuBackPolys();
    SfxSetVolume(config->effectVolume * 255 / 100);
    joyPressed = -1;
    menuKeys = 0xFFFF;
    padPressed = 0xFFFF;
    PollKeyboard();
    while (!flipView) {
        if (quitRequested == 1) break;
        menuButtonCount = 0;
        PollKeyboard();
        quadIndex = CurrentScreen * 300;
        if (pauseRequested == 1) {
            textDepth = 0.05f;
            DrawMenuBox(120, 160, 520, 300, 0.06f, 0.07f);
            CentreText("ARE YOU SURE YOU", 77, 95, 8, 0);
            CentreText("WANT TO QUIT ?", 105, 105, 8, 0);
            if (keysPressed & MENU_KEY_LEFT) {
                if (!quitMenuSelection) PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                quitMenuSelection = 1;
            }
            if (keysPressed & MENU_KEY_RIGHT) {
                if (quitMenuSelection) PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                quitMenuSelection = 0;
            }
            if (keysPressed & MENU_KEY_BACK) {
                quitMenuSelection = 0;
                pauseRequested = 0;
                prevMenuHeld = MENU_KEY_BACK;
                menuHeld = MENU_KEY_BACK;
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            }
            if (keysPressed & MENU_KEY_ACCEPT) {
                if (quitMenuSelection) {
                    SfxSetVolume(0);
                    raceInProgress = 0;
                    quitRequested = 1;
                    PostQuitMessage(0);
                } else {
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                    pauseRequested = 0;
                    prevMenuHeld = MENU_KEY_ACCEPT;
                    menuHeld = MENU_KEY_ACCEPT;
                }
            }
            if (quitMenuSelection) {
                if (blinkFlag) AddText("YES", 75, 120, 8, 1);
                else AddText("YES", 75, 120, 8, 0);
                AddText("NO", 220, 120, 8, 0);
            } else {
                AddText("YES", 75, 120, 8, 0);
                if (blinkFlag) AddText("NO", 220, 120, 8, 1);
                else AddText("NO", 220, 120, 8, 0);
            }
            textDepth = 0.1f;
        }
        if (rgbTextures) {
            for (i = 0; i < 14; i++) DrawScaledTexturedQuad(0.99f, &menuBackPolys[i], 0);
        } else {
            for (i = 0; i < 14; i++) DrawScaledTexturedQuad(0.98f, &menuBackPolys[i], 0);
        }
        Update2DParticle();
        for (i = 0; i < menuTileCount; i++) ((MenuTileSubmit)DrawPixelTexturedQuad)(0.99f, &menuTiles[i]);
        if (!menuSlideActive) {
            ShowMenu(config, &bars);
            switch (currMenu) {
            case 4: TeamSelect(config); break;
            case 5: ClassSelect(config); break;
            case 6: ClassSelect(config); break;
            case 7: ClassSelect(config); break;
            case 8: ClassSelect(config); break;
            case 9: AudioConfig(config); break;
            case 10: SelectController(config); break;
            case 11: DisplayConfig(config, &score); break;
            case 12: LoadSave(config); break;
            case 13: DispHiScores(config, 4); break;
            case 14: DispHiScores(config, 5); break;
            case 15: RedefineController(config); break;
            }
        } else {
            SlideScreen();
        }
        if (inputDelay > 0) {
            steps = 2;
            inputDelay--;
        } else steps = FR60;
        cardNum.angle -= steps * 30;
        menuHeading.angle -= steps * 15;
        RaceFrameStart();
        ShakeScreen();
        if (!menuEditorActive) {
            if (GetMenuKey(5) && !menuSlideActive && (currMenu != 15 || redefineStep == 1)
                && !(currMenu == 0 && menu[currMenu].selection == 2 && gameType == 2)) {
                if (currMenu != 12 && currMenu != 15 && currMenu != 13 && currMenu != 14)
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                if (currMenu == 0 && menu[currMenu].selection == 3) {
                    savedRaceSelection = menu[1].selection;
                    flipView = 1;
                    savedTeamSelection = menu[4].selection;
                    savedClassSelection = menu[2].selection;
                    if (config->shipClass == 0) savedTrackSelection = menu[5].selection;
                    if (config->shipClass == 1) savedTrackSelection = menu[6].selection;
                    if (config->shipClass == 2) savedTrackSelection = menu[7].selection;
                    if (config->shipClass == 3) savedTrackSelection = menu[8].selection;
                    savedMainSelection = menu[0].selection;
                } else if (menu[currMenu].numOptions && menuPrevious != 12 && menuPrevious != 13 && menuPrevious != 14) {
                    CopyGameData(config, 1, &score);
                    currMenu = menu[currMenu].options[menu[currMenu].selection].nextMenu;
                    CopyGameData(config, 0, &score);
                    if (currMenu == 4) InitMenuDisplay(110, 0);
                    else InitMenuDisplay(150, 0);
                    menuSlideX = 0;
                    menuSlideY = 0;
                    menuSlideFrame = 0;
                    menuSlideActive = 1;
                    menuSlideStep = 0;
                } else if (currMenu != 12 && menuPrevious != 12 && currMenu != 13 && menuPrevious != 13 && currMenu != 14 && menuPrevious != 14) {
                    CopyGameData(config, 1, &score);
                    if (currMenu == 10 && menuState.current) currMenu = 15;
                    else currMenu = menu[currMenu].nextMenu;
                    CopyGameData(config, 0, &score);
                    menuSlideX = 0;
                    menuSlideY = 0;
                    menuSlideStep = 0;
                    menuSlideFrame = 0;
                    menuSlideActive = 1;
                    if (currMenu == 4) InitMenuDisplay(110, 0);
                    else InitMenuDisplay(150, 0);
                }
            }
            if (redefineCancelled) {
                PlayNote(SAMPLE_MENU_SLIDE, 255, 15, 15, 256);
                CopyGameData(config, 1, &score);
                currMenu = menu[currMenu].prevMenu;
                CopyGameData(config, 0, &score);
                menuSlideX = 0;
                menuSlideY = 0;
                menuSlideFrame = 0;
                menuSlideStep = 0;
                redefineCancelled = 0;
                menuSlideActive = 1;
                if (currMenu == 4) InitMenuDisplay(110, 0);
                else InitMenuDisplay(150, 0);
            } else if (GetMenuKey(4)) {
                if (currMenu != 0) {
                    if (menu[currMenu].prevMenu != 999 && currMenu != 12 && currMenu != 15) {
                        PlayNote(SAMPLE_MENU_SLIDE, 255, 15, 15, 256);
                        CopyGameData(config, 0, &score);
                        currMenu = menu[currMenu].prevMenu;
                        if (currMenu == 4) InitMenuDisplay(110, 0);
                        else InitMenuDisplay(150, 0);
                        menuSlideX = 0;
                        menuSlideY = 0;
                        menuSlideStep = 0;
                        menuSlideFrame = 0;
                        menuSlideActive = 1;
                    }
                } else {
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                    quitMenuSelection = 0;
                    pauseRequested ^= 1;
                }
            }
            menuPrevious = 0;
        }
        if (currMenu == 0 && !pauseRequested) {
            if (padButtons) idleCount = 0;
            else idleCount += FR60;
            if (idleCount >= 1800) {
                attractMode = 1;
                flipView = 1;
            }
        }
    }
    raceInProgress = 0;
    if (gameType == 1) {
        cheatWeapon = 0;
        cheatDamage = 0;
        cheatRain = 0;
        cheatTime = 0;
        cheatDrone = 0;
    }
    ownShip = menuState.ship;
    if (gameType != 2) continuesLeft = 0;
    else {
        continuesLeft = 3;
        config->savedClass = config->shipClass;
        config->savedActTrackNum = config->actTrackNum;
        config->savedTrackNum = config->trackNum;
    }
    config->class1Race = 0;
    config->class2Race = 0;
    DrawSync(0);
    SetDefDispEnv(&DispEnv0, 0, 0, 320, 240);
    SetGeomOffset(160, 120);
    SetGeomScreen(160);
    PutDispEnvStub(g_menuDisplayEnv);
    CdMusicStop();
    SfxSetVolume(0);
    Close2DParticle();
    if (quitRequested != 1) {
        if (attractMode) {
            attractSettingsSaved = 1;
            attractTrackNum = config->trackNum;
            savedRaceSelection = menu[1].selection;
            gameType = 99;
            attractTrack = trackNo;
            raceContext = 1;
            quitRequested = 3;
            menuAttractShip = ownShip;
            attractActTrackNum = config->actTrackNum;
        } else {
            /* The menu cleanup calls omit UpdateNotes' parameter in the PC binary. */
            for (updateCount = 5; updateCount != 0; updateCount--) ((void (*)())UpdateNotes)();
            quitRequested = 2;
            raceContext = 0;
        }
    } else raceContext = 1;
    menuDisplayActive = 0;
    SetShieldHum(0);
}
#else
void MenuControl(PcGameData *config, int timeout)
{
    /* The PC menu calls omit the renderer's unused third parameter. */
    typedef void (*MenuTileSubmit)(float depth, POLY_FT4 *poly);
    struct MenuControlBuffers {
        unsigned char menuStorage[0xc0];
        short tileIndex;
        int shipIndex;
        int unusedMenuValue;
        int unusedMenuCount;
        POLY_F4 options[8][2];
        unsigned char menuTitlePolys[0x50]; /* NET frame storage; this function never reads the title polys. */
    };
    int menuDelay;
    int frameDelta;
    int menuTexture;
    short demoStatus;
    int menuStatus;
    HiScoreData currentScore;
    struct MenuControlBuffers menuDataInfo;
    int idleCount;

    menuDelay = 2;
    menuDataInfo.unusedMenuCount = 0;
    menuDataInfo.unusedMenuValue = 0;
    menuStatus = 1;
    idleCount = 0;
    demoStatus = 0;
    InitDynamicMem(reserveHeap, 0x1C2000);
    SetShieldHum(1);
    cardNum.angle = 0;
    gstSessionScanTime = 0;
    D_4d8164 = 0;
    flipView = 0;
    menuHeading.angle = 0;
    tempGameData = config;
    redefineCancelled = 0;
    SfxSetReverbDepth(0);
    KillControlledSound();
    SetSkeletonPosition(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonDirection(residentSkeleton.camPos, 0, 0, 0);
    SetSkeletonPosition(upperSkeleton, 0, 0, 0);
    SetSkeletonDirection(upperSkeleton, 0, 0, 0);
    InitMenuDisplay(150, 0);
    for (menuDataInfo.shipIndex = 0; menuDataInfo.shipIndex < 15; menuDataInfo.shipIndex++) {
        InitMatrix(&shipShapeTable[menuDataInfo.shipIndex]->skeleton->relative);
        InitMatrix(&shipShapeTable[menuDataInfo.shipIndex]->skeleton->absolute);
        shipShapeTable[menuDataInfo.shipIndex]->skeleton->update = 1;
    }
    UpdateNotes(0);
    RaceFrameStart();
    menuDisplayActive = 1;
    menu = DAlloc(reserveHeap, 0x6194);
    menuLayout = DAlloc(reserveHeap, 0xF170);
    InitMenus();
    SetupUnlocks();
    if (!raceFlag) {
        if (attractSettingsSaved) {
            ownShip = menuAttractShip;
            config->actTrackNum = attractActTrackNum;
            config->trackNum = attractTrackNum;
            trackNo = attractTrack;
        } else {
            ownShip = 9;
            trackNo = 0;
            config->trackNum = trackNo;
            config->actTrackNum = raceTracks[config->trackNum];
            gameType = 99;
        }
    } else {
        raceFlag = 0;
    }
    menu[1].selection = savedRaceSelection;
    menu[4].selection = savedTeamSelection;
    menu[2].selection = savedClassSelection;
    if (config->shipClass == 0) menu[5].selection = savedTrackSelection;
    if (config->shipClass == 1) menu[6].selection = savedTrackSelection;
    if (config->shipClass == 2) menu[7].selection = savedTrackSelection;
    if (config->shipClass == 3) menu[8].selection = savedTrackSelection;
    menu[0].selection = savedMainSelection;
    gameType = 0;
    switch (menu[1].selection) {
    case 0: gameType = 0; break;
    case 1: gameType = 1; break;
    case 2:
        /* Both branches store 2 (the exe tests rapierComplete and stores 2 either way; the
           original presumably meant a different value for one of them). */
        if ((char)config->rapierComplete) gameType = 2;
        else gameType = 2;
        break;
    }
    SetShake(0);
    InitMenuBackdropPolys(menuDataInfo.options);
    InitMenuState(config);
    Init2DParticle();
    InitMenuBars(&menuDataInfo);
    menuTexture = textureTable[D_4d8208];
    LoadMenuModel(teamShipModels, "cntrl");
    LoadMenuLineData();
    NormaliseMenuBoxes();
    LoadModel(menuModels, "harry");
    LoadModel(shipModels, "june");
    if (gameType == 2) {
        config->shipClass = config->savedClass;
        config->actTrackNum = config->savedActTrackNum;
        config->trackNum = config->savedTrackNum;
    }
    InitScoreTabData(&currentScore);
    currentScore.nameEndX = 180;
    SfxSetCdVolume(config->musicVolume * 255 / 100);
    if (config->cdTrack == 1) {
        cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
        cdMusicTrack = cdMusicTable[cdMusicIndex];
        CdMusicPlay(cdMusicTrack, 0);
    } else {
        CdMusicPlay(config->cdTrack, 0);
    }
    if (networkGame) {
        tempGameData->trackCheat = 1;
        menu[2].options[3].enabled = 1;
    }
    ResetFrameClock();
    raceInProgress = 2;
    InitMenuBackPolys();
    SfxSetVolume(config->effectVolume * 255 / 100);
    joyPressed = 0xffff;
    menuKeys = 0xFFFF;
    padPressed = 0xFFFF;
    PollKeyboard();
    while (!flipView) {
        if (quitRequested == 1) break;
        menuButtonCount = 0;
        PollKeyboard();
        quadIndex = CurrentScreen * 300;
        if (pauseRequested == 1) {
            textDepth = 0.05f;
            DrawMenuBox(120, 160, 520, 300, 0.06f, 0.07f);
            CentreText("ARE YOU SURE YOU", 77, 95, 8, 0);
            CentreText("WANT TO QUIT ?", 105, 105, 8, 0);
            if (keysPressed & MENU_KEY_LEFT) {
                if (!quitMenuSelection) PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                quitMenuSelection = 1;
            }
            if (keysPressed & MENU_KEY_RIGHT) {
                if (quitMenuSelection) PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                quitMenuSelection = 0;
            }
            if (keysPressed & MENU_KEY_BACK) {
                quitMenuSelection = 0;
                pauseRequested = 0;
                prevMenuHeld = MENU_KEY_BACK;
                menuHeld = MENU_KEY_BACK;
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            }
            if (keysPressed & MENU_KEY_ACCEPT) {
                if (quitMenuSelection) {
                    SfxSetVolume(0);
                    raceInProgress = 0;
                    quitRequested = 1;
                    PostQuitMessage(0);
                } else {
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                    pauseRequested = 0;
                    prevMenuHeld = MENU_KEY_ACCEPT;
                    menuHeld = MENU_KEY_ACCEPT;
                }
            }
            if (quitMenuSelection) {
                if (blinkFlag) AddText("YES", 75, 120, 8, 1);
                else AddText("YES", 75, 120, 8, 0);
                AddText("NO", 220, 120, 8, 0);
            } else {
                AddText("YES", 75, 120, 8, 0);
                if (blinkFlag) AddText("NO", 220, 120, 8, 1);
                else AddText("NO", 220, 120, 8, 0);
            }
            textDepth = 0.1f;
        }
        DrawMenuBackPolys();
        Update2DParticle();
        for (menuDataInfo.tileIndex = 0; menuDataInfo.tileIndex < menuTileCount; menuDataInfo.tileIndex++) ((MenuTileSubmit)DrawPixelTexturedQuad)(0.99f, &menuTiles[menuDataInfo.tileIndex]);
        if (!menuSlideActive) {
            ShowMenu(config, (MenuBars *)menuDataInfo.options);
            RunMenuScreen(config, &currentScore);
        } else {
            SlideScreen();
        }
        if (menuDelay > 0) {
            frameDelta = 2;
            menuDelay--;
        } else frameDelta = FR60;
        cardNum.angle -= frameDelta * 30;
        menuHeading.angle -= frameDelta * 15;
        RaceFrameStart();
        ShakeScreen();
        if (!menuEditorActive) {
            if (GetMenuKey(5) && !menuSlideActive && (currMenu != 15 || redefineStep == 1)
                && !(currMenu == 0 && menu[currMenu].selection == 2 && gameType == 2)) {
                if (currMenu != 12 && currMenu != 15 && currMenu != 13 && currMenu != 14
                    && currMenu != 17 && currMenu != 18 && currMenu != 19 && currMenu != 20 && currMenu != 21)
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                if (currMenu == 0 && menu[currMenu].selection == 3) {
                    if (networkGame) {
                        if (gstLaunchMode == 0) {
                            gstPlayerNameLength = 0;
                            gstSessionNameLength = 0;
                            gstOptionsSelected = 0;
                            currMenu = 17;
                            InitSlider();
                        } else if (gstLaunchMode == 2) {
                            gstIsHost = 1;
                            sprintf(gstSessionName, "LAUNCHED SESSION");
                            currMenu = 18;
                            CopyGameData(0, 0, 0);
                            InitSlider();
                        } else if (gstLaunchMode == 1 && JoinSessionClient(0)) {
                            currMenu = 21;
                            CopyGameData(0, 0, 0);
                            InitSlider();
                        }
                    } else {
                        flipView = 1;
                        savedRaceSelection = menu[1].selection;
                        savedTeamSelection = menu[4].selection;
                        savedClassSelection = menu[2].selection;
                        if (config->shipClass == 0) savedTrackSelection = menu[5].selection;
                        if (config->shipClass == 1) savedTrackSelection = menu[6].selection;
                        if (config->shipClass == 2) savedTrackSelection = menu[7].selection;
                        if (config->shipClass == 3) savedTrackSelection = menu[8].selection;
                        savedMainSelection = menu[0].selection;
                    }
                } else if (menu[currMenu].numOptions && menuPrevious != 12 && menuPrevious != 13 && menuPrevious != 14) {
                    CopyGameData(config, 1, &currentScore);
                    currMenu = menu[currMenu].options[menu[currMenu].selection].nextMenu;
                    CopyGameData(config, 0, &currentScore);
                    if (currMenu == 4) InitMenuDisplay(110, 0);
                    else InitMenuDisplay(150, 0);
                    InitSlider();
                } else if (currMenu != 17 && currMenu != 18 && currMenu != 19 && currMenu != 20 && currMenu != 21
                    && currMenu != 12 && menuPrevious != 12 && currMenu != 13 && menuPrevious != 13 && currMenu != 14 && menuPrevious != 14) {
                    CopyGameData(config, 1, &currentScore);
                    if (currMenu == 10 && menuState.current) currMenu = 15;
                    else currMenu = menu[currMenu].nextMenu;
                    CopyGameData(config, 0, &currentScore);
                    InitSlider();
                    if (currMenu == 4) InitMenuDisplay(110, 0);
                    else InitMenuDisplay(150, 0);
                }
            }
            if (redefineCancelled) {
                PlayNote(SAMPLE_MENU_SLIDE, 255, 15, 15, 256);
                CopyGameData(config, 1, &currentScore);
                currMenu = menu[currMenu].prevMenu;
                CopyGameData(config, 0, &currentScore);
                InitSlider();
                redefineCancelled = 0;
                if (currMenu == 4) InitMenuDisplay(110, 0);
                else InitMenuDisplay(150, 0);
            } else if (GetMenuKey(4)) {
                if (currMenu != 0) {
                    if (menu[currMenu].prevMenu != 999 && currMenu != 12 && currMenu != 15 && currMenu != 17 && currMenu != 18 && currMenu != 19 && currMenu != 20 && currMenu != 21) {
                        PlayNote(SAMPLE_MENU_SLIDE, 255, 15, 15, 256);
                        CopyGameData(config, 0, &currentScore);
                        currMenu = menu[currMenu].prevMenu;
                        if (currMenu == 4) InitMenuDisplay(110, 0);
                        else InitMenuDisplay(150, 0);
                        InitSlider();
                    }
                } else {
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                    quitMenuSelection = 0;
                    pauseRequested ^= 1;
                }
            }
            menuPrevious = 0;
        }
        if (currMenu == 0 && !pauseRequested) {
            if (padButtons) idleCount = 0;
            else idleCount += FR60;
            if (idleCount >= 1800 && !networkGame) {
                flipView = 1;
                demoStatus = 1;
            }
        }
    }
    raceInProgress = 0;
    if (gameType == 1) {
        cheatWeapon = 0;
        cheatDamage = 0;
        cheatRain = 0;
        cheatTime = 0;
        cheatDrone = 0;
    }
    if (!networkGame) ownShip = menuState.ship;
    if (gameType == 2) {
        config->savedClass = config->shipClass;
        config->savedActTrackNum = config->actTrackNum;
        config->savedTrackNum = config->trackNum;
        continuesLeft = 3;
    } else continuesLeft = 0;
    config->class1Race = 0;
    config->class2Race = 0;
    DrawSync(0);
    ((void (*)(ConfigData *))InitNewDisplay)(config);
    CdMusicStop();
    SfxSetVolume(0);
    Close2DParticle();
    if (quitRequested != 1) {
        if (demoStatus) {
            attractTrackNum = config->trackNum;
            savedRaceSelection = menu[1].selection;
            demoStatus = 0;
            attractSettingsSaved = 1;
            menuAttractShip = ownShip;
            attractActTrackNum = config->actTrackNum;
            attractTrack = trackNo;
            gameType = 99;
            quitRequested = 3;
            raceContext = 1;
        } else {
            /* The menu cleanup calls omit UpdateNotes' parameter in the PC binary. */
            for (menuDataInfo.shipIndex = 0; menuDataInfo.shipIndex < 5; menuDataInfo.shipIndex++) ((void (*)())UpdateNotes)();
            quitRequested = 2;
            raceContext = 0;
        }
    } else raceContext = 1;
    menuDisplayActive = 0;
    SetShieldHum(0);
}
#endif
#endif

#ifdef NETPLAY
/* Copies the settings between the game data and the menu state when a menu is entered (0) or left (1). */
#pragma intrinsic(strcpy)
void CopyGameData(PcGameData *gameData, short saveSelection, HiScoreData *score)
{
    int i;

    switch (currMenu) {
    case 1:
        if (saveSelection == 0) {
            menu[currMenu].selection = gameType;
        } else {
            switch (menu[currMenu].selection) {
            case 0:
                gameType = 0;
                gstRaceMode = 0;
                break;
            case 1:
                if (networkGame) {
                    gameType = 0;
                    gstRaceMode = 1;
                } else {
                    gameType = 1;
                }
                break;
            case 2:
                if (networkGame) {
                    gameType = 0;
                    gstRaceMode = 3;
                } else if (gameData->piranhaChallenge) {
                    gameData->actTrackNum = 1;
                    gameData->trackNum = 0;
                    gameType = 2;
                    gameData->class2Race = 0;
                } else {
                    gameData->actTrackNum = 1;
                    gameData->trackNum = 0;
                    gameType = 2;
                    gameData->class1Race = 0;
                }
                break;
            }
        }
        break;
    case 2:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.trackNum = gameData->trackNum;
            menu[2].selection = lastClassSelection;
            if (gameData->shipClass == 0)
                menuState.trackSelection = menu[5].selection;
            if (gameData->shipClass == 1)
                menuState.trackSelection = menu[6].selection;
            if (gameData->shipClass == 2)
                menuState.trackSelection = menu[7].selection;
            if (gameData->shipClass == 3)
                menuState.trackSelection = menu[8].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            gameData->trackNum = menuState.trackNum;
            lastClassSelection = menu[2].selection;
            if (gameData->shipClass == 0)
                menu[5].selection = menuState.trackSelection;
            if (gameData->shipClass == 1)
                menu[6].selection = menuState.trackSelection;
            if (gameData->shipClass == 2)
                menu[7].selection = menuState.trackSelection;
            if (gameData->shipClass == 3)
                menu[8].selection = menuState.trackSelection;
        }
        break;
    case 5:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 6:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 7:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 8:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 4:
        if (saveSelection == 0) {
            menuState.ship = ownShip;
            menuState.current = menu[currMenu].selection;
        } else {
            ownShip = menuState.ship;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 9:
        if (saveSelection == 0) {
            menuState.musicVolume = gameData->musicVolume;
            menuState.otherMusic = gameData->otherMusicVolume;
            menuState.effectVolume = gameData->effectVolume;
            menuState.otherEffect = gameData->otherEffectVolume;
            menuState.music = gameData->cdTrack;
            menuState.current = 0;
            SfxSetCdVolume(gameData->musicVolume * 255 / 100);
            SfxSetVolume(gameData->effectVolume * 255 / 100);
            menu[currMenu].selection = 0;
            SetNoteVolume(shieldHumVoice, 0);
            SfxEffectLRPan(shieldHumVoice, 15);
            SfxEffectFBPan(shieldHumVoice, 15);
            TerminateSound();
            StartControlledSound();
            soundChanged = 1;
        } else {
            gameData->musicVolume = menuState.musicVolume;
            gameData->otherMusicVolume = menuState.otherMusic;
            gameData->effectVolume = menuState.effectVolume;
            gameData->otherEffectVolume = menuState.otherEffect;
            gameData->cdTrack = menuState.music;
            menuState.current = 0;
            menu[currMenu].selection = 0;
            SfxSetCdVolume(gameData->musicVolume * 255 / 100);
            SfxSetVolume(gameData->effectVolume * 255 / 100);
            SetNoteVolume(shieldHumVoice, 0);
            SfxEffectLRPan(shieldHumVoice, 15);
            SfxEffectFBPan(shieldHumVoice, 15);
            TerminateSound();
            StartControlledSound();
            soundChanged = 1;
        }
        break;
    case 10:
        if (saveSelection == 0) {
            deviceType = savedDeviceType;
            joystickPresent = savedDeviceType & 1;
            deviceChosen = joystickPresent;
            mouseEnabled = (savedDeviceType & 2) >> 1;
            menu[10].selection = 0;
            menuState.current = 0;
        } else {
            savedDeviceType = deviceType;
            joystickPresent = savedDeviceType & 1;
            mouseEnabled = (savedDeviceType & 2) >> 1;
        }
        break;
    case 15:
        if (saveSelection == 0) {
            redefineStep = 0;
            redefineKey = 0;
            g_keyboardCode = -1;
            for (i = 0; i < 10; i++)
                keyboardKeys[i] = gameKeys[i];
            for (i = 0; i < 6; i++) {
                joystickButtons[i] = keyBindings[i];
                mouseButtons[i] = padBindings[i];
            }
        } else if (redefineCancelled == 0) {
            for (i = 0; i < 10; i++)
                gameKeys[i] = keyboardKeys[i];
            for (i = 0; i < 6; i++) {
                keyBindings[i] = joystickButtons[i];
                padBindings[i] = mouseButtons[i];
            }
        }
        break;
    case 11:
        if (saveSelection == 0) {
            menuState.weapons = gameData->weaponsOn;
            menuState.internalView = gameData->internalView;
            strcpy(menuState.name, gameData->name);
            strcpy(score->nameEntered, menuState.name);
            menuState.current = 0;
            menu[currMenu].selection = 0;
            InitDefaultName(gameData, score, 1);
        } else {
            gameData->weaponsOn = menuState.weapons;
            gameData->internalView = menuState.internalView;
            strcpy(menuState.name, score->nameEntered);
            strcpy(gameData->name, menuState.name);
            menu[currMenu].selection = 0;
            menuState.current = 0;
            InitMenuDisplay(120, 0);
        }
        break;
    case 12:
        if (saveSelection == 0)
            saveState = 0;
        break;
    case 3:
    case 13:
    case 14:
        break;
    }
}
#pragma function(strcpy)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Copies the settings between the game data and the menu state when a menu is entered (0) or left (1). */
void CopyGameData(PcGameData *gameData, short saveSelection, HiScoreData *score)
{
    int i;
    int *source;
    int *destination;
    int *source2;
    int *destination2;

    switch (currMenu) {
    case 1:
        if (saveSelection == 0) {
            menu[currMenu].selection = gameType;
        } else {
            switch (menu[currMenu].selection) {
            case 0:
                gameType = 0;
                break;
            case 1:
                gameType = 1;
                break;
            case 2:
                gameType = 2;
                gameData->trackNum = 0;
                gameData->actTrackNum = 1;
                if (gameData->rapierComplete)
                    gameData->class2Race = 0;
                else
                    gameData->class1Race = 0;
                break;
            }
        }
        break;
    case 2:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.trackNum = gameData->trackNum;
            menu[2].selection = lastClassSelection;
            if (gameData->shipClass == 0)
                menuState.trackSelection = menu[5].selection;
            if (gameData->shipClass == 1)
                menuState.trackSelection = menu[6].selection;
            if (gameData->shipClass == 2)
                menuState.trackSelection = menu[7].selection;
            if (gameData->shipClass == 3)
                menuState.trackSelection = menu[8].selection;
        } else {
            /* Keeps the selected class as a byte while saving and restoring the menu state. */
            u_char selectedClass = menuState.shipClass;

            gameData->shipClass = selectedClass;
            gameData->actTrackNum = menuState.actTrackNum;
            gameData->trackNum = menuState.trackNum;
            lastClassSelection = menu[2].selection;
            if (selectedClass == 0)
                menu[5].selection = menuState.trackSelection;
            if (selectedClass == 1)
                menu[6].selection = menuState.trackSelection;
            if (selectedClass == 2)
                menu[7].selection = menuState.trackSelection;
            if (selectedClass == 3)
                menu[8].selection = menuState.trackSelection;
        }
        break;
    case 4:
        if (saveSelection == 0) {
            menuState.ship = ownShip;
            menuState.current = menu[currMenu].selection;
        } else {
            ownShip = menuState.ship;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 5:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 6:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 7:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 8:
        if (saveSelection == 0) {
            menuState.shipClass = gameData->shipClass;
            menuState.actTrackNum = gameData->actTrackNum;
            menuState.current = menu[currMenu].selection;
        } else {
            gameData->shipClass = menuState.shipClass;
            gameData->actTrackNum = menuState.actTrackNum;
            menu[currMenu].selection = menuState.current;
        }
        break;
    case 9:
        if (saveSelection == 0) {
            menuState.musicVolume = gameData->musicVolume;
            menuState.otherMusic = gameData->otherMusicVolume;
            menuState.effectVolume = gameData->effectVolume;
            menuState.otherEffect = gameData->otherEffectVolume;
            menuState.music = gameData->cdTrack;
            menuState.current = 0;
            SfxSetCdVolume(gameData->musicVolume * 255 / 100);
            SfxSetVolume(gameData->effectVolume * 255 / 100);
            menu[currMenu].selection = 0;
            SetNoteVolume(shieldHumVoice, 0);
            SfxEffectLRPan(shieldHumVoice, 15);
            SfxEffectFBPan(shieldHumVoice, 15);
            TerminateSound();
            StartControlledSound();
            soundChanged = 1;
        } else {
            gameData->musicVolume = menuState.musicVolume;
            gameData->otherMusicVolume = menuState.otherMusic;
            gameData->effectVolume = menuState.effectVolume;
            gameData->otherEffectVolume = menuState.otherEffect;
            gameData->cdTrack = menuState.music;
            menuState.current = 0;
            menu[currMenu].selection = 0;
            SfxSetCdVolume(gameData->musicVolume * 255 / 100);
            SfxSetVolume(gameData->effectVolume * 255 / 100);
            SetNoteVolume(shieldHumVoice, 0);
            SfxEffectLRPan(shieldHumVoice, 15);
            SfxEffectFBPan(shieldHumVoice, 15);
            TerminateSound();
            StartControlledSound();
            soundChanged = 1;
        }
        break;
    case 10:
        if (saveSelection == 0) {
            deviceType = savedDeviceType;
            joystickPresent = deviceType & 1;
            deviceChosen = joystickPresent;
            mouseEnabled = (deviceType & 2) >> 1;
            menu[10].selection = 0;
            menuState.current = 0;
        } else {
            savedDeviceType = deviceType;
            joystickPresent = deviceType & 1;
            mouseEnabled = (deviceType & 2) >> 1;
        }
        break;
    case 11:
        if (saveSelection == 0) {
            menuState.weapons = gameData->weaponsOn;
            menuState.internalView = gameData->internalView;
            strcpy(nameBuffer, gameData->name);
            strcpy(score->nameEntered, nameBuffer);
            menuState.current = 0;
            menu[currMenu].selection = 0;
            if (strlen(gameData->name) != 0)
                strcpy(score->nameEntered, gameData->name);
            score->nameCursor = 0;
        } else {
            gameData->weaponsOn = menuState.weapons;
            gameData->internalView = menuState.internalView;
            strcpy(nameBuffer, score->nameEntered);
            strcpy(gameData->name, nameBuffer);
            menu[currMenu].selection = 0;
            menuState.current = 0;
            InitMenuDisplay(120, 0);
        }
        break;
    case 12:
        if (saveSelection == 0)
            saveState = 0;
        break;
    case 15:
        if (saveSelection == 0) {
            redefineStep = 0;
            redefineKey = 0;
            g_keyboardCode = -1;
            source = gameKeys;
            destination = keyboardKeys;
            for (i = 0; i < 10; i++)
                destination[i] = source[i];
            source = keyBindings;
            destination = joystickButtons;
            source2 = padBindings;
            destination2 = mouseButtons;
            for (i = 0; i < 6; i++) {
                destination[i] = source[i];
                destination2[i] = source2[i];
            }
        } else if (redefineCancelled == 0) {
            source = keyboardKeys;
            destination = gameKeys;
            for (i = 0; i < 10; i++)
                destination[i] = source[i];
            source = joystickButtons;
            destination = keyBindings;
            source2 = mouseButtons;
            destination2 = padBindings;
            for (i = 0; i < 6; i++) {
                destination[i] = source[i];
                destination2[i] = source2[i];
            }
        }
        break;
    }
}
#endif
#endif

/* The game data record as the name screens read it (the player's name is at 0x1f). */
typedef ConfigData PcNameData;

/* Starts a name entry from the player's saved name, if there is one. */
#ifdef NETPLAY
#pragma intrinsic(strlen, strcpy)
#endif
/* The callers pass a third argument (0 or 1) that the routine never reads. */
void InitDefaultName(PcNameData *gameData, HiScoreData *data, int unused)
{
    if (strlen(gameData->name) != 0)
        strcpy(data->nameEntered, gameData->name);
    data->nameCursor = 0;
}
#ifdef NETPLAY
#pragma function(strlen, strcpy)
#endif

extern short menuTileCount;
extern POLY_FT4 menuTiles[];
extern POLY_F4 menuBars[2];

void InitMenuBackground(void);

/* Sets up the menu background tiles and the two white bars drawn over them. */
void InitMenuBars(void)
{
    short i;

    InitMenuBackground();
    for (i = 0; i < 2; i++) {
        SetPolyF4(&menuBars[i]);
        if (rgbTextures)
            menuBars[i].r0 = 255;
        else
            menuBars[i].r0 = 99;
        menuBars[i].g0 = 255;
        menuBars[i].b0 = 255;
    }
}

/* Same computation as the exe; it keeps the column and parity counters on the stack and a zero in ebx. */
/* Builds the textured tiles of the menu background, 64x60 pixels each, scaled to the
   screen and laid out in rows with alternating texture halves. */
#ifdef NETPLAY
/* Builds the textured tiles of the menu background, 64x60 pixels each, scaled to the
   screen and laid out in rows with alternating texture halves. */
void InitMenuBackground(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short colIndex;
    short rowIndex;
    short curColX;
    short lastTop;
    short localSwap;
    short parity;
    short nextW;
    short lastTileH;
    short curTilesX;
    short outVertical;

    curColX = 0;
    lastTop = 0;
    nextW = 0x40;
    lastTileH = 0x3c;
    parity = 1;
    localSwap = 1;
    curTilesX = (short)(screenScaleX / 64.0f + 0.5f);
    outVertical = (short)(screenScaleY / 60.0f + 0.5f);
    menuTileCount = curTilesX * outVertical;
    for (rowIndex = 0; outVertical > rowIndex; rowIndex++) {
        curColX = 0;
        if (localSwap == 0)
            parity = 1;
        else
            parity = 0;
        localSwap = parity;
        for (colIndex = 0; curTilesX > colIndex; colIndex++) {
            SetPolyFT4(&menuTiles[curTilesX * rowIndex + colIndex]);
            if (rgbTextures == 0) {
                menuTiles[curTilesX * rowIndex + colIndex].r0 = 0x60;
            } else if (mmxAvailable) {
                menuTiles[curTilesX * rowIndex + colIndex].r0 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].g0 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].b0 = 0x2e;
            } else {
                menuTiles[curTilesX * rowIndex + colIndex].r0 = 0x80;
                menuTiles[curTilesX * rowIndex + colIndex].g0 = 0x80;
                menuTiles[curTilesX * rowIndex + colIndex].b0 = 0x80;
            }
            if (texture16Bit == 0)
                menuTiles[curTilesX * rowIndex + colIndex].tpage = 0x24;
            else
                menuTiles[curTilesX * rowIndex + colIndex].tpage = 5;
            menuTiles[curTilesX * rowIndex + colIndex].x0 = curColX;
            menuTiles[curTilesX * rowIndex + colIndex].y0 = lastTop;
            menuTiles[curTilesX * rowIndex + colIndex].x1 = curColX + nextW;
            menuTiles[curTilesX * rowIndex + colIndex].y1 = lastTop;
            menuTiles[curTilesX * rowIndex + colIndex].x2 = curColX;
            menuTiles[curTilesX * rowIndex + colIndex].y2 = lastTop + lastTileH;
            menuTiles[curTilesX * rowIndex + colIndex].x3 = curColX + nextW;
            menuTiles[curTilesX * rowIndex + colIndex].y3 = lastTop + lastTileH;
            if (parity % 2 == 0) {
                menuTiles[curTilesX * rowIndex + colIndex].u0 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].v0 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].u1 = 0x3f;
                menuTiles[curTilesX * rowIndex + colIndex].v1 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].u2 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].v2 = 0x3b;
                menuTiles[curTilesX * rowIndex + colIndex].u3 = 0x3f;
                menuTiles[curTilesX * rowIndex + colIndex].v3 = 0x3b;
            } else {
                menuTiles[curTilesX * rowIndex + colIndex].u0 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].v0 = 0x3c;
                menuTiles[curTilesX * rowIndex + colIndex].u1 = 0x3f;
                menuTiles[curTilesX * rowIndex + colIndex].v1 = 0x3c;
                menuTiles[curTilesX * rowIndex + colIndex].u2 = 0;
                menuTiles[curTilesX * rowIndex + colIndex].v2 = 0x77;
                menuTiles[curTilesX * rowIndex + colIndex].u3 = 0x3f;
                menuTiles[curTilesX * rowIndex + colIndex].v3 = 0x77;
            }
            parity++;
            curColX = curColX + nextW;
        }
        lastTop = lastTop + lastTileH;
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
void InitMenuBackground(void)
{
    short tilesX;
    short tilesY;
    short row;
    short col;
    short x;
    short y;
    short checker;
    short parity;
    int tileOffset;

    checker = 1;
    y = 0;
    tilesX = (short)(screenScaleX * 0.015625f + 0.5f);
    tilesY = (short)(screenScaleY * 0.016666668f + 0.5f);
    menuTileCount = tilesY * tilesX;
    for (row = 0; row < tilesY; row++) {
        x = 0;
        col = 0;
        tileOffset = row * tilesX;
        parity = (checker == 0);
        checker = parity;
        for (; col < tilesX; col++) {
            SetPolyFT4(&menuTiles[tileOffset + col]);
            if (rgbTextures == 0) {
                menuTiles[tileOffset + col].r0 = 0x60;
            } else if (mmxAvailable) {
                menuTiles[tileOffset + col].r0 = 0;
                menuTiles[tileOffset + col].g0 = 0;
                menuTiles[tileOffset + col].b0 = 0x2e;
            } else {
                menuTiles[tileOffset + col].r0 = 0x80;
                menuTiles[tileOffset + col].g0 = 0x80;
                menuTiles[tileOffset + col].b0 = 0x80;
            }
            menuTiles[tileOffset + col].tpage = 0x24;
            if (texture16Bit)
                menuTiles[tileOffset + col].tpage = 5;
            menuTiles[tileOffset + col].x0 = x;
            menuTiles[tileOffset + col].y0 = y;
            menuTiles[tileOffset + col].x1 = x + 0x40;
            menuTiles[tileOffset + col].y1 = y;
            menuTiles[tileOffset + col].x2 = x;
            menuTiles[tileOffset + col].y2 = y + 0x3c;
            menuTiles[tileOffset + col].x3 = x + 0x40;
            menuTiles[tileOffset + col].y3 = y + 0x3c;
            menuTiles[tileOffset + col].u0 = 0;
            if (parity % 2 == 0) {
                menuTiles[tileOffset + col].v0 = 0;
                menuTiles[tileOffset + col].u1 = 0x3f;
                menuTiles[tileOffset + col].v1 = 0;
                menuTiles[tileOffset + col].u2 = 0;
                menuTiles[tileOffset + col].v2 = 0x3b;
                menuTiles[tileOffset + col].u3 = 0x3f;
                menuTiles[tileOffset + col].v3 = 0x3b;
            } else {
                menuTiles[tileOffset + col].v0 = 0x3c;
                menuTiles[tileOffset + col].u1 = 0x3f;
                menuTiles[tileOffset + col].v1 = 0x3c;
                menuTiles[tileOffset + col].u2 = 0;
                menuTiles[tileOffset + col].v2 = 0x77;
                menuTiles[tileOffset + col].u3 = 0x3f;
                menuTiles[tileOffset + col].v3 = 0x77;
            }
            x += 0x40;
            parity++;
        }
        y += 0x3c;
    }
}
#endif
#endif

extern signed char raceTracks[];
extern short trackModelOrder[];
extern Object *shipModels[];
extern int blinkFlag;
char *GetTrackName(short type, short x, short y, short draw);
void DoButton(short x, short y, short icon, char *text);

#ifdef NETPLAY
/* The two empty labels follow the session title in the literal section. */
void DispHiScores(PcGameData *gameData, char raceMode)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short maxTracks = 6;
    short j;
    short trackNo = 0;
    short originalTrack = 0;

    if (gameData->phantomUnlocked || gameData->trackCheatSigned || gameData->phantomCheatSigned)
        maxTracks = 8;
    if (GetMenuKey(3)) {
        menu[currMenu].selection++;
        if (maxTracks <= menu[currMenu].selection)
            menu[currMenu].selection = maxTracks - 1;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    } else if (GetMenuKey(2)) {
        if (--menu[currMenu].selection < 0)
            menu[currMenu].selection = 0;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (blinkFlag) {
        if (menu[currMenu].selection < maxTracks - 1)
            DoButton(275, 184, 4, "");
        if (menu[currMenu].selection > 0)
            DoButton(30, 184, 3, "");
    }
    trackNo = menu[currMenu].selection;
    PrintCX(GetTrackName(raceTracks[trackNo], 0, 40, 0), 160, 186, 0);
    originalTrack = trackNo;
    if (raceMode == 5)
        trackNo += 8;
    Print("BEST", 30, 90, 0);
    Print("RACE", 30, 97, 0);
    Print("TIME", 30, 104, 0);
    Print("BEST", 30, 135, 0);
    Print("LAP", 30, 142, 0);
    Print("TIME", 30, 149, 0);
    for (j = 0; j < 4; j++) {
        if ((j == 0 || j == 1) && (trackNo == 6 || trackNo == 7 || trackNo == 14 || trackNo == 15)) {
        } else {
            AddText(hiScoreTable[trackNo][j].name, 75, 90 + j * 10, 8, 1);
            AddTime(hiScoreTable[trackNo][j].time, 130, 90 + j * 10, 8, 1, 1);
            AddText(hiScoreTable[trackNo][j].lapName, 75, 135 + j * 10, 8, 1);
            AddTime(hiScoreTable[trackNo][j].lapTime, 130, 135 + j * 10, 8, 1, 1);
        }
        if ((j == 0 || j == 1) && (trackNo == 6 || trackNo == 7 || trackNo == 14 || trackNo == 15)) {
        } else {
            switch (hiScoreTable[trackNo][j].ship / 3) {
            case 0: Print("AG SYSTEMS", 240, 92 + j * 10, 0); break;
            case 1: Print("AURICOM", 240, 92 + j * 10, 0); break;
            case 2: Print("QIREX", 240, 92 + j * 10, 0); break;
            case 3: Print("FEISAR", 240, 92 + j * 10, 0); break;
            case 4: Print("PIRANHA", 240, 92 + j * 10, 0); break;
            case 5: Print("WIPEOUT", 240, 92 + j * 10, 0); break;
            }
        }
        if ((j == 0 || j == 1) && (trackNo == 6 || trackNo == 7 || trackNo == 14 || trackNo == 15)) {
        } else {
            switch (hiScoreTable[trackNo][j].lapShip / 3) {
            case 0: Print("AG SYSTEMS", 240, 137 + j * 10, 0); break;
            case 1: Print("AURICOM", 240, 137 + j * 10, 0); break;
            case 2: Print("QIREX", 240, 137 + j * 10, 0); break;
            case 3: Print("FEISAR", 240, 137 + j * 10, 0); break;
            case 4: Print("PIRANHA", 240, 137 + j * 10, 0); break;
            case 5: Print("WIPEOUT", 240, 137 + j * 10, 0); break;
            }
        }
        switch (j) {
        case 0:
            Print("VECTOR", 190, 92 + j * 10, 0);
            Print("VECTOR", 190, 137 + j * 10, 0);
            break;
        case 1:
            Print("VENOM", 190, 92 + j * 10, 0);
            Print("VENOM", 190, 137 + j * 10, 0);
            break;
        case 2:
            Print("RAPIER", 190, 92 + j * 10, 0);
            Print("RAPIER", 190, 137 + j * 10, 0);
            break;
        case 3:
            Print("PHANTOM", 190, 92 + j * 10, 0);
            Print("PHANTOM", 190, 137 + j * 10, 0);
            break;
        }
    }
    DrawTeamShips(0, 0, 1500, menuHeading.angle, 220, shipModels[trackModelOrder[originalTrack]]);
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* The high-score table screen: the records of one track, with the team of each pilot.
   Proven equivalent rather than byte-exact: the decrement uses di where the exe uses ax, and
   the menu globals reload in a different order. */
void DispHiScores(PcGameData *gameData, unsigned char raceMode)
{
    short i;
    short maxTracks;
    short trackNo;
    short originalTrack;
    signed char ship;

    maxTracks = 6;
    if (gameData->class1Unlocked || gameData->trackCheat || gameData->phantomCheat)
        maxTracks = 8;
    if (GetMenuKey(3)) {
        menu[currMenu].selection++;
        if (maxTracks <= menu[currMenu].selection)
            menu[currMenu].selection = maxTracks - 1;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    } else if (GetMenuKey(2)) {
        if (--menu[currMenu].selection < 0)
            menu[currMenu].selection = 0;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (blinkFlag) {
        if (menu[currMenu].selection < maxTracks - 1)
            DoButton(275, 184, 4, "");
        if (menu[currMenu].selection > 0)
            DoButton(30, 184, 3, "");
    }
    trackNo = menu[currMenu].selection;
    originalTrack = trackNo;
    PrintCX(GetTrackName(raceTracks[trackNo], 0, 40, 0), 160, 186, 0);
    if (raceMode == 5)
        trackNo += 8;
    Print("BEST", 30, 90, 0);
    Print("RACE", 30, 97, 0);
    Print("TIME", 30, 104, 0);
    Print("BEST", 30, 135, 0);
    Print("LAP", 30, 142, 0);
    Print("TIME", 30, 149, 0);
    for (i = 0; i < 4; i++) {
        if (!((i == 0 || i == 1) && (trackNo == 6 || trackNo == 7 || trackNo == 14 || trackNo == 15))) {
            AddText(hiScoreTable[trackNo][i].name, 75, 90 + i * 10, 8, 1);
            AddTime(hiScoreTable[trackNo][i].time, 130, 90 + i * 10, 8, 1, 1);
            AddText(hiScoreTable[trackNo][i].lapName, 75, 135 + i * 10, 8, 1);
            AddTime(hiScoreTable[trackNo][i].lapTime, 130, 135 + i * 10, 8, 1, 1);
        }
        if (!((i == 0 || i == 1) && (trackNo == 6 || trackNo == 7 || trackNo == 14 || trackNo == 15))) {
            ship = hiScoreTable[trackNo][i].ship / 3;
            switch (ship) {
            case 0: Print("AG SYSTEMS", 240, 92 + i * 10, 0); break;
            case 1: Print("AURICOM", 240, 92 + i * 10, 0); break;
            case 2: Print("QIREX", 240, 92 + i * 10, 0); break;
            case 3: Print("FEISAR", 240, 92 + i * 10, 0); break;
            case 4: Print("PIRANHA", 240, 92 + i * 10, 0); break;
            case 5: Print("WIPEOUT", 240, 92 + i * 10, 0); break;
            }
        }
        if (!((i == 0 || i == 1) && (trackNo == 6 || trackNo == 7 || trackNo == 14 || trackNo == 15))) {
            ship = hiScoreTable[trackNo][i].lapShip / 3;
            switch (ship) {
            case 0: Print("AG SYSTEMS", 240, 137 + i * 10, 0); break;
            case 1: Print("AURICOM", 240, 137 + i * 10, 0); break;
            case 2: Print("QIREX", 240, 137 + i * 10, 0); break;
            case 3: Print("FEISAR", 240, 137 + i * 10, 0); break;
            case 4: Print("PIRANHA", 240, 137 + i * 10, 0); break;
            case 5: Print("WIPEOUT", 240, 137 + i * 10, 0); break;
            }
        }
        switch (i) {
        case 0:
            Print("VECTOR", 190, 92 + i * 10, 0);
            Print("VECTOR", 190, 137 + i * 10, 0);
            break;
        case 1:
            Print("VENOM", 190, 92 + i * 10, 0);
            Print("VENOM", 190, 137 + i * 10, 0);
            break;
        case 2:
            Print("RAPIER", 190, 92 + i * 10, 0);
            Print("RAPIER", 190, 137 + i * 10, 0);
            break;
        case 3:
            Print("PHANTOM", 190, 92 + i * 10, 0);
            Print("PHANTOM", 190, 137 + i * 10, 0);
            break;
        }
    }
    DrawTeamShips(0, 0, 1500, menuHeading.angle, 220, shipModels[trackModelOrder[originalTrack]]);
}
#endif
#endif

extern DISPENV DispEnv0;
extern void *g_menuDisplayEnv;

void InitNewDisplay(void)
{
    SetDefDispEnv(&DispEnv0, 0, 0, 0x140, 0xF0);
    SetGeomOffset(0xA0, 0x78);
    SetGeomScreen(0xA0);
    PutDispEnvStub(g_menuDisplayEnv);
}

extern u_short menuKeys;
extern int blinkFlag;


/* The options screen: weapons on or off, default view and default name. */
void DisplayConfig(ConfigData *gameData, HiScoreData *score)
{
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0)
            menuState.current = 0;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current > 2)
            menuState.current = 2;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (menuState.weapons != 0) {
        AddText("ON", 180, 90, 8, 1);
        AddText("OFF", 210, 90, 8, 0);
    } else {
        AddText("ON", 180, 90, 8, 0);
        AddText("OFF", 210, 90, 8, 1);
    }
    if (menuState.internalView != 0) {
        AddText("INT", 181, 110, 8, 1);
        AddText("EXT", 210, 110, 8, 0);
    } else {
        AddText("INT", 181, 110, 8, 0);
        AddText("EXT", 210, 110, 8, 1);
    }
    switch (menuState.current) {
    case 0:
        if (blinkFlag)
            AddText("WEAPONS", 30, 90, 8, 1);
        AddText("DEFAULT VIEW", 30, 110, 8, 0);
        AddText("DEFAULT NAME", 30, 130, 8, 0);
        score->enteringName = 0;
        EnterName(gameData, score);
        if (menuKeys & MENU_KEY_RIGHT) {
            if (menuState.weapons != 0)
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            menuState.weapons = 0;
        }
        if (menuKeys & MENU_KEY_LEFT) {
            if (menuState.weapons == 0)
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            menuState.weapons = 1;
        }
        break;
    case 1:
        AddText("WEAPONS", 30, 90, 8, 0);
        if (blinkFlag)
            AddText("DEFAULT VIEW", 30, 110, 8, 1);
        AddText("DEFAULT NAME", 30, 130, 8, 0);
        score->nameCursor = 0;
        score->enteringName = 0;
        EnterName(gameData, score);
        if (menuKeys & MENU_KEY_RIGHT) {
            if (menuState.internalView == 1) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                menuState.internalView = 0;
            }
        }
        if (menuKeys & MENU_KEY_LEFT) {
            if (menuState.internalView == 0) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                menuState.internalView = 1;
            }
        }
        break;
    case 2:
        AddText("WEAPONS", 30, 90, 8, 0);
        AddText("DEFAULT VIEW", 30, 110, 8, 0);
        if (blinkFlag)
            AddText("DEFAULT NAME", 30, 130, 8, 1);
        PrintCX("NAME IS USED AS DEFAULT HIGH SCORE ENTRY", 160, 186, 0);
        score->enteringName = 1;
        EnterName(gameData, score);
        break;
    }
}

extern Object *teamShipModels[];
extern Object *teamShip;

#ifdef NETPLAY
/* Picks the controller (keyboard, joystick or mouse) and lets the player go on to redefine it.
   Proven equivalent rather than byte-exact: the strcat chain lays out the same, but the exe keeps
   deviceType in ebp for the first test. */
#pragma intrinsic(strcpy, strcat)
void SelectController(PcGameData *gameData)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short models[3] = {1, 2, 0};
    char name[24];

    strcpy(name, "KEYBOARD");

    if (deviceType == 1)
        strcat(name, " AND JOYSTICK");
    else if (deviceType == 2)
        strcat(name, " AND MOUSE");
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0) {
            menuState.current = 0;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    } else if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current > 1) {
            menuState.current = 1;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    } else if (GetMenuKey(3)) {
        if (menuState.current == 0) {
            deviceType++;
            if (deviceType > 2) {
                deviceType = 2;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
    } else if (GetMenuKey(2)) {
        if (menuState.current == 0) {
            deviceType--;
            if (deviceType < 0) {
                deviceType = 0;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
    }
    if (menuState.current == 0) {
        if (blinkFlag && deviceType < 2)
            DoButton(275, 110, 4, "");
        if (blinkFlag && deviceType > 0)
            DoButton(30, 110, 3, "");
        if (deviceChosen == 0) {
            joystickPresent = deviceType & 1;
            mouseEnabled = (deviceType & 2) >> 1;
        }
    }
    switch (menuState.current) {
    case 0:
        if (blinkFlag)
            CentreText(name, 0, 110, 8, 0);
        else
            CentreText(name, 0, 110, 8, 1);
        CentreText("REDEFINE CONTROLLER", 0, 130, 8, 0);
        break;
    case 1:
        CentreText(name, 0, 110, 8, 0);
        if (blinkFlag)
            CentreText("REDEFINE CONTROLLER", 0, 130, 8, 0);
        else
            CentreText("REDEFINE CONTROLLER", 0, 130, 8, 1);
        break;
    }
    if (deviceType == 2)
        DrawTeamShips(0, 0, 6000, menuHeading.angle, 220, teamShipModels[models[deviceType]]);
    else
        DrawTeamShips(0, 0, 6000, menuHeading.angle, 0, teamShipModels[models[deviceType]]);
}
#pragma function(strcpy, strcat)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Picks the controller (keyboard, joystick or mouse) and lets the player go on to redefine it.
   Proven equivalent rather than byte-exact: the strcat chain lays out the same, but the exe keeps
   deviceType in ebp for the first test. */
void SelectController(PcGameData *gameData)
{
    char name[24];
    short order[3] = {1, 2, 0};

    strcpy(name, "KEYBOARD");

    if (deviceType == 1)
        strcat(name, " AND JOYSTICK");
    else if (deviceType == 2)
        strcat(name, " AND MOUSE");
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0) {
            menuState.current = 0;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    } else if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current > 1) {
            menuState.current = 1;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    } else if (GetMenuKey(3)) {
        if (menuState.current == 0) {
            deviceType++;
            if (deviceType > 2) {
                deviceType = 2;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
    } else if (GetMenuKey(2)) {
        if (menuState.current == 0) {
            deviceType--;
            if (deviceType < 0) {
                deviceType = 0;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
    }
    if (menuState.current == 0) {
        if (blinkFlag) {
            if (deviceType < 2)
                DoButton(275, 110, 4, "");
            if (blinkFlag && deviceType > 0)
                DoButton(30, 110, 3, "");
        }
        if (deviceChosen == 0) {
            joystickPresent = deviceType & 1;
            mouseEnabled = (deviceType & 2) >> 1;
        }
    }
    switch (menuState.current) {
    case 0:
        if (blinkFlag)
            CentreText(name, 0, 110, 8, 0);
        else
            CentreText(name, 0, 110, 8, 1);
        CentreText("REDEFINE CONTROLLER", 0, 130, 8, 0);
        break;
    case 1:
        CentreText(name, 0, 110, 8, 0);
        if (blinkFlag)
            CentreText("REDEFINE CONTROLLER", 0, 130, 8, 0);
        else
            CentreText("REDEFINE CONTROLLER", 0, 130, 8, 1);
        break;
    }
    if (deviceType == 2)
        DrawTeamShips(0, 0, 6000, menuHeading.angle, 220, teamShipModels[order[deviceType]]);
    else
        DrawTeamShips(0, 0, 6000, menuHeading.angle, 0, teamShipModels[order[deviceType]]);
}
#endif
#endif

extern int redefineMode;
extern int mouseButton0Held, mouseButton1Held, mouseButton2Held;
extern int keyCursorLeft[22], keyCursorRight[22];
extern char *controlActionNames[9], *keyboardKeyNames[], *joystickButtonNames[32];
int CheckControlConflicts(void);

#ifdef NETPLAY
/* Keyboard, joystick and mouse binding editor. Proven equivalent rather than byte-exact:
   register allocation differs. */
#pragma intrinsic(strcpy, strcat)
void RedefineController(ConfigData *config)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    int bit;
    int action;
    char outBuffer[32];

    if (redefineMode == 1) {
        if (g_keyboardCode != -1) {
            if (g_keyboardCode == 15 /* DIK_TAB */ || g_keyboardCode == 0xD3 /* DIK_DELETE */ ||
                (g_keyboardCode >= 0x3B && g_keyboardCode <= 0x58)) {
            } else if (g_keyboardCode == 1 /* DIK_ESCAPE */) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                redefineMode = 0;
            } else {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                keyboardKeys[redefineKey] = g_keyboardCode;
                redefineMode = 0;
            }
            g_keyboardCode = -1;
        }
        Print("PRESS KEY TO USE", 30, 182, 0);
    } else if (redefineMode == 2) {
        if (g_keyboardCode == 0xD3 /* DIK_DELETE */) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            joystickButtons[redefineKey - 10] = 0;
            redefineMode = 0;
            g_keyboardCode = -1;
        } else if (g_keyboardCode == 1 /* DIK_ESCAPE */) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            redefineMode = 0;
            g_keyboardCode = -1;
        } else if (joyPressed != 0 && joyPressed != 8) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            joystickButtons[redefineKey - 10] = joyPressed;
            redefineMode = 0;
        }
        Print("PRESS JOYSTICK BUTTON TO USE", 30, 182, 0);
        Print("IF NO BUTTON AVAILABLE PRESS DELETE", 30, 189, 0);
    } else if (redefineMode == 3) {
        if (g_keyboardCode != -1) {
            if (g_keyboardCode == 0xD3 /* DIK_DELETE */) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 0;
                redefineMode = 0;
            } else if (g_keyboardCode == 1 /* DIK_ESCAPE */) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                redefineMode = 0;
                g_keyboardCode = -1;
            } else if (mouseButton0Held) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 1;
                redefineMode = 0;
                g_keyboardCode = -1;
            } else if (mouseButton1Held) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 2;
                redefineMode = 0;
                g_keyboardCode = -1;
            } else if (mouseButton2Held) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 3;
                redefineMode = 0;
                g_keyboardCode = -1;
            }
        }
        Print("PRESS MOUSE BUTTON TO USE", 30, 182, 0);
        Print("IF NO BUTTON AVAILABLE PRESS DELETE", 30, 189, 0);
    } else {
        if (GetMenuKey(0) && redefineKey > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            redefineKey--;
        } else if (GetMenuKey(1) && redefineKey < 21) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            redefineKey++;
        } else if (GetMenuKey(2)) {
            if (redefineKey != keyCursorLeft[redefineKey]) {
                redefineKey = keyCursorLeft[redefineKey];
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        } else if (GetMenuKey(3)) {
            if (redefineKey != keyCursorRight[redefineKey]) {
                redefineKey = keyCursorRight[redefineKey];
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
        if (g_keyboardCode == 1 /* DIK_ESCAPE */ || (keysPressed & MENU_KEY_BACK)) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            redefineCancelled = 1;
            g_keyboardCode = -1;
        }
        if (GetMenuKey(5)) {
            if (redefineKey == 9 && !CheckControlConflicts()) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineStep = 1;
            }
            if (redefineKey == 15) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                keyboardKeys[0] = 0x1F;
                keyboardKeys[2] = 0x38;
                keyboardKeys[1] = 0x39;
                keyboardKeys[3] = 0x1E;
                keyboardKeys[4] = 0x20;
                keyboardKeys[7] = 0xCB;
                keyboardKeys[6] = 0xD0;
                keyboardKeys[8] = 0xCD;
                keyboardKeys[5] = 0xC8;
                keyboardKeys[9] = 0x1C;
                keyboardKeys[10] = 0x3B;
                joystickButtons[0] = 2;
                joystickButtons[1] = 4;
                joystickButtons[2] = 1;
                joystickButtons[3] = 0x40;
                joystickButtons[4] = 0x80;
                mouseButtons[0] = 1;
                mouseButtons[1] = 2;
                mouseButtons[2] = 3;
                mouseButtons[3] = 0;
                mouseButtons[4] = 0;
            }
            if (redefineKey == 21) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineCancelled = 1;
            }
            if (redefineKey >= 0 && redefineKey <= 8) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineMode = 1;
                g_keyboardCode = -1;
            }
            if (redefineKey >= 10 && redefineKey <= 14) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineMode = 2;
            }
            if (redefineKey >= 16 && redefineKey <= 20) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineMode = 3;
            }
        }
        if (redefineKey == 9) {
            if (CheckControlConflicts()) {
                Print("CANNOT ACCEPT AS TWO OR MORE ACTIONS", 30, 182, 0);
                Print("USE THE SAME KEY/BUTTON CONFIG", 30, 189, 0);
            } else Print("ACCEPT CURRENT SETTINGS", 30, 182, 0);
        } else if (redefineKey == 15) Print("USE DEFAULT GAME KEYS/BUTTONS", 30, 182, 0);
        else if (redefineKey == 21) Print("IGNORE ANY CHANGES AND GO BACK", 30, 182, 0);
        else Print("PRESS ENTER TO REDEFINE KEY/BUTTON", 30, 182, 0);
    }
    Print("ACTION", 30, 90, 1);
    Print("KEYBOARD", 110, 90, 1);
    Print("JOYSTICK", 180, 90, 1);
    Print("MOUSE", 250, 90, 1);
    for (action = 0; action < 9; action++) {
        Print(controlActionNames[action], 30, 102 + action * 7, 0);
        if (redefineKey != action) {
            Print(keyboardKeyNames[keyboardKeys[action]], 110, 102 + action * 7, 0);
        } else if (redefineKey == action && blinkFlag) {
            if (!redefineMode)
                Print(keyboardKeyNames[keyboardKeys[action]], 110, 102 + action * 7, 0);
            else
                Print("---", 110, 102 + action * 7, 0);
        }
    }
    for (action = 0; action < 5; action++) {
        for (bit = 0; bit < 32; bit++) {
            if (joystickButtons[action] & (1 << bit))
                break;
        }
        if (joystickButtons[action] == 0) {
            strcpy(outBuffer, "NONE");
        } else {
            strcpy(outBuffer, "BUTT ");
            strcat(outBuffer, joystickButtonNames[bit]);
        }
        if (action + 10 != redefineKey) {
            Print(outBuffer, 180, 102 + action * 7, 0);
        } else if (action + 10 == redefineKey && blinkFlag) {
            if (!redefineMode)
                Print(outBuffer, 180, 102 + action * 7, 0);
            else
                Print("---", 180, 102 + action * 7, 0);
        }
    }
    for (action = 0; action < 5; action++) {
        switch (mouseButtons[action]) {
        case 2:
            strcpy(outBuffer, "MOUSE 2");
            break;
        case 1:
            strcpy(outBuffer, "MOUSE 1");
            break;
        case 4:
            strcpy(outBuffer, "MOUSE 3");
            break;
        default:
            strcpy(outBuffer, "NONE");
            break;
        }
        if (action + 16 != redefineKey) {
            Print(outBuffer, 250, 102 + action * 7, 0);
        } else if (action + 16 == redefineKey && blinkFlag) {
            if (!redefineMode)
                Print(outBuffer, 250, 102 + action * 7, 0);
            else
                Print("---", 250, 102 + action * 7, 0);
        }
    }
    if (redefineKey != 9 || blinkFlag) Print("ACCEPT", 110, 169, 0);
    if (redefineKey != 15 || blinkFlag) Print("DEFAULT", 180, 169, 0);
    if (redefineKey != 21 || blinkFlag) Print("QUIT", 250, 169, 0);
}
#pragma function(strcpy, strcat)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Keyboard, joystick and mouse binding editor. Proven equivalent rather than byte-exact:
   register allocation differs. */
#pragma intrinsic(strcpy, strcat)
void RedefineController(ConfigData *config)
{
    char name[32];
    int *button;
    int i, bit;

    if (redefineMode == 1) {
        if (g_keyboardCode != -1) {
            if (g_keyboardCode != 15 /* DIK_TAB */ && g_keyboardCode != 0xD3 /* DIK_DELETE */ &&
                !(g_keyboardCode >= 0x3B && g_keyboardCode <= 0x58)) {
                if (g_keyboardCode == 1 /* DIK_ESCAPE */) PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                else {
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                    keyboardKeys[redefineKey] = g_keyboardCode;
                }
                redefineMode = 0;
            }
            g_keyboardCode = -1;
        }
        Print("PRESS KEY TO USE", 30, 182, 0);
    } else if (redefineMode == 2) {
        if (g_keyboardCode == 0xD3 /* DIK_DELETE */) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            g_keyboardCode = -1;
            joystickButtons[redefineKey - 10] = 0;
            redefineMode = 0;
        } else if (g_keyboardCode == 1 /* DIK_ESCAPE */) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            redefineMode = 0;
            g_keyboardCode = -1;
        } else if (joyPressed != 0 && joyPressed != 8) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            redefineMode = 0;
            joystickButtons[redefineKey - 10] = joyPressed;
        }
        Print("PRESS JOYSTICK BUTTON TO USE", 30, 182, 0);
        Print("IF NO BUTTON AVAILABLE PRESS DELETE", 30, 189, 0);
    } else if (redefineMode == 3) {
        if (g_keyboardCode != -1) {
            if (g_keyboardCode == 0xD3 /* DIK_DELETE */) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                redefineMode = 0;
                mouseButtons[redefineKey - 16] = 0;
            } else if (g_keyboardCode == 1 /* DIK_ESCAPE */) {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
                redefineMode = 0;
                g_keyboardCode = -1;
            } else if (mouseButton0Held) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 1;
                redefineMode = 0;
                g_keyboardCode = -1;
            } else if (mouseButton1Held) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 2;
                redefineMode = 0;
                g_keyboardCode = -1;
            } else if (mouseButton2Held) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                mouseButtons[redefineKey - 16] = 3;
                redefineMode = 0;
                g_keyboardCode = -1;
            }
        }
        Print("PRESS MOUSE BUTTON TO USE", 30, 182, 0);
        Print("IF NO BUTTON AVAILABLE PRESS DELETE", 30, 189, 0);
    } else {
        if (GetMenuKey(0) && redefineKey > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            redefineKey--;
        } else if (GetMenuKey(1) && redefineKey < 21) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            redefineKey++;
        } else if (GetMenuKey(2)) {
            if (redefineKey != keyCursorLeft[redefineKey]) {
                redefineKey = keyCursorLeft[redefineKey];
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        } else if (GetMenuKey(3)) {
            if (redefineKey != keyCursorRight[redefineKey]) {
                redefineKey = keyCursorRight[redefineKey];
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
        if (g_keyboardCode == 1 /* DIK_ESCAPE */ || (keysPressed & MENU_KEY_BACK)) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            redefineCancelled = 1;
            g_keyboardCode = -1;
        }
        if (GetMenuKey(5)) {
            if (redefineKey == 9 && !CheckControlConflicts()) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineStep = 1;
            }
            if (redefineKey == 15) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                keyboardKeys[0] = 0x1F;
                keyboardKeys[2] = 0x38;
                keyboardKeys[1] = 0x39;
                keyboardKeys[3] = 0x1E;
                keyboardKeys[4] = 0x20;
                keyboardKeys[7] = 0xCB;
                keyboardKeys[6] = 0xD0;
                keyboardKeys[8] = 0xCD;
                keyboardKeys[5] = 0xC8;
                joystickButtons[0] = 2;
                keyboardKeys[9] = 0x1C;
                keyboardKeys[10] = 0x3B;
                joystickButtons[2] = 1;
                mouseButtons[0] = 1;
                joystickButtons[1] = 4;
                joystickButtons[3] = 0x40;
                joystickButtons[4] = 0x80;
                mouseButtons[2] = 3;
                mouseButtons[1] = 2;
                mouseButtons[3] = 0;
                mouseButtons[4] = 0;
            }
            if (redefineKey == 21) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineCancelled = 1;
            }
            if (redefineKey >= 0 && redefineKey <= 8) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineMode = 1;
                g_keyboardCode = -1;
            }
            if (redefineKey >= 10 && redefineKey <= 14) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineMode = 2;
            }
            if (redefineKey >= 16 && redefineKey <= 20) {
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                redefineMode = 3;
            }
        }
        if (redefineKey == 9) {
            if (CheckControlConflicts()) {
                Print("CANNOT ACCEPT AS TWO OR MORE ACTIONS", 30, 182, 0);
                Print("USE THE SAME KEY/BUTTON CONFIG", 30, 189, 0);
            } else Print("ACCEPT CURRENT SETTINGS", 30, 182, 0);
        } else if (redefineKey == 15) Print("USE DEFAULT GAME KEYS/BUTTONS", 30, 182, 0);
        else if (redefineKey == 21) Print("IGNORE ANY CHANGES AND GO BACK", 30, 182, 0);
        else Print("PRESS ENTER TO REDEFINE KEY/BUTTON", 30, 182, 0);
    }
    Print("ACTION", 30, 90, 1);
    Print("KEYBOARD", 110, 90, 1);
    Print("JOYSTICK", 180, 90, 1);
    Print("MOUSE", 250, 90, 1);
    for (i = 0; i < 9; i++) {
        Print(controlActionNames[i], 30, 102 + i * 7, 0);
        if (redefineKey != i) Print(keyboardKeyNames[keyboardKeys[i]], 110, 102 + i * 7, 0);
        else if (blinkFlag) {
            if (!redefineMode) Print(keyboardKeyNames[keyboardKeys[i]], 110, 102 + i * 7, 0);
            else Print("---", 110, 102 + i * 7, 0);
        }
    }
    for (i = 0, button = joystickButtons; button < joystickButtons + 5; button++, i++) {
        for (bit = 0; bit < 32; bit++) {
            if (*button & (1U << bit)) break;
        }
        if (*button == 0) strcpy(name, "NONE");
        else {
            strcpy(name, "BUTT ");
            strcat(name, joystickButtonNames[bit]);
        }
        if (i - redefineKey != -10) Print(name, 180, 102 + i * 7, 0);
        else if (blinkFlag) {
            if (!redefineMode) Print(name, 180, 102 + i * 7, 0);
            else Print("---", 180, 102 + i * 7, 0);
        }
    }
    for (i = 0, button = mouseButtons; button < mouseButtons + 5; button++, i++) {
        switch (*button) {
        case 1: strcpy(name, "MOUSE 1"); break;
        case 2: strcpy(name, "MOUSE 2"); break;
        case 4: strcpy(name, "MOUSE 3"); break;
        default: strcpy(name, "NONE"); break;
        }
        if (i - redefineKey != -16) Print(name, 250, 102 + i * 7, 0);
        else if (blinkFlag) {
            if (!redefineMode) Print(name, 250, 102 + i * 7, 0);
            else Print("---", 250, 102 + i * 7, 0);
        }
    }
    if (redefineKey != 9 || blinkFlag) Print("ACCEPT", 110, 169, 0);
    if (redefineKey != 15 || blinkFlag) Print("DEFAULT", 180, 169, 0);
    if (redefineKey != 21 || blinkFlag) Print("QUIT", 250, 169, 0);
}
#endif
#endif

/* 1, 2 or 3 when two actions of the keyboard, joystick or mouse set share a key
   or button (unassigned entries don't count), else 0. */
int CheckControlConflicts(void)
{
    int i, j;

    for (i = 0; i < 9; i++) {
        for (j = 0; j < 9; j++) {
            if (j != i && keyboardKeys[j] == keyboardKeys[i] && keyboardKeys[i] != 0)
                return 1;
        }
    }
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            if (j != i && joystickButtons[j] == joystickButtons[i] && joystickButtons[i] != 0)
                return 2;
        }
    }
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            if (j != i && mouseButtons[j] == mouseButtons[i] && mouseButtons[i] != 0)
                return 3;
        }
    }
    return 0;
}

extern volatile short g_keyboardCode;
extern char g_keyCharTable[];

/* One saved game: the progress flags, the unlock bytes of the game data and the tracks won. */
typedef struct SaveRecord {
    short flags[7];
    u_char unlock[7];
    u_char tracksWon[8];
    u_char reserved29[11];
} SaveRecord;

typedef struct SlotName {
    char text[16];
} SlotName;

typedef struct SaveSlot {
    SlotName name;
    short occupied;
    SaveRecord rec;
} SaveSlot;

typedef ConfigData PcProgressFlags;

extern short sixTracks, sixXPlayed, eightTracks, eightXPlayed;
extern short phantomDone, piranhaDone, lastClassSelection;
extern char wobbleDir0[];
extern SaveSlot saveSlots[8];
extern SaveSlot defaultSlot;
extern ConfigData *gameData;

extern int saveCursor, nameCursor, textExtendedSpacing, saveMessageFrames;
extern char g_keyCharTable[];
int ReadSaveFile(void);

#ifdef NETPLAY
/* Draft: load/save state machine and name editor; register scheduling still differs. */
extern __inline void LoadSaveSlot(int index);
static __inline int SaveSlotToFile(int index);
void ClearUnlocks(void);
int StoreSaveSlot(int index);
char GetKeyboardChar(void);
void LoadSave(ConfigData *config)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    int newI;
    int lastHighlight;
    int baseX;
    int curResult;
    char currentSlotLabel[32];
    char *curLetterPtr;
    signed char outLetter;

    curResult = 0;
    lastHighlight = 1;
    baseX = 0;
    switch (saveState) {
    case 0:
        lastHighlight = blinkFlag;
        if (GetMenuKey(2)) {
            if (menuState.current > 0) {
                menuState.current = 0;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        } else if (GetMenuKey(3)) {
            if (menuState.current < 1) {
                menuState.current = 1;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
        if (GetMenuKey(5)) {
            saveState = 1;
            saveCursor = 0;
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            ReadSaveFile();
        }
        if (g_keyboardCode == 1 || (keysPressed & MENU_KEY_BACK)) {
            redefineCancelled = 1;
            g_keyboardCode = -1;
        }
        break;
    case 1:
        if (GetMenuKey(0) && saveCursor > 0) {
            saveCursor--;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        if (GetMenuKey(1) && saveCursor < 7) {
            saveCursor++;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        if (GetMenuKey(5)) {
            if (menuState.current == 0) {
                if (saveSlots[saveCursor].occupied) {
                    saveState = 3;
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                }
            } else {
                saveState = 2;
                if (!saveSlots[saveCursor].occupied) {
                    for (newI = 0; newI < 16; newI++) {
                        curLetterPtr = &saveSlots[saveCursor].name.text[newI];
                        *curLetterPtr = ' ';
                    }
                    nameCursor = 0;
                }
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            }
            nameCursor = 0;
            g_keyboardCode = -1;
        }
        if (g_keyboardCode == 1 || (keysPressed & MENU_KEY_BACK)) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            saveState = 0;
            g_keyboardCode = -1;
        }
        break;
    case 2:
        if (GetMenuKey(2)) {
            if (nameCursor > 0) {
                nameCursor--;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        } else if (GetMenuKey(3)) {
            if (nameCursor < 11) {
                nameCursor++;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
        if (GetMenuKey(5)) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            if (menuState.current == 0)
                PrintCX("LOADING GAME", 70, 182, 0);
            else
                PrintCX("SAVING GAME", 270, 182, 0);
            saveState = 3;
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
        }
        if (g_keyboardCode == 14) {
            if (nameCursor == 11) {
                curLetterPtr = &saveSlots[saveCursor].name.text[nameCursor];
                *curLetterPtr = ' ';
            }
            if (nameCursor > 0)
                nameCursor--;
            curLetterPtr = &saveSlots[saveCursor].name.text[nameCursor];
            *curLetterPtr = ' ';
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            g_keyboardCode = -1;
        }
        if (g_keyboardCode == 0xD3) {
            for (newI = 0; newI < 16; newI++) {
                curLetterPtr = &saveSlots[saveCursor].name.text[newI];
                *curLetterPtr = ' ';
            }
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            g_keyboardCode = -1;
            nameCursor = 0;
        }
        if (g_keyboardCode == 1 || (keysPressed & MENU_KEY_BACK)) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            saveState = 1;
            g_keyboardCode = -1;
            ReadSaveFile();
        }
        outLetter = GetKeyboardChar();
        if (outLetter) {
            curLetterPtr = &saveSlots[saveCursor].name.text[nameCursor];
            *curLetterPtr = outLetter;
            if (nameCursor < 11)
                nameCursor++;
        }
        break;
    case 3:
        if (menuState.current == 0) {
            ClearUnlocks();
            LoadSaveSlot(saveCursor);
            SetupUnlocks();
        } else {
            saveSlots[saveCursor].occupied = 1;
            if (StoreSaveSlot(saveCursor))
                curResult = 1;
        }
        saveState = 4;
        saveMessageFrames = 0;
        break;
    case 4:
        if (saveMessageFrames < 180) {
            saveMessageFrames += FR60;
            if (menuState.current == 0)
                PrintCX("LOADED GAME OK", 70, 182, 0);
            else
                PrintCX("SAVED GAME OK", 260, 182, 0);
        } else {
            redefineCancelled = 1;
        }
    }
    if (menuState.current != 0)
        baseX = 142;
    if (menuState.current == 0) {
        if (lastHighlight)
            AddText("LOAD GAME", 38, 90, 8, 0);
        else
            AddText("LOAD GAME", 38, 90, 8, 1);
        AddText("SAVE GAME", 182, 90, 8, 0);
    } else {
        AddText("LOAD GAME", 38, 90, 8, 0);
        if (lastHighlight)
            AddText("SAVE GAME", 182, 90, 8, 0);
        else
            AddText("SAVE GAME", 182, 90, 8, 1);
    }
    if (blinkFlag && saveState == 2)
        Print("-", nameCursor * 6 + baseX + 80, saveCursor * 8 + 114, 1);
    for (newI = 0; newI < 8; newI++) {
        wsprintfA(currentSlotLabel, "GAME %d", newI + 1);
        if (saveState == 1) {
            if (saveCursor != newI || (saveCursor == newI && blinkFlag)) {
                Print(currentSlotLabel, baseX + 36, newI * 8 + 110, 0);
                textExtendedSpacing = 1;
                Print(saveSlots[newI].name.text, baseX + 80, newI * 8 + 110, 0);
                textExtendedSpacing = 0;
            }
        }
        if (saveState == 2) {
            Print(currentSlotLabel, baseX + 36, newI * 8 + 110, 0);
            textExtendedSpacing = 1;
            Print(saveSlots[newI].name.text, baseX + 80, newI * 8 + 110, 0);
            textExtendedSpacing = 0;
        }
    }
    if (saveState == 1) {
        if (menuState.current == 0) {
            Print("USE THE UP/DOWN CURSOR KEYS AND ENTER", 30, 182, 0);
            Print("TO SELECT THE GAME SLOT TO LOAD.", 30, 189, 0);
        } else {
            Print("USE THE UP/DOWN CURSOR KEYS AND ENTER", 30, 182, 0);
            Print("TO SELECT THE SAVE GAME SLOT.", 30, 189, 0);
        }
    } else if (saveState == 2) {
        Print("TYPE IN THE NAME FOR THIS SAVE GAME.", 30, 182, 0);
        Print("USE CURSOR KEYS TO EDIT.", 30, 189, 0);
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Draft: load/save state machine and name editor; register scheduling still differs. */
extern __inline void LoadSaveSlot(int index);
static __inline int SaveSlotToFile(int index);
void LoadSave(ConfigData *config)
{
    char label[32];
    char c;
    short key;
    int flash;
    int x;
    SaveSlot *slot;
    int i;

    x = 0;
    flash = 1;
    switch (saveState) {
    case 0:
        flash = blinkFlag;
        if (GetMenuKey(2)) {
            if (menuState.current > 0) {
                menuState.current = 0;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        } else if (GetMenuKey(3)) {
            if (menuState.current < 1) {
                menuState.current = 1;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
        if (GetMenuKey(5)) {
            saveState = 1;
            saveCursor = 0;
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            ReadSaveFile();
        }
        if (g_keyboardCode == 1 || (keysPressed & MENU_KEY_BACK)) {
            redefineCancelled = 1;
            g_keyboardCode = -1;
        }
        break;
    case 1:
        if (GetMenuKey(0) && saveCursor > 0) {
            saveCursor--;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        if (GetMenuKey(1) && saveCursor < 7) {
            saveCursor++;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        if (GetMenuKey(5)) {
            if (menuState.current == 0) {
                if (saveSlots[saveCursor].occupied) {
                    saveState = 3;
                    PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                }
            } else {
                saveState = 2;
                if (!saveSlots[saveCursor].occupied) {
                    nameCursor = 0;
                    memset(saveSlots[saveCursor].name.text, ' ', 16);
                }
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            }
            nameCursor = 0;
            g_keyboardCode = -1;
        }
        if (g_keyboardCode == 1 || (keysPressed & MENU_KEY_BACK)) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            saveState = 0;
            g_keyboardCode = -1;
        }
        break;
    case 2:
        if (GetMenuKey(2)) {
            if (nameCursor > 0) {
                nameCursor--;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        } else if (GetMenuKey(3)) {
            if (nameCursor < 11) {
                nameCursor++;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
        if (GetMenuKey(5)) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            if (menuState.current == 0) PrintCX("LOADING GAME", 70, 182, 0);
            else PrintCX("SAVING GAME", 270, 182, 0);
            saveState = 3;
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
        }
        key = g_keyboardCode;
        if (key == 14) {
            if (nameCursor == 11) saveSlots[saveCursor].name.text[nameCursor] = ' ';
            if (nameCursor > 0) nameCursor--;
            key = -1;
            saveSlots[saveCursor].name.text[nameCursor] = ' ';
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        g_keyboardCode = key;
        if (key == 0xD3) {
            memset(saveSlots[saveCursor].name.text, ' ', 16);
            key = -1;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            nameCursor = 0;
        }
        g_keyboardCode = key;
        if (key == 1 || (keysPressed & MENU_KEY_BACK)) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            saveState = 1;
            g_keyboardCode = -1;
            ReadSaveFile();
            key = g_keyboardCode;
        }
        g_keyboardCode = key;
        if (key != -1) {
            c = g_keyCharTable[key];
            if (key != 0x39 && !(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'Z')) c = 0;
            g_keyboardCode = key;
            if (key == 12) c = '-';
        } else c = 0;
        g_keyboardCode = -1;
        if (c) {
            saveSlots[saveCursor].name.text[nameCursor] = c;
            if (nameCursor < 11) nameCursor++;
        }
        break;
    case 3:
        if (menuState.current == 0) {
            if (gameType == 2 && tempGameData->venomComplete) gameType = 0;
            if (gameType == 2 && tempGameData->rapierComplete) gameType = 0;
            menu[1].numOptions = 2;
            menu[1].selection = 0;
            tempGameData->phantomCheat = 0;
            menu[2].options[3].enabled = 0;
            tempGameData->trackCheat = 0;
            menu[2].selection = 0;
            tempGameData->teamCheat = 0;
            menu[2].mode = 0;
            tempGameData->venomComplete = 0;
            tempGameData->rapierComplete = 0;
            tempGameData->class1Unlocked = 0;
            tempGameData->class2Unlocked = 0;
            LoadSaveSlot(saveCursor);
            SetupUnlocks();
        } else {
            saveSlots[saveCursor].occupied = 1;
            SaveSlotToFile(saveCursor);
        }
        saveState = 4;
        saveMessageFrames = 0;
        break;
    case 4:
        if (saveMessageFrames < 180) {
            saveMessageFrames += FR60;
            if (menuState.current == 0) PrintCX("LOADED GAME OK", 70, 182, 0);
            else PrintCX("SAVED GAME OK", 260, 182, 0);
        } else redefineCancelled = 1;
        break;
    }
    if (menuState.current != 0) x = 142;
    if (menuState.current == 0) {
        if (flash) AddText("LOAD GAME", 38, 90, 8, 0);
        else AddText("LOAD GAME", 38, 90, 8, 1);
        AddText("SAVE GAME", 182, 90, 8, 0);
    } else {
        AddText("LOAD GAME", 38, 90, 8, 0);
        if (flash) AddText("SAVE GAME", 182, 90, 8, 0);
        else AddText("SAVE GAME", 182, 90, 8, 1);
    }
    if (blinkFlag && saveState == 2) Print("-", x + nameCursor * 6 + 80, saveCursor * 8 + 114, 1);
    for (i = 0, slot = saveSlots; i < 8; i++, slot++) {
        wsprintfA(label, "GAME %d", i + 1);
        if (saveState == 1 && (i != saveCursor || blinkFlag)) {
            Print(label, x + 36, 110 + i * 8, 0);
            textExtendedSpacing = 1;
            Print(slot->name.text, x + 80, 110 + i * 8, 0);
            textExtendedSpacing = 0;
        }
        if (saveState == 2) {
            Print(label, x + 36, 110 + i * 8, 0);
            textExtendedSpacing = 1;
            Print(slot->name.text, x + 80, 110 + i * 8, 0);
            textExtendedSpacing = 0;
        }
    }
    if (saveState == 1) {
        if (menuState.current == 0) {
            Print("USE THE UP/DOWN CURSOR KEYS AND ENTER", 30, 182, 0);
            Print("TO SELECT THE GAME SLOT TO LOAD.", 30, 189, 0);
        } else {
            Print("USE THE UP/DOWN CURSOR KEYS AND ENTER", 30, 182, 0);
            Print("TO SELECT THE SAVE GAME SLOT.", 30, 189, 0);
        }
    } else if (saveState == 2) {
        Print("TYPE IN THE NAME FOR THIS SAVE GAME.", 30, 182, 0);
        Print("USE CURSOR KEYS TO EDIT.", 30, 189, 0);
    }
}
#endif
#endif

/* Letter, digit or '-' for the key just pressed, 0 for any other key; consumes the key. */
char GetKeyboardChar(void)
{
    short code;
    char c;

    code = g_keyboardCode;
    if (code != -1) {
        c = g_keyCharTable[code];
        if (code != 0x39 /* DIK_SPACE */) {
            if (!(c >= '0' && c <= '9')) {
                if (!(c >= 'A' && c <= 'Z'))
                    c = 0;
            }
        }
        if (code == 12 /* DIK_MINUS */)
            c = '-';
    } else {
        c = 0;
    }
    g_keyboardCode = -1;
    return c;
}

extern __inline void RestoreProgress(SaveRecord *rec);
extern __inline void StoreProgress(SaveRecord *rec);

/* Loads the progress stored in save slot index into the game data. */
extern __inline void LoadSaveSlot(int index)
{
    RestoreProgress(&saveSlots[index].rec);
}

/* Stores progress in slot index and writes the save file; LoadSave expands it inline. */
static __inline int SaveSlotToFile(int index)
{
    char path[256];
    FILE *file;

    StoreProgress(&saveSlots[index].rec);
    strcpy(path, wobbleDir0);
    strcat(path, "\\SAVEGAME.BIN");
    file = fopen(path, "wb");
    if (file != 0) {
        fwrite(saveSlots, sizeof(saveSlots), 1, file);
        fclose(file);
        OutputDebugString("Saving savefile\n");
        return 0;
    }
    return -1;
}

#ifdef NETPLAY
/* Writes the save slot file in the network build. SaveSlotToFile is expanded here, and strcpy and strcat
   are inline in the executable. */
#pragma intrinsic(strcpy, strcat)
int StoreSaveSlot(int index)
{
    char path[256];
    FILE *stream;

    StoreProgress(&saveSlots[index].rec);
    strcpy(path, wobbleDir0);
    strcat(path, "\\SAVEGAME.BIN");
    stream = fopen(path, "wb");
    if (stream != 0) {
        fwrite(saveSlots, sizeof(saveSlots), 1, stream);
        fclose(stream);
        OutputDebugString("Saving savefile\n");
    } else {
        return -1;
    }
    return 0;
}
#pragma function(strcpy, strcat)
#else
/* The out-of-line entry for SaveSlotToFile. */
int StoreSaveSlot(int index)
{
    return SaveSlotToFile(index);
}
#endif

#ifdef NETPLAY
/* Reads the save file in the network build (strcpy and strcat inline). */
#pragma intrinsic(strcpy, strcat)
int ReadSaveFile(void)
{
    char fullPath[256];
    FILE *stream;
    int i;

    strcpy(fullPath, wobbleDir0);
    strcat(fullPath, "\\SAVEGAME.BIN");
    stream = fopen(fullPath, "rb");
    if (stream != 0) {
        fread(saveSlots, sizeof(saveSlots), 1, stream);
        fclose(stream);
        OutputDebugString("Reading savefile\n");
    } else {
        for (i = 0; i < 8; i++) {
            saveSlots[i].name = defaultSlot.name;
            saveSlots[i].occupied = defaultSlot.occupied;
            saveSlots[i].rec = defaultSlot.rec;
        }
        stream = fopen(fullPath, "wb");
        if (stream != 0) {
            fwrite(saveSlots, sizeof(saveSlots), 1, stream);
            fclose(stream);
            OutputDebugString("Creating new savefile\n");
        } else {
            return -1;
        }
    }
    return 0;
}
#pragma function(strcpy, strcat)
#else
/* Reads the save file, or fills it with default setups and writes a new one. */
int ReadSaveFile(void)
{
    char path[256];
    FILE *file;
    SaveSlot *set;
    SaveSlot *src;

    strcpy(path, wobbleDir0);
    strcat(path, "\\SAVEGAME.BIN");
    file = fopen(path, "rb");
    if (file != 0) {
        fread(saveSlots, sizeof(saveSlots), 1, file);
        fclose(file);
        OutputDebugString("Reading savefile\n");
        return 0;
    }
    for (set = saveSlots; set < &saveSlots[8]; set++) {
        src = &defaultSlot;
        set->name = src->name;
        set->occupied = src->occupied;
        set->rec = src->rec;
    }
    file = fopen(path, "wb");
    if (file != 0) {
        fwrite(saveSlots, sizeof(saveSlots), 1, file);
        fclose(file);
        OutputDebugString("Creating new savefile\n");
        return 0;
    }
    return -1;
}
#endif

#ifdef NETPLAY
/* Copies current progress into a save record (out of line in the network build). */
void StoreProgress(SaveRecord *rec)
{
    short *sp;
    int i;
    u_char *cp;

    sp = rec->flags;
    *sp = sixTracks;
    sp++;
    *sp = sixXPlayed;
    sp++;
    *sp = eightTracks;
    sp++;
    *sp = eightXPlayed;
    sp++;
    *sp = phantomDone;
    sp++;
    *sp = piranhaDone;
    sp++;
    *sp = lastClassSelection;
    sp++;
    cp = (u_char *)sp;
    *cp++ = gameData->venomComplete;
    *cp++ = gameData->rapierComplete;
    *cp++ = gameData->teamCheat;
    *cp++ = gameData->class1Unlocked;
    *cp++ = gameData->class2Unlocked;
    *cp++ = gameData->trackCheat;
    *cp++ = gameData->phantomCheat;
    for (i = 0; i < 8; i++)
        *cp++ = tracksWon[i].savedMedal;
}

/* Fills the default unused slot: the name "--NOT USED--" and the current progress. The network build
   copies the literal name inline (strcpy as an intrinsic). */
#pragma intrinsic(strcpy)
void InitDefaultSlot(void)
{
    strcpy(defaultSlot.name.text, "--NOT USED--");
    defaultSlot.occupied = 0;
    StoreProgress(&defaultSlot.rec);
}
#pragma function(strcpy)
#else
/* Fills the default unused slot with the current progress. */
void InitDefaultSlot(void)
{
    short *sp;
    char *cp;
    int i;
    PcProgressFlags *options;

    strcpy(defaultSlot.name.text, "--NOT USED--");
    defaultSlot.occupied = 0;
    sp = defaultSlot.rec.flags;
    *sp = sixTracks;
    sp++;
    *sp = sixXPlayed;
    sp++;
    *sp = eightTracks;
    sp++;
    *sp = eightXPlayed;
    sp++;
    *sp = phantomDone;
    sp++;
    *sp = piranhaDone;
    sp++;
    *sp = lastClassSelection;
    sp++;
    cp = (char *)sp;
    options = (PcProgressFlags *)gameData;
    *cp++ = options->venomComplete;
    *cp++ = options->rapierComplete;
    *cp++ = options->teamCheat;
    *cp++ = options->class1Unlocked;
    *cp++ = options->class2Unlocked;
    *cp++ = options->trackCheat;
    *cp++ = options->phantomCheat;
    for (i = 0; i < 8; i++)
        *cp++ = tracksWon[i].savedMedal;
}
#endif

#ifndef NETPLAY
/* Copies current progress into a save record. */
extern __inline void StoreProgress(SaveRecord *rec)
{
    short *sp;
    char *cp;
    int i;
    PcProgressFlags *options;

    sp = rec->flags;
    *sp = sixTracks;
    sp++;
    *sp = sixXPlayed;
    sp++;
    *sp = eightTracks;
    sp++;
    *sp = eightXPlayed;
    sp++;
    *sp = phantomDone;
    sp++;
    *sp = piranhaDone;
    sp++;
    *sp = lastClassSelection;
    sp++;
    cp = (char *)sp;
    options = (PcProgressFlags *)gameData;
    *cp++ = options->venomComplete;
    *cp++ = options->rapierComplete;
    *cp++ = options->teamCheat;
    *cp++ = options->class1Unlocked;
    *cp++ = options->class2Unlocked;
    *cp++ = options->trackCheat;
    *cp++ = options->phantomCheat;
    for (i = 0; i < 8; i++)
        *cp++ = tracksWon[i].savedMedal;
}
#endif

/* Restores progress from a save record. */
#ifdef NETPLAY
/* Local names chosen to reproduce the exe's frame layout. */
extern __inline void RestoreProgress(SaveRecord *rec)
{
    char *bytes;
    short *words;
    int i;

    words = rec->flags;
    sixTracks = *words;
    words++;
    sixXPlayed = *words;
    words++;
    eightTracks = *words;
    words++;
    eightXPlayed = *words;
    words++;
    phantomDone = *words;
    words++;
    piranhaDone = *words;
    words++;
    lastClassSelection = *words;
    words++;
    bytes = (char *)words;
    gameData->venomComplete = *bytes++;
    gameData->rapierComplete = *bytes++;
    gameData->teamCheat = *bytes++;
    gameData->class1Unlocked = *bytes++;
    gameData->class2Unlocked = *bytes++;
    gameData->trackCheat = *bytes++;
    gameData->phantomCheat = *bytes++;
    for (i = 0; i < 8; i++)
        tracksWon[i].savedMedal = *bytes++;
}
#else
extern __inline void RestoreProgress(SaveRecord *rec)
{
    short *sp;
    char *cp;
    int i;
    PcProgressFlags *options;

    sp = rec->flags;
    sixTracks = *sp;
    sp++;
    sixXPlayed = *sp;
    sp++;
    eightTracks = *sp;
    sp++;
    eightXPlayed = *sp;
    sp++;
    phantomDone = *sp;
    sp++;
    piranhaDone = *sp;
    sp++;
    lastClassSelection = *sp;
    sp++;
    cp = (char *)sp;
    options = (PcProgressFlags *)gameData;
    options->venomComplete = *cp++;
    options->rapierComplete = *cp++;
    options->teamCheat = *cp++;
    options->class1Unlocked = *cp++;
    options->class2Unlocked = *cp++;
    options->trackCheat = *cp++;
    options->phantomCheat = *cp++;
    for (i = 0; i < 8; i++)
        tracksWon[i].savedMedal = *cp++;
}
#endif

extern float textDepth;
extern int hudTexturePage;
extern char menuTextureA[];
extern char menuTextureB[];
extern int cdMusicCount;
extern signed char cdMusicIndex;
extern signed char cdMusicTable[];
extern int cdMusicTrack;

void SetInGameCDTrack(void *a, void *b, PcGameData *gameData, int *c);
void SetNoteVolume(short voice, unsigned char volume);
void SetTextQuad(POLY_FT4 *poly, short x, short y, u_char u, u_char v, u_char width, u_char height, short tpage, int color);
char *GetCdTrackName(int track);

#ifdef NETPLAY
/* The audio screen: music and effect volume, and the CD track. */
void AudioConfig(PcGameData *gameData)
{
    /* Local names chosen to reproduce the exe's frame layout; the exe sets six words and never reads them. */
    int sfxPos;
    short height;
    int musicBarX;
    short y;
    short h;
    short x;
    POLY_FT4 knob;
    signed char savedTrack;
    short column;
    short labelX;

    x = 0x14;
    y = 0x32;
    labelX = 0x14;
    column = 0x6e;
    height = 0xdc;
    h = 0x1e;

    savedTrack = gameData->cdTrack;
    gameData->cdTrack = menuState.music;
    SetInGameCDTrack(menuTextureA, menuTextureB, gameData, &soundChanged);
    gameData->cdTrack = savedTrack;
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0)
            menuState.current = 0;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current >= 3)
            menuState.current = 2;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (menuState.current != 1) {
        SetNoteVolume(shieldHumVoice, 0);
        SfxEffectLRPan(shieldHumVoice, 15);
        SfxEffectFBPan(shieldHumVoice, 15);
    }
    switch (menuState.current) {
    case 0:
        if (blinkFlag)
            AddText("MUSIC", 30, 108, 8, 1);
        AddText("SFX", 30, 147, 8, 0);
        AddText("CD TRACK", 25, 184, 8, 0);
        if (padButtons & PAD_BUTTON_RIGHT) {
            menuState.musicVolume++;
            if (menuState.musicVolume >= 100)
                menuState.musicVolume = 100;
        }
        if (padButtons & PAD_BUTTON_LEFT) {
            menuState.musicVolume--;
            if (menuState.musicVolume < 0)
                menuState.musicVolume = 0;
        }
        menuState.otherMusic = menuState.musicVolume;
        SfxSetCdVolume(menuState.musicVolume * 255 / 100);
        break;
    case 1:
        AddText("MUSIC", 30, 108, 8, 0);
        if (blinkFlag)
            AddText("SFX", 30, 147, 8, 1);
        AddText("CD TRACK", 25, 184, 8, 0);
        if (padButtons & PAD_BUTTON_RIGHT) {
            menuState.effectVolume++;
            if (menuState.effectVolume >= 100)
                menuState.effectVolume = 100;
        }
        if (padButtons & PAD_BUTTON_LEFT) {
            menuState.effectVolume--;
            if (menuState.effectVolume < 0)
                menuState.effectVolume = 0;
        }
        menuState.otherEffect = menuState.effectVolume;
        SfxSetVolume(menuState.effectVolume * 255 / 100);
        SetNoteVolume(shieldHumVoice, 255);
        SfxEffectLRPan(shieldHumVoice, 15);
        SfxEffectFBPan(shieldHumVoice, 15);
        break;
    case 2:
        AddText("MUSIC", 30, 108, 8, 0);
        AddText("SFX", 30, 147, 8, 0);
        if (blinkFlag)
            AddText("CD TRACK", 25, 184, 8, 1);
        if (GetMenuKey(2)) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            menuState.music--;
            if (menuState.music < 1)
                menuState.music = cdMusicCount;
            if (menuState.music != 1) {
                CdMusicPlay(menuState.music, 0);
            } else {
                InitShufflePlay();
                cdMusicIndex = 0;
                cdMusicTrack = cdMusicTable[cdMusicIndex];
                CdMusicPlay(cdMusicTrack, 0);
            }
        } else if (GetMenuKey(3)) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            menuState.music++;
            if (menuState.music > cdMusicCount)
                menuState.music = 1;
            if (menuState.music != 1) {
                CdMusicPlay(menuState.music, 0);
            } else {
                InitShufflePlay();
                cdMusicIndex = 0;
                cdMusicTrack = cdMusicTable[cdMusicIndex];
                CdMusicPlay(cdMusicTrack, 0);
            }
        }
        break;
    }
    if (menuState.music == 0) {
        Print(GetCdTrackName(menuState.music), 210, 190, 0);
        AddNumber(menuState.music, 135, 184, 8, 0);
        Print(GetCdTrackArtist(menuState.music), 210, 182, 0);
    } else {
        Print(GetCdTrackName(menuState.music - 1), 210, 190, 0);
        AddNumber(menuState.music - 1, 135, 184, 8, 0);
        Print(GetCdTrackArtist(menuState.music - 1), 210, 182, 0);
    }
    Print("ARTIST", 170, 182, 0);
    Print("TITLE", 170, 190, 0);
    musicBarX = menuState.musicVolume * 6430 >> 12;
    sfxPos = menuState.effectVolume * 6430 >> 12;
    SetTextQuad(&knob, musicBarX + 105, 103, 116, 178, 32, 36, 35, 0);
    DrawScaledTexturedQuad(textDepth, &knob, hudTexturePage);
    SetTextQuad(&knob, sfxPos + 105, 142, 116, 178, 32, 36, 35, 0);
    DrawScaledTexturedQuad(textDepth, &knob, hudTexturePage);
    SetTextQuad(&knob, 100, 100, 0, 128, 254, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &knob, hudTexturePage);
    SetTextQuad(&knob, 227, 100, 0, 178, 114, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &knob, hudTexturePage);
    SetTextQuad(&knob, 100, 139, 0, 128, 254, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &knob, hudTexturePage);
    SetTextQuad(&knob, 227, 139, 0, 178, 114, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &knob, hudTexturePage);
}
#else
/* The audio screen: music and effect volume, and the CD track. */
void AudioConfig(PcGameData *gameData)
{
    POLY_FT4 bar;
    signed char savedTrack;
    int musicX, sfxX;

    savedTrack = gameData->cdTrack;
    gameData->cdTrack = menuState.music;
    SetInGameCDTrack(menuTextureA, menuTextureB, gameData, &soundChanged);
    gameData->cdTrack = savedTrack;
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0)
            menuState.current = 0;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current >= 3)
            menuState.current = 2;
        else
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
    }
    if (menuState.current != 1) {
        SetNoteVolume(shieldHumVoice, 0);
        SfxEffectLRPan(shieldHumVoice, 15);
        SfxEffectFBPan(shieldHumVoice, 15);
    }
    switch (menuState.current) {
    case 0:
        if (blinkFlag)
            AddText("MUSIC", 30, 108, 8, 1);
        AddText("SFX", 30, 147, 8, 0);
        AddText("CD TRACK", 25, 184, 8, 0);
        if (padButtons & PAD_BUTTON_RIGHT) {
            menuState.musicVolume++;
            if (menuState.musicVolume >= 100)
                menuState.musicVolume = 100;
        }
        if (padButtons & PAD_BUTTON_LEFT) {
            menuState.musicVolume--;
            if (menuState.musicVolume < 0)
                menuState.musicVolume = 0;
        }
        menuState.otherMusic = menuState.musicVolume;
        SfxSetCdVolume(menuState.otherMusic * 255 / 100);
        break;
    case 1:
        AddText("MUSIC", 30, 108, 8, 0);
        if (blinkFlag)
            AddText("SFX", 30, 147, 8, 1);
        AddText("CD TRACK", 25, 184, 8, 0);
        if (padButtons & PAD_BUTTON_RIGHT) {
            menuState.effectVolume++;
            if (menuState.effectVolume >= 100)
                menuState.effectVolume = 100;
        }
        if (padButtons & PAD_BUTTON_LEFT) {
            menuState.effectVolume--;
            if (menuState.effectVolume < 0)
                menuState.effectVolume = 0;
        }
        menuState.otherEffect = menuState.effectVolume;
        SfxSetVolume(menuState.otherEffect * 255 / 100);
        SetNoteVolume(shieldHumVoice, 255);
        SfxEffectLRPan(shieldHumVoice, 15);
        SfxEffectFBPan(shieldHumVoice, 15);
        break;
    case 2:
        AddText("MUSIC", 30, 108, 8, 0);
        AddText("SFX", 30, 147, 8, 0);
        if (blinkFlag)
            AddText("CD TRACK", 25, 184, 8, 1);
        if (GetMenuKey(2)) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            menuState.music--;
            if (menuState.music < 1)
                menuState.music = cdMusicCount;
            if (menuState.music != 1) {
                CdMusicPlay(menuState.music, 0);
            } else {
                InitShufflePlay();
                cdMusicIndex = 0;
                cdMusicTrack = cdMusicTable[0];
                CdMusicPlay(cdMusicTrack, 0);
            }
        } else if (GetMenuKey(3)) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            menuState.music++;
            if (menuState.music > cdMusicCount)
                menuState.music = 1;
            if (menuState.music != 1) {
                CdMusicPlay(menuState.music, 0);
            } else {
                InitShufflePlay();
                cdMusicIndex = 0;
                cdMusicTrack = cdMusicTable[0];
                CdMusicPlay(cdMusicTrack, 0);
            }
        }
        break;
    }
    if (menuState.music == 0) {
        Print(GetCdTrackName(menuState.music), 210, 190, 0);
        AddNumber(menuState.music, 135, 184, 8, 0);
        Print(GetCdTrackArtist(menuState.music), 210, 182, 0);
    } else {
        Print(GetCdTrackName(menuState.music - 1), 210, 190, 0);
        AddNumber(menuState.music - 1, 135, 184, 8, 0);
        Print(GetCdTrackArtist(menuState.music - 1), 210, 182, 0);
    }
    Print("ARTIST", 170, 182, 0);
    Print("TITLE", 170, 190, 0);
    musicX = menuState.musicVolume * 6430 >> 12;
    sfxX = menuState.effectVolume * 6430 >> 12;
    SetTextQuad(&bar, musicX + 105, 103, 116, 178, 32, 36, 35, 0);
    DrawScaledTexturedQuad(textDepth, &bar, hudTexturePage);
    SetTextQuad(&bar, sfxX + 105, 142, 116, 178, 32, 36, 35, 0);
    DrawScaledTexturedQuad(textDepth, &bar, hudTexturePage);
    SetTextQuad(&bar, 100, 100, 0, 128, 254, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &bar, hudTexturePage);
    SetTextQuad(&bar, 227, 100, 0, 178, 114, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &bar, hudTexturePage);
    SetTextQuad(&bar, 100, 139, 0, 128, 254, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &bar, hudTexturePage);
    SetTextQuad(&bar, 227, 139, 0, 178, 114, 48, 35, 0);
    DrawScaledTexturedQuad(textDepth + 0.1f, &bar, hudTexturePage);
}
#endif


/* Draws the five performance bars of the selected ship's team at x, y.
   The team and ratings storage become bar widths after the lookup. */
#ifdef EQUIV
#ifdef NETPLAY
void ShipParameters(ConfigData *config, short x, short y)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short curSpeedPoints;
    short curThrustBar;
    short lastAerodynamicsWidth;
    short curStart;
    short curFull;
    short localTurning;
    short nextShieldPixels;
    short lastEntry;
    POLY_FT4 curMeter;
    short curRatio;
    u_char outRating[5][5];

    outRating[0][0] = 7;
    outRating[0][1] = 5;
    outRating[0][2] = 6;
    outRating[0][3] = 3;
    outRating[0][4] = 6;
    outRating[1][0] = 5;
    outRating[1][1] = 6;
    outRating[1][2] = 5;
    outRating[1][3] = 5;
    outRating[1][4] = 5;
    outRating[2][0] = 4;
    outRating[2][1] = 7;
    outRating[2][2] = 3;
    outRating[2][3] = 7;
    outRating[2][4] = 5;
    outRating[3][0] = 6;
    outRating[3][1] = 3;
    outRating[3][2] = 7;
    outRating[3][3] = 6;
    outRating[3][4] = 7;
    outRating[4][0] = 10;
    outRating[4][1] = 10;
    outRating[4][2] = 10;
    outRating[4][3] = 10;
    outRating[4][4] = 10;

    curThrustBar = 0;
    curSpeedPoints = 0;
    localTurning = 0;
    nextShieldPixels = 0;
    lastAerodynamicsWidth = 0;
    curRatio = 7;
    curStart = 0x4d;
    curFull = curRatio * 10;
    lastEntry = menuState.ship / 3;
    Print("THRUST", x, y, 0);
    Print("TOP SPEED", x, y + 7, 0);
    Print("TURNING ABILITY", x, y + 14, 0);
    Print("SHIELD ENERGY", x, y + 21, 0);
    Print("AERODYNAMICS", x, y + 28, 0);
    curThrustBar = outRating[lastEntry][0] * curRatio;
    curSpeedPoints = outRating[lastEntry][1] * curRatio;
    localTurning = outRating[lastEntry][2] * curRatio;
    nextShieldPixels = outRating[lastEntry][3] * curRatio;
    lastAerodynamicsWidth = outRating[lastEntry][4] * curRatio;
    x += curStart;
    SetTextQuad(&curMeter, x, y, 0, 100, curThrustBar * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    SetTextQuad(&curMeter, x + curThrustBar, y, 0, 112, (curFull - curThrustBar) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    y += 7;
    SetTextQuad(&curMeter, x, y, 0, 100, curSpeedPoints * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    SetTextQuad(&curMeter, x + curSpeedPoints, y, 0, 112, (curFull - curSpeedPoints) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    y += 7;
    SetTextQuad(&curMeter, x, y, 0, 100, localTurning * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    SetTextQuad(&curMeter, x + localTurning, y, 0, 112, (curFull - localTurning) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    y += 7;
    SetTextQuad(&curMeter, x, y, 0, 100, nextShieldPixels * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    SetTextQuad(&curMeter, x + nextShieldPixels, y, 0, 112, (curFull - nextShieldPixels) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    y += 7;
    SetTextQuad(&curMeter, x, y, 0, 100, lastAerodynamicsWidth * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
    SetTextQuad(&curMeter, x + lastAerodynamicsWidth, y, 0, 112, (curFull - lastAerodynamicsWidth) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &curMeter, hudTexturePage);
}
#else
void ShipParameters(ConfigData *config, short x, short y)
{
    struct {
        union {
            u_char ratings[5][5];
            short aerodynamicsWidth;
        } values;
        POLY_FT4 bar;
    } chart;
    short barX, barY;
    short team;
    union {
        short team;
        short topSpeedWidth;
    } selection;
    short shieldWidth, turningWidth, thrustWidth;
    u_char aerodynamicsFill;

    /* Initialize the ratings while leaving the quad for SetTextQuad to fill. */
    chart.values.ratings[0][0] = 7;
    chart.values.ratings[0][1] = 5;
    chart.values.ratings[0][2] = 6;
    chart.values.ratings[0][3] = 3;
    chart.values.ratings[0][4] = 6;
    chart.values.ratings[1][0] = 5;
    chart.values.ratings[1][1] = 6;
    chart.values.ratings[1][2] = 5;
    chart.values.ratings[1][3] = 5;
    chart.values.ratings[1][4] = 5;
    chart.values.ratings[2][0] = 4;
    chart.values.ratings[2][1] = 7;
    chart.values.ratings[2][2] = 3;
    chart.values.ratings[2][3] = 7;
    chart.values.ratings[2][4] = 5;
    chart.values.ratings[3][0] = 6;
    chart.values.ratings[3][1] = 3;
    chart.values.ratings[3][2] = 7;
    chart.values.ratings[3][3] = 6;
    chart.values.ratings[3][4] = 7;
    chart.values.ratings[4][0] = 10;
    chart.values.ratings[4][1] = 10;
    chart.values.ratings[4][2] = 10;
    chart.values.ratings[4][3] = 10;
    chart.values.ratings[4][4] = 10;

    selection.team = menuState.ship / 3;
    Print("THRUST", x, y, 0);
    Print("TOP SPEED", x, y + 7, 0);
    Print("TURNING ABILITY", x, y + 14, 0);
    Print("SHIELD ENERGY", x, y + 21, 0);
    Print("AERODYNAMICS", x, y + 28, 0);
    team = selection.team;
    thrustWidth = chart.values.ratings[team][0] * 7;
    selection.topSpeedWidth = chart.values.ratings[team][1] * 7;
    turningWidth = chart.values.ratings[team][2] * 7;
    shieldWidth = chart.values.ratings[team][3] * 7;
    chart.values.aerodynamicsWidth = chart.values.ratings[team][4] * 7;
    barX = x + 0x4d;
    barY = y;
    SetTextQuad(&chart.bar, barX, barY, 0, 100, thrustWidth * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    SetTextQuad(&chart.bar, barX + thrustWidth, barY, 0, 112, (198 - thrustWidth) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    barY += 7;
    SetTextQuad(&chart.bar, barX, barY, 0, 100, selection.topSpeedWidth * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    SetTextQuad(&chart.bar, barX + selection.topSpeedWidth, barY, 0, 112, (198 - selection.topSpeedWidth) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    barY += 7;
    SetTextQuad(&chart.bar, barX, barY, 0, 100, turningWidth * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    SetTextQuad(&chart.bar, barX + turningWidth, barY, 0, 112, (198 - turningWidth) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    barY += 7;
    SetTextQuad(&chart.bar, barX, barY, 0, 100, shieldWidth * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    SetTextQuad(&chart.bar, barX + shieldWidth, barY, 0, 112, (198 - shieldWidth) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    barY += 7;
    /* Keep the byte fill width across the draws; the full width positions the quad. */
    aerodynamicsFill = chart.values.aerodynamicsWidth;
    SetTextQuad(&chart.bar, barX, barY, 0, 100, aerodynamicsFill * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
    barX += chart.values.aerodynamicsWidth;
    SetTextQuad(&chart.bar, barX, barY, 0, 112, (198 - aerodynamicsFill) * 2, 10, 32, 0);
    DrawScaledTexturedQuad(textDepth, &chart.bar, hudTexturePage);
}
#endif
#endif


extern int gameType;
#ifndef NETPLAY
extern short lastTopOption;
#else
extern short lastTopMenuSelection;
#endif

#ifdef NETPLAY
extern signed char networkGame;
#endif
#ifdef NETPLAY
/* The team screen: the ring of teams, the ship of the chosen team and its description. */
void TeamSelect(PcGameData *gameData)
{
    /* Local names chosen to reproduce the exe's frame layout; the exe sets first and never reads it. */
    signed char first = 0;
    signed char shipsPerTeam = 3;
    signed char teams = 4;
    TeamOrder teamOrder = { { 3, 0, 1, 2, 4 } };

    if (gameData->piranhaUnlocked != 0) {
        teams = 5;
    }
    if (gameData->teamCheatSigned != 0) {
        teams = 5;
        PrintCX("TEAM CHEAT ACTIVE", 160, 204, 0);
    }
    if (networkGame) {
        teams = 5;
    }
    if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current >= teams) {
            menuState.current = teams - 1;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0) {
            menuState.current = 0;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    menuState.ship = teamOrder.order[menuState.current] * shipsPerTeam;
    if (sillyShips == 1) {
        DrawTeamShips(450, 0, 1200, menuHeading.angle, 0, shipShapeTable[highResTable[menuState.ship]]);
    } else {
        DrawTeamShips(450, -50, 1200, menuHeading.angle, 0, shipShapeTable[highResTable[menuState.ship]]);
    }
    if (menuState.current == 0) {
        AddText("FEISAR", 30, 90, 8, 1);
    } else {
        AddText("FEISAR", 30, 90, 8, 0);
    }
    if (menuState.current == 1) {
        AddText("AG SYSTEMS", 30, 100, 8, 1);
    } else {
        AddText("AG SYSTEMS", 30, 100, 8, 0);
    }
    if (menuState.current == 2) {
        AddText("AURICOM", 30, 110, 8, 1);
    } else {
        AddText("AURICOM", 30, 110, 8, 0);
    }
    if (menuState.current == 3) {
        AddText("QIREX", 30, 120, 8, 1);
    } else {
        AddText("QIREX", 30, 120, 8, 0);
    }
    if (menuState.current == 4) {
        AddText("PIRANHA", 30, 130, 8, 1);
    } else if (gameData->piranhaUnlocked || gameData->teamCheatSigned || networkGame) {
        AddText("PIRANHA", 30, 130, 8, 0);
    }
    switch (menuState.current) {
    case 0:
        Print("EXCELLENT TURNING ABILITY AND EASY TO CONTROL", 30, 182, 0);
        Print("BUT NOT TOO QUICK.  [BEGINNER]", 30, 189, 0);
        break;
    case 1:
        Print("GOOD ACCELERATION AND TURNING ABILITY BUT", 30, 182, 0);
        Print("WEAK SHIELD.  [AMATEUR]", 30, 189, 0);
        break;
    case 2:
        Print("GOOD ALL ROUNDER BUT AVERAGE PERFORMANCE.", 30, 182, 0);
        Print("[INTERMEDIATE]", 30, 189, 0);
        break;
    case 3:
        Print("FAST WITH GOOD SHIELD CAPABILITY BUT SLIGHTLY", 30, 182, 0);
        Print("HEAVY AT TURNING.  [EXPERT]", 30, 189, 0);
        break;
    case 4:
        Print("VERY FAST WITH EXCELLENT SHIELD BUT NO", 30, 182, 0);
        Print("WEAPON TECHNOLOGY.  [MASTER]", 30, 189, 0);
        break;
    }
    ShipParameters(gameData, 155, 142);
}
#else
/* The team screen: the ring of teams, the ship of the chosen team and its description. */
void TeamSelect(PcGameData *gameData)
{
    TeamOrder teamOrder = { { 3, 0, 1, 2, 4 } };
    signed char teamCount;

    teamCount = 4;
    if (gameData->class2Unlocked != 0) {
        teamCount = 5;
    }
    if (gameData->teamCheat != 0) {
        teamCount = 5;
        PrintCX("TEAM CHEAT ACTIVE", 160, 204, 0);
    }
    if (GetMenuKey(1)) {
        menuState.current++;
        if (menuState.current >= teamCount) {
            menuState.current = teamCount - 1;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    if (GetMenuKey(0)) {
        menuState.current--;
        if (menuState.current < 0) {
            menuState.current = 0;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    menuState.ship = 3 * teamOrder.order[menuState.current];
    if (sillyShips == 1) {
        DrawTeamShips(450, 0, 1200, menuHeading.angle, 0, shipShapeTable[highResTable[menuState.ship]]);
    } else {
        DrawTeamShips(450, -50, 1200, menuHeading.angle, 0, shipShapeTable[highResTable[menuState.ship]]);
    }
    if (menuState.current == 0) {
        AddText("FEISAR", 30, 90, 8, 1);
    } else {
        AddText("FEISAR", 30, 90, 8, 0);
    }
    if (menuState.current == 1) {
        AddText("AG SYSTEMS", 30, 100, 8, 1);
    } else {
        AddText("AG SYSTEMS", 30, 100, 8, 0);
    }
    if (menuState.current == 2) {
        AddText("AURICOM", 30, 110, 8, 1);
    } else {
        AddText("AURICOM", 30, 110, 8, 0);
    }
    if (menuState.current == 3) {
        AddText("QIREX", 30, 120, 8, 1);
    } else {
        AddText("QIREX", 30, 120, 8, 0);
    }
    if (menuState.current == 4) {
        AddText("PIRANHA", 30, 130, 8, 1);
    } else if (gameData->class2Unlocked || gameData->teamCheat) {
        AddText("PIRANHA", 30, 130, 8, 0);
    }
    switch (menuState.current) {
    case 0:
        Print("EXCELLENT TURNING ABILITY AND EASY TO CONTROL", 30, 182, 0);
        Print("BUT NOT TOO QUICK.  [BEGINNER]", 30, 189, 0);
        break;
    case 1:
        Print("GOOD ACCELERATION AND TURNING ABILITY BUT", 30, 182, 0);
        Print("WEAK SHIELD.  [AMATEUR]", 30, 189, 0);
        break;
    case 2:
        Print("GOOD ALL ROUNDER BUT AVERAGE PERFORMANCE.", 30, 182, 0);
        Print("[INTERMEDIATE]", 30, 189, 0);
        break;
    case 3:
        Print("FAST WITH GOOD SHIELD CAPABILITY BUT SLIGHTLY", 30, 182, 0);
        Print("HEAVY AT TURNING.  [EXPERT]", 30, 189, 0);
        break;
    case 4:
        Print("VERY FAST WITH EXCELLENT SHIELD BUT NO", 30, 182, 0);
        Print("WEAPON TECHNOLOGY.  [MASTER]", 30, 189, 0);
        break;
    }
    ShipParameters(gameData, 155, 142);
}
#endif


extern Object *menuModels[];
extern int awardedMedal;
extern u_char envmapr, envmapg, envmapb;


/* The class screen: the track ring of the chosen class, its ship and the medal of the track.
   Proven equivalent rather than byte-exact: the same blocks in the same order, but the exe keeps
   gameData in esi and loads the menu index in other registers. */
#ifdef NETPLAY
void ClassSelect(PcGameData *gameData)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    signed char trackIndex = 0;
    signed char tracks = 2;
    ClassOrder classOrder = { { 1, 0, 3, 5 } };

    if (gameData->phantomUnlocked != 0 || gameData->phantomCheatSigned != 0) {
        tracks = 6;
        if (menu[currMenu].selection == 3) {
            tracks = 2;
        }
    }
    if (gameData->phantomCheatSigned != 0) {
        PrintCX("PHANTOM TRACK CHEAT ACTIVE", 210, 75, 0);
    }
    if (gameData->piranhaChallenge != 0 || gameData->trackCheatSigned != 0) {
        if (menu[currMenu].selection == 0 || menu[currMenu].selection == 1) {
            tracks = 6;
        } else {
            tracks = 8;
        }
    }
    if (gameData->trackCheatSigned != 0) {
        PrintCX("TRACK CHEAT ACTIVE", 160, 204, 0);
    }
    if (GetMenuKey(3)) {
        menuState.trackSelection++;
        if (tracks > menuState.trackSelection) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    if (menuState.trackSelection >= tracks - 1) {
        menuState.trackSelection = tracks - 1;
    }
    if (GetMenuKey(2)) {
        menuState.trackSelection--;
        if (menuState.trackSelection < 0) {
            menuState.trackSelection = 0;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    if (gameData->phantomUnlocked != 0 || gameData->piranhaChallenge != 0 || gameData->phantomCheatSigned != 0 || gameData->trackCheatSigned != 0) {
        menuState.actTrackNum = raceTracks[menuState.trackSelection];
        menuState.trackNum = menuState.trackSelection;
    }
    if (menu[2].selection == 0) {
        trackIndex = menuState.trackSelection;
        if (gameData->phantomUnlocked == 0 || gameData->piranhaChallenge == 0 || gameData->phantomCheatSigned == 0 || gameData->trackCheatSigned == 0) {
            menuState.actTrackNum = raceTracks[trackIndex];
            menuState.trackNum = trackIndex;
        }
        menuState.shipClass = 0;
    }
    if (menu[2].selection == 1) {
        trackIndex = menuState.trackSelection + 2;
        if (gameData->phantomUnlocked == 0 && gameData->piranhaChallenge == 0 && gameData->phantomCheatSigned == 0 && gameData->trackCheatSigned == 0) {
            menuState.actTrackNum = raceTracks[trackIndex];
            menuState.trackNum = trackIndex;
        }
        menuState.shipClass = 1;
    }
    if (menu[2].selection == 2) {
        trackIndex = menuState.trackSelection + 4;
        if (gameData->phantomUnlocked == 0 && gameData->piranhaChallenge == 0 && gameData->phantomCheatSigned == 0 && gameData->trackCheatSigned == 0) {
            menuState.actTrackNum = raceTracks[trackIndex];
            menuState.trackNum = trackIndex;
        }
        menuState.shipClass = 2;
    }
    if (menu[2].selection == 3) {
        trackIndex = menuState.trackSelection + 6;
        if (gameData->piranhaChallenge == 0 && gameData->trackCheatSigned == 0) {
            menuState.actTrackNum = raceTracks[trackIndex];
            menuState.trackNum = trackIndex;
        }
        menuState.shipClass = 3;
    }
    switch (menu[2].selection) {
    case 0:
        PrintCX("NOVICE", 68, 186, 0);
        break;
    case 1:
        PrintCX("INTERMEDIATE", 68, 186, 0);
        break;
    case 2:
        PrintCX("EXPERT", 68, 186, 0);
        break;
    case 3:
        PrintCX("SPEED DEMON", 68, 186, 0);
        break;
    }
    switch (menuState.trackNum) {
    case 0:
        Print("A MAJOR CANADIAN INDUSTRIAL", 117, 182, 0);
        Print("COMPLEX. [EASY]", 117, 189, 0);
        break;
    case 1:
        Print("A SNOWY TIBETAN MOUNTAIN CIRCUIT.", 117, 182, 0);
        Print("[EASY]", 117, 189, 0);
        break;
    case 2:
        Print("A SOUTH AMERICAN JUNGLE CIRCUIT.", 117, 182, 0);
        Print("[TRICKY]", 117, 189, 0);
        break;
    case 3:
        Print("A NEWLY CONSTRUCTED GERMAN", 117, 182, 0);
        Print("COMMERCIAL PARK.  [TRICKY]", 117, 189, 0);
        break;
    case 4:
        Print("A DISUSED FRENCH METRO SYSTEM.", 117, 182, 0);
        Print("[HARD]", 117, 189, 0);
        break;
    case 5:
        Print("A HUGE SUSPENDED CIRCUIT OVER THE", 117, 182, 0);
        Print("BLACK SEA. [HARD]", 117, 189, 0);
        break;
    case 6:
        Print("AN OBSCURE VOLCANIC ISLAND LOCATED", 117, 182, 0);
        Print("IN THE SOUTH PACIFIC.  [VERY HARD]", 117, 189, 0);
        break;
    case 7:
        Print("AN AMERICAN CITY CIRCUIT AFFECTED", 117, 182, 0);
        Print("BY SEISMIC ACTIVITY.  [VERY HARD]", 117, 189, 0);
        break;
    }
    if (blinkFlag != 0) {
        if (menuState.trackSelection < tracks - 1) {
            DoButton(280, 163, 4, "");
        }
        if (menuState.trackSelection > 0) {
            DoButton(125, 163, 3, "");
        }
    }
    DrawTeamShips(400, 100, 1800, menuHeading.angle, 220, shipModels[trackModelOrder[menuState.trackNum]]);
    PrintCX(GetTrackName(menuState.actTrackNum, 90, 90, 0), 160, 90, 0);
    switch (tracksWon[menuState.trackNum].medal) {
    case 1:
        if (blinkFlag != 0) {
            PrintCX("GOLD", 260, 90, 0);
            PrintCX("AWARDED", 260, 97, 0);
        }
        awardedMedal = 1;
        envmapr = 0xFF;
        envmapg = 0x80;
        envmapb = 0;
        break;
    case 2:
        if (blinkFlag != 0) {
            PrintCX("SILVER", 260, 90, 0);
            PrintCX("AWARDED", 260, 97, 0);
        }
        awardedMedal = 2;
        envmapr = 0xE0;
        envmapg = 0xE0;
        envmapb = 0xE0;
        break;
    case 3:
        if (blinkFlag != 0) {
            PrintCX("BRONZE", 260, 90, 0);
            PrintCX("AWARDED", 260, 97, 0);
        }
        awardedMedal = 3;
        envmapr = 0xC0;
        envmapg = 0x20;
        envmapb = 0;
        break;
    }
    if (tracksWon[menuState.trackNum].medal != 0) {
        DrawTeamShips(-700, 300, 1400, menuHeading.angle, -1, menuModels[classOrder.order[menu[currMenu].selection]]);
    } else {
        DrawTeamShips(-700, 300, 1400, menuHeading.angle, 0, menuModels[classOrder.order[menu[currMenu].selection]]);
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
void ClassSelect(PcGameData *gameData)
{
    ClassOrder classOrder = { { 1, 0, 3, 5 } };
    signed char track;
    signed char trackCount;
    signed char medal;

    trackCount = 2;
    if (gameData->class1Unlocked != 0 || gameData->phantomCheat != 0) {
        trackCount = 6;
        if (menu[currMenu].selection == 3) {
            trackCount = 2;
        }
    }
    if (gameData->phantomCheat != 0) {
        PrintCX("PHANTOM TRACK CHEAT ACTIVE", 210, 75, 0);
    }
    if (gameData->rapierComplete != 0 || gameData->trackCheat != 0) {
        trackCount = 8;
        if (menu[currMenu].selection == 0 || menu[currMenu].selection == 1) {
            trackCount = 6;
        }
        if (gameData->trackCheat != 0) {
            PrintCX("TRACK CHEAT ACTIVE", 160, 204, 0);
        }
    }
    if (GetMenuKey(3)) {
        menuState.trackSelection++;
        if (trackCount > menuState.trackSelection) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    if (menuState.trackSelection >= trackCount - 1) {
        menuState.trackSelection = trackCount - 1;
    }
    if (GetMenuKey(2)) {
        menuState.trackSelection--;
        if (menuState.trackSelection < 0) {
            menuState.trackSelection = 0;
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
    if (gameData->class1Unlocked != 0 || gameData->rapierComplete != 0 || gameData->phantomCheat != 0 || gameData->trackCheat != 0) {
        menuState.trackNum = menuState.trackSelection;
        menuState.actTrackNum = raceTracks[menuState.trackSelection];
    }
    if (menu[2].selection == 0) {
        track = menuState.trackSelection;
        if (gameData->class1Unlocked == 0 || gameData->rapierComplete == 0 || gameData->phantomCheat == 0 || gameData->trackCheat == 0) {
            menuState.trackNum = track;
            menuState.actTrackNum = raceTracks[track];
        }
        menuState.shipClass = 0;
    }
    if (menu[2].selection == 1) {
        track = menuState.trackSelection + 2;
        if (gameData->class1Unlocked == 0 && gameData->rapierComplete == 0 && gameData->phantomCheat == 0 && gameData->trackCheat == 0) {
            menuState.trackNum = track;
            menuState.actTrackNum = raceTracks[track];
        }
        menuState.shipClass = 1;
    }
    if (menu[2].selection == 2) {
        track = menuState.trackSelection + 4;
        if (gameData->class1Unlocked == 0 && gameData->rapierComplete == 0 && gameData->phantomCheat == 0 && gameData->trackCheat == 0) {
            menuState.trackNum = track;
            menuState.actTrackNum = raceTracks[track];
        }
        menuState.shipClass = 2;
    }
    if (menu[2].selection == 3) {
        track = menuState.trackSelection + 6;
        if (gameData->rapierComplete == 0 && gameData->trackCheat == 0) {
            menuState.trackNum = track;
            menuState.actTrackNum = raceTracks[track];
        }
        menuState.shipClass = 3;
    }
    switch (menu[2].selection) {
    case 0:
        PrintCX("NOVICE", 68, 186, 0);
        break;
    case 1:
        PrintCX("INTERMEDIATE", 68, 186, 0);
        break;
    case 2:
        PrintCX("EXPERT", 68, 186, 0);
        break;
    case 3:
        PrintCX("SPEED DEMON", 68, 186, 0);
        break;
    }
    switch (menuState.trackNum) {
    case 0:
        Print("A MAJOR CANADIAN INDUSTRIAL", 117, 182, 0);
        Print("COMPLEX. [EASY]", 117, 189, 0);
        break;
    case 1:
        Print("A SNOWY TIBETAN MOUNTAIN CIRCUIT.", 117, 182, 0);
        Print("[EASY]", 117, 189, 0);
        break;
    case 2:
        Print("A SOUTH AMERICAN JUNGLE CIRCUIT.", 117, 182, 0);
        Print("[TRICKY]", 117, 189, 0);
        break;
    case 3:
        Print("A NEWLY CONSTRUCTED GERMAN", 117, 182, 0);
        Print("COMMERCIAL PARK.  [TRICKY]", 117, 189, 0);
        break;
    case 4:
        Print("A DISUSED FRENCH METRO SYSTEM.", 117, 182, 0);
        Print("[HARD]", 117, 189, 0);
        break;
    case 5:
        Print("A HUGE SUSPENDED CIRCUIT OVER THE", 117, 182, 0);
        Print("BLACK SEA. [HARD]", 117, 189, 0);
        break;
    case 6:
        Print("AN OBSCURE VOLCANIC ISLAND LOCATED", 117, 182, 0);
        Print("IN THE SOUTH PACIFIC.  [VERY HARD]", 117, 189, 0);
        break;
    case 7:
        Print("AN AMERICAN CITY CIRCUIT AFFECTED", 117, 182, 0);
        Print("BY SEISMIC ACTIVITY.  [VERY HARD]", 117, 189, 0);
        break;
    }
    if (blinkFlag != 0) {
        if (menuState.trackSelection < trackCount - 1) {
            DoButton(280, 163, 4, "");
        }
        if (menuState.trackSelection > 0) {
            DoButton(125, 163, 3, "");
        }
    }
    DrawTeamShips(400, 100, 1800, menuHeading.angle, 220, shipModels[trackModelOrder[menuState.trackNum]]);
    PrintCX(GetTrackName(menuState.actTrackNum, 90, 90, 0), 160, 90, 0);
    medal = tracksWon[menuState.trackNum].medal;
    switch (medal) {
    case 1:
        if (blinkFlag != 0) {
            PrintCX("GOLD", 260, 90, 0);
            PrintCX("AWARDED", 260, 97, 0);
        }
        awardedMedal = 1;
        envmapr = 0xFF;
        envmapg = 0x80;
        envmapb = 0;
        break;
    case 2:
        if (blinkFlag != 0) {
            PrintCX("SILVER", 260, 90, 0);
            PrintCX("AWARDED", 260, 97, 0);
        }
        awardedMedal = 2;
        envmapr = 0xE0;
        envmapg = 0xE0;
        envmapb = 0xE0;
        break;
    case 3:
        if (blinkFlag != 0) {
            PrintCX("BRONZE", 260, 90, 0);
            PrintCX("AWARDED", 260, 97, 0);
        }
        awardedMedal = 3;
        envmapr = 0xC0;
        envmapg = 0x20;
        envmapb = 0;
        break;
    }
    if (tracksWon[menuState.trackNum].medal != 0) {
        DrawTeamShips(-700, 300, 1400, menuHeading.angle, -1, menuModels[classOrder.order[menu[currMenu].selection]]);
    } else {
        DrawTeamShips(-700, 300, 1400, menuHeading.angle, 0, menuModels[classOrder.order[menu[currMenu].selection]]);
    }
}
#endif
#endif




#ifdef NETPLAY
/* The network build's lobby and session code sits between the class screen and the menu box editor. */
#pragma intrinsic(strlen, strcat, memcmp)
extern GlyphWidth glyphWidth[];

/* True when `value` starts with the text. */
#define STARTS_WITH(value, text) (memcmp(value, text, strlen(text)) == 0)

/* Sends the player's name as a chat line through the default port; true when the layer accepted it. */
int gstChangePlayerName(const char *name, unsigned long id)
{
    int result;

    result = gstSendChatGPI(0, 1, id, name);
    return result < 1 ? 0 : 1;
}

/* Queues a session packet for the network game and logs a failure from the gst layer. */
void gstPostSessionAsynch(int destination, void *data, int size, u_char port)
{
    int result;

    result = gstNewUpPacket(port, 1, destination, size, data);
    if (result < 1) {
        gstDebugPrintf("gstPostSessionAsynch: Error gstNewUpPacket %d [%s]", result, gstErrorString(result));
        return;
    }
}

/* Queues a lockstep packet; the error text still names gstNewUpPacket, as the network build does. */
void gstPostLockstepAsynch(void *data, int size, u_char port)
{
    int result;

    result = gstNewLockstepUpPacket(port, 1, size, data);
    if (result < 1) {
        gstDebugPrintf("gstPostLockstepAsynch: Error gstNewUpPacket %d [%s]", result, gstErrorString(result));
        return;
    }
}

/* Asks the host for the session's level block (the race settings, as "key=value" lines) and
   applies it. */
int gstGetLevelBlock(unsigned long sessionId)
{
    char *pValue;
    char *left;
    int n;
    char *endPtr;
    unsigned short word;
    unsigned char end2;
    unsigned char tag;
    unsigned long handle;
    unsigned long parent;
    unsigned char part;
    unsigned char objKind;
    char l1;
    GstCodec *rx;
    char descBytesLen;
    int ret;
    char description[100];
    char sessName[100];
    GstCodec *upPacket;

    gstAllocUpPacket(&upPacket, 0, 100);
    gstEncodeU8(upPacket, 'B');
    gstEncodeU8(upPacket, 3);
    gstEncodePackedU24(upPacket, sessionId);
    gstEncodeU8(upPacket, 3);
    gstEncodeU16(upPacket, 0x303);
    gstRegisterPolledPackets(1, 'B', 0);
    gstSendEncodedUpPacket(upPacket, 0);
    ret = gstGetDownPacket(&rx, 1000);
    gstRegisterPolledPackets(0, 'B', 0);
    if (ret < 0) {
        gstDebugPrintf("gstGetLevelBlock: gstGetDownPacket %d [%s]", ret, gstErrorString(ret));
        return 0;
    }
    gstDecodeU8(rx, &tag);
    gstDecodeU8(rx, &tag);
    gstDecodePackedU24(rx, &handle);
    gstDecodeU8(rx, &tag);
    gstDecodeU16(rx, &word);
    gstDecodeU8(rx, &part);
    gstDecodePackedU24(rx, &parent);
    gstDecodeU8(rx, &objKind);
    gstDecodeU8(rx, &l1);
    gstDecodeBytes(rx, l1, sessName);
    gstDecodeU8(rx, &descBytesLen);
    gstDecodeBytes(rx, descBytesLen, description);
    gstDecodePackedU32(rx, &gstLevelBlockSize);
    gstDecodeBytes(rx, gstLevelBlockSize, gstLevelBlock);
    gstDecodeU8(rx, &end2);
    sessName[l1] = 0;
    description[descBytesLen] = 0;
    if (gstLevelBlockSize == 0) {
        gameType = 0;
        gstRaceMode = 1;
        gameData->shipClass = 0;
        gameData->actTrackNum = 1;
        gameData->trackNum = 0;
        gstLevelJumpIn = 0;
        gstLevelJoinable = 1;
        gstLevelPiranha = 0;
    } else {
        if (gstDebugOutputEnabled) {
            gstLevelBlock[gstLevelBlockSize] = 0;
            printf("\n******************** JOIN: LevelBlockBuffer Size %d********************\n", gstLevelBlockSize);
            printf("%s", gstLevelBlock);
            printf("******************** JOIN: LevelBlockBuffer Size %d********************\n\n", gstLevelBlockSize);
        }
        for (n = 0; n < 9; n++)
            gstLevelWeapons[n] = 0;
        left = gstLevelBlock;
        for (n = 0; n < 9; n++) {
            endPtr = strchr(left, '=');
            if (endPtr) {
                *endPtr = 0;
            } else {
                gstDebugPrintf("gstGetLevelBlock: 1: unexpected end of Level Block");
                return 0;
            }
            endPtr++;
            pValue = endPtr;
            endPtr = strchr(pValue, '\n');
            if (endPtr) {
                *endPtr = 0;
            } else {
                gstDebugPrintf("gstGetLevelBlock: 2: unexpected end of Level Block");
                return 0;
            }
            if (STARTS_WITH(left, gstSessionText[TEXT_RACE_TYPE])) {
                if (STARTS_WITH(pValue, gstSessionText[TEXT_TIMED])) {
                    gameType = 0;
                    gstRaceMode = 0;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_DEATH_RACE])) {
                    gameType = 0;
                    gstRaceMode = 1;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_TOURNAMENT])) {
                    gameType = 0;
                    gstRaceMode = 3;
                } else {
                    gameType = 0;
                    gstRaceMode = 0;
                }
            } else if (STARTS_WITH(left, gstSessionText[TEXT_CLASS])) {
                if (gstRaceMode == 3) {
                    char *cp;
                    int slot;

                    slot = 0;
                    while ((cp = strchr(pValue, ',')) != 0) {
                        *cp = 0;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_VECTOR]))
                            gstLevelClasses[slot] = 0;
                        else if (STARTS_WITH(pValue, gstSessionText[TEXT_VENOM]))
                            gstLevelClasses[slot] = 1;
                        else if (STARTS_WITH(pValue, gstSessionText[TEXT_RAPIER]))
                            gstLevelClasses[slot] = 2;
                        else if (STARTS_WITH(pValue, gstSessionText[TEXT_PHANTOM]))
                            gstLevelClasses[slot] = 3;
                        slot++;
                        cp++;
                        pValue = cp;
                    }
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_VECTOR]))
                        gstLevelClasses[slot] = 0;
                    else if (STARTS_WITH(pValue, gstSessionText[TEXT_VENOM]))
                        gstLevelClasses[slot] = 1;
                    else if (STARTS_WITH(pValue, gstSessionText[TEXT_RAPIER]))
                        gstLevelClasses[slot] = 2;
                    else if (STARTS_WITH(pValue, gstSessionText[TEXT_PHANTOM]))
                        gstLevelClasses[slot] = 3;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_VECTOR])) {
                    gameData->shipClass = 0;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_VENOM])) {
                    gameData->shipClass = 1;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_RAPIER])) {
                    gameData->shipClass = 2;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_PHANTOM])) {
                    gameData->shipClass = 3;
                } else {
                    gameData->shipClass = 0;
                }
            }
            if (STARTS_WITH(left, gstSessionText[TEXT_TRACK_NAME])) {
                if (gstRaceMode == 3) {
                    char *cp;
                    int slot;

                    slot = 0;
                    while ((cp = strchr(pValue, ',')) != 0) {
                        *cp = 0;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_TALONS_REACH]))
                            gstLevelTracks[slot] = 0;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_SAGARMATHA]))
                            gstLevelTracks[slot] = 1;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_VALPARAISO]))
                            gstLevelTracks[slot] = 2;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_PHENITIA_PARK]))
                            gstLevelTracks[slot] = 3;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_GARE_D_EUROPA]))
                            gstLevelTracks[slot] = 4;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_ODESSA_KEYS]))
                            gstLevelTracks[slot] = 5;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_VOSTOK]))
                            gstLevelTracks[slot] = 6;
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_SPILSKINANKE]))
                            gstLevelTracks[slot] = 7;
                        slot++;
                        cp++;
                        pValue = cp;
                    }
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_TALONS_REACH]))
                        gstLevelTracks[slot] = 0;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_SAGARMATHA]))
                        gstLevelTracks[slot] = 1;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_VALPARAISO]))
                        gstLevelTracks[slot] = 2;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_PHENITIA_PARK]))
                        gstLevelTracks[slot] = 3;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_GARE_D_EUROPA]))
                        gstLevelTracks[slot] = 4;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_ODESSA_KEYS]))
                        gstLevelTracks[slot] = 5;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_VOSTOK]))
                        gstLevelTracks[slot] = 6;
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_SPILSKINANKE]))
                        gstLevelTracks[slot] = 7;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_TALONS_REACH])) {
                    gameData->trackNum = 0;
                    gameData->actTrackNum = 1;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_SAGARMATHA])) {
                    gameData->trackNum = 1;
                    gameData->actTrackNum = 8;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_VALPARAISO])) {
                    gameData->trackNum = 2;
                    gameData->actTrackNum = 13;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_PHENITIA_PARK])) {
                    gameData->trackNum = 3;
                    gameData->actTrackNum = 20;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_GARE_D_EUROPA])) {
                    gameData->trackNum = 4;
                    gameData->actTrackNum = 2;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_ODESSA_KEYS])) {
                    gameData->trackNum = 5;
                    gameData->actTrackNum = 17;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_VOSTOK])) {
                    gameData->trackNum = 6;
                    gameData->actTrackNum = 6;
                } else if (STARTS_WITH(pValue, gstSessionText[TEXT_SPILSKINANKE])) {
                    gameData->trackNum = 7;
                    gameData->actTrackNum = 7;
                } else {
                    gameData->actTrackNum = 1;
                    gameData->trackNum = 0;
                }
            } else if (STARTS_WITH(left, gstSessionText[TEXT_JUMP_IN])) {
                if (STARTS_WITH(pValue, gstSessionText[TEXT_TRUE]))
                    gstLevelJumpIn = 1;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_FALSE]))
                    gstLevelJumpIn = 0;
                else
                    gstLevelJumpIn = 0;
            } else if (STARTS_WITH(left, gstSessionText[TEXT_JOINABLE])) {
                if (STARTS_WITH(pValue, gstSessionText[TEXT_TRUE]))
                    gstLevelJoinable = 1;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_FALSE]))
                    gstLevelJoinable = 0;
                else
                    gstLevelJoinable = 1;
            } else if (STARTS_WITH(left, gstSessionText[TEXT_LAPS])) {
                if (STARTS_WITH(pValue, gstSessionText[TEXT_DEFAULT]))
                    gstLevelLaps = 0;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_ONE]))
                    gstLevelLaps = 1;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_TWO]))
                    gstLevelLaps = 2;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_THREE]))
                    gstLevelLaps = 3;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_FOUR]))
                    gstLevelLaps = 4;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_FIVE]))
                    gstLevelLaps = 5;
                else
                    gstLevelLaps = 0;
            } else if (STARTS_WITH(left, gstSessionText[TEXT_LIVES])) {
                if (STARTS_WITH(pValue, gstSessionText[TEXT_UNLIMITED]))
                    gstLevelLives = 0;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_ONE]))
                    gstLevelLives = 1;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_TWO]))
                    gstLevelLives = 2;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_THREE]))
                    gstLevelLives = 3;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_FOUR]))
                    gstLevelLives = 4;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_FIVE]))
                    gstLevelLives = 5;
                else
                    gstLevelLives = 0;
            } else if (STARTS_WITH(left, gstSessionText[TEXT_WEAPONS])) {
                char *cp;
                int j;

                while ((cp = strchr(pValue, ',')) != 0) {
                    *cp = 0;
                    for (j = 0; j < 9; j++) {
                        if (STARTS_WITH(pValue, gstSessionText[TEXT_SMART_BOMB + j]))
                            gstLevelWeapons[j] = 1;
                    }
                    cp++;
                    pValue = cp;
                }
                for (j = 0; j < 9; j++) {
                    if (STARTS_WITH(pValue, gstSessionText[TEXT_SMART_BOMB + j]))
                        gstLevelWeapons[j] = 1;
                }
            } else if (STARTS_WITH(left, gstSessionText[TEXT_PIRANHA])) {
                if (STARTS_WITH(pValue, gstSessionText[TEXT_YES]))
                    gstLevelPiranha = 1;
                else if (STARTS_WITH(pValue, gstSessionText[TEXT_NO]))
                    gstLevelPiranha = 0;
                else
                    gstLevelPiranha = 1;
            }
            endPtr++;
            left = endPtr;
        }
    }
    gstFreeDownPacket(rx);
    return 1;
}

/* Joins a session as a client: fetches the host's race settings, picks a free ship and enters
   the lockstep object. Returns 1 on success; gstJoinError says why not. */
int JoinSessionClient(unsigned long sessionId)
{
    int result;
    int i;

    gstSessionId = sessionId;
    result = gstJoinSessionGPI(gstJoinSessionGPI_Callback);
    if (result < 1) {
        gstDebugPrintf("gstJoinSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    if (!gstGetLevelBlock(gstSessionId)) {
        gstDebugPrintf("JoinSessionClient: gstGetLevelBlock Failed, Quitting Session");
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    if (gstLevelJoinable != 1) {
        gstJoinError = 3;
        gstDebugPrintf("JoinSessionClient: Game is not Joinable! LevelBlockJoinable %d, Quitting Session.", gstLevelJoinable);
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    for (i = 0; i < 15; i++)
        gstShipTaken[i] = 0;
    for (i = 0; i < 5; i++)
        gstTeamSlots[i] = 3;
    if (!gstEnumLS(gstSessionId)) {
        gstDebugPrintf("JoinSessionClient: Enumerate Lockstep Players Failed, Quitting Session.");
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    for (i = 0; i < 15; i++) {
        if (gstShipTaken[i] == 1) {
            if (i < 3)
                gstTeamSlots[1]--;
            else if (i < 6)
                gstTeamSlots[2]--;
            else if (i < 9)
                gstTeamSlots[3]--;
            else if (i < 12)
                gstTeamSlots[0]--;
            else if (i < 15)
                gstTeamSlots[4]--;
        }
    }
    if (gstLevelPiranha != 1 && ownShip >= 12) {
        gstDebugPrintf("JoinSessionClient: Piranha Ships not Allowed! ownShip %d", ownShip);
        gstJoinError = 2;
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    if (gstShipTaken[ownShip] == 1) {
        if (gstShipTaken[ownShip + 1] != 1) {
            ownShip = ownShip + 1;
        } else if (gstShipTaken[ownShip + 2] != 1) {
            ownShip = ownShip + 2;
        } else {
            gstDebugPrintf("JoinSessionClient: Unable to join w/ownShip %d, Quitting Session", ownShip);
            result = gstExitSessionGPI();
            if (result < 1)
                gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
            gstJoinError = 1;
            return 0;
        }
    }
    if (gstJoinLockstep()) {
        result = gstIDRequestGPI(1, &gstPlayerId);
        if (result < 1)
            gstDebugPrintf("gstIDRequestGPI: Error: %d [%s]", result, gstErrorString(result));
        result = gstIDRequestGPI(0, &gstSessionId);
        if (result < 1)
            gstDebugPrintf("gstIDRequestGPI: Error: %d [%s]", result, gstErrorString(result));
        gstSendCommand(gstPlayerId, gstSessionId, 1, ownShip);
        return 1;
    } else {
        gstDebugPrintf("JoinSessionClient: Error Joining Lockstep Object, Quitting Session");
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    gstDebugPrintf("JoinSessionClient: undefined error? return(0)");
    return 0;
}

/* Lists the players of the lockstep object of a session. */
int gstEnumLS(unsigned long sessionId)
{
    int error;
    unsigned char kinds[2];

    kinds[0] = 1;
    kinds[1] = 0;
    error = gstEnumLockstepGPI(sessionId, kinds, gstEnumLS_Callback, 1000);
    if (error < 1) {
        gstDebugPrintf("gstEnumLS: %d [%s]", error, gstErrorString(error));
        return 0;
    }
    return 1;
}

/* Called for each lockstep player: remembers the ships taken and the names, in capitals. */
int gstEnumLS_Callback(unsigned long id, char *name, char *description, unsigned char count,
                       unsigned char *kinds, short *scores)
{
    unsigned char slot;
    char *text;

    for (slot = 0; slot < count; slot++) {
        if (kinds[slot] == 1) {
            gstShipTaken[scores[slot]] = 1;
            gstLobbyIds[gstLobbyCount] = scores[slot];
        }
    }
    text = name;
    if (text) {
        while (*text) {
            if (islower(*text))
                *text = toupper(*text);
            text++;
        }
        sprintf(gstLobbyNames[gstLobbyCount], "%s", name);
    } else {
        sprintf(gstLobbyNames[gstLobbyCount], "NO NAME");
    }
    gstLobbyCount++;
    return 1;
}

/* Sends a command packet (the sender and target ids, the command and its argument) through the default port;
   true when the layer accepted it. */
int gstSendCommand(unsigned long sender, unsigned long target, int command, short argument)
{
    int result;

    result = gstSendCommandGPI(0, 1, target, sender, command, argument);
    return result < 1 ? 0 : 1;
}

/* Joins the lockstep object of the session and gives the player its name. */
int gstJoinLockstep(void)
{
    int result;

    result = gstJoinLockstepGPI(gstLockstepGPI_Callback);
    if (result < 1) {
        gstDebugPrintf("gstJoinLockstepGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    if (!gstChangePlayerName(gstPlayerName, gstPlayerId)) {
        gstDebugPrintf("gstJoinLockstep: gstChangePlayerName failed!");
        return 0;
    }
    return 1;
}

/* Answers the questions gstJoinLockstepGPI asks: the lockstep object type. */
int gstLockstepGPI_Callback(int what, char *result)
{
    switch (what) {
    case 0:
        *result = 6;
        break;
    default:
        gstDebugPrintf("gstLockstepGPI_Callback: Invalid Parameter");
        return GST_ERR_INVALID_PARAM;
    }
    return 1;
}

/* Answers the questions gstJoinSessionGPI and gstCreateSessionGPI ask. */
int gstJoinSessionGPI_Callback(int what, GstQueryResult *result)
{
    gstJoinInfo.count = 1;
    gstJoinInfo.flags = 6;
    gstJoinInfo.values = gstJoinValues;
    gstSessionRequest.name = gstSessionName;
    gstSessionRequest.description = "Session Description";
    gstSessionRequest.dataSize = gstLevelBlockSize;
    gstSessionRequest.data = gstLevelBlock;
    switch (what) {
    case 7:
        result->dword = 15;
        break;
    case 8:
        result->byte = 1;
        break;
    case 4:
        result->dword = 0x20000;
        break;
    case 5:
        result->dword = 0x2000;
        break;
    case 6:
        result->dword = 100;
        break;
    case 9:
        result->byte = 6;
        break;
    case GST_QUERY_SESSION_RESULT:
        result->pointer = &gstSessionId;
        break;
    case GST_QUERY_TITLE:
        result->dword = gstSessionId;
        break;
    case GST_QUERY_SESSION:
        result->pointer = &gstSessionRequest;
        break;
    case GST_QUERY_CAMPAIGN:
        result->pointer = &gstJoinInfo;
        break;
    default:
        gstDebugPrintf("gstJoinSessionGPI_Callback: Invalid Parameter");
        return GST_ERR_INVALID_PARAM;
    }
    return 1;
}

/* The session list screen. Rescans once a second; with no sessions it asks the player to try
   again, with a join error it explains why the last attempt failed. */
void doSessionJoinMenu(void)
{
    int idx;
    int row;
    int ret;
    char text[100];

    if (GetTickCount() - gstSessionScanTime > 1000) {
        gstSessionScanTime = GetTickCount();
        gstSessionCount = 0;
        for (idx = 0; idx < 10; idx++)
            sprintf(gstSessionNames[idx], "%d:", idx + 1);
        ret = gstEnumSessionsGPI(gstEnumSessions_Callback);
        if (ret < 1) {
            gstDebugPrintf("gstEnumSessionsGPI: Error: %d [%s]", ret, gstErrorString(ret));
            return;
        }
        if (gstSessionSelected > gstSessionCount)
            gstSessionSelected = 0;
    }
    Print("JOIN SESSION MODE", 30, 75, 0);
    AddText("JOIN RACING SESSION", 40, 50, 8, 0);
    if (blinkFlag && gstSessionCount == 0) {
        CentreText("NO SESSIONS FOUND", 0, 90, 8, 1);
    } else if (gstSessionCount > 0) {
        for (row = 0; row < gstSessionCount; row++) {
            if (row == gstSessionSelected && blinkFlag)
                Print(gstSessionNames[row], 36, row * 8 + 92, 1);
            else if (row != gstSessionSelected)
                Print(gstSessionNames[row], 36, row * 8 + 92, 0);
        }
    }
    if (gstSessionCount > 0)
        DoButton(30, 198, 2, "JOIN SESSION");
    else if (gstJoinError)
        DoButton(30, 198, 2, "ACCEPT");
    DoButton(278, 198, 1, "BACK");
    if (gstJoinError == 1) {
        textDepth = 0.05f;
        DrawMenuBox(90, 140, 550, 380, 0.06f, 0.07f);
        if (ownShip < 3)
            sprintf(text, "TEAM FULL: AG SYSTEMS");
        else if (ownShip < 6)
            sprintf(text, "TEAM FULL: AURICOM");
        else if (ownShip < 9)
            sprintf(text, "TEAM FULL: QIREX");
        else if (ownShip < 12)
            sprintf(text, "TEAM FULL: FEISAR");
        else if (ownShip < 15)
            sprintf(text, "TEAM FULL: PIRANHA");
        CentreText(text, 0, 85, 8, 1);
        CentreText("TEAMS AVAILABLE:", 0, 105, 8, 1);
        row = 0;
        if (gstTeamSlots[0] > 0) {
            sprintf(text, "FEISAR");
            AddText(text, 60, row * 10 + 125, 8, 0);
            sprintf(text, "%d SLOTS", gstTeamSlots[0]);
            AddText(text, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[1] > 0) {
            sprintf(text, "AG SYSTEMS");
            AddText(text, 60, row * 10 + 125, 8, 0);
            sprintf(text, "%d SLOTS", gstTeamSlots[1]);
            AddText(text, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[2] > 0) {
            sprintf(text, "AURICOM");
            AddText(text, 60, row * 10 + 125, 8, 0);
            sprintf(text, "%d SLOTS", gstTeamSlots[2]);
            AddText(text, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[3] > 0) {
            sprintf(text, "QIREX");
            AddText(text, 60, row * 10 + 125, 8, 0);
            sprintf(text, "%d SLOTS", gstTeamSlots[3]);
            AddText(text, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[4] > 0) {
            sprintf(text, "PIRANHA");
            AddText(text, 60, row * 10 + 125, 8, 0);
            sprintf(text, "%d SLOTS", gstTeamSlots[4]);
            AddText(text, 190, row * 10 + 125, 8, 0);
            row++;
        }
        textDepth = 0.1f;
        if (GetMenuKey(5) || GetMenuKey(4)) {
            gstJoinError = 0;
            PlayNote(49, 255, 15, 15, 256);
            currMenu = 4;
            CopyGameData(0, 0, 0);
            InitSlider();
            PollKeyboard();
        }
        return;
    }
    if (gstJoinError == 2) {
        char text2[100];

        textDepth = 0.05f;
        DrawMenuBox(90, 140, 550, 380, 0.06f, 0.07f);
        CentreText("PIRANHA NOT ALLOWED", 0, 85, 8, 1);
        CentreText("TEAMS AVAILABLE:", 0, 105, 8, 1);
        row = 0;
        if (gstTeamSlots[0] > 0) {
            sprintf(text2, "FEISAR");
            AddText(text2, 60, row * 10 + 125, 8, 0);
            sprintf(text2, "%d SLOTS", gstTeamSlots[0]);
            AddText(text2, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[1] > 0) {
            sprintf(text2, "AG SYSTEMS");
            AddText(text2, 60, row * 10 + 125, 8, 0);
            sprintf(text2, "%d SLOTS", gstTeamSlots[1]);
            AddText(text2, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[2] > 0) {
            sprintf(text2, "AURICOM");
            AddText(text2, 60, row * 10 + 125, 8, 0);
            sprintf(text2, "%d SLOTS", gstTeamSlots[2]);
            AddText(text2, 190, row * 10 + 125, 8, 0);
            row++;
        }
        if (gstTeamSlots[3] > 0) {
            sprintf(text2, "QIREX");
            AddText(text2, 60, row * 10 + 125, 8, 0);
            sprintf(text2, "%d SLOTS", gstTeamSlots[3]);
            AddText(text2, 190, row * 10 + 125, 8, 0);
            row++;
        }
        textDepth = 0.1f;
        if (GetMenuKey(5) || GetMenuKey(4)) {
            gstJoinError = 0;
            PlayNote(9, 255, 15, 15, 256);
            currMenu = 4;
            CopyGameData(0, 0, 0);
            InitSlider();
            PollKeyboard();
        }
        return;
    } else if (gstJoinError == 3) {
        textDepth = 0.05f;
        DrawMenuBox(90, 140, 550, 380, 0.06f, 0.07f);
        CentreText("CAN NOT JOIN SESSION", 0, 85, 8, 1);
        CentreText("GAME IN PROGRESS", 0, 105, 8, 1);
        textDepth = 0.1f;
        if (GetMenuKey(5) || GetMenuKey(4)) {
            gstJoinError = 0;
            PlayNote(9, 255, 15, 15, 256);
            PollKeyboard();
        }
        return;
    }
    if (GetMenuKey(0)) {
        if (gstSessionSelected > 0) {
            PlayNote(8, 255, 15, 15, 256);
            gstSessionSelected--;
        }
    } else if (GetMenuKey(1)) {
        if (gstSessionCount - 1 > gstSessionSelected) {
            PlayNote(8, 255, 15, 15, 256);
            gstSessionSelected++;
        }
    } else if (GetMenuKey(4)) {
        PlayNote(49, 255, 15, 15, 256);
        gstPlayerNameLength = 0;
        gstSessionNameLength = 0;
        gstOptionsSelected = 0;
        currMenu = 17;
        InitSlider();
        PollKeyboard();
    } else if (GetMenuKey(5)) {
        if (gstSessionCount > 0) {
            PlayNote(9, 255, 15, 15, 256);
            if (JoinSessionClient(gstSessionIds[gstSessionSelected])) {
                currMenu = 21;
                CopyGameData(0, 0, 0);
                InitSlider();
                PollKeyboard();
            } else {
                gstDebugPrintf("doSessionJoinMenu: JoinSessionClient Failed!");
            }
        }
    }
}

/* Called for each session found: lists its name in capitals, up to eleven of them. */
int gstEnumSessions_Callback(unsigned long id, char *name, char *description, unsigned char a, unsigned char b)
{
    int i;

    if (gstSessionCount > 10)
        return 1;
    sprintf(gstSessionNames[gstSessionCount], "%d: ", gstSessionCount + 1);
    strcat(gstSessionNames[gstSessionCount], name);
    for (i = 0; (int)strlen(gstSessionNames[gstSessionCount]) > i; i++) {
        if (isalpha(gstSessionNames[gstSessionCount][i])) {
            if (islower(gstSessionNames[gstSessionCount][i]))
                gstSessionNames[gstSessionCount][i] = toupper(gstSessionNames[gstSessionCount][i]);
        }
    }
    gstSessionIds[gstSessionCount] = id;
    gstSessionCount++;
    return 1;
}

/* The lobby: the players of the session so far. The host can start the game, everybody can
   leave. */
void doLobbyMenu(void)
{
    int status;
    int i;
    int row;
    int x;
    char text[100];
    char cmd;

    if (gstRestartSeen) {
        gstLobbyCount = 0;
        if (!gstEnumLS(gstSessionId))
            gstDebugPrintf("doLobbyMenu: Error Enumerating Lockstep Players!");
        gstRestartSeen = 0;
    }
    AddText("MULTIPLAYER LOBBY", 40, 50, 8, 0);
    Print("LOBBY MODE", 30, 75, 0);
    AddText("CURRENT PLAYER LIST", 38, 90, 8, 0);
    for (i = 0; racePlayerCount > i; i++) {
        if (i < 8) {
            row = i * 8;
            x = 0;
        } else {
            row = (i - 8) * 8;
            x = 142;
        }
        sprintf(text, "%2d: %s", i + 1, gstLobbyNames[i]);
        if (i == 0)
            Print("HOST", 125, row + 110, 1);
        Print(text, x + 36, row + 110, 0);
    }
    if (gstIsHost)
        DoButton(30, 198, 2, "START GAME");
    DoButton(278, 198, 1, "BACK");
    if (GetMenuKey(4)) {
        status = gstExitLockstepGPI();
        if (status < 1)
            gstDebugPrintf("gstExitLockstepGPI: Error: %d [%s]", status, gstErrorString(status));
        status = gstExitSessionGPI();
        if (status < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", status, gstErrorString(status));
        if (!gstLaunchMode) {
            PlayNote(49, 255, 15, 15, 256);
            gstPlayerNameLength = 0;
            gstSessionNameLength = 0;
            gstOptionsSelected = 0;
            currMenu = 17;
            InitSlider();
        } else {
            PlayNote(49, 255, 15, 15, 256);
            currMenu = 0;
            InitSlider();
        }
        PollKeyboard();
    } else if (GetMenuKey(5)) {
        if (gstIsHost) {
            cmd = 0;
            PlayNote(9, 255, 15, 15, 256);
            gstLevelJoinable = 0;
            gstBuildLevelBlock(gstLevelBlock, &gstLevelBlockSize);
            gstPostLevelBlock(gstLevelBlock, gstLevelBlockSize, gstSessionId);
            gstPostSessionAsynch(gstSessionId, &cmd, 1, 0);
        }
    }
}

#endif

/* Appends a string (without its terminator) to a codec. */
#define ENCODE_TEXT(codec, text) gstEncodeBytes(codec, strlen(text), text)

#ifdef EQUIV
#ifdef NETPLAY
/* Writes the level block: the race settings as "key=value" lines, "Class=Venom Class,Rapier
   Class" style lists for a tournament. `size` gets the number of bytes written. */
int gstBuildLevelBlock(char *buffer, unsigned long *size)
{
    GstCodec codec;
    char raceText[100];
    char classText[100];
    char trackText[100];
    char jumpInText[12];
    char joinableText[12];
    char piranhaText[100];
    char lapsText[100];
    char livesText[100];
    int weaponCount;
    int trackCount;
    int classCount;
    int i;

    weaponCount = 0;
    trackCount = 0;
    classCount = 0;
    switch (gstRaceMode) {
    case 0:
        sprintf(raceText, gstSessionText[TEXT_TIMED]);
        break;
    case 1:
        sprintf(raceText, gstSessionText[TEXT_DEATH_RACE]);
        break;
    case 3:
        sprintf(raceText, gstSessionText[TEXT_TOURNAMENT]);
        break;
    default:
        sprintf(raceText, gstSessionText[TEXT_TIMED]);
        break;
    }
    if (gstRaceMode != 3) {
        switch (gameData->shipClass) {
        case 0:
            sprintf(classText, gstSessionText[TEXT_VECTOR]);
            break;
        case 1:
            sprintf(classText, gstSessionText[TEXT_VENOM]);
            break;
        case 2:
            sprintf(classText, gstSessionText[TEXT_RAPIER]);
            break;
        case 3:
            sprintf(classText, gstSessionText[TEXT_PHANTOM]);
            break;
        default:
            sprintf(classText, gstSessionText[TEXT_VECTOR]);
            break;
        }
        switch (gameData->trackNum) {
        case 0:
            sprintf(trackText, gstSessionText[TEXT_TALONS_REACH]);
            break;
        case 1:
            sprintf(trackText, gstSessionText[TEXT_SAGARMATHA]);
            break;
        case 2:
            sprintf(trackText, gstSessionText[TEXT_VALPARAISO]);
            break;
        case 3:
            sprintf(trackText, gstSessionText[TEXT_PHENITIA_PARK]);
            break;
        case 4:
            sprintf(trackText, gstSessionText[TEXT_GARE_D_EUROPA]);
            break;
        case 5:
            sprintf(trackText, gstSessionText[TEXT_ODESSA_KEYS]);
            break;
        case 6:
            sprintf(trackText, gstSessionText[TEXT_VOSTOK]);
            break;
        case 7:
            sprintf(trackText, gstSessionText[TEXT_SPILSKINANKE]);
            break;
        default:
            sprintf(trackText, gstSessionText[TEXT_TALONS_REACH]);
            break;
        }
    }
    if (gstLevelJumpIn)
        sprintf(jumpInText, gstSessionText[TEXT_TRUE]);
    else
        sprintf(jumpInText, gstSessionText[TEXT_FALSE]);
    if (gstLevelJoinable)
        sprintf(joinableText, gstSessionText[TEXT_TRUE]);
    else
        sprintf(joinableText, gstSessionText[TEXT_FALSE]);
    if (gstLevelPiranha)
        sprintf(piranhaText, gstSessionText[TEXT_YES]);
    else
        sprintf(piranhaText, gstSessionText[TEXT_NO]);
    switch (gstLevelLaps) {
    case 0:
        sprintf(lapsText, gstSessionText[TEXT_DEFAULT]);
        break;
    case 1:
        sprintf(lapsText, gstSessionText[TEXT_ONE]);
        break;
    case 2:
        sprintf(lapsText, gstSessionText[TEXT_TWO]);
        break;
    case 3:
        sprintf(lapsText, gstSessionText[TEXT_THREE]);
        break;
    case 4:
        sprintf(lapsText, gstSessionText[TEXT_FOUR]);
        break;
    case 5:
        sprintf(lapsText, gstSessionText[TEXT_FIVE]);
        break;
    default:
        sprintf(lapsText, gstSessionText[TEXT_DEFAULT]);
        break;
    }
    switch (gstLevelLives) {
    case 0:
        sprintf(livesText, gstSessionText[TEXT_UNLIMITED]);
        break;
    case 1:
        sprintf(livesText, gstSessionText[TEXT_ONE]);
        break;
    case 2:
        sprintf(livesText, gstSessionText[TEXT_TWO]);
        break;
    case 3:
        sprintf(livesText, gstSessionText[TEXT_THREE]);
        break;
    case 4:
        sprintf(livesText, gstSessionText[TEXT_FOUR]);
        break;
    case 5:
        sprintf(livesText, gstSessionText[TEXT_FIVE]);
        break;
    default:
        sprintf(livesText, gstSessionText[TEXT_THREE]);
        break;
    }
    codec.pos = 0;
    codec.size = 0;
    codec.maxSize = 1000;
    codec.data = (unsigned char *)buffer;
    ENCODE_TEXT(&codec, gstSessionText[TEXT_RACE_TYPE]);
    gstEncodeU8(&codec, '=');
    ENCODE_TEXT(&codec, raceText);
    gstEncodeU8(&codec, '\n');
    ENCODE_TEXT(&codec, gstSessionText[TEXT_CLASS]);
    gstEncodeU8(&codec, '=');
    if (gstRaceMode == 3) {
        for (i = 0; i < 8; i++) {
            if (classCount > 0)
                gstEncodeU8(&codec, ',');
            switch (gstLevelClasses[i]) {
            case 0:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_VECTOR]);
                break;
            case 1:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_VENOM]);
                break;
            case 2:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_RAPIER]);
                break;
            case 3:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_PHANTOM]);
                break;
            default:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_VECTOR]);
                break;
            }
            classCount++;
        }
        gstEncodeU8(&codec, '\n');
    } else {
        ENCODE_TEXT(&codec, classText);
        gstEncodeU8(&codec, '\n');
    }
    ENCODE_TEXT(&codec, gstSessionText[TEXT_TRACK_NAME]);
    gstEncodeU8(&codec, '=');
    if (gstRaceMode == 3) {
        for (i = 0; i < 8; i++) {
            if (trackCount > 0)
                gstEncodeU8(&codec, ',');
            switch (gstLevelTracks[i]) {
            case 0:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_TALONS_REACH]);
                break;
            case 1:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_SAGARMATHA]);
                break;
            case 2:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_VALPARAISO]);
                break;
            case 3:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_PHENITIA_PARK]);
                break;
            case 4:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_GARE_D_EUROPA]);
                break;
            case 5:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_ODESSA_KEYS]);
                break;
            case 6:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_VOSTOK]);
                break;
            case 7:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_SPILSKINANKE]);
                break;
            default:
                ENCODE_TEXT(&codec, gstSessionText[TEXT_TALONS_REACH]);
                break;
            }
            trackCount++;
        }
        gstEncodeU8(&codec, '\n');
    } else {
        ENCODE_TEXT(&codec, trackText);
        gstEncodeU8(&codec, '\n');
    }
    ENCODE_TEXT(&codec, gstSessionText[TEXT_JUMP_IN]);
    gstEncodeU8(&codec, '=');
    ENCODE_TEXT(&codec, jumpInText);
    gstEncodeU8(&codec, '\n');
    ENCODE_TEXT(&codec, gstSessionText[TEXT_JOINABLE]);
    gstEncodeU8(&codec, '=');
    ENCODE_TEXT(&codec, joinableText);
    gstEncodeU8(&codec, '\n');
    ENCODE_TEXT(&codec, gstSessionText[TEXT_LAPS]);
    gstEncodeU8(&codec, '=');
    ENCODE_TEXT(&codec, lapsText);
    gstEncodeU8(&codec, '\n');
    ENCODE_TEXT(&codec, gstSessionText[TEXT_LIVES]);
    gstEncodeU8(&codec, '=');
    ENCODE_TEXT(&codec, livesText);
    gstEncodeU8(&codec, '\n');
    ENCODE_TEXT(&codec, gstSessionText[TEXT_WEAPONS]);
    gstEncodeU8(&codec, '=');
    for (i = 0; i < 9; i++) {
        if (gstLevelWeapons[i]) {
            if (weaponCount > 0)
                gstEncodeU8(&codec, ',');
            ENCODE_TEXT(&codec, gstSessionText[TEXT_SMART_BOMB + i]);
            weaponCount++;
        }
    }
    gstEncodeU8(&codec, '\n');
    ENCODE_TEXT(&codec, gstSessionText[TEXT_PIRANHA]);
    gstEncodeU8(&codec, '=');
    ENCODE_TEXT(&codec, piranhaText);
    gstEncodeU8(&codec, '\n');
    *size = codec.size;
    return 1;
}
#endif
#endif

#ifdef NETPLAY
/* Posts the level block to the session: the block, its size and the session id. */
void gstPostLevelBlock(char *block, unsigned long size, unsigned long sessionId)
{
    GstCodec *packet;

    gstAllocUpPacket(&packet, 0, 0x400);
    gstEncodeU8(packet, 0x43);
    gstEncodeU8(packet, 3);
    gstEncodePackedU24(packet, sessionId);
    gstEncodeU16(packet, 1);
    gstEncodePackedU32(packet, size);
    gstEncodeBytes(packet, size, block);
    gstSendEncodedUpPacket(packet, 0);
    return;
}

#endif

#ifdef EQUIV
#ifdef NETPLAY
/* The network options screen: join a session, create one, and type the player and session
   names. */
void doNetworkOptionsMenu(void)
{
    short playerX;
    short playerY;
    short sessionX;
    short sessionY;
    short x;
    int i;
    char c;
    char cursor[2];

    playerX = 170;
    playerY = 130;
    sessionX = 170;
    sessionY = 150;
    AddText("MULTIPLAYER OPTIONS", 40, 50, 8, 0);
    Print("NETWORK OPTIONS MODE", 30, 75, 0);
    if (gstOptionsSelected == 0 && blinkFlag)
        AddText("JOIN SESSION", 30, 90, 8, 1);
    else if (gstOptionsSelected != 0)
        AddText("JOIN SESSION", 30, 90, 8, 0);
    if (gstOptionsSelected == 1 && blinkFlag)
        AddText("CREATE SESSION", 30, 110, 8, 1);
    else if (gstOptionsSelected != 1)
        AddText("CREATE SESSION", 30, 110, 8, 0);
    if (gstOptionsSelected == 2 && blinkFlag)
        AddText("PLAYER NAME", 30, 130, 8, 1);
    else if (gstOptionsSelected != 2)
        AddText("PLAYER NAME", 30, 130, 8, 0);
    if (gstOptionsSelected == 3 && blinkFlag)
        AddText("SESSION NAME", 30, 150, 8, 1);
    else if (gstOptionsSelected != 3)
        AddText("SESSION NAME", 30, 150, 8, 0);
    AddText(gstPlayerName, playerX, playerY, 8, 0);
    AddText(gstSessionName, sessionX, sessionY, 8, 0);
    if (gstOptionsSelected == 2) {
        x = playerX;
        for (i = 0; i < gstPlayerNameLength; i++) {
            c = glyphIndex.character[gstPlayerName[i]];
            if (c != 50)
                x += glyphWidth[c].width >> 1;
            else
                x += 8;
        }
        cursor[0] = gstPlayerName[gstPlayerNameLength];
        cursor[1] = 0;
        AddText(cursor, x, playerY, 8, 1);
    } else if (gstOptionsSelected == 3) {
        x = sessionX;
        for (i = 0; i < gstSessionNameLength; i++) {
            c = glyphIndex.character[gstSessionName[i]];
            if (c != 50)
                x += glyphWidth[c].width >> 1;
            else
                x += 8;
        }
        cursor[0] = gstSessionName[gstSessionNameLength];
        cursor[1] = 0;
        AddText(cursor, x, sessionY, 8, 1);
    }
    if (gstOptionsSelected == 0)
        PrintCX("CHOSE FROM A LIST OF SESSIONS TO JOIN", 160, 186, 0);
    if (gstOptionsSelected == 1)
        PrintCX("CREATE A SESSION AND CUSTOMIZE RACING OPTIONS", 160, 186, 0);
    if (gstOptionsSelected == 2)
        PrintCX("NAME OF LOCAL PLAYER AS SEEN BY OTHER RACERS", 160, 186, 0);
    if (gstOptionsSelected == 3)
        PrintCX("NAME OF SESSION TO CREATE", 160, 186, 0);
    if (gstCdError == 1)
        DoButton(30, 198, 2, "ACCEPT");
    else if (gstOptionsSelected == 0 || gstOptionsSelected == 1)
        DoButton(30, 198, 2, "SELECT");
    else
        DoButton(30, 198, 2, "ACCEPT");
    DoButton(278, 198, 1, "BACK");
    if (gstCdError == 1) {
        textDepth = 0.05f;
        DrawMenuBox(100, 160, 540, 300, 0.06f, 0.07f);
        CentreText("CAN NOT", 0, 85, 8, 1);
        CentreText("CREATE SESSION", 0, 95, 8, 1);
        CentreText("INSERT WIPEOUT XL CD", 0, 125, 8, 1);
        CentreText("AND TRY AGAIN", 0, 135, 8, 1);
        textDepth = 0.1f;
        if (GetMenuKey(5)) {
            gstCdError = 0;
            PlayNote(9, 255, 15, 15, 256);
            PollKeyboard();
        }
        if (GetMenuKey(4)) {
            gstCdError = 0;
            PollKeyboard();
            PlayNote(9, 255, 15, 15, 256);
        }
        return;
    }
    if (GetMenuKey(4)) {
        PlayNote(49, 255, 15, 15, 256);
        currMenu = 1;
        InitSlider();
        return;
    }
    if (GetMenuKey(5)) {
        if (gstOptionsSelected == 0) {
            gstIsHost = 0;
            PlayNote(9, 255, 15, 15, 256);
            gstSessionCount = 0;
            gstSessionSelected = 0;
            currMenu = 20;
            gstSessionScanTime = GetTickCount();
            InitSlider();
            PollKeyboard();
            return;
        }
        if (gstOptionsSelected == 1) {
            gstIsHost = 1;
            PlayNote(9, 255, 15, 15, 256);
            CdReadTrackTable();
            if (netCdTrackError == 1) {
                gstCdError = 1;
                PollKeyboard();
                return;
            } else {
                currMenu = 18;
                InitSlider();
                PollKeyboard();
                return;
            }
        }
        if (gstOptionsSelected == 2 || gstOptionsSelected == 3) {
            PlayNote(49, 255, 15, 15, 256);
            currMenu = 0;
            InitSlider();
            PollKeyboard();
            return;
        }
    }
    if (g_keyboardCode == 14) {
        PlayNote(8, 255, 15, 15, 256);
        if (gstOptionsSelected == 2 && gstPlayerNameLength > 0)
            gstPlayerNameLength--;
        if (gstOptionsSelected == 3 && gstSessionNameLength > 0)
            gstSessionNameLength--;
        g_keyboardCode = -1;
    }
    if (GetMenuKey(0)) {
        if (gstOptionsSelected > 0) {
            PlayNote(8, 255, 15, 15, 256);
            gstOptionsSelected--;
            gstSessionNameLength = 0;
            gstPlayerNameLength = 0;
        }
    }
    if (GetMenuKey(1)) {
        if (gstOptionsSelected < 3) {
            PlayNote(8, 255, 15, 15, 256);
            gstOptionsSelected++;
            gstSessionNameLength = 0;
            gstPlayerNameLength = 0;
        }
    }
    if (GetMenuKey(2)) {
        if (gstOptionsSelected == 2 && gstPlayerNameLength > 0) {
            PlayNote(8, 255, 15, 15, 256);
            gstPlayerNameLength--;
        }
        if (gstOptionsSelected == 3 && gstSessionNameLength > 0) {
            PlayNote(8, 255, 15, 15, 256);
            gstSessionNameLength--;
        }
    }
    if (GetMenuKey(3)) {
        if (gstOptionsSelected == 2 && gstPlayerNameLength < 7) {
            PlayNote(8, 255, 15, 15, 256);
            gstPlayerNameLength++;
        }
        if (gstOptionsSelected == 3 && gstSessionNameLength < 7) {
            PlayNote(8, 255, 15, 15, 256);
            gstSessionNameLength++;
        }
    }
    c = GetKeyboardChar();
    if (c) {
        if (gstOptionsSelected == 2 && gstPlayerNameLength < 8) {
            PlayNote(8, 255, 15, 15, 256);
            gstPlayerName[gstPlayerNameLength] = c;
            gstPlayerNameLength++;
        }
        if (gstOptionsSelected == 3 && gstSessionNameLength < 8) {
            PlayNote(8, 255, 15, 15, 256);
            gstSessionName[gstSessionNameLength] = c;
            gstSessionNameLength++;
        }
    }
}
#endif
#endif

#ifdef NETPLAY
/* Hosts a session: publishes the race settings picked on the menu as the level block, creates
   the session and enters its lockstep object. Returns 1 on success. */
int JoinSessionServer(void)
{
    int result;

    gstLevelJumpIn = 0;
    gstLevelJoinable = 1;
    gstLevelLaps = gstMenuLaps;
    gstLevelLives = gstMenuLives;
    gstLevelPiranha = gstMenuPiranha;
    if (gstLevelPiranha == 0 && ownShip >= 12) {
        gstDebugPrintf("JoinSessionServer: Piranha ships denied, ownShip %d, can't host like this", ownShip);
        return 0;
    }
    gstLevelTracks[0] = gstMenuRaces[0].track;
    gstLevelTracks[1] = gstMenuRaces[1].track;
    gstLevelTracks[2] = gstMenuRaces[2].track;
    gstLevelTracks[3] = gstMenuRaces[3].track;
    gstLevelTracks[4] = gstMenuRaces[4].track;
    gstLevelTracks[5] = gstMenuRaces[5].track;
    gstLevelTracks[6] = gstMenuRaces[6].track;
    gstLevelTracks[7] = gstMenuRaces[7].track;
    gstLevelClasses[0] = gstMenuRaces[0].shipClass;
    gstLevelClasses[1] = gstMenuRaces[1].shipClass;
    gstLevelClasses[2] = gstMenuRaces[2].shipClass;
    gstLevelClasses[3] = gstMenuRaces[3].shipClass;
    gstLevelClasses[4] = gstMenuRaces[4].shipClass;
    gstLevelClasses[5] = gstMenuRaces[5].shipClass;
    gstLevelClasses[6] = gstMenuRaces[6].shipClass;
    gstLevelClasses[7] = gstMenuRaces[7].shipClass;
    gstBuildLevelBlock(gstLevelBlock, &gstLevelBlockSize);
    if (gstDebugOutputEnabled) {
        gstLevelBlock[gstLevelBlockSize] = 0;
        printf("\n***** JoinSessionServer: HOST: LevelBlockBuffer Size %d ********************\n", gstLevelBlockSize);
        printf("%s", gstLevelBlock);
        printf("***** JoinSessionServer: HOST: LevelBlockBuffer Size %d ********************\n\n", gstLevelBlockSize);
    }
    gstSessionId = 0;
    result = gstCreateSessionGPI(gstJoinSessionGPI_Callback);
    if (result < 1) {
        gstDebugPrintf("gstCreateSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    if (gstJoinLockstep()) {
        result = gstIDRequestGPI(1, &gstPlayerId);
        if (result < 1)
            gstDebugPrintf("gstIDRequestGPI: Player: Error: %d [%s]", result, gstErrorString(result));
        result = gstIDRequestGPI(0, &gstSessionId);
        if (result < 1)
            gstDebugPrintf("gstIDRequestGPI: Error: %d [%s]", result, gstErrorString(result));
        gstSendCommand(gstPlayerId, gstSessionId, 1, ownShip);
        return 1;
    } else {
        gstDebugPrintf("JoinSessionServer: Error Joining Lockstep Object, Quitting Session");
        result = gstExitSessionGPI();
        if (result < 1)
            gstDebugPrintf("gstExitSessionGPI: Error: %d [%s]", result, gstErrorString(result));
        return 0;
    }
    gstDebugPrintf("JoinSessionServer: undefined error? return(0)");
    return 0;
}

/* Unnamed state of the tournament screen: the highlighted entry (0-15, 16 is the CREATE SESSION button) and the
   tables the entries are drawn from. Entry 2n is the track of race n, entry 2n+1 its racing class. */
extern int gstTournamentCursor;
extern char *gstTrackNames[8];
extern char *gstClassNames[4];
extern int gstEntryX[16];
extern int gstEntryY[16];
#define gstRaceBytes ((unsigned char *)gstMenuRaces)

/* The tournament set-up screen: eight races, each with a track and a racing class to cycle through. */
void doTournamentMenu(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    char label[12];
    int i;
    int lastNum;

    lastNum = 1;
    Print("TOURNAMENT CONSISTS OF EIGHT CONSECUTIVE RACES", 30, 182, 0);
    Print("OF USER DEFINED TRACKS AND RACING CLASSES.", 30, 189, 0);
    Print("CREATE MODE", 30, 75, 0);
    AddText(menu[currMenu].title.text, 40, 50, 8, 0);
    if (gstTournamentCursor == 16 && blinkFlag)
        CentreText("CREATE SESSION", 0, 165, 8, 1);
    else if (gstTournamentCursor != 16)
        CentreText("CREATE SESSION", 0, 165, 8, 0);
    if (gstTournamentCursor == 16)
        DoButton(30, 198, 2, "ACCEPT");
    else
        DoButton(30, 198, 2, "CYCLE");
    DoButton(278, 198, 1, "BACK");
    for (i = 0; i < 16; i++) {
        if (i % 2 == 0) {
            sprintf(label, "TRACK %d: ", lastNum);
            Print(label, 30, gstEntryY[i], 0);
            if (gstTournamentCursor == i && blinkFlag)
                Print(gstTrackNames[gstRaceBytes[i]], gstEntryX[i], gstEntryY[i], 1);
            else if (gstTournamentCursor != i)
                Print(gstTrackNames[gstRaceBytes[i]], gstEntryX[i], gstEntryY[i], 0);
            lastNum++;
        } else {
            if (gstTournamentCursor == i && blinkFlag)
                Print(gstClassNames[gstRaceBytes[i]], gstEntryX[i], gstEntryY[i], 1);
            else if (gstTournamentCursor != i)
                Print(gstClassNames[gstRaceBytes[i]], gstEntryX[i], gstEntryY[i], 0);
        }
    }
    if (GetMenuKey(0)) {
        if (gstTournamentCursor >= 2) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstTournamentCursor -= 2;
        }
    } else if (GetMenuKey(1)) {
        if (gstTournamentCursor == 15) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstTournamentCursor = 16;
        } else if (gstTournamentCursor <= 14) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstTournamentCursor += 2;
        }
    } else if (GetMenuKey(2)) {
        if (gstTournamentCursor % 2 != 0 && gstTournamentCursor != 16) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstTournamentCursor--;
        }
    } else if (GetMenuKey(3)) {
        if (gstTournamentCursor % 2 == 0 && gstTournamentCursor != 16) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstTournamentCursor++;
        }
    } else if (GetMenuKey(4)) {
        PlayNote(49, 255, 15, 15, 256);
        currMenu = 18;
        CopyGameData(0, 0, 0);
        InitSlider();
        PollKeyboard();
        return;
    } else if (GetMenuKey(5)) {
        if (gstTournamentCursor == 16) {
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            if (JoinSessionServer()) {
                currMenu = 21;
                InitSlider();
                PollKeyboard();
                return;
            } else {
                gstDebugPrintf("doTournamentMenu: JoinSessionServer Failed! Doh!");
            }
        } else {
            if (gstTournamentCursor % 2 == 0) {
                gstRaceBytes[gstTournamentCursor]++;
                gstRaceBytes[gstTournamentCursor] = gstRaceBytes[gstTournamentCursor] % 8;
            } else {
                gstRaceBytes[gstTournamentCursor]++;
                gstRaceBytes[gstTournamentCursor] = gstRaceBytes[gstTournamentCursor] % 4;
            }
            PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
            PollKeyboard();
        }
    }
}

#endif

extern MenuLayout *menuLayout;

/* Puts the corners of every menu box in order (smallest first) after the menu editor has moved them. */
#ifdef NETPLAY
void NormaliseMenuBoxes(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    u_short maxY;
    short l;
    u_short maxX;
    short k;
    u_short minY;
    u_short minX;

    maxX = 0;
    minX = 0xffff;
    maxY = 0;
    minY = 0xffff;
    for (k = 0; k < 22; k++) {
        for (l = 0; l < 20; l++) {
            maxX = maxY = 0;
            minX = minY = 0xffff;
            if (menuLayout[k].box[l].x0 > maxX)
                maxX = menuLayout[k].box[l].x0;
            if (menuLayout[k].box[l].x1 > maxX)
                maxX = menuLayout[k].box[l].x1;
            if (menuLayout[k].box[l].x0 < minX)
                minX = menuLayout[k].box[l].x0;
            if (menuLayout[k].box[l].x1 < minX)
                minX = menuLayout[k].box[l].x1;
            if (menuLayout[k].box[l].y0 > maxY)
                maxY = menuLayout[k].box[l].y0;
            if (menuLayout[k].box[l].y1 > maxY)
                maxY = menuLayout[k].box[l].y1;
            if (menuLayout[k].box[l].y0 < minY)
                minY = menuLayout[k].box[l].y0;
            if (menuLayout[k].box[l].y1 < minY)
                minY = menuLayout[k].box[l].y1;
            menuLayout[k].box[l].x0 = minX;
            menuLayout[k].box[l].y0 = minY;
            menuLayout[k].box[l].x1 = maxX;
            menuLayout[k].box[l].y1 = maxY;
        }
    }
}
#else
void NormaliseMenuBoxes(void)
{
    MenuBox *box;
    u_short minX, minY, maxX, maxY;
    short i, j;

    for (i = 0; i < 17; i++) {
        for (j = 0; j < 20; j++) {
            maxX = maxY = 0;
            minX = minY = 0xffff;
            box = &menuLayout[i].box[j];
            if (box->x0 > 0)
                maxX = box->x0;
            if (maxX < box->x1)
                maxX = box->x1;
            if (box->x0 < 0xffff)
                minX = box->x0;
            if (minX > box->x1)
                minX = box->x1;
            if (box->y0 > 0)
                maxY = box->y0;
            if (maxY < box->y1)
                maxY = box->y1;
            if (box->y0 < 0xffff)
                minY = box->y0;
            if (minY > box->y1)
                minY = box->y1;
            box->x0 = minX;
            box->y0 = minY;
            box->x1 = maxX;
            box->y1 = maxY;
        }
    }
}
#endif

/* Proven equivalent rather than byte-exact: under /Oa the compiler cannot bind the selection
   pointer to the menu base plus 0x468 at 0x43ccbe->0x43ccea. */
#ifdef EQUIV
#ifdef NETPLAY
void ShowMenu(PcGameData *config, MenuBars *bars)
{
    ClassOrder classOrder = { { 1, 0, 3, 5 } };
    RaceOrder raceOrder = { { 9, 2, 8, 10, 6, 4 } };
    char optionDisabled = 0;
    u_char highlightColour = 0x55;
    u_char baseBarColour = 0;
    short optionTextIndex, optionIndex;

    if (currMenu == 21 || currMenu == 17 || currMenu == 18 || currMenu == 19 || currMenu == 20)
        return;
    if (!rgbTextures) {
        highlightColour = 0x4b;
        baseBarColour = 0x60;
    }
    for (optionIndex = 0; optionIndex < menu[currMenu].numOptions; optionIndex++) {
        if (currMenu == 0) {
            if (optionIndex == menu[currMenu].selection) {
                bars->options[optionIndex][CurrentScreen].r0 = highlightColour;
                bars->options[optionIndex][CurrentScreen].g0 = 0;
                bars->options[optionIndex][CurrentScreen].b0 = 0x28;
            } else {
                bars->options[optionIndex][CurrentScreen].r0 = baseBarColour;
                bars->options[optionIndex][CurrentScreen].g0 = 0;
                bars->options[optionIndex][CurrentScreen].b0 = 0x2e;
            }
            if (optionIndex == 3 || optionIndex == 4 || optionIndex == menu[currMenu].selection) {
                DrawMenuOptionBackdrop(bars->options,
                    menuLayout[currMenu].box[optionIndex].x0 + 1, menuLayout[currMenu].box[optionIndex].y0 + 1,
                    menuLayout[currMenu].box[optionIndex].x1 - menuLayout[currMenu].box[optionIndex].x0, menuLayout[currMenu].box[optionIndex].y1 - menuLayout[currMenu].box[optionIndex].y0,
                    optionIndex, menu[currMenu].selection);
            }
        }
        for (optionTextIndex = 0; optionTextIndex < menu[currMenu].options[optionIndex].numText; optionTextIndex++) {
            if (currMenu == 0) {
                CentreText2(menu[currMenu].options[optionIndex].text[optionTextIndex].text,
                    menu[currMenu].options[optionIndex].x,
                    menu[currMenu].options[optionIndex].y + optionTextIndex * 10, 8, 0);
            } else if (menu[currMenu].options[optionIndex].enabled) {
                if (currMenu == 2) {
                    if (optionIndex == menu[currMenu].selection) {
                        CentreText2(menu[currMenu].options[optionIndex].text[optionTextIndex].text, 68,
                            menu[currMenu].options[0].y + optionIndex * 10, 8, 1);
                    } else {
                        CentreText2(menu[currMenu].options[optionIndex].text[optionTextIndex].text, 68,
                            menu[currMenu].options[0].y + optionIndex * 10, 8, 0);
                    }
                } else if (currMenu == 3) {
                    if (optionIndex == menu[currMenu].selection) {
                        CentreText(menu[currMenu].options[optionIndex].text[optionTextIndex].text, 68,
                           menu[currMenu].options[0].y + optionIndex * 14, 8, 1);
                    } else {
                        CentreText(menu[currMenu].options[optionIndex].text[optionTextIndex].text, 68,
                           menu[currMenu].options[0].y + optionIndex * 14, 8, 0);
                    }
                } else {
                    if (optionIndex == menu[currMenu].selection) {
                        AddText(menu[currMenu].options[optionIndex].text[optionTextIndex].text,
                        menu[currMenu].options[0].x,
                        menu[currMenu].options[0].y + optionIndex * 10, 8, 1);
                    } else {
                        AddText(menu[currMenu].options[optionIndex].text[optionTextIndex].text,
                        menu[currMenu].options[0].x,
                        menu[currMenu].options[0].y + optionIndex * 10, 8, 0);
                    }
                }
            }
        }
    }

    if (currMenu == 9) {
        bars->options[0][CurrentScreen].r0 = highlightColour;
        bars->options[0][CurrentScreen].g0 = 0;
        bars->options[0][CurrentScreen].b0 = 0x28;

    }


    if (currMenu != 13 && currMenu != 15 && currMenu != 14 && currMenu != 9 &&
        currMenu != 10 && currMenu != 11)
        DoButton(30, 198, 2, "SELECT");
    if (currMenu == 9 || currMenu == 10 || currMenu == 11)
        DoButton(30, 198, 2, "ACCEPT");
    if (currMenu != 0)
        DoButton(278, 198, 1, "BACK");
    else
        DoButton(278, 198, 1, "QUIT");

    if (currMenu == 2)
        ClassSelect(config);
    if (currMenu == 1) {
        switch (menu[currMenu].selection) {
        case 0:
            if (networkGame) {
                Print("SINGLE RACE AGAINST HUMAN COMPETITORS.", 30, 182, 0);
                Print("CHECKPOINTS MUST BE CROSSED WITHIN TIME LIMITS.", 30, 189, 0);
            } else {
                Print("SINGLE RACE AGAINST A FULL", 30, 182, 0);
                Print("GRID OF COMPETITORS.", 30, 189, 0);
            }
            break;
        case 1:
            if (networkGame) {
                Print("SINGLE RACE AGAINST HUMAN COMPETITORS.", 30, 182, 0);
                Print("TIME LIMIT CHECKPOINTS ARE NOT ENFORCED.", 30, 189, 0);
            } else {
                Print("SINGLE RACE TIME TRIAL.", 30, 182, 0);
                Print("NO COMPETITORS.", 30, 189, 0);
            }
            break;
        case 2:
            if (networkGame) {
                Print("EIGHT USER DEFINED RACES AGAINST HUMAN COMPETITORS.", 30, 182, 0);
                Print("TIME LIMIT CHECKPOINTS ARE NOT ENFORCED.", 30, 189, 0);
            } else {
                if (config->phantomChallenge) {
                    Print("PHANTOM CHALLENGE. SIX RACES", 30, 182, 0);
                    Print("AT RAPIER CLASS. WEAPONS ARE ACTIVATED.", 30, 189, 0);
                } else if (config->piranhaChallenge) {
                    Print("PIRANHA CHALLENGE. EIGHT RACES", 30, 182, 0);
                    Print("AT PHANTOM CLASS. WEAPONS ARE ACTIVATED.", 30, 189, 0);
                }
            }
            break;
        }
    }

    if (!menuEditorActive) {
        for (optionIndex = 0; optionIndex < menuLayout[currMenu].numBoxes; optionIndex++) {

            menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].x0;
            menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].y0;
            menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].x1;
            menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].y0;
            menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].x1;
            menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].y0;
            menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].x1;
            menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].y1;
            menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].x1;
            menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].y1;
            menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].x0;
            menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].y1;
            menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].x0;
            menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].y1;
            menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].x0;
            menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].y0;
            if (menuLayout[currMenu].box[optionIndex].x0 != 160 && menuLayout[currMenu].box[optionIndex].y0 != 120 && menuLayout[currMenu].box[optionIndex].x1 != 160 && menuLayout[currMenu].box[optionIndex].y1 != 120) {
                menuBars[CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].x0;
                menuBars[CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].y0;
                menuBars[CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].x1;
                menuBars[CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].y1;
                menuBars[CurrentScreen].x2 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].x0;
                menuBars[CurrentScreen].y2 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].y0 + 1;
                menuBars[CurrentScreen].x3 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].x1;
                menuBars[CurrentScreen].y3 = menuLayout[currMenu].box[optionIndex].edge[0][CurrentScreen].y1 + 1;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
                menuBars[CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].x0;
                menuBars[CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].y0;
                menuBars[CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].y0;
                menuBars[CurrentScreen].x2 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].x1;
                menuBars[CurrentScreen].y2 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].y1 + 1;
                menuBars[CurrentScreen].x3 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].x1 + 1;
                menuBars[CurrentScreen].y3 = menuLayout[currMenu].box[optionIndex].edge[1][CurrentScreen].y1 + 1;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
                menuBars[CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].x1;
                menuBars[CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].y1;
                menuBars[CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].y0;
                menuBars[CurrentScreen].x2 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].x1;
                menuBars[CurrentScreen].y2 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].y1 + 1;
                menuBars[CurrentScreen].x3 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y3 = menuLayout[currMenu].box[optionIndex].edge[2][CurrentScreen].y0 + 1;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
                menuBars[CurrentScreen].x0 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].x1;
                menuBars[CurrentScreen].y0 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].y1;
                menuBars[CurrentScreen].x1 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].x1 + 1;
                menuBars[CurrentScreen].y1 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].y1;
                menuBars[CurrentScreen].x2 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].x0;
                menuBars[CurrentScreen].y2 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].y0;
                menuBars[CurrentScreen].x3 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y3 = menuLayout[currMenu].box[optionIndex].edge[3][CurrentScreen].y0;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
            }
        }
    }

    if (currMenu == 3)
        DrawTeamShips(0, 80, 150, menuHeading.angle, 0, menuModels[7]);
    if (currMenu == 0) {
        DrawTeamShips(0, 140, 1800, cardNum.angle, 173,
                      shipShapeTable[highResTable[ownShip]]);
        if (networkGame) {
            DrawTeamShips(-540, 110, 800, cardNum.angle, 0, menuModels[(signed char)raceOrder.order[0]]);
        } else {
            switch (gameType) {
            case 1:
                DrawTeamShips(-600, 110, 1000, cardNum.angle, 0, menuModels[(signed char)raceOrder.order[gameType]]);
                break;
            case 2:
                if (config->phantomChallenge) {
                    DrawTeamShips(-600, 110, 1000, cardNum.angle, 0,
                                  menuModels[(signed char)raceOrder.order[4]]);
                } else {
                    DrawTeamShips(-655, 110, 1200, cardNum.angle, 0,
                                  menuModels[(signed char)raceOrder.order[5]]);
                }
                break;
            default:
                DrawTeamShips(-540, 110, 800, cardNum.angle, 0, menuModels[(signed char)raceOrder.order[gameType]]);
            }
        }
        DrawTeamShips(620, 150, 1000, cardNum.angle, 0,
                      menuModels[(signed char)classOrder.order[config->shipClass]]);
    } else if (currMenu == 1) {
        if (networkGame) {
            DrawTeamShips(440, 100, 800, cardNum.angle, 0, menuModels[(signed char)raceOrder.order[0]]);
        } else if (menu[currMenu].selection == 2) {
            if (config->phantomChallenge) {
                DrawTeamShips(440, 100, 800, cardNum.angle, 0,
                              menuModels[(signed char)raceOrder.order[4]]);
            } else {
                DrawTeamShips(440, 100, 800, cardNum.angle, 0,
                              menuModels[(signed char)raceOrder.order[5]]);
            }
        } else {
            DrawTeamShips(440, 100, 800, cardNum.angle, 0,
                          menuModels[(signed char)raceOrder.order[menu[currMenu].selection]]);
        }
    }

    if (currMenu == 0) {
        FlushMessages();
    } else {
        AddText(menu[currMenu].title.text, menu[currMenu].title.size, menu[currMenu].title.colour, 8, 0);
    }
    AddTextBits(config);

    if (menuEditorActive)
        return;
    if (currMenu == 0) {
        if (menu[currMenu].numOptions <= 0)
            return;
        if (GetMenuKey(3)) {
            menu[currMenu].selection++;
            if (!menu[currMenu].options[menu[currMenu].selection].enabled)
                menu[currMenu].selection++;
            if (menu[currMenu].selection >= menu[currMenu].numOptions)
                menu[currMenu].selection = menu[currMenu].numOptions - 1;
            else
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            return;
        }
        if (GetMenuKey(2)) {
            menu[currMenu].selection--;
            if (menu[currMenu].selection < 0) {
                menu[currMenu].selection = 0;
                if (!menu[currMenu].options[menu[currMenu].selection].enabled)
                    menu[currMenu].selection--;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
            return;
        }
        if (GetMenuKey(0)) {
            if (menu[currMenu].selection < menu[currMenu].numOptions - 2)
                return;
            menu[currMenu].selection--;
            if (menu[currMenu].selection == 2)
                menu[currMenu].selection = lastTopMenuSelection;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            return;
        }
        if (GetMenuKey(1)) {
            if (menu[currMenu].selection >= 0 && menu[currMenu].selection <= 2) {
                lastTopMenuSelection = menu[currMenu].selection;
                menu[currMenu].selection = 3;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            } else {
                menu[currMenu].selection++;
                if (menu[currMenu].selection >= menu[currMenu].numOptions)
                    menu[currMenu].selection = 4;
                else
                    PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
            return;
        }
    } else {
        if (menu[currMenu].numOptions <= 0)
            return;
        if (GetMenuKey(1)) {
            menu[currMenu].selection++;
            if (!menu[currMenu].options[menu[currMenu].selection].enabled) {
                menu[currMenu].selection--;
                optionDisabled = 1;
            }
            if (menu[currMenu].selection >= menu[currMenu].numOptions)
                menu[currMenu].selection = menu[currMenu].numOptions - 1;
            else if (!optionDisabled)
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        if (GetMenuKey(0)) {
            menu[currMenu].selection--;
            if (menu[currMenu].selection < 0) {
                menu[currMenu].selection = 0;
                if (!menu[currMenu].options[menu[currMenu].selection].enabled &&
                    menu[currMenu].selection != 0)
                    menu[currMenu].selection--;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
        }
    }
}
#else
void ShowMenu(ConfigData *config, MenuBars *bars)
{
    ClassOrder classOrder = { { 1, 0, 3, 5 } };
    RaceOrder raceOrder = { { 9, 2, 8, 10, 6, 4 } };
    u_char blocked = 0;
    short i, j;
    u_char selectedBarColour = 0x55;
    u_char normalBarColour = 0;

    if (!rgbTextures) {
        selectedBarColour = 0x4b;
        normalBarColour = 0x60;
    }
    for (i = 0; i < menu[currMenu].numOptions; i++) {
        if (currMenu == 0) {
            if (i == menu[currMenu].selection) {
                bars->options[i][CurrentScreen].r0 = selectedBarColour;
                bars->options[i][CurrentScreen].g0 = 0;
                bars->options[i][CurrentScreen].b0 = 0x28;
            } else {
                bars->options[i][CurrentScreen].r0 = normalBarColour;
                bars->options[i][CurrentScreen].g0 = 0;
                bars->options[i][CurrentScreen].b0 = 0x2e;
            }
            if (i == 3 || i == 4 || i == menu[currMenu].selection) {
                DrawMenuOptionBackdrop(bars->options,
                    menuLayout[currMenu].box[i].x0 + 1, menuLayout[currMenu].box[i].y0 + 1,
                    menuLayout[currMenu].box[i].x1 - menuLayout[currMenu].box[i].x0, menuLayout[currMenu].box[i].y1 - menuLayout[currMenu].box[i].y0,
                    i, menu[currMenu].selection);
            }
        }
        for (j = 0; j < menu[currMenu].options[i].numText; j++) {
            if (currMenu == 0) {
                CentreText2(menu[currMenu].options[i].text[j].text,
                    menu[currMenu].options[i].x,
                    menu[currMenu].options[i].y + j * 10, 8, 0);
            } else if (menu[currMenu].options[i].enabled) {
                if (currMenu == 2) {
                    if (i == menu[currMenu].selection) {
                        CentreText2(menu[currMenu].options[i].text[j].text, 68,
                            menu[currMenu].options[0].y + i * 10, 8, 1);
                    } else {
                        CentreText2(menu[currMenu].options[i].text[j].text, 68,
                            menu[currMenu].options[0].y + i * 10, 8, 0);
                    }
                } else if (currMenu == 3) {
                    if (i == menu[currMenu].selection) {
                        CentreText(menu[currMenu].options[i].text[j].text, 68,
                           menu[currMenu].options[0].y + i * 14, 8, 1);
                    } else {
                        CentreText(menu[currMenu].options[i].text[j].text, 68,
                           menu[currMenu].options[0].y + i * 14, 8, 0);
                    }
                } else {
                    if (i == menu[currMenu].selection) {
                        AddText(menu[currMenu].options[i].text[j].text,
                        menu[currMenu].options[0].x,
                        menu[currMenu].options[0].y + i * 10, 8, 1);
                    } else {
                        AddText(menu[currMenu].options[i].text[j].text,
                        menu[currMenu].options[0].x,
                        menu[currMenu].options[0].y + i * 10, 8, 0);
                    }
                }
            }
        }
    }

    if (currMenu == 9) {
        bars->options[0][CurrentScreen].r0 = selectedBarColour;
        bars->options[0][CurrentScreen].g0 = 0;
        bars->options[0][CurrentScreen].b0 = 0x28;

    }


    if (currMenu != 13 && currMenu != 15 && currMenu != 14 && currMenu != 9 &&
        currMenu != 10 && currMenu != 11)
        DoButton(30, 198, 2, "SELECT");
    if (currMenu == 9 || currMenu == 10 || currMenu == 11)
        DoButton(30, 198, 2, "ACCEPT");
    if (currMenu != 0)
        DoButton(278, 198, 1, "BACK");
    else
        DoButton(278, 198, 1, "QUIT");

    if (currMenu == 2)
        ClassSelect(config);
    if (currMenu == 1) {
        switch (menu[currMenu].selection) {
        case 0:
            Print("SINGLE RACE AGAINST A FULL", 30, 182, 0);
            Print("GRID OF COMPETITORS.", 30, 189, 0);
            break;
        case 1:
            Print("SINGLE RACE TIME TRIAL.", 30, 182, 0);
            Print("NO COMPETITORS.", 30, 189, 0);
            break;
        case 2:
            if (config->venomComplete) {
                Print("PHANTOM CHALLENGE. SIX RACES", 30, 182, 0);
                Print("AT RAPIER CLASS. WEAPONS ARE ACTIVATED.", 30, 189, 0);
            } else if (config->rapierComplete) {
                Print("PIRANHA CHALLENGE. EIGHT RACES", 30, 182, 0);
                Print("AT PHANTOM CLASS. WEAPONS ARE ACTIVATED.", 30, 189, 0);
            }
            break;
        }
    }

    if (!menuEditorActive) {
        for (i = 0; i < menuLayout[currMenu].numBoxes; i++) {
            MenuBox *box = &menuLayout[currMenu].box[i];

            box->edge[0][CurrentScreen].x0 = box->x0;
            box->edge[0][CurrentScreen].y0 = box->y0;
            box->edge[0][CurrentScreen].x1 = box->x1;
            box->edge[0][CurrentScreen].y1 = box->y0;
            box->edge[1][CurrentScreen].x0 = box->x1;
            box->edge[1][CurrentScreen].y0 = box->y0;
            box->edge[1][CurrentScreen].x1 = box->x1;
            box->edge[1][CurrentScreen].y1 = box->y1;
            box->edge[2][CurrentScreen].x0 = box->x1;
            box->edge[2][CurrentScreen].y0 = box->y1;
            box->edge[2][CurrentScreen].x1 = box->x0;
            box->edge[2][CurrentScreen].y1 = box->y1;
            box->edge[3][CurrentScreen].x0 = box->x0;
            box->edge[3][CurrentScreen].y0 = box->y1;
            box->edge[3][CurrentScreen].x1 = box->x0;
            box->edge[3][CurrentScreen].y1 = box->y0;
            if (box->x0 != 160 && box->y0 != 120 && box->x1 != 160 && box->y1 != 120) {
                menuBars[CurrentScreen].x0 = box->edge[0][CurrentScreen].x0;
                menuBars[CurrentScreen].y0 = box->edge[0][CurrentScreen].y0;
                menuBars[CurrentScreen].x1 = box->edge[0][CurrentScreen].x1;
                menuBars[CurrentScreen].y1 = box->edge[0][CurrentScreen].y1;
                menuBars[CurrentScreen].x2 = box->edge[0][CurrentScreen].x0;
                menuBars[CurrentScreen].y2 = box->edge[0][CurrentScreen].y0 + 1;
                menuBars[CurrentScreen].x3 = box->edge[0][CurrentScreen].x1;
                menuBars[CurrentScreen].y3 = box->edge[0][CurrentScreen].y1 + 1;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
                box = &menuLayout[currMenu].box[i];

                menuBars[CurrentScreen].x0 = box->edge[1][CurrentScreen].x0;
                menuBars[CurrentScreen].y0 = box->edge[1][CurrentScreen].y0;
                menuBars[CurrentScreen].x1 = box->edge[1][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y1 = box->edge[1][CurrentScreen].y0;
                menuBars[CurrentScreen].x2 = box->edge[1][CurrentScreen].x1;
                menuBars[CurrentScreen].y2 = box->edge[1][CurrentScreen].y1 + 1;
                menuBars[CurrentScreen].x3 = box->edge[1][CurrentScreen].x1 + 1;
                menuBars[CurrentScreen].y3 = box->edge[1][CurrentScreen].y1 + 1;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
                box = &menuLayout[currMenu].box[i];

                menuBars[CurrentScreen].x0 = box->edge[2][CurrentScreen].x1;
                menuBars[CurrentScreen].y0 = box->edge[2][CurrentScreen].y1;
                menuBars[CurrentScreen].x1 = box->edge[2][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y1 = box->edge[2][CurrentScreen].y0;
                menuBars[CurrentScreen].x2 = box->edge[2][CurrentScreen].x1;
                menuBars[CurrentScreen].y2 = box->edge[2][CurrentScreen].y1 + 1;
                menuBars[CurrentScreen].x3 = box->edge[2][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y3 = box->edge[2][CurrentScreen].y0 + 1;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
                box = &menuLayout[currMenu].box[i];

                menuBars[CurrentScreen].x0 = box->edge[3][CurrentScreen].x1;
                menuBars[CurrentScreen].y0 = box->edge[3][CurrentScreen].y1;
                menuBars[CurrentScreen].x1 = box->edge[3][CurrentScreen].x1 + 1;
                menuBars[CurrentScreen].y1 = box->edge[3][CurrentScreen].y1;
                menuBars[CurrentScreen].x2 = box->edge[3][CurrentScreen].x0;
                menuBars[CurrentScreen].y2 = box->edge[3][CurrentScreen].y0;
                menuBars[CurrentScreen].x3 = box->edge[3][CurrentScreen].x0 + 1;
                menuBars[CurrentScreen].y3 = box->edge[3][CurrentScreen].y0;
                DrawPolyF4(0.13f, &menuBars[CurrentScreen]);
            }
        }
    }

    if (currMenu == 3)
        DrawTeamShips(0, 80, 150, menuHeading.angle, 0, menuModels[7]);
    if (currMenu == 0) {
        DrawTeamShips(0, 140, 1800, cardNum.angle, 173,
                      shipShapeTable[highResTable[ownShip]]);
        switch (gameType) {
        case 1:
            DrawTeamShips(-600, 110, 1000, cardNum.angle, 0, menuModels[(signed char)raceOrder.order[gameType]]);
            break;
        case 2:
            if (config->venomComplete) {
                DrawTeamShips(-600, 110, 1000, cardNum.angle, 0, menuModels[6]);
            } else {
                DrawTeamShips(-655, 110, 1200, cardNum.angle, 0, menuModels[4]);
            }
            break;
        default:
            DrawTeamShips(-540, 110, 800, cardNum.angle, 0, menuModels[(signed char)raceOrder.order[gameType]]);
        }
        DrawTeamShips(620, 150, 1000, cardNum.angle, 0,
                      menuModels[(signed char)classOrder.order[config->shipClass]]);
    } else if (currMenu == 1) {
        if (menu[currMenu].selection == 2) {
            if (config->venomComplete) {
                DrawTeamShips(440, 100, 800, cardNum.angle, 0, menuModels[6]);
            } else {
                DrawTeamShips(440, 100, 800, cardNum.angle, 0, menuModels[4]);
            }
        } else {
            DrawTeamShips(440, 100, 800, cardNum.angle, 0,
                          menuModels[(signed char)raceOrder.order[menu[currMenu].selection]]);
        }
    }

    if (currMenu == 0) {
        POLY_FT4 title;
        SetTextQuad(&title, 25, 38, 0, 0, 254, 64, 35, 0);
        DrawScaledTexturedQuad(textDepth, &title, hudTexturePage);
        SetTextQuad(&title, 151, 38, 0, 64, 210, 64, 35, 0);
        DrawScaledTexturedQuad(textDepth, &title, hudTexturePage);
    } else {
        AddText(menu[currMenu].title.text, menu[currMenu].title.size, menu[currMenu].title.colour, 8, 0);
    }
    AddTextBits(config);

    if (menuEditorActive)
        return;
    if (currMenu == 0) {
        if (menu[currMenu].numOptions <= 0)
            return;
        if (GetMenuKey(3)) {
            menu[currMenu].selection++;
            if (!menu[currMenu].options[menu[currMenu].selection].enabled)
                menu[currMenu].selection++;
            if (menu[currMenu].selection >= menu[currMenu].numOptions)
                menu[currMenu].selection = menu[currMenu].numOptions - 1;
            else
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            return;
        }
        if (GetMenuKey(2)) {
            menu[currMenu].selection--;
            if (menu[currMenu].selection < 0) {
                menu[currMenu].selection = 0;
                if (!menu[currMenu].options[0].enabled)
                    menu[currMenu].selection = -1;
            } else {
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
            return;
        }
        if (GetMenuKey(0)) {
            if (menu[currMenu].selection < menu[currMenu].numOptions - 2)
                return;
            menu[currMenu].selection--;
            if (menu[currMenu].selection == 2)
                menu[currMenu].selection = lastTopOption;
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            return;
        }
        if (GetMenuKey(1)) {
            if (menu[currMenu].selection >= 0 && menu[currMenu].selection <= 2) {
                lastTopOption = menu[currMenu].selection;
                menu[currMenu].selection = 3;
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            } else {
                menu[currMenu].selection++;
                if (menu[currMenu].selection >= menu[currMenu].numOptions)
                    menu[currMenu].selection = 4;
                else
                    PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            }
            return;
        }
    } else {
        if (menu[currMenu].numOptions <= 0)
            return;
        if (GetMenuKey(1)) {
            menu[currMenu].selection++;
            if (!menu[currMenu].options[menu[currMenu].selection].enabled) {
                blocked = 1;
                menu[currMenu].selection--;
            }
            if (menu[currMenu].selection >= menu[currMenu].numOptions)
                menu[currMenu].selection = menu[currMenu].numOptions - 1;
            else if (!blocked)
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
        if (GetMenuKey(0)) {
            menu[currMenu].selection--;
            if (menu[currMenu].selection < 0)
                menu[currMenu].selection = 0;
            else
                PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
        }
    }
}
#endif
#endif

/* Runs the screen of the current menu (the PC build keeps one routine per menu number). */
#ifdef NETPLAY
void doSessionConfigMenu(void);
void doTournamentMenu(void);
void RunMenuScreen(PcGameData *gameData, HiScoreData *score)
{
    switch (currMenu) {
    case 5:
        ClassSelect(gameData);
        break;
    case 6:
        ClassSelect(gameData);
        break;
    case 7:
        ClassSelect(gameData);
        break;
    case 8:
        ClassSelect(gameData);
        break;
    case 4:
        TeamSelect(gameData);
        break;
    case 9:
        AudioConfig(gameData);
        break;
    case 10:
        SelectController(gameData);
        break;
    case 15:
        RedefineController(gameData);
        break;
    case 11:
        DisplayConfig(gameData, score);
        break;
    case 12:
        LoadSave(gameData);
        break;
    case 13:
        DispHiScores(gameData, 4);
        break;
    case 14:
        DispHiScores(gameData, 5);
        break;
    case 17:
        doNetworkOptionsMenu();
        break;
    case 18:
        doSessionConfigMenu();
        break;
    case 19:
        doTournamentMenu();
        break;
    case 20:
        doSessionJoinMenu();
        break;
    case 21:
        doLobbyMenu();
        break;
    }
}
#else
void RunMenuScreen(PcGameData *gameData, HiScoreData *score)
{
    switch (currMenu) {
    case 4:
        TeamSelect(gameData);
        break;
    case 5:
        ClassSelect(gameData);
        break;
    case 6:
        ClassSelect(gameData);
        break;
    case 7:
        ClassSelect(gameData);
        break;
    case 8:
        ClassSelect(gameData);
        break;
    case 9:
        AudioConfig(gameData);
        break;
    case 10:
        SelectController(gameData);
        break;
    case 11:
        DisplayConfig(gameData, score);
        break;
    case 12:
        LoadSave(gameData);
        break;
    case 13:
        DispHiScores(gameData, 4);
        break;
    case 14:
        DispHiScores(gameData, 5);
        break;
    case 15:
        RedefineController(gameData);
        break;
    }
}
#endif

#ifdef NETPLAY
#pragma intrinsic(strcpy)
/* Draws the team, mode and track captions of the main menu screens. */
void AddTextBits(PcGameData *gameData)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    char formattedTrack[32];
    char heading[28];

    switch (currMenu) {
    case 0:
        if (ownShip >= 0 && ownShip <= 2) {
            strcpy(heading, "AG SYSTEMS");
        } else if (ownShip >= 3 && ownShip <= 5) {
            strcpy(heading, "AURICOM");
        } else if (ownShip >= 6 && ownShip <= 8) {
            strcpy(heading, "QIREX");
        } else if (ownShip >= 9 && ownShip <= 11) {
            strcpy(heading, "FEISAR");
        } else if (ownShip >= 12 && ownShip <= 14) {
            strcpy(heading, "PIRANHA");
        }
        PrintCX(heading, 160, 172, 0);
        if (networkGame) {
            switch (gstRaceMode) {
            case 0:
                strcpy(heading, "TIMED");
                break;
            case 1:
                strcpy(heading, "DEATH RACE");
                break;
            case 3:
                strcpy(heading, "TOURNAMENT");
                break;
            default:
                strcpy(heading, "DEFAULT");
                break;
            }
        } else {
            switch (gameType) {
            case 0:
                strcpy(heading, "ARCADE");
                break;
            case 1:
                strcpy(heading, "TIME TRIAL");
                break;
            case 2:
                if (gameData->phantomChallenge != 0) {
                    strcpy(heading, "CHALLENGE I");
                } else if (gameData->piranhaChallenge != 0) {
                    strcpy(heading, "CHALLENGE II");
                }
                break;
            }
        }
        PrintCX(heading, 63, 172, 0);
        strcpy(formattedTrack, GetTrackName(gameData->actTrackNum, 70, 100, 0));
        PrintCX(formattedTrack, 256, 172, 0);
        strcpy(heading, "ESSENTIAL OPTIONS");
        break;
    case 2:
        strcpy(heading, "RACING MODE");
        break;
    case 1:
        strcpy(heading, "STRUCTURE MODE");
        break;
    case 3:
        strcpy(heading, "OPTIONS MODE");
        break;
    case 5:
        strcpy(heading, "TRACK MODE");
        break;
    case 6:
        strcpy(heading, "TRACK MODE");
        break;
    case 7:
        strcpy(heading, "TRACK MODE");
        break;
    case 8:
        strcpy(heading, "TRACK MODE");
        break;
    case 4:
        strcpy(heading, "TEAM MODE");
        break;
    case 9:
        strcpy(heading, "PRECISION LEVELS");
        break;
    case 10:
        strcpy(heading, "INTERACTION MODE");
        break;
    case 15:
        strcpy(heading, "CONTROLLER SETUP MODE");
        break;
    case 12:
        strcpy(heading, "DATA MODE");
        break;
    case 11:
        strcpy(heading, "PREFERENCES MODE");
        break;
    case 13:
        strcpy(heading, "ARCADE MODE");
        break;
    case 14:
        strcpy(heading, "TIME TRIAL MODE");
        break;
    }
    Print(heading, 30, 75, 0);
}
#pragma function(strcpy)
#else
/* Draws the team, mode and track captions of the main menu screens. */
void AddTextBits(PcGameData *gameData)
{
    char menuLabel[28];
    short ship;
    char formattedTrack[32];

    switch (currMenu) {
    case 0:
        ship = ownShip;
        if (ship >= 0 && ship <= 2) {
            strcpy(menuLabel, "AG SYSTEMS");
        } else if (ship >= 3 && ship <= 5) {
            strcpy(menuLabel, "AURICOM");
        } else if (ship >= 6 && ship <= 8) {
            strcpy(menuLabel, "QIREX");
        } else if (ship >= 9 && ship <= 11) {
            strcpy(menuLabel, "FEISAR");
        } else if (ship >= 12 && ship <= 14) {
            strcpy(menuLabel, "PIRANHA");
        }
        PrintCX(menuLabel, 160, 172, 0);
        switch (gameType) {
        case 0:
            strcpy(menuLabel, "ARCADE");
            break;
        case 1:
            strcpy(menuLabel, "TIME TRIAL");
            break;
        case 2:
            if (gameData->venomComplete != 0) {
                strcpy(menuLabel, "CHALLENGE I");
            } else if (gameData->rapierComplete != 0) {
                strcpy(menuLabel, "CHALLENGE II");
            }
            break;
        }
        PrintCX(menuLabel, 63, 172, 0);
        strcpy(formattedTrack, GetTrackName(gameData->actTrackNum, 70, 100, 0));
        PrintCX(formattedTrack, 256, 172, 0);
        strcpy(menuLabel, "ESSENTIAL OPTIONS");
        break;
    case 2:
        strcpy(menuLabel, "RACING MODE");
        break;
    case 1:
        strcpy(menuLabel, "STRUCTURE MODE");
        break;
    case 3:
        strcpy(menuLabel, "OPTIONS MODE");
        break;
    case 5:
        strcpy(menuLabel, "TRACK MODE");
        break;
    case 6:
        strcpy(menuLabel, "TRACK MODE");
        break;
    case 7:
        strcpy(menuLabel, "TRACK MODE");
        break;
    case 8:
        strcpy(menuLabel, "TRACK MODE");
        break;
    case 4:
        strcpy(menuLabel, "TEAM MODE");
        break;
    case 9:
        strcpy(menuLabel, "PRECISION LEVELS");
        break;
    case 10:
        strcpy(menuLabel, "INTERACTION MODE");
        break;
    case 15:
        strcpy(menuLabel, "CONTROLLER SETUP MODE");
        break;
    case 12:
        strcpy(menuLabel, "DATA MODE");
        break;
    case 11:
        strcpy(menuLabel, "PREFERENCES MODE");
        break;
    case 13:
        strcpy(menuLabel, "ARCADE MODE");
        break;
    case 14:
        strcpy(menuLabel, "TIME TRIAL MODE");
        break;
    }
    Print(menuLabel, 30, 75, 0);
}
#endif

extern char errorString[];

MenuLayout *menuLayout;

/* Reads menu.dat from the data directory into the menu layout table. */
#pragma auto_inline(on)
#ifdef NETPLAY
#pragma intrinsic(strcpy, strcat)
#endif
void LoadMenuLineData(void)
{
    char fullPath[256];
    int fileLength;
    void *fileBuffer;

    strcpy(fullPath, wobbleDir2);
    strcat(fullPath, "menu.dat");
    fileLength = FileLength(fullPath);
    if (fileLength <= 0) {
        FileAccessError(fullPath);
        return;
    }
    fileBuffer = DAlloc(reserveHeap, fileLength + 0x20);
    if (fileBuffer == 0) {
        wsprintf(errorString, "LoadMenuLineData (): Failed to allocate memory for file \n");
        Error(errorString, 2);
    }
    if (LoadFile(fullPath, fileBuffer) != fileLength) {
        wsprintf(errorString, "LoadMenuLineData(): Failed to load file \n");
        Error(errorString, 2);
    }
    *(MenuFile *)menuLayout = *(MenuFile *)fileBuffer;
    DFree(reserveHeap, fileBuffer);
}
#ifdef NETPLAY
#pragma function(strcpy, strcat)
#endif
#pragma auto_inline(off)

/* Empty stub that sits just before InitMenus. Name not known yet. */
void InitLineEditor(void) {}

#ifdef NETPLAY
#pragma intrinsic(strcpy)
/* Sets the menu labels and per-screen defaults used by MenuControl; the network build adds the
   tournament, session and lobby screens. */
void InitMenus(void)
{
    strcpy(menu[0].options[0].text[0].text, "RACE");
    menu[0].options[0].text[0].size = 8;
    menu[0].options[0].text[0].colour = 0;
    strcpy(menu[0].options[0].text[1].text, "TYPE");
    menu[0].options[0].text[1].size = 8;
    menu[0].options[0].text[1].colour = 0;
    menu[0].options[0].x = 63;
    menu[0].options[0].y = 87;
    menu[0].options[0].numText = 2;
    menu[0].options[0].font = 9;
    menu[0].options[0].nextMenu = 1;
    menu[0].options[0].selection = 0;
    menu[0].options[0].enabled = 1;
    menu[0].options[0].flags = 0;
    strcpy(menu[0].options[2].text[0].text, "CLASS");
    menu[0].options[2].text[0].size = 8;
    menu[0].options[2].text[0].colour = 0;
    strcpy(menu[0].options[2].text[1].text, "AND");
    menu[0].options[2].text[1].size = 8;
    menu[0].options[2].text[1].colour = 0;
    strcpy(menu[0].options[2].text[2].text, "TRACK");
    menu[0].options[2].text[2].size = 8;
    menu[0].options[2].text[2].colour = 0;
    menu[0].options[2].x = 255;
    menu[0].options[2].y = 87;
    menu[0].options[2].numText = 3;
    menu[0].options[2].font = 9;
    menu[0].options[2].nextMenu = 2;
    menu[0].options[2].selection = 0;
    menu[0].options[2].enabled = 1;
    menu[0].options[2].flags = 0;
    strcpy(menu[0].options[1].text[0].text, "TEAM");
    menu[0].options[1].text[0].size = 8;
    menu[0].options[1].text[0].colour = 0;
    menu[0].options[1].x = 161;
    menu[0].options[1].y = 87;
    menu[0].options[1].numText = 1;
    menu[0].options[1].font = 9;
    menu[0].options[1].nextMenu = 4;
    menu[0].options[1].selection = 0;
    menu[0].options[1].enabled = 1;
    menu[0].options[1].flags = 0;
    strcpy(menu[0].options[4].text[0].text, "OPTIONS");
    menu[0].options[4].text[0].size = 8;
    menu[0].options[4].text[0].colour = 0;
    menu[0].options[4].x = 162;
    menu[0].options[4].y = 203;
    menu[0].options[4].numText = 1;
    menu[0].options[4].font = 9;
    menu[0].options[4].nextMenu = 3;
    menu[0].options[4].selection = 0;
    menu[0].options[4].enabled = 1;
    menu[0].options[4].flags = 0;
    strcpy(menu[0].options[3].text[0].text, "START");
    menu[0].options[3].text[0].size = 8;
    menu[0].options[3].text[0].colour = 0;
    menu[0].options[3].x = 162;
    menu[0].options[3].y = 185;
    menu[0].options[3].numText = 1;
    menu[0].options[3].font = 9;
    menu[0].options[3].nextMenu = 1;
    menu[0].options[3].selection = 0;
    menu[0].options[3].enabled = 1;
    menu[0].options[3].flags = 0;
    strcpy(menu[0].title.text, "WIPEOUT");
    menu[0].title.size = 40;
    menu[0].title.colour = 50;
    menu[0].id = 0;
    menu[0].flags = 1;
    menu[0].numOptions = 5;
    menu[0].prevMenu = 999;
    menu[0].selection = 0;
    menu[0].mode = 0;
    menu[0].updateCount = 0;
    strcpy(menu[17].title.text, "MULTIPLAYER RACE");
    menu[17].title.size = 40;
    menu[17].title.colour = 50;
    menu[17].id = 0;
    menu[17].flags = 0;
    menu[17].numOptions = 0;
    menu[17].prevMenu = 0;
    menu[17].nextMenu = 21;
    menu[17].selection = 0;
    menu[17].mode = 0;
    menu[17].updateCount = 0;
    strcpy(menu[21].title.text, "RACE LOBBY");
    menu[21].title.size = 40;
    menu[21].title.colour = 50;
    menu[21].id = 0;
    menu[21].flags = 0;
    menu[21].numOptions = 0;
    menu[21].prevMenu = 17;
    menu[21].nextMenu = 0;
    menu[21].selection = 0;
    menu[21].mode = 0;
    menu[21].updateCount = 0;
    strcpy(menu[18].title.text, "RACE CUSTOMIZATION");
    menu[18].title.size = 40;
    menu[18].title.colour = 50;
    menu[18].id = 0;
    menu[18].flags = 0;
    menu[18].numOptions = 0;
    menu[18].prevMenu = 17;
    menu[18].nextMenu = 0;
    menu[18].selection = 0;
    menu[18].mode = 0;
    menu[18].updateCount = 0;
    strcpy(menu[19].title.text, "TOURNAMENT CUSTOMIZATION");
    menu[19].title.size = 40;
    menu[19].title.colour = 50;
    menu[19].id = 0;
    menu[19].flags = 0;
    menu[19].numOptions = 0;
    menu[19].prevMenu = 17;
    menu[19].nextMenu = 0;
    menu[19].selection = 0;
    menu[19].mode = 0;
    menu[19].updateCount = 0;
    strcpy(menu[20].title.text, "JOIN RACING SESSION ");
    menu[20].title.size = 40;
    menu[20].title.colour = 50;
    menu[20].id = 0;
    menu[20].flags = 0;
    menu[20].numOptions = 0;
    menu[20].prevMenu = 17;
    menu[20].nextMenu = 0;
    menu[20].selection = 0;
    menu[20].mode = 0;
    menu[20].updateCount = 0;
    strcpy(menu[2].options[0].text[0].text, "VECTOR");
    menu[2].options[0].text[0].size = 8;
    menu[2].options[0].text[0].colour = 0;
    menu[2].options[0].x = 30;
    menu[2].options[0].y = 90;
    menu[2].options[0].numText = 1;
    menu[2].options[0].font = 9;
    menu[2].options[0].nextMenu = 0;
    menu[2].options[0].selection = 0;
    menu[2].options[0].enabled = 1;
    menu[2].options[0].flags = 0;
    strcpy(menu[2].options[1].text[0].text, "VENOM");
    menu[2].options[1].text[0].size = 8;
    menu[2].options[1].text[0].colour = 0;
    menu[2].options[1].x = 128;
    menu[2].options[1].y = 64;
    menu[2].options[1].numText = 1;
    menu[2].options[1].font = 9;
    menu[2].options[1].nextMenu = 0;
    menu[2].options[1].selection = 0;
    menu[2].options[1].enabled = 1;
    menu[2].options[1].flags = 0;
    strcpy(menu[2].options[2].text[0].text, "RAPIER");
    menu[2].options[2].text[0].size = 8;
    menu[2].options[2].text[0].colour = 0;
    menu[2].options[2].x = 205;
    menu[2].options[2].y = 64;
    menu[2].options[2].numText = 1;
    menu[2].options[2].font = 9;
    menu[2].options[2].nextMenu = 0;
    menu[2].options[2].selection = 0;
    menu[2].options[2].enabled = 1;
    menu[2].options[2].flags = 0;
    strcpy(menu[2].options[3].text[0].text, "PHANTOM");
    menu[2].options[3].text[0].size = 8;
    menu[2].options[3].text[0].colour = 0;
    menu[2].options[3].x = 281;
    menu[2].options[3].y = 64;
    menu[2].options[3].numText = 1;
    menu[2].options[3].font = 9;
    menu[2].options[3].nextMenu = 0;
    menu[2].options[3].selection = 0;
    menu[2].options[3].enabled = 0;
    menu[2].options[3].flags = 0;
    strcpy(menu[2].title.text, "RACING CLASS");
    menu[2].title.size = 40;
    menu[2].title.colour = 50;
    menu[2].id = 0;
    menu[2].flags = 0;
    menu[2].numOptions = 4;
    menu[2].prevMenu = 0;
    menu[2].selection = 0;
    menu[2].mode = 0;
    menu[2].updateCount = 0;
    strcpy(menu[1].options[0].text[0].text, "ARCADE");
    menu[1].options[0].text[0].size = 8;
    menu[1].options[0].text[0].colour = 0;
    menu[1].options[0].x = 30;
    menu[1].options[0].y = 90;
    menu[1].options[0].numText = 1;
    menu[1].options[0].font = 9;
    menu[1].options[0].nextMenu = 0;
    menu[1].options[0].selection = 0;
    menu[1].options[0].enabled = 1;
    menu[1].options[0].flags = 0;
    if (networkGame) {
        strcpy(menu[1].options[0].text[0].text, "TIMED");
    }
    strcpy(menu[1].options[1].text[0].text, "TIME TRIAL");
    menu[1].options[1].text[0].size = 8;
    menu[1].options[1].text[0].colour = 0;
    menu[1].options[1].x = 128;
    menu[1].options[1].y = 64;
    menu[1].options[1].numText = 1;
    menu[1].options[1].font = 9;
    menu[1].options[1].nextMenu = 0;
    menu[1].options[1].selection = 0;
    menu[1].options[1].enabled = 1;
    menu[1].options[1].flags = 0;
    if (networkGame) {
        strcpy(menu[1].options[1].text[0].text, "DEATH RACE");
    }
    strcpy(menu[1].options[2].text[0].text, "CHALLENGE I");
    menu[1].options[2].text[0].size = 8;
    menu[1].options[2].text[0].colour = 0;
    menu[1].options[2].x = 167;
    menu[1].options[2].y = 144;
    menu[1].options[2].numText = 1;
    menu[1].options[2].font = 9;
    menu[1].options[2].nextMenu = 0;
    menu[1].options[2].selection = 0;
    menu[1].options[2].enabled = 1;
    menu[1].options[2].flags = 0;
    if (networkGame) {
        strcpy(menu[1].options[2].text[0].text, "TOURNAMENT");
    }
    strcpy(menu[1].title.text, "RACE TYPE");
    menu[1].title.size = 40;
    menu[1].title.colour = 50;
    menu[1].id = 0;
    menu[1].flags = 0;
    menu[1].numOptions = 2;
    menu[1].prevMenu = 0;
    menu[1].selection = 0;
    menu[1].mode = 0;
    menu[1].updateCount = 0;
    if (networkGame) {
        menu[1].numOptions = 3;
    }
    strcpy(menu[3].options[0].text[0].text, "AUDIO CONFIG");
    menu[3].options[0].text[0].size = 8;
    menu[3].options[0].text[0].colour = 0;
    menu[3].options[0].x = 30;
    menu[3].options[0].y = 90;
    menu[3].options[0].numText = 1;
    menu[3].options[0].font = 9;
    menu[3].options[0].nextMenu = 9;
    menu[3].options[0].selection = 0;
    menu[3].options[0].enabled = 1;
    menu[3].options[0].flags = 0;
    strcpy(menu[3].options[1].text[0].text, "CONTROLLER SETUP");
    menu[3].options[1].text[0].size = 8;
    menu[3].options[1].text[0].colour = 0;
    menu[3].options[1].x = 128;
    menu[3].options[1].y = 64;
    menu[3].options[1].numText = 1;
    menu[3].options[1].font = 9;
    menu[3].options[1].nextMenu = 10;
    menu[3].options[1].selection = 0;
    menu[3].options[1].enabled = 1;
    menu[3].options[1].flags = 0;
    strcpy(menu[3].options[2].text[0].text, "PREFERENCES");
    menu[3].options[2].text[0].size = 8;
    menu[3].options[2].text[0].colour = 0;
    menu[3].options[2].x = 205;
    menu[3].options[2].y = 64;
    menu[3].options[2].numText = 1;
    menu[3].options[2].font = 9;
    menu[3].options[2].nextMenu = 11;
    menu[3].options[2].selection = 0;
    menu[3].options[2].enabled = 1;
    menu[3].options[2].flags = 0;
    strcpy(menu[3].options[3].text[0].text, "LOAD AND SAVE");
    menu[3].options[3].text[0].size = 8;
    menu[3].options[3].text[0].colour = 0;
    menu[3].options[3].x = 281;
    menu[3].options[3].y = 64;
    menu[3].options[3].numText = 1;
    menu[3].options[3].font = 9;
    menu[3].options[3].nextMenu = 12;
    menu[3].options[3].selection = 0;
    menu[3].options[3].enabled = 1;
    menu[3].options[3].flags = 0;
    strcpy(menu[3].options[4].text[0].text, "BEST ARCADE TIMES");
    menu[3].options[4].text[0].size = 8;
    menu[3].options[4].text[0].colour = 0;
    menu[3].options[4].x = 129;
    menu[3].options[4].y = 144;
    menu[3].options[4].numText = 1;
    menu[3].options[4].font = 9;
    menu[3].options[4].nextMenu = 13;
    menu[3].options[4].selection = 0;
    menu[3].options[4].enabled = 1;
    menu[3].options[4].flags = 0;
    strcpy(menu[3].options[5].text[0].text, "BEST TIME TRIAL TIMES");
    menu[3].options[5].text[0].size = 8;
    menu[3].options[5].text[0].colour = 0;
    menu[3].options[5].x = 129;
    menu[3].options[5].y = 144;
    menu[3].options[5].numText = 1;
    menu[3].options[5].font = 9;
    menu[3].options[5].nextMenu = 14;
    menu[3].options[5].selection = 0;
    menu[3].options[5].enabled = 1;
    menu[3].options[5].flags = 0;
    strcpy(menu[3].title.text, "OPTIONS MENU");
    menu[3].title.size = 40;
    menu[3].title.colour = 50;
    menu[3].id = 0;
    menu[3].flags = 0;
    menu[3].numOptions = 6;
    menu[3].prevMenu = 0;
    menu[3].selection = 0;
    menu[3].mode = 0;
    menu[3].updateCount = 0;
    strcpy(menu[5].title.text, "VECTOR CLASS");
    menu[5].title.size = 40;
    menu[5].title.colour = 50;
    menu[5].id = 0;
    menu[5].flags = 0;
    menu[5].numOptions = 0;
    menu[5].prevMenu = 2;
    menu[5].nextMenu = 0;
    menu[5].selection = 0;
    menu[5].mode = 0;
    menu[5].updateCount = 0;
    strcpy(menu[6].title.text, "VENOM CLASS");
    menu[6].title.size = 40;
    menu[6].title.colour = 50;
    menu[6].id = 0;
    menu[6].flags = 0;
    menu[6].numOptions = 0;
    menu[6].prevMenu = 2;
    menu[6].nextMenu = 0;
    menu[6].selection = 0;
    menu[6].mode = 0;
    menu[6].updateCount = 0;
    strcpy(menu[7].title.text, "RAPIER CLASS");
    menu[7].title.size = 40;
    menu[7].title.colour = 50;
    menu[7].id = 0;
    menu[7].flags = 0;
    menu[7].numOptions = 0;
    menu[7].prevMenu = 2;
    menu[7].nextMenu = 0;
    menu[7].selection = 0;
    menu[7].mode = 0;
    menu[7].updateCount = 0;
    strcpy(menu[8].title.text, "PHANTOM CLASS");
    menu[8].title.size = 40;
    menu[8].title.colour = 50;
    menu[8].id = 0;
    menu[8].flags = 0;
    menu[8].numOptions = 0;
    menu[8].prevMenu = 2;
    menu[8].nextMenu = 0;
    menu[8].selection = 0;
    menu[8].mode = 0;
    menu[8].updateCount = 0;
    strcpy(menu[4].title.text, "TEAM MENU");
    menu[4].title.size = 40;
    menu[4].title.colour = 50;
    menu[4].id = 0;
    menu[4].flags = 0;
    menu[4].numOptions = 0;
    menu[4].prevMenu = 0;
    menu[4].nextMenu = 0;
    menu[4].selection = 0;
    menu[4].mode = 0;
    menu[4].updateCount = 0;
    strcpy(menu[9].title.text, "AUDIO CONFIG");
    menu[9].title.size = 40;
    menu[9].title.colour = 50;
    menu[9].id = 0;
    menu[9].flags = 0;
    menu[9].numOptions = 0;
    menu[9].prevMenu = 3;
    menu[9].nextMenu = 3;
    menu[9].selection = 0;
    menu[9].mode = 0;
    menu[9].updateCount = 0;
    strcpy(menu[10].title.text, "CONTROLLER");
    menu[10].title.size = 40;
    menu[10].title.colour = 50;
    menu[10].id = 0;
    menu[10].flags = 0;
    menu[10].numOptions = 0;
    menu[10].prevMenu = 3;
    menu[10].nextMenu = 3;
    menu[10].selection = 0;
    menu[10].mode = 0;
    menu[10].updateCount = 0;
    strcpy(menu[12].title.text, "LOAD AND SAVE");
    menu[12].title.size = 40;
    menu[12].title.colour = 50;
    menu[12].id = 0;
    menu[12].flags = 0;
    menu[12].numOptions = 0;
    menu[12].prevMenu = 3;
    menu[12].nextMenu = 3;
    menu[12].selection = 0;
    menu[12].mode = 0;
    menu[12].updateCount = 0;
    strcpy(menu[11].title.text, "GAME CONFIG");
    menu[11].title.size = 40;
    menu[11].title.colour = 50;
    menu[11].id = 0;
    menu[11].flags = 0;
    menu[11].numOptions = 0;
    menu[11].prevMenu = 3;
    menu[11].nextMenu = 3;
    menu[11].selection = 0;
    menu[11].mode = 0;
    menu[11].updateCount = 0;
    strcpy(menu[13].title.text, "BEST TIMES");
    menu[13].title.size = 40;
    menu[13].title.colour = 50;
    menu[13].id = 0;
    menu[13].flags = 0;
    menu[13].numOptions = 0;
    menu[13].prevMenu = 3;
    menu[13].nextMenu = 3;
    menu[13].selection = 0;
    menu[13].mode = 0;
    menu[13].updateCount = 0;
    strcpy(menu[14].title.text, "BEST TIMES");
    menu[14].title.size = 40;
    menu[14].title.colour = 50;
    menu[14].id = 0;
    menu[14].flags = 0;
    menu[14].numOptions = 0;
    menu[14].prevMenu = 3;
    menu[14].nextMenu = 3;
    menu[14].selection = 0;
    menu[14].mode = 0;
    menu[14].updateCount = 0;
    strcpy(menu[15].title.text, "CONTROLLER");
    menu[15].title.size = 40;
    menu[15].title.colour = 50;
    menu[15].id = 0;
    menu[15].flags = 0;
    menu[15].numOptions = 0;
    menu[15].prevMenu = 10;
    menu[15].nextMenu = 10;
    menu[15].selection = 0;
    menu[15].mode = 0;
    menu[15].updateCount = 0;
}
#pragma function(strcpy)
#else
/* Sets the menu labels and per-screen defaults used by MenuControl. */
void InitMenus(void)
{
    MenuEntry *menus = menu;

    strcpy(menus[0].options[0].text[0].text, "RACE");
    menus[0].options[0].text[0].size = 8;
    menus[0].options[0].text[0].colour = 0;
    strcpy(menus[0].options[0].text[1].text, "TYPE");
    menus[0].options[0].x = 63;
    menus[0].options[0].y = 87;
    menus[0].options[0].text[1].size = 8;
    menus[0].options[0].numText = 2;
    menus[0].options[0].font = 9;
    menus[0].options[0].text[1].colour = 0;
    menus[0].options[0].nextMenu = 1;
    menus[0].options[0].selection = 0;
    menus[0].options[0].enabled = 1;
    menus[0].options[0].flags = 0;
    strcpy(menus[0].options[2].text[0].text, "CLASS");
    menus[0].options[2].text[0].size = 8;
    menus[0].options[2].text[0].colour = 0;
    strcpy(menus[0].options[2].text[1].text, "AND");
    menus[0].options[2].text[1].size = 8;
    menus[0].options[2].text[1].colour = 0;
    strcpy(menus[0].options[2].text[2].text, "TRACK");
    menus[0].options[2].x = 255;
    menus[0].options[2].y = 87;
    menus[0].options[2].numText = 3;
    menus[0].options[2].font = 9;
    menus[0].options[2].nextMenu = 2;
    menus[0].options[2].text[2].size = 8;
    menus[0].options[2].text[2].colour = 0;
    menus[0].options[2].selection = 0;
    menus[0].options[2].enabled = 1;
    menus[0].options[2].flags = 0;
    strcpy(menus[0].options[1].text[0].text, "TEAM");
    menus[0].options[1].x = 161;
    menus[0].options[1].y = 87;
    menus[0].options[1].font = 9;
    menus[0].options[1].nextMenu = 4;
    menus[0].options[1].text[0].size = 8;
    menus[0].options[1].text[0].colour = 0;
    menus[0].options[1].numText = 1;
    menus[0].options[1].selection = 0;
    menus[0].options[1].enabled = 1;
    menus[0].options[1].flags = 0;
    strcpy(menus[0].options[4].text[0].text, "OPTIONS");
    menus[0].options[4].x = 162;
    menus[0].options[4].y = 203;
    menus[0].options[4].font = 9;
    menus[0].options[4].nextMenu = 3;
    menus[0].options[4].text[0].size = 8;
    menus[0].options[4].text[0].colour = 0;
    menus[0].options[4].numText = 1;
    menus[0].options[4].selection = 0;
    menus[0].options[4].enabled = 1;
    menus[0].options[4].flags = 0;
    strcpy(menus[0].options[3].text[0].text, "START");
    menus[0].options[3].x = 162;
    menus[0].options[3].y = 185;
    menus[0].options[3].font = 9;
    menus[0].options[3].text[0].size = 8;
    menus[0].options[3].text[0].colour = 0;
    menus[0].options[3].numText = 1;
    menus[0].options[3].nextMenu = 1;
    menus[0].options[3].selection = 0;
    menus[0].options[3].enabled = 1;
    menus[0].options[3].flags = 0;
    strcpy(menus[0].title.text, "WIPEOUT");
    menus[0].title.size = 40;
    menus[0].title.colour = 50;
    menus[0].numOptions = 5;
    menus[0].prevMenu = 999;
    menus[0].id = 0;
    menus[0].flags = 1;
    menus[0].selection = 0;
    menus[0].mode = 0;
    menus[0].updateCount = 0;
    strcpy(menus[2].options[0].text[0].text, "VECTOR");
    menus[2].options[0].x = 30;
    menus[2].options[0].y = 90;
    menus[2].options[0].font = 9;
    menus[2].options[0].text[0].size = 8;
    menus[2].options[0].text[0].colour = 0;
    menus[2].options[0].numText = 1;
    menus[2].options[0].nextMenu = 0;
    menus[2].options[0].selection = 0;
    menus[2].options[0].enabled = 1;
    menus[2].options[0].flags = 0;
    strcpy(menus[2].options[1].text[0].text, "VENOM");
    menus[2].options[1].x = 128;
    menus[2].options[1].y = 64;
    menus[2].options[1].font = 9;
    menus[2].options[1].text[0].size = 8;
    menus[2].options[1].text[0].colour = 0;
    menus[2].options[1].numText = 1;
    menus[2].options[1].nextMenu = 0;
    menus[2].options[1].selection = 0;
    menus[2].options[1].enabled = 1;
    menus[2].options[1].flags = 0;
    strcpy(menus[2].options[2].text[0].text, "RAPIER");
    menus[2].options[2].x = 205;
    menus[2].options[2].y = 64;
    menus[2].options[2].font = 9;
    menus[2].options[2].text[0].size = 8;
    menus[2].options[2].text[0].colour = 0;
    menus[2].options[2].numText = 1;
    menus[2].options[2].nextMenu = 0;
    menus[2].options[2].selection = 0;
    menus[2].options[2].enabled = 1;
    menus[2].options[2].flags = 0;
    strcpy(menus[2].options[3].text[0].text, "PHANTOM");
    menus[2].options[3].text[0].size = 8;
    menus[2].options[3].text[0].colour = 0;
    menus[2].options[3].x = 281;
    menus[2].options[3].y = 64;
    menus[2].options[3].font = 9;
    menus[2].options[3].numText = 1;
    menus[2].options[3].nextMenu = 0;
    menus[2].options[3].selection = 0;
    menus[2].options[3].enabled = 0;
    menus[2].options[3].flags = 0;
    strcpy(menus[2].title.text, "RACING CLASS");
    menus[2].title.size = 40;
    menus[2].title.colour = 50;
    menus[2].numOptions = 4;
    menus[2].id = 0;
    menus[2].flags = 0;
    menus[2].prevMenu = 0;
    menus[2].selection = 0;
    menus[2].mode = 0;
    menus[2].updateCount = 0;
    strcpy(menus[1].options[0].text[0].text, "ARCADE");
    menus[1].options[0].x = 30;
    menus[1].options[0].y = 90;
    menus[1].options[0].font = 9;
    menus[1].options[0].text[0].size = 8;
    menus[1].options[0].text[0].colour = 0;
    menus[1].options[0].numText = 1;
    menus[1].options[0].nextMenu = 0;
    menus[1].options[0].selection = 0;
    menus[1].options[0].enabled = 1;
    menus[1].options[0].flags = 0;
    strcpy(menus[1].options[1].text[0].text, "TIME TRIAL");
    menus[1].options[1].x = 128;
    menus[1].options[1].y = 64;
    menus[1].options[1].font = 9;
    menus[1].options[1].text[0].size = 8;
    menus[1].options[1].text[0].colour = 0;
    menus[1].options[1].numText = 1;
    menus[1].options[1].nextMenu = 0;
    menus[1].options[1].selection = 0;
    menus[1].options[1].enabled = 1;
    menus[1].options[1].flags = 0;
    strcpy(menus[1].options[2].text[0].text, "CHALLENGE I");
    menus[1].options[2].x = 167;
    menus[1].options[2].y = 144;
    menus[1].options[2].font = 9;
    menus[1].options[2].text[0].size = 8;
    menus[1].options[2].text[0].colour = 0;
    menus[1].options[2].numText = 1;
    menus[1].options[2].nextMenu = 0;
    menus[1].options[2].selection = 0;
    menus[1].options[2].enabled = 1;
    menus[1].options[2].flags = 0;
    strcpy(menus[1].title.text, "RACE TYPE");
    menus[1].title.size = 40;
    menus[1].title.colour = 50;
    menus[1].numOptions = 2;
    menus[1].id = 0;
    menus[1].flags = 0;
    menus[1].prevMenu = 0;
    menus[1].selection = 0;
    menus[1].mode = 0;
    menus[1].updateCount = 0;
    strcpy(menus[3].options[0].text[0].text, "AUDIO CONFIG");
    menus[3].options[0].x = 30;
    menus[3].options[0].y = 90;
    menus[3].options[0].font = 9;
    menus[3].options[0].nextMenu = 9;
    menus[3].options[0].text[0].size = 8;
    menus[3].options[0].text[0].colour = 0;
    menus[3].options[0].numText = 1;
    menus[3].options[0].selection = 0;
    menus[3].options[0].enabled = 1;
    menus[3].options[0].flags = 0;
    strcpy(menus[3].options[1].text[0].text, "CONTROLLER SETUP");
    menus[3].options[1].x = 128;
    menus[3].options[1].y = 64;
    menus[3].options[1].font = 9;
    menus[3].options[1].nextMenu = 10;
    menus[3].options[1].text[0].size = 8;
    menus[3].options[1].text[0].colour = 0;
    menus[3].options[1].numText = 1;
    menus[3].options[1].selection = 0;
    menus[3].options[1].enabled = 1;
    menus[3].options[1].flags = 0;
    strcpy(menus[3].options[2].text[0].text, "PREFERENCES");
    menus[3].options[2].x = 205;
    menus[3].options[2].y = 64;
    menus[3].options[2].font = 9;
    menus[3].options[2].nextMenu = 11;
    menus[3].options[2].text[0].size = 8;
    menus[3].options[2].text[0].colour = 0;
    menus[3].options[2].numText = 1;
    menus[3].options[2].selection = 0;
    menus[3].options[2].enabled = 1;
    menus[3].options[2].flags = 0;
    strcpy(menus[3].options[3].text[0].text, "LOAD AND SAVE");
    menus[3].options[3].x = 281;
    menus[3].options[3].y = 64;
    menus[3].options[3].font = 9;
    menus[3].options[3].nextMenu = 12;
    menus[3].options[3].text[0].size = 8;
    menus[3].options[3].text[0].colour = 0;
    menus[3].options[3].numText = 1;
    menus[3].options[3].selection = 0;
    menus[3].options[3].enabled = 1;
    menus[3].options[3].flags = 0;
    strcpy(menus[3].options[4].text[0].text, "BEST ARCADE TIMES");
    menus[3].options[4].x = 129;
    menus[3].options[4].y = 144;
    menus[3].options[4].font = 9;
    menus[3].options[4].nextMenu = 13;
    menus[3].options[4].text[0].size = 8;
    menus[3].options[4].text[0].colour = 0;
    menus[3].options[4].numText = 1;
    menus[3].options[4].selection = 0;
    menus[3].options[4].enabled = 1;
    menus[3].options[4].flags = 0;
    strcpy(menus[3].options[5].text[0].text, "BEST TIME TRIAL TIMES");
    menus[3].options[5].x = 129;
    menus[3].options[5].y = 144;
    menus[3].options[5].font = 9;
    menus[3].options[5].nextMenu = 14;
    menus[3].options[5].text[0].size = 8;
    menus[3].options[5].text[0].colour = 0;
    menus[3].options[5].numText = 1;
    menus[3].options[5].selection = 0;
    menus[3].options[5].enabled = 1;
    menus[3].options[5].flags = 0;
    strcpy(menus[3].title.text, "OPTIONS MENU");
    menus[3].title.size = 40;
    menus[3].title.colour = 50;
    menus[3].numOptions = 6;
    menus[3].id = 0;
    menus[3].flags = 0;
    menus[3].prevMenu = 0;
    menus[3].selection = 0;
    menus[3].mode = 0;
    menus[3].updateCount = 0;
    strcpy(menus[5].title.text, "VECTOR CLASS");
    menus[5].title.size = 40;
    menus[5].title.colour = 50;
    menus[5].prevMenu = 2;
    menus[5].id = 0;
    menus[5].flags = 0;
    menus[5].numOptions = 0;
    menus[5].nextMenu = 0;
    menus[5].selection = 0;
    menus[5].mode = 0;
    menus[5].updateCount = 0;
    strcpy(menus[6].title.text, "VENOM CLASS");
    menus[6].title.size = 40;
    menus[6].title.colour = 50;
    menus[6].prevMenu = 2;
    menus[6].id = 0;
    menus[6].flags = 0;
    menus[6].numOptions = 0;
    menus[6].nextMenu = 0;
    menus[6].selection = 0;
    menus[6].mode = 0;
    menus[6].updateCount = 0;
    strcpy(menus[7].title.text, "RAPIER CLASS");
    menus[7].title.size = 40;
    menus[7].title.colour = 50;
    menus[7].prevMenu = 2;
    menus[7].id = 0;
    menus[7].flags = 0;
    menus[7].numOptions = 0;
    menus[7].nextMenu = 0;
    menus[7].selection = 0;
    menus[7].mode = 0;
    menus[7].updateCount = 0;
    strcpy(menus[8].title.text, "PHANTOM CLASS");
    menus[8].title.size = 40;
    menus[8].title.colour = 50;
    menus[8].prevMenu = 2;
    menus[8].id = 0;
    menus[8].flags = 0;
    menus[8].numOptions = 0;
    menus[8].nextMenu = 0;
    menus[8].selection = 0;
    menus[8].mode = 0;
    menus[8].updateCount = 0;
    strcpy(menus[4].title.text, "TEAM MENU");
    menus[4].title.size = 40;
    menus[4].title.colour = 50;
    menus[4].id = 0;
    menus[4].flags = 0;
    menus[4].numOptions = 0;
    menus[4].prevMenu = 0;
    menus[4].nextMenu = 0;
    menus[4].selection = 0;
    menus[4].mode = 0;
    menus[4].updateCount = 0;
    strcpy(menus[9].title.text, "AUDIO CONFIG");
    menus[9].title.size = 40;
    menus[9].title.colour = 50;
    menus[9].prevMenu = 3;
    menus[9].nextMenu = 3;
    menus[9].id = 0;
    menus[9].flags = 0;
    menus[9].numOptions = 0;
    menus[9].selection = 0;
    menus[9].mode = 0;
    menus[9].updateCount = 0;
    strcpy(menus[10].title.text, "CONTROLLER");
    menus[10].title.size = 40;
    menus[10].title.colour = 50;
    menus[10].id = 0;
    menus[10].flags = 0;
    menus[10].prevMenu = 3;
    menus[10].nextMenu = 3;
    menus[10].numOptions = 0;
    menus[10].selection = 0;
    menus[10].mode = 0;
    menus[10].updateCount = 0;
    strcpy(menus[12].title.text, "LOAD AND SAVE");
    menus[12].title.size = 40;
    menus[12].title.colour = 50;
    menus[12].prevMenu = 3;
    menus[12].nextMenu = 3;
    menus[12].id = 0;
    menus[12].flags = 0;
    menus[12].numOptions = 0;
    menus[12].selection = 0;
    menus[12].mode = 0;
    menus[12].updateCount = 0;
    strcpy(menus[11].title.text, "GAME CONFIG");
    menus[11].title.size = 40;
    menus[11].title.colour = 50;
    menus[11].prevMenu = 3;
    menus[11].nextMenu = 3;
    menus[11].id = 0;
    menus[11].flags = 0;
    menus[11].numOptions = 0;
    menus[11].selection = 0;
    menus[11].mode = 0;
    menus[11].updateCount = 0;
    strcpy(menus[13].title.text, "BEST TIMES");
    menus[13].title.size = 40;
    menus[13].title.colour = 50;
    menus[13].prevMenu = 3;
    menus[13].id = 0;
    menus[13].flags = 0;
    menus[13].numOptions = 0;
    menus[13].nextMenu = 3;
    menus[13].selection = 0;
    menus[13].mode = 0;
    menus[13].updateCount = 0;
    strcpy(menus[14].title.text, "BEST TIMES");
    menus[14].title.size = 40;
    menus[14].title.colour = 50;
    menus[14].prevMenu = 3;
    menus[14].nextMenu = 3;
    menus[14].id = 0;
    menus[14].flags = 0;
    menus[14].numOptions = 0;
    menus[14].selection = 0;
    menus[14].mode = 0;
    menus[14].updateCount = 0;
    strcpy(menus[15].title.text, "CONTROLLER");
    menus[15].title.size = 40;
    menus[15].title.colour = 50;
    menus[15].prevMenu = 10;
    menus[15].nextMenu = 10;
    menus[15].id = 0;
    menus[15].flags = 0;
    menus[15].numOptions = 0;
    menus[15].selection = 0;
    menus[15].mode = 0;
    menus[15].updateCount = 0;
}
#endif

/* Clears the 8 x 2 background polygons to dark blue (rgb 0, 0, 46). */
void InitMenuBackdropPolys(POLY_F4 (*polys)[2])
{
    short i, j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 2; j++) {
            SetPolyF4(&polys[i][j]);
            polys[i][j].r0 = 0;
            polys[i][j].g0 = 0;
            polys[i][j].b0 = 0x2E;
        }
    }
}

#include "font.h"
extern GlyphWidth glyphWidth[];
extern char g_keyCharTable[];


#ifdef NETPLAY
/* Draws an option's inset backdrop in the network build: a textured texQuad for the first three options
   and the name-entry screen, a blocks for the lower ones. */
void DrawMenuOptionBackdrop(POLY_F4 (*polys)[2], short x, short y, short width,
                            short height, short option, short selected)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    POLY_FT4 texQuad;
    BLK_FILL *blocks[2];
    u_char g;
    u_char r;
    u_char b;

    if (option < 3 || nameDrawFirst != 0) {
        if (option == selected)
            SetTextQuad(&texQuad, 0, 0, 130, 56, 1, 1, 32, 0);
        else
            SetTextQuad(&texQuad, 0, 0, 130, 62, 1, 1, 32, 0);
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
        texQuad.x0 = x;
        texQuad.y0 = y;
        texQuad.x1 = x + width;
        texQuad.y1 = y;
        texQuad.x2 = x;
        texQuad.y2 = y + height;
        texQuad.x3 = x + width;
        texQuad.y3 = y + height;
        DrawScaledTexturedQuad(0.975f, &texQuad, 0);
    } else {
        if (rgbTextures == 0) {
            if (option == selected)
                r = 75;
            else
                r = 96;
            g = 0;
            b = 0;
        } else {
            r = polys[option][CurrentScreen].r0;
            g = polys[option][CurrentScreen].g0;
            b = polys[option][CurrentScreen].b0;
        }
        blocks[0] = BlockFill(x, y, width, height, r, g, b);
        blocks[1] = BlockFill(x, y, width, height, r, g, b);
        AddFill(0, (Tile *)blocks[CurrentScreen]);
    }
}
#endif

#ifdef EQUIV
#if !defined(NETPLAY) && !defined(POWERVR)
/* Draws an option's inset backdrop, using a fill for lower options and a textured quad for the first three.
   Built without /Oa: with it, MSVC overlays the two fills on the quad's stack slot (a 0x30-byte frame). */
#pragma optimize("a", off)
void DrawMenuOptionBackdrop(POLY_F4 (*polys)[2], short x, short y, short width,
                            short height, short option, short selected)
{
    u_char red, green, blue;
    POLY_FT4 quad;
    BLK_FILL *fill[2];

    if (option >= 3 && nameDrawFirst == 0) {
        if (rgbTextures == 0) {
            option -= selected;
            red = option ? 96 : 75;
            green = 0;
            blue = 0;
        } else {
            red = polys[option][CurrentScreen].r0;
            green = polys[option][CurrentScreen].g0;
            blue = polys[option][CurrentScreen].b0;
        }
        fill[0] = BlockFill(x, y, width, height, red, green, blue);
        /* Both fills use the same coordinates in this routine. */
        fill[1] = BlockFill(x, y, width, height, red, green, blue);
        AddFill(0, (Tile *)fill[CurrentScreen]);
    } else {
        if (option == selected)
            SetTextQuad(&quad, 0, 0, 130, 56, 1, 1, 32, 0);
        else
            SetTextQuad(&quad, 0, 0, 130, 62, 1, 1, 32, 0);
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
        quad.x0 = x;
        quad.y0 = y;
        quad.x1 = x + width;
        quad.y1 = y;
        quad.x2 = x;
        quad.y2 = y + height;
        quad.x3 = x + width;
        quad.y3 = y + height;
#ifdef POWERVR
        DrawScaledTexturedQuad(0.975f, &quad, 0);
#else
        DrawScaledTexturedQuad(0.96f, &quad, 0);
#endif
    }
}
#pragma optimize("a", on)
#endif
#endif

#ifdef EQUIV
#ifdef POWERVR
/* PowerVR keeps this equivalent form; the register allocation differs from its image. */
#pragma optimize("a", off)
void DrawMenuOptionBackdrop(POLY_F4 (*polys)[2], short x, short y, short width,
                            short height, short option, short selected)
{
    u_char red, green, blue;
    POLY_FT4 quad;
    BLK_FILL *fill[2];

    if (option >= 3 && nameDrawFirst == 0) {
        if (rgbTextures == 0) {
            if (option == selected)
                red = 75;
            else
                red = 96;
            green = 0;
            blue = 0;
        } else {
            red = polys[option][CurrentScreen].r0;
            green = polys[option][CurrentScreen].g0;
            blue = polys[option][CurrentScreen].b0;
        }
        fill[0] = BlockFill(x, y, width, height, red, green, blue);
        fill[1] = BlockFill(x, y, width, height, red, green, blue);
        AddFill(0, (Tile *)fill[CurrentScreen]);
    } else {
        if (option == selected)
            SetTextQuad(&quad, 0, 0, 130, 56, 1, 1, 32, 0);
        else
            SetTextQuad(&quad, 0, 0, 130, 62, 1, 1, 32, 0);
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
        quad.x0 = x;
        quad.y0 = y;
        quad.x1 = x + width;
        quad.y1 = y;
        quad.x2 = x;
        quad.y2 = y + height;
        quad.x3 = x + width;
        quad.y3 = y + height;
        DrawScaledTexturedQuad(0.975f, &quad, 0);
    }
}
#pragma optimize("a", on)
#endif
#endif

#ifdef NETPLAY
/* Name entry for a new record in the network build, which reads the typed character from the
   keyboard handler. */
void EnterName(ConfigData *gameData, HiScoreData *data)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short fontSize;
    int n;
    short cursorX;
    short top;
    short x0;
    char c;
    char cursor[2];

    fontSize = 8;
    x0 = 0xB4;
    top = 0x82;

    if (data->enteringName != 0) {
        if (g_keyboardCode == 0xE /* DIK_BACK */ && data->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameCursor--;
            g_keyboardCode = -1;
        }
        c = GetKeyboardChar();
        if (c != 0 && c != '-' && data->nameCursor < 3) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameEntered[data->nameCursor] = c;
            data->nameCursor++;
            data->charactersTyped++;
        }
        if (GetMenuKey(2) && data->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameCursor--;
        }
        if (GetMenuKey(3) && data->nameCursor < 2) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameCursor++;
        }
        if (GetMenuKey(5)) {
            data->enteringName = 0;
        }
    }
    if (nameDrawFirst != 0) {
        AddText(data->nameEntered, x0, top, fontSize, 0);
    }
    if (data->enteringName != 0) {

        cursorX = x0;
        for (n = 0; data->nameCursor > n; n++) {
            c = glyphIndex.character[data->nameEntered[n]];
            if (c != 0x32) {
                cursorX += glyphWidth[c].width >> 1;
            } else {
                cursorX += 8;
            }
        }
        cursor[0] = data->nameEntered[data->nameCursor];
        cursor[1] = 0;
        AddText(cursor, cursorX, top, fontSize, 1);
    }
    if (nameDrawFirst == 0) {
        AddText(data->nameEntered, x0, top, fontSize, 0);
    }
}
#else
/* Name entry for a new record. The draw flag is reloaded after each AddText call.
   Compiled without /Oa so the cursor is re-read after PlayNote calls. */
#pragma optimize("a", off)
void EnterName(ConfigData *gameData, HiScoreData *data)
{
    signed char glyph;
    int drawFirst;
    short key;
    char c;
    char dispChar[2];
    short x;
    int numChars;

    if (data->enteringName != 0) {
        key = g_keyboardCode;
        if (key == 0xE /* DIK_BACK */ && data->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameCursor--;
            key = -1;
        }
        g_keyboardCode = key;
        if (key != -1) {
            c = g_keyCharTable[key];
            if (key != 0x39 /* DIK_SPACE */ && !(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'Z')) {
                c = 0;
            }
            g_keyboardCode = key;
            if (key == 0xC /* DIK_MINUS */)
                c = '-';
        } else {
            c = 0;
        }
        g_keyboardCode = -1;
        if (c != 0 && c != '-' && data->nameCursor < 3) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            numChars = data->nameCursor;
            data->nameEntered[numChars] = c;
            data->nameCursor++;
            data->charactersTyped++;
        }
        if (GetMenuKey(2) && data->nameCursor > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameCursor--;
        }
        if (GetMenuKey(3) && data->nameCursor < 2) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            data->nameCursor++;
        }
        if (GetMenuKey(5)) {
            data->enteringName = 0;
        }
    }
    drawFirst = nameDrawFirst;
    if (drawFirst != 0) {
        AddText(data->nameEntered, 0xB4, 0x82, 8, 0);
        drawFirst = nameDrawFirst;
    }
    if (data->enteringName != 0) {
        int i;
        x = 0xB4;
        for (i = 0; i < data->nameCursor; i++) {
            glyph = glyphIndex.character[data->nameEntered[i]];
            if (glyph != 0x32) {
                x += glyphWidth[glyph].width >> 1;
            } else {
                x += 8;
            }
        }
        dispChar[1] = 0;
        dispChar[0] = data->nameEntered[data->nameCursor];
        AddText(dispChar, x, 0x82, 8, 1);
        drawFirst = nameDrawFirst;
    }
    if (drawFirst == 0) {
        AddText(data->nameEntered, 0xB4, 0x82, 8, 0);
    }
}
#pragma optimize("a", on)
#endif



/* Track name for a track type; with draw set it prints the name at x, y instead. */
char *GetTrackName(short type, short x, short y, short draw)
{
    switch (type) {
    case 1:
        if (draw) AddText("TALON'S REACH", x, y, 8, 0);
        else return "TALON'S REACH";
        break;
    case 2:
        if (draw) AddText("GARE D'EUROPA", x, y, 8, 0);
        else return "GARE D'EUROPA";
        break;
    case 6:
        if (draw) AddText("VOSTOK ISLAND", x, y, 8, 0);
        else return "VOSTOK ISLAND";
        break;
    case 7:
        if (draw) AddText("SPILSKINANKE", x, y, 8, 0);
        else return "SPILSKINANKE";
        break;
    case 8:
        if (draw) AddText("SAGARMATHA", x, y, 8, 0);
        else return "SAGARMATHA";
        break;
    case 13:
        if (draw) AddText("VALPARAISO", x, y, 8, 0);
        else return "VALPARAISO";
        break;
    case 17:
        if (draw) AddText("ODESSA KEYS", x, y, 8, 0);
        else return "ODESSA KEYS";
        break;
    case 20:
        if (draw) AddText("PHENITIA PARK", x, y, 8, 0);
        else return "PHENITIA PARK";
        break;
    }
}


extern void *preCalcObject;
extern SVECTOR VertexNormal[];
#ifdef NETPLAY
extern short shipSpin;
#else
extern int shipSpin;
#endif

/* Loads a 16-bit TIM into video memory at its own position, or if doubled is set at both
   (0, 0) and (0, 256). */
#ifdef NETPLAY
/* Local names chosen to reproduce the exe's frame layout. */
LoadedImage *LoadMenuImage(char *name, char doubled)
{
    LoadedImage *image;
    long length;
    char *buffer;
    char *fileStart;
    RECT area;
    short *coords;

    /* The original reserves 0x34 bytes but initializes only this record prefix. */
    image = DAlloc(reserveHeap, 0x34);
    length = FileLength(name);
    if (length <= 0) {
        wsprintf(errorString, "LoadTexture: File not found\n");
        Error(errorString, 0);
        return 0;
    }
    buffer = DAlloc(reserveHeap, length + 0x20);
    if (buffer == 0) {
        wsprintf(errorString, "LoadTexture(): Failed to allocate memory for file \n");
        Error(errorString, 2);
    }
    if (LoadFile(name, buffer) != length) {
        wsprintf(errorString, "LoadTexture(): Failed to load file \n");
        Error(errorString, 2);
    }
    fileStart = buffer;
    buffer += 12; /* Skip the TIM id, flags and pixel-block length. */
    coords = (short *)buffer;
    image->x = *coords++;
    image->y = *coords++;
    image->width = *coords++;
    image->height = *coords++;
    buffer += 8; /* The four 16-bit image coordinates precede the (u_long *)buffer. */
    if (doubled == 0) {
        area.x = image->x;
        area.y = image->y;
        area.w = image->width;
        area.h = image->height;
    } else {
        area.w = image->width;
        area.h = image->height;
        area.x = 0;
        area.y = 0;
        LoadImageStub(&area, (u_long *)buffer);
        DrawSync(0);
        area.y = 0x100;
    }
    LoadImageStub(&area, (u_long *)buffer);
    DrawSync(0);
    DFree(reserveHeap, fileStart);
    return image;
}
#else
LoadedImage *LoadMenuImage(char *name, char doubled)
{
    LoadedImage *image;
    long length;
    char *buffer;
    char *fileStart;
    RECT rect;
    short *header;
    u_long *pixels;

    /* The original reserves 0x34 bytes but initializes only this record prefix. */
    image = DAlloc(reserveHeap, 0x34);
    length = FileLength(name);
    if (length <= 0) {
        wsprintf(errorString, "LoadTexture: File not found\n");
        Error(errorString, 0);
        return 0;
    }
    buffer = DAlloc(reserveHeap, length + 0x20);
    if (buffer == 0) {
        wsprintf(errorString, "LoadTexture(): Failed to allocate memory for file \n");
        Error(errorString, 2);
    }
    if (LoadFile(name, buffer) != length) {
        wsprintf(errorString, "LoadTexture(): Failed to load file \n");
        Error(errorString, 2);
    }
    fileStart = buffer;
    buffer += 12; /* Skip the TIM id, flags and pixel-block length. */
    header = (short *)buffer;
    image->x = *header++;
    image->y = *header++;
    image->width = *header++;
    image->height = *header;
    buffer += 8; /* The four 16-bit image coordinates precede the pixels. */
    pixels = (u_long *)buffer;
    if (doubled == 0) {
        rect.x = image->x;
        rect.y = image->y;
        rect.w = image->width;
        rect.h = image->height;
    } else {
        rect.w = image->width;
        rect.h = image->height;
        rect.x = 0;
        rect.y = 0;
        LoadImageStub(&rect, pixels);
        DrawSync(0);
        rect.y = 0x100;
    }
    LoadImageStub(&rect, pixels);
    DrawSync(0);
    DFree(reserveHeap, fileStart);
    return image;
}
#endif

extern HeapBlock *mainheapcopy;

#pragma auto_inline(on)
#ifdef NETPLAY
#pragma intrinsic(strcpy, strcat)
#endif
void LoadModel(Object **models, char *name)
{
    char path[256];
#ifdef NETPLAY
    short textureStart = 0;
#endif
#ifdef NETPLAY
    Object *model;
    int idx = 0;
#else
    Object *object;
    int i = 0;
#endif

    heap = reserveHeap;
    strcpy(path, wobbleDir2);
    strcat(path, name);
    strcat(path, ".prm");
#ifdef NETPLAY
    /* Local names chosen to reproduce the exe's frame layout. */
    model = *models = LoadPrm(path, textureStart);
    while (model) {
        model->skeleton = NewSkeleton();
        models[idx++] = model;
        model = model->next;
    }
#else
    object = *models = LoadPrm(path, 0);
    while (object) {
        object->skeleton = NewSkeleton();
        models[i++] = object;
        object = object->next;
    }
#endif
    heap = mainheapcopy;
}
#ifdef NETPLAY
#pragma function(strcpy, strcat)
#endif
#pragma auto_inline(off)

extern short TextureTableCount;

/* Like LoadModel, but the model comes with a .cmp texture sequence of the same name. */
#ifdef NETPLAY
/* Name and first-texture register allocation still differs. */
#pragma auto_inline(on)
#pragma intrinsic(strcpy, strcat)
/* Local names chosen to reproduce the exe's frame layout. */
void LoadMenuModel(Object **models, char *name)
{
    TIMlist *sequence;
    char path[256];
    short textureStart = 0;
    Object *model;
    int idx = 0;

    heap = reserveHeap;
    strcpy(path, wobbleDir2);
    strcat(path, name);
    strcat(path, ".cmp");
    textureStart = TextureTableCount;
    sequence = LoadCompressedTextureSequence(path);
    LoadCmpFiles(sequence);
    strcpy(path, wobbleDir2);
    strcat(path, name);
    strcat(path, ".prm");
    model = *models = LoadPrm(path, textureStart);
    while (model) {
        model->skeleton = NewSkeleton();
        models[idx++] = model;
        model = model->next;
    }
    heap = mainheapcopy;
}
#pragma function(strcpy, strcat)
#pragma auto_inline(off)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Name and first-texture register allocation still differs. */
#pragma auto_inline(on)
void LoadMenuModel(Object **models, char *name)
{
    char path[256];
    short firstTexture;
    Object *object;
    int i = 0;

    heap = reserveHeap;
    strcpy(path, wobbleDir2);
    strcat(path, name);
    strcat(path, ".cmp");
    firstTexture = TextureTableCount;
    LoadCmpFiles(LoadCompressedTextureSequence(path));
    strcpy(path, wobbleDir2);
    strcat(path, name);
    strcat(path, ".prm");
    object = *models = LoadPrm(path, firstTexture);
    while (object) {
        object->skeleton = NewSkeleton();
        models[i++] = object;
        object = object->next;
    }
    heap = mainheapcopy;
}
#pragma auto_inline(off)
#endif
#endif

void DrawEnvMap(Object *obj, SVECTOR *normals, int x, int y, float scale);

/* Draws one ship of the team and class screens. The team ship is spun and lit with the environment
   map; the ship at scale -1 is the environment-mapped backdrop. Proven equivalent rather than
   byte-exact: the exe keeps dir and scale in bp and bx and reloads x from the stack. */
#ifdef NETPLAY
void DrawTeamShips(int x, int y, int z, short dir, short scale, Object *obj)
{
    SetSkeletonPosition(obj->skeleton, x, y, z);
    SetSkeletonDirection(obj->skeleton, scale, dir, 0);
    if (obj == teamShip) {
        if (preCalcObject != obj) {
            PreCalcNormals(obj, VertexNormal);
            preCalcObject = obj;
        }
        SetSkeletonPosition(obj->skeleton, x, y, 0x104);
        SetSkeletonDirection(obj->skeleton, scale - shipSpin, dir, 0);
        if (gameData->shipClass == 0) {
            DrawEnvMap(obj, VertexNormal, 0x70, 0x20, 0.75f);
        } else {
            DrawEnvMap(obj, VertexNormal, 0x60, 0x20, 0.875f);
        }
        return;
    }
    if (scale == -1) {
        if (preCalcObject != obj) {
            PreCalcNormals(obj, VertexNormal);
            preCalcObject = obj;
        }
        DrawEnvMap(obj, VertexNormal, -0x6D, -0xF, 1.125f);
        return;
    }
    AddRenderObject(obj, 0, 0);
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
void DrawTeamShips(int x, int y, int z, short dir, short scale, Object *obj)
{
    SetSkeletonPosition(obj->skeleton, x, y, z);
    SetSkeletonDirection(obj->skeleton, scale, dir, 0);
    if (obj == teamShip) {
        if (preCalcObject != obj) {
            PreCalcNormals(obj, VertexNormal);
            preCalcObject = obj;
        }
        SetSkeletonPosition(obj->skeleton, x, y, 0x104);
        SetSkeletonDirection(obj->skeleton, scale - shipSpin, dir, 0);
        if (gameData->shipClass == 0) {
            DrawEnvMap(obj, VertexNormal, 0x70, 0x20, 0.75f);
        } else {
            DrawEnvMap(obj, VertexNormal, 0x60, 0x20, 0.875f);
        }
        return;
    }
    if (scale == -1) {
        if (preCalcObject != obj) {
            PreCalcNormals(obj, VertexNormal);
            preCalcObject = obj;
        }
        DrawEnvMap(obj, VertexNormal, -0x6D, -0xF, 1.125f);
        return;
    }
    AddRenderObject(obj, 0, 0);
}
#endif
#endif


#ifdef NETPLAY
/* Displays the initialization banner once in each screen buffer; the network build only pumps messages. */
void DrawMenuInitBanner(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short frame;
    BLK_FILL *fill[2];

    fill[0] = BlockFill(0, 0, 320, 240, 0, 0, 46);
    fill[1] = BlockFill(0, 256, 320, 240, 0, 0, 46);
    for (frame = 0; frame < 2; frame++) {
        quadIndex = CurrentScreen * 300;
        FlushMessages();
        AddFill(0, (Tile *)fill[CurrentScreen]);
        RaceFrameStart();
    }
    OutputDebugStringA("init ok\n");
}
#else
/* Displays the initialization banner once in each screen buffer. */
void DrawMenuInitBanner(void)
{
    BLK_FILL *fill[2];
    POLY_FT4 quad;
    short frames;

    fill[0] = BlockFill(0, 0, 320, 240, 0, 0, 46);
    fill[1] = BlockFill(0, 256, 320, 240, 0, 0, 46);
    for (frames = 2; frames != 0; frames--) {
        quadIndex = CurrentScreen * 300;
        SetTextQuad(&quad, 25, 38, 0, 0, 254, 64, 35, 0);
        DrawScaledTexturedQuad(textDepth, &quad, hudTexturePage);
        SetTextQuad(&quad, 151, 38, 0, 64, 210, 64, 35, 0);
        DrawScaledTexturedQuad(textDepth, &quad, hudTexturePage);
        AddFill(0, (Tile *)fill[CurrentScreen]);
        RaceFrameStart();
    }
    OutputDebugStringA("init ok\n");
}
#endif

extern BLK_FILL *screenFillTop;
extern BLK_FILL *screenFill1;

/* Creates the two black fills that clear the screen buffers of the menus. */
void InitMenuFills(void)
{
    screenFillTop = BlockFill(0, 0, 320, 240, 0, 0, 0);
    screenFill1 = BlockFill(0, 240, 320, 240, 0, 0, 0);
}


/* Starts a fresh set of menu options from the game data. */
#ifdef NETPLAY
#pragma intrinsic(strcpy)
#endif
void InitMenuState(PcGameData *gameData)
{
#ifdef NETPLAY
    short i;

#endif
    menuState.weapons = gameData->weaponsOn;
    menuState.internalView = gameData->internalView;
    menuState.trailDetail = gameData->trailDetail;
    menuState.skyEnabled = gameData->skyEnabled;
    menuState.viewDetail = gameData->viewDetail;
    menuState.musicVolume = gameData->musicVolume;
    menuState.effectVolume = gameData->effectVolume;
    menuState.otherMusic = gameData->otherMusicVolume;
    menuState.otherEffect = gameData->otherEffectVolume;
    menuState.music = gameData->cdTrack;
    menuState.actTrackNum = gameData->actTrackNum;
    menuState.shipClass = gameData->shipClass;
    menuState.ship = ownShip;
    menuState.trackSelection = 0;
    menuState.resetWord5E = 0;
    menuState.resetWord8A = 0;
    menuState.resetWord8C = 0;
    menuState.resetWord8E = 0;
    menuState.resetWord90 = 0;
    menuState.resetWord92 = 0;
    deviceType = savedDeviceType;
#ifdef NETPLAY
    strcpy(menuState.name, gameData->name);
#else
    strcpy(nameBuffer, gameData->name);
#endif
#ifdef NETPLAY
    for (i = 0; i < 10; i++)
        menuState.resetBytes60[i] = 0;
#else
    memset(menuState.resetBytes60, 0, sizeof(menuState.resetBytes60));
#endif
}
#ifdef NETPLAY
#pragma function(strcpy)
#endif

extern signed char raceTracks[];

/* The options read from the config file at start-up, in the order of the game data fields. */
typedef struct SavedOptions {
    short musicVolume, effectVolume, otherMusicVolume, otherEffectVolume;
    u_char sfxMode;
    u_char cdTrack;
    u_char reserved0A;
    u_char weaponsOn;
    u_char internalView;
    u_char trailDetail;
    u_char skyEnabled;
#ifdef POWERVR
    u_char fogEnabled;
#else
    u_char reserved0F;
#endif
    u_char smoothTextures;
    u_char viewDetail;
    u_char extraGraphicsOption;
    u_char texturesOn;
    char name[9];
} SavedOptions;
extern SavedOptions savedOptions;
extern HiScore savedHiScores[16][4];

/* The config file image, written as one 0x5b4-byte block at 0x918360: the display setup, the
   options, the controls and the hi-score table. savedOptions and savedHiScores are the options
   and hi-score parts of this block. */
typedef struct SavedConfig {
    int pixelFormatGroup;
    int displayBits;
    int pixelFormatIndex;
    short videoModeIndex;
    short displayDriverCount;
    short displayDriverConfigTag;
    SavedOptions options;
    int savedDeviceType;
    int joystickPresent;
    int mouseEnabled;
    int mouseSensitivity;
    short turnDivisor;
    int gameKeys[12];
    int keyBindings[8];
    int padBindings[8];
    HiScore hiScores[16][4];
} SavedConfig;
extern SavedConfig savedConfig;

#ifdef NETPLAY
#pragma intrinsic(strcpy, memcpy)
#endif
/* Sets up a new game data record: the track and ship at random, nothing unlocked, and the options
   either as the defaults or as read from the config file. */
void InitGameData(PcGameData *gameData)
{
#ifdef POWERVR
    extern signed char fogEnabled;
#endif
    int i;
#ifndef NETPLAY
    int rand;
#endif

    gameData->initialConfigValue = 0x96;
    gameData->paused = 0;
#ifdef NETPLAY
    gameData->shipNum = newRand() % 15;
    gameData->trackNum = 0;
    gameData->shipClass = 0;
    gameData->raceInfo = 0;
    gameData->complete = 0;
    gameData->highScore = 0;
#else
    rand = newRand();
    gameData->raceInfo = 0;
    gameData->complete = 0;
    gameData->shipNum = rand % 15;
    gameData->highScore = 0;
    gameData->trackNum = 0;
    gameData->shipClass = 0;
#endif
    gameData->actTrackNum = raceTracks[0];
    gameData->venomComplete = 0;
    gameData->rapierComplete = 0;
    gameData->teamCheat = 0;
    gameData->class1Unlocked = 0;
    gameData->class2Unlocked = 0;
    gameData->class1Race = 0;
    gameData->class2Race = 0;
    gameData->trackCheat = 0;
    gameData->phantomCheat = 0;
    if (noSavedConfig) {
#ifdef NETPLAY
        gameData->musicVolume = 0x4B;
        gameData->effectVolume = 0x4B;
        gameData->otherMusicVolume = 0x4B;
        gameData->otherEffectVolume = 0x4B;
        gameData->cdTrack = 1;
        gameData->weaponsOn = 1;
        gameData->internalView = 0;
        if (rgbTextures)
            gameData->trailDetail = 1;
        else
            gameData->trailDetail = 0;
        gameData->skyEnabled = 1;
        gameData->smoothTextures = 1;
        gameData->viewDetail = 0;
        gameData->extraGraphicsOption = 1;
        gameData->texturesOn = 1;
#else
        gameData->musicVolume = 0x4B;
        gameData->cdTrack = 1;
        gameData->effectVolume = 0x4B;
        gameData->weaponsOn = 1;
        gameData->otherMusicVolume = 0x4B;
        gameData->otherEffectVolume = 0x4B;
        gameData->internalView = 0;
        if (rgbTextures)
            gameData->trailDetail = 1;
        else
            gameData->trailDetail = 0;
#ifdef POWERVR
        gameData->skyEnabled = 0;
        if (fogEnabled == 0)
            gameData->skyEnabled = 1;
#else
        gameData->skyEnabled = 1;
#endif
        gameData->viewDetail = 0;
        gameData->smoothTextures = 1;
        gameData->extraGraphicsOption = 1;
        gameData->texturesOn = 1;
#endif
        strcpy(gameData->name, "AAA");
        ScoreTabInit(gameData);
    } else {
        gameData->musicVolume = savedOptions.musicVolume;
        gameData->effectVolume = savedOptions.effectVolume;
        gameData->otherMusicVolume = savedOptions.otherMusicVolume;
        gameData->otherEffectVolume = savedOptions.otherEffectVolume;
        gameData->cdTrack = savedOptions.cdTrack;
        gameData->weaponsOn = savedOptions.weaponsOn;
        gameData->internalView = savedOptions.internalView;
#ifdef POWERVR
        fogEnabled = savedOptions.fogEnabled;
#endif
        gameData->trailDetail = savedOptions.trailDetail;
        gameData->skyEnabled = savedOptions.skyEnabled;
        gameData->smoothTextures = savedOptions.smoothTextures;
        gameData->viewDetail = savedOptions.viewDetail;
        gameData->extraGraphicsOption = savedOptions.extraGraphicsOption;
        gameData->texturesOn = savedOptions.texturesOn;
        for (i = 0; i < 9; i++)
            gameData->name[i] = savedOptions.name[i];
#ifdef NETPLAY
        memcpy(hiScoreTable, savedConfig.hiScores, sizeof(hiScoreTable));
#else
        memcpy(hiScoreTable, savedHiScores, sizeof(hiScoreTable));
#endif
    }
    SfxSetVolume(gameData->effectVolume * 255 / 100);
    SfxSetCdVolume(gameData->musicVolume * 255 / 100);
}
#ifdef NETPLAY
#pragma function(strcpy, memcpy)
#endif

extern short sixTracks, sixXPlayed, eightTracks, eightXPlayed;

/* Fills in the race menu for what the player has unlocked: the challenge entry, the extra class
   and track choices and the progress flags. */
#ifdef NETPLAY
#pragma intrinsic(strcpy)
void SetupUnlocks(void)
{
    menu[2].selection = lastClassSelection;
    if (gameData->phantomChallenge) {
        menu[1].numOptions = 3;
        strcpy(menu[1].options[2].text[0].text, "CHALLENGE I");
        menu[1].options[2].text[0].size = 8;
        menu[1].options[2].text[0].colour = 0;
    } else if (gameData->piranhaChallenge) {
        menu[1].numOptions = 3;
        strcpy(menu[1].options[2].text[0].text, "CHALLENGE II");
        menu[1].options[2].text[0].size = 8;
        menu[1].options[2].text[0].colour = 0;
    }
    if (gameData->phantomUnlocked || gameData->phantomCheatSigned)
        menu[2].options[3].enabled = 1;
    if (gameData->piranhaChallenge || gameData->trackCheatSigned)
        menu[2].options[3].enabled = 1;
    sixXPlayed = 0;
    eightXPlayed = 0;
    sixTracks = 0;
    eightTracks = 0;
    if (gameData->phantomChallenge) {
        sixXPlayed = 1;
        sixTracks = 1;
    }
    if (gameData->piranhaChallenge) {
        eightXPlayed = 1;
        eightTracks = 1;
        sixXPlayed = 1;
        sixTracks = 1;
    }
}
#pragma function(strcpy)
#else
void SetupUnlocks(void)
{
    menu[2].selection = lastClassSelection;
    if (gameData->venomComplete) {
        menu[1].numOptions = 3;
        strcpy(menu[1].options[2].text[0].text, "CHALLENGE I");
        menu[1].options[2].text[0].size = 8;
        menu[1].options[2].text[0].colour = 0;
    } else if (gameData->rapierComplete) {
        menu[1].numOptions = 3;
        strcpy(menu[1].options[2].text[0].text, "CHALLENGE II");
        menu[1].options[2].text[0].size = 8;
        menu[1].options[2].text[0].colour = 0;
    }
    if (gameData->class1Unlocked || gameData->phantomCheat)
        menu[2].options[3].enabled = 1;
    if (gameData->rapierComplete || gameData->trackCheat)
        menu[2].options[3].enabled = 1;
    sixXPlayed = 0;
    eightXPlayed = 0;
    sixTracks = 0;
    eightTracks = 0;
    if (gameData->venomComplete) {
        sixXPlayed = 1;
        sixTracks = 1;
    }
    if (gameData->rapierComplete) {
        eightXPlayed = 1;
        eightTracks = 1;
        sixXPlayed = 1;
        sixTracks = 1;
    }
}
#endif

/* Takes the unlocks away again when the game goes back to the first menu. */
#ifdef NETPLAY
void ClearUnlocks(void)
{
    if (gameType == 2 && tempGameData->phantomChallenge)
        gameType = 0;
    if (gameType == 2 && tempGameData->piranhaChallenge)
        gameType = 0;
    menu[1].numOptions = 2;
    menu[1].selection = 0;
    menu[2].options[3].enabled = 0;
    menu[2].selection = 0;
    lastClassSelection = menu[2].selection;
    menu[2].mode = 0;
    tempGameData->phantomCheat = 0;
    tempGameData->trackCheat = 0;
    tempGameData->teamCheat = 0;
    tempGameData->venomComplete = 0;
    tempGameData->rapierComplete = 0;
    tempGameData->class1Unlocked = 0;
    tempGameData->class2Unlocked = 0;
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
void ClearUnlocks(void)
{
    int currentGameType;

    currentGameType = gameType;
    if (currentGameType == 2 && tempGameData->venomComplete)
        currentGameType = 0;
    gameType = currentGameType;
    if (currentGameType == 2 && tempGameData->rapierComplete)
        gameType = 0;
    menu[1].numOptions = 2;
    menu[1].selection = 0;
    tempGameData->phantomCheat = 0;
    menu[2].options[3].enabled = 0;
    tempGameData->trackCheat = 0;
    menu[2].selection = 0;
    tempGameData->teamCheat = 0;
    lastClassSelection = 0;
    tempGameData->venomComplete = 0;
    menu[2].mode = 0;
    tempGameData->rapierComplete = 0;
    tempGameData->class1Unlocked = 0;
    tempGameData->class2Unlocked = 0;
}
#endif
#endif


/* Starts a menu slide with all three offsets at zero. */
void InitSlider(void)
{
    menuSlideActive = 1;
#ifdef NETPLAY
    menuSlideFrame = 0;
    menuSlideX = 0;
#else
    menuSlideX = 0;
    menuSlideFrame = 0;
#endif
    menuSlideY = 0;
    menuSlideStep = 0;
}

/* Moves the menu's three bands and emits sparks while their edges separate. */
#ifdef NETPLAY
void SlideScreen(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    short stride = 12;
    int frames;
    short rightX, leftX, gap;
    short i;

    frames = FR60;
    while (frames != 0) {
        if (menuSlideFrame >= 0 && menuSlideFrame < 30) {
            menuSlideX += stride;
            menuSlideY -= stride;
            menuSlideStep += stride;
        } else {
            menuSlideX -= stride;
            menuSlideY += stride;
            menuSlideStep -= stride;
        }
        rightX = menuSlideY + 308;
        leftX = menuSlideStep + 20;
        gap = rightX - leftX;
        if (menuSlideFrame > 0 && menuSlideFrame < 15 && gap > 0) {
            for (i = 0; i < 20; i++) {
                Insert2DParticle(leftX, 82);
                Insert2DParticle(leftX, 180);
                Insert2DParticle(rightX, 82);
                Insert2DParticle(rightX, 180);
            }
        }
        menuSlideFrame++;
        if (menuSlideFrame == 60) {
            menuSlideFrame = 0;
            menuSlideActive = 0;
            PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
            frames = 1;
        }
        frames--;
    }
    for (i = 0; i < 14; i++) {
        if (i == 0 || i == 1 || i == 2) {
            menuBackPolys[i].x0 = (menuBackdropRects[i].x + menuSlideX) * 2;
            menuBackPolys[i].y0 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x1 = (menuBackdropRects[i].w + menuSlideX + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y1 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x2 = (menuBackdropRects[i].x + menuSlideX) * 2;
            menuBackPolys[i].y2 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
            menuBackPolys[i].x3 = (menuBackdropRects[i].w + menuSlideX + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y3 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
        } else if (i == 3 || i == 4 || i == 5) {
            menuBackPolys[i].x0 = (menuBackdropRects[i].x + menuSlideStep) * 2;
            menuBackPolys[i].y0 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x1 = (menuBackdropRects[i].w + menuSlideStep + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y1 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x2 = (menuBackdropRects[i].x + menuSlideStep) * 2;
            menuBackPolys[i].y2 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
            menuBackPolys[i].x3 = (menuBackdropRects[i].w + menuSlideStep + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y3 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
        } else {
            menuBackPolys[i].x0 = (menuBackdropRects[i].x + menuSlideY) * 2;
            menuBackPolys[i].y0 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x1 = (menuBackdropRects[i].w + menuSlideY + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y1 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x2 = (menuBackdropRects[i].x + menuSlideY) * 2;
            menuBackPolys[i].y2 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
            menuBackPolys[i].x3 = (menuBackdropRects[i].w + menuSlideY + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y3 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
        }
    }
}
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Proven equivalent rather than byte-exact: the timing and particle calls agree, but the
   coordinate branches and register allocation differ. */
void SlideScreen(void)
{
    int steps;
    short i;
    short left, right, gap;

    steps = FR60;
    while (steps != 0) {
        if (menuSlideFrame >= 0 && menuSlideFrame < 30) {
            menuSlideX += 12;
            menuSlideY -= 12;
            menuSlideStep += 12;
        } else {
            menuSlideX -= 12;
            menuSlideY += 12;
            menuSlideStep -= 12;
        }
        right = menuSlideY + 308;
        left = menuSlideStep + 20;
        gap = right - left;
        if (menuSlideFrame > 0 && menuSlideFrame < 15 && gap > 0) {
            for (i = 20; i != 0; i--) {
                Insert2DParticle(left, 82);
                Insert2DParticle(left, 180);
                Insert2DParticle(right, 82);
                Insert2DParticle(right, 180);
            }
        }
        menuSlideFrame++;
        if (menuSlideFrame == 60) {
            menuSlideFrame = 0;
            menuSlideActive = 0;
            PlayNote(SAMPLE_WEAPON_HIT, 255, 15, 15, 256);
            steps = 1;
        }
        steps--;
    }
    for (i = 0; i < 14; i++) {
        if (i == 0 || i == 1 || i == 2) {
            menuBackPolys[i].x0 = (menuBackdropRects[i].x + menuSlideX) * 2;
            menuBackPolys[i].y0 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x1 = (menuBackdropRects[i].w + menuSlideX + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y1 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x2 = (menuBackdropRects[i].x + menuSlideX) * 2;
            menuBackPolys[i].y2 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
            menuBackPolys[i].x3 = (menuBackdropRects[i].w + menuSlideX + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y3 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
        } else if (i == 3 || i == 4 || i == 5) {
            menuBackPolys[i].x0 = (menuBackdropRects[i].x + menuSlideStep) * 2;
            menuBackPolys[i].y0 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x1 = (menuBackdropRects[i].w + menuSlideStep + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y1 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x2 = (menuBackdropRects[i].x + menuSlideStep) * 2;
            menuBackPolys[i].y2 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
            menuBackPolys[i].x3 = (menuBackdropRects[i].w + menuSlideStep + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y3 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
        } else {
            menuBackPolys[i].x0 = (menuBackdropRects[i].x + menuSlideY) * 2;
            menuBackPolys[i].y0 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x1 = (menuBackdropRects[i].w + menuSlideY + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y1 = menuBackdropRects[i].y * 2;
            menuBackPolys[i].x2 = (menuBackdropRects[i].x + menuSlideY) * 2;
            menuBackPolys[i].y2 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
            menuBackPolys[i].x3 = (menuBackdropRects[i].w + menuSlideY + menuBackdropRects[i].x) * 2;
            menuBackPolys[i].y3 = (menuBackdropRects[i].y + menuBackdropRects[i].h) * 2;
        }
    }
}
#endif
#endif

extern POLY_FT4 menuBackPolys[14];
void DrawScaledTexturedQuad(float depth, POLY_FT4 *poly, int flags);

/* Queues the fourteen background polygons of the menu screens. */
#ifdef NETPLAY
/* The network build draws both kinds of backdrop at the same depth. */
void DrawMenuBackPolys(void)
{
    short i;

    if (rgbTextures) {
        for (i = 0; i < 14; i++)
            DrawScaledTexturedQuad(0.98f, &menuBackPolys[i], 0);
    } else {
        for (i = 0; i < 14; i++)
            DrawScaledTexturedQuad(0.98f, &menuBackPolys[i], 0);
    }
}
#else
void DrawMenuBackPolys(void)
{
    short i;

    if (rgbTextures) {
        for (i = 0; i < 14; i++)
            DrawScaledTexturedQuad(0.99f, &menuBackPolys[i], 0);
    } else {
        for (i = 0; i < 14; i++)
            DrawScaledTexturedQuad(0.98f, &menuBackPolys[i], 0);
    }
}
#endif


/* Sets the fourteen menu backdrop tiles and their texture coordinates. */
#ifdef EQUIV
/* Proven equivalent rather than byte-exact: all rectangle and UV values agree, but the constant
   stores are scheduled differently and the coordinates use other registers. */
void InitMenuBackPolys(void)
{
    short i;
    short left, top, right, bottom;

    for (i = 0; i < 14; i++) {
        SetPolyFT4(&menuBackPolys[i]);
        menuBackPolys[i].r0 = 255;
        menuBackPolys[i].g0 = 255;
        menuBackPolys[i].b0 = 255;
    }

    menuBackdropRects[0].x = 20;
    menuBackdropRects[0].y = 32;
    menuBackdropRects[0].w = 128;
    menuBackdropRects[0].h = 50;

    menuBackdropRects[1].x = 148;
    menuBackdropRects[1].y = 32;
    menuBackdropRects[1].w = 128;
    menuBackdropRects[1].h = 50;

    menuBackdropRects[2].x = 276;
    menuBackdropRects[2].y = 32;
    menuBackdropRects[2].w = 32;
    menuBackdropRects[2].h = 50;

    menuBackdropRects[3].x = 20;
    menuBackdropRects[3].y = 180;
    menuBackdropRects[3].w = 128;
    menuBackdropRects[3].h = 36;

    menuBackdropRects[4].x = 148;
    menuBackdropRects[4].y = 180;
    menuBackdropRects[4].w = 128;
    menuBackdropRects[4].h = 36;

    menuBackdropRects[5].x = 276;
    menuBackdropRects[5].y = 180;
    menuBackdropRects[5].w = 32;
    menuBackdropRects[5].h = 36;

    menuBackdropRects[6].x = 20;
    menuBackdropRects[6].y = 82;
    menuBackdropRects[6].w = 2;
    menuBackdropRects[6].h = 98;

    menuBackdropRects[7].x = 306;
    menuBackdropRects[7].y = 82;
    menuBackdropRects[7].w = 4;
    menuBackdropRects[7].h = 98;

    menuBackdropRects[8].x = 20;
    menuBackdropRects[8].y = 82;
    menuBackdropRects[8].w = 128;
    menuBackdropRects[8].h = 1;

    menuBackdropRects[9].x = 148;
    menuBackdropRects[9].y = 82;
    menuBackdropRects[9].w = 128;
    menuBackdropRects[9].h = 1;

    menuBackdropRects[10].x = 276;
    menuBackdropRects[10].y = 82;
    menuBackdropRects[10].w = 32;
    menuBackdropRects[10].h = 1;

    menuBackdropRects[11].x = 20;
    menuBackdropRects[11].y = 179;
    menuBackdropRects[11].w = 128;
    menuBackdropRects[11].h = 1;

    menuBackdropRects[12].x = 148;
    menuBackdropRects[12].y = 179;
    menuBackdropRects[12].w = 128;
    menuBackdropRects[12].h = 1;

    menuBackdropRects[13].x = 276;
    menuBackdropRects[13].y = 179;
    menuBackdropRects[13].w = 32;
    menuBackdropRects[13].h = 1;

    menuBackPolys[0].tpage = 6;
    menuBackPolys[1].tpage = 7;
    menuBackPolys[2].tpage = 8;
    menuBackPolys[1].u0 = menuBackPolys[0].u0 = 0;
    menuBackPolys[1].v0 = menuBackPolys[0].v0 = 0;
    menuBackPolys[1].u1 = menuBackPolys[0].u1 = 254;
    menuBackPolys[1].v1 = menuBackPolys[0].v1 = 0;
    menuBackPolys[1].u2 = menuBackPolys[0].u2 = 0;
    menuBackPolys[1].v2 = menuBackPolys[0].v2 = 98;
    menuBackPolys[1].u3 = menuBackPolys[0].u3 = 254;
    menuBackPolys[1].v3 = menuBackPolys[0].v3 = 98;
    menuBackPolys[2].u0 = 0;
    menuBackPolys[2].v0 = 0;
    menuBackPolys[2].u1 = 64;
    menuBackPolys[2].v1 = 0;
    menuBackPolys[2].u2 = 0;
    menuBackPolys[2].v2 = 98;
    menuBackPolys[2].u3 = 64;
    menuBackPolys[2].v3 = 98;

    menuBackPolys[3].tpage = 6;
    menuBackPolys[4].tpage = 7;
    menuBackPolys[5].tpage = 8;
    menuBackPolys[4].u0 = menuBackPolys[3].u0 = 0;
    menuBackPolys[4].v0 = menuBackPolys[3].v0 = 114;
    menuBackPolys[4].u1 = menuBackPolys[3].u1 = 254;
    menuBackPolys[4].v1 = menuBackPolys[3].v1 = 114;
    menuBackPolys[4].u2 = menuBackPolys[3].u2 = 0;
    menuBackPolys[4].v2 = menuBackPolys[3].v2 = 186;
    menuBackPolys[4].u3 = menuBackPolys[3].u3 = 254;
    menuBackPolys[4].v3 = menuBackPolys[3].v3 = 186;
    menuBackPolys[5].u0 = 0;
    menuBackPolys[5].v0 = 114;
    menuBackPolys[5].u1 = 64;
    menuBackPolys[5].v1 = 114;
    menuBackPolys[5].u2 = 0;
    menuBackPolys[5].v2 = 186;
    menuBackPolys[5].u3 = 64;
    menuBackPolys[5].v3 = 186;

    menuBackPolys[6].tpage = 8;
    menuBackPolys[6].u0 = 88;
    menuBackPolys[6].v0 = 0;
    menuBackPolys[6].u1 = 91;
    menuBackPolys[6].v1 = 0;
    menuBackPolys[6].u2 = 88;
    menuBackPolys[6].v2 = 194;
    menuBackPolys[6].u3 = 91;
    menuBackPolys[6].v3 = 194;

    menuBackPolys[7].tpage = 8;
    menuBackPolys[7].u0 = 98;
    menuBackPolys[7].v0 = 0;
    menuBackPolys[7].u1 = 102;
    menuBackPolys[7].v1 = 0;
    menuBackPolys[7].u2 = 98;
    menuBackPolys[7].v2 = 194;
    menuBackPolys[7].u3 = 102;
    menuBackPolys[7].v3 = 194;

    menuBackPolys[8].tpage = 6;
    menuBackPolys[9].tpage = 7;
    menuBackPolys[10].tpage = 8;
    menuBackPolys[8].u0 = menuBackPolys[9].u0 = 0;
    menuBackPolys[8].v0 = menuBackPolys[9].v0 = 100;
    menuBackPolys[8].u1 = menuBackPolys[9].u1 = 254;
    menuBackPolys[8].v1 = menuBackPolys[9].v1 = 100;
    menuBackPolys[8].u2 = menuBackPolys[9].u2 = 0;
    menuBackPolys[8].v2 = menuBackPolys[9].v2 = 101;
    menuBackPolys[8].u3 = menuBackPolys[9].u3 = 254;
    menuBackPolys[8].v3 = menuBackPolys[9].v3 = 101;
    menuBackPolys[10].u0 = 0;
    menuBackPolys[10].v0 = 100;
    menuBackPolys[10].u1 = 64;
    menuBackPolys[10].v1 = 100;
    menuBackPolys[10].u2 = 0;
    menuBackPolys[10].v2 = 101;
    menuBackPolys[10].u3 = 64;
    menuBackPolys[10].v3 = 101;

    menuBackPolys[11].tpage = 6;
    menuBackPolys[12].tpage = 7;
    menuBackPolys[13].tpage = 8;
    menuBackPolys[11].u0 = menuBackPolys[12].u0 = 0;
    menuBackPolys[11].v0 = menuBackPolys[12].v0 = 113;
    menuBackPolys[11].u1 = menuBackPolys[12].u1 = 254;
    menuBackPolys[11].v1 = menuBackPolys[12].v1 = 113;
    menuBackPolys[11].u2 = menuBackPolys[12].u2 = 0;
    menuBackPolys[11].v2 = menuBackPolys[12].v2 = 114;
    menuBackPolys[11].u3 = menuBackPolys[12].u3 = 254;
    menuBackPolys[11].v3 = menuBackPolys[12].v3 = 114;
    menuBackPolys[13].u0 = 0;
    menuBackPolys[13].v0 = 113;
    menuBackPolys[13].u1 = 64;
    menuBackPolys[13].v1 = 113;
    menuBackPolys[13].u2 = 0;
    menuBackPolys[13].v2 = 114;
    menuBackPolys[13].u3 = 64;
    menuBackPolys[13].v3 = 114;

    for (i = 0; i < 14; i++) {
        left = menuBackdropRects[i].x;
        left += left;
        top = menuBackdropRects[i].y;
        top += top;
        right = menuBackdropRects[i].w + menuBackdropRects[i].x;
        right += right;
        bottom = (short)(menuBackdropRects[i].h + menuBackdropRects[i].y) * 2;
        menuBackPolys[i].x0 = left;
        menuBackPolys[i].y0 = top;
        menuBackPolys[i].x1 = right;
        menuBackPolys[i].y1 = top;
        menuBackPolys[i].x2 = left;
        menuBackPolys[i].y2 = bottom;
        menuBackPolys[i].x3 = right;
        menuBackPolys[i].y3 = bottom;
    }
}
#endif

/* Reads CONFIG.DAT from the game directory. The video mode is restored only when the file's display
   driver count and second display word match the current ones; the controls and sound settings are
   always restored. Returns 1 when there is no usable file, after setting the defaults. */

#ifdef NETPLAY
#pragma intrinsic(strcpy, strcat)
int LoadConfig(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    char fullPath[256];
    int i;
    FILE *stream;
    int noConfig = 0;

    strcpy(fullPath, wobbleDir0);
    strcat(fullPath, "\\CONFIG.DAT");
    stream = fopen(fullPath, "rb");
    if (stream != NULL) {
        fread(&savedConfig, sizeof(savedConfig), 1, stream);
        fclose(stream);
        /* A stream whose header holds these two test values is not used. */
        if (savedConfig.displayDriverCount == 1234 && savedConfig.displayDriverConfigTag == 5678) {
            noConfig = 1;
        } else {
            if (savedConfig.displayDriverCount == displayDriverCount && savedConfig.displayDriverConfigTag == displayDriverConfigTag) {
                videoSettingsLoaded = 1;
                videoModeIndex = savedConfig.videoModeIndex;
                pixelFormatGroup = savedConfig.pixelFormatGroup;
                displayBits = savedConfig.displayBits;
                pixelFormatIndex = savedConfig.pixelFormatIndex;
            }
            sfxMode = savedConfig.options.sfxMode;
            savedDeviceType = savedConfig.savedDeviceType;
            joystickPresent = savedConfig.joystickPresent;
            mouseEnabled = savedConfig.mouseEnabled;
            turnDivisor = savedConfig.turnDivisor;
            mouseSensitivity = savedConfig.mouseSensitivity;
            for (i = 0; i < 12; i++)
                gameKeys[i] = savedConfig.gameKeys[i];
            for (i = 0; i < 8; i++)
                keyBindings[i] = savedConfig.keyBindings[i];
            for (i = 0; i < 8; i++)
                padBindings[i] = savedConfig.padBindings[i];
        }
    } else {
        noConfig = 1;
    }
    if (noConfig) {
        savedDeviceType = 0;
        joystickPresent = 0;
        mouseEnabled = 0;
        turnDivisor = 200;
        mouseSensitivity = 3;
        sfxMode = 2;
        InitPadSettings();
        return 1;
    } else {
        return 0;
    }
}
#pragma function(strcpy, strcat)
#else
int LoadConfig(void)
{
    char path[256];
    FILE *file;
    int missing = 0;

    strcpy(path, wobbleDir0);
    strcat(path, "\\CONFIG.DAT");
    file = fopen(path, "rb");
    if (file != NULL) {
        fread(&savedConfig, sizeof(savedConfig), 1, file);
        fclose(file);
        /* A file whose header holds these two test values is not used. */
        if (savedConfig.displayDriverCount != 1234 || savedConfig.displayDriverConfigTag != 5678) {
            if (savedConfig.displayDriverCount == displayDriverCount && savedConfig.displayDriverConfigTag == displayDriverConfigTag) {
                videoSettingsLoaded = 1;
                videoModeIndex = savedConfig.videoModeIndex;
                pixelFormatGroup = savedConfig.pixelFormatGroup;
                displayBits = savedConfig.displayBits;
#ifdef POWERVR
                if (savedConfig.pixelFormatIndex < 3)
                    pixelFormatIndex = savedConfig.pixelFormatIndex;
#else
                pixelFormatIndex = savedConfig.pixelFormatIndex;
#endif
            }
            sfxMode = savedConfig.options.sfxMode;
            savedDeviceType = savedConfig.savedDeviceType;
            joystickPresent = savedConfig.joystickPresent;
            mouseEnabled = savedConfig.mouseEnabled;
            turnDivisor = savedConfig.turnDivisor;
            mouseSensitivity = savedConfig.mouseSensitivity;
            memcpy(gameKeys, savedConfig.gameKeys, sizeof(gameKeys));
            memcpy(keyBindings, savedConfig.keyBindings, sizeof(keyBindings));
            memcpy(padBindings, savedConfig.padBindings, sizeof(padBindings));
            goto loaded;
        }
    }
    missing = 1;
loaded:
    if (missing) {
        savedDeviceType = 0;
        joystickPresent = 0;
        mouseSensitivity = 3;
        sfxMode = 2;
        turnDivisor = 200;
        mouseEnabled = 0;
        InitPadSettings();
        return 1;
    }
    return 0;
}
#endif


/* Copies the current settings into the config file image and writes it to CONFIG.DAT. Start-up
   calls it when no config file was found, so the defaults reach the disk. Returns 1 if the file
   cannot be created. */
#ifdef NETPLAY
#pragma intrinsic(strcpy, strcat, memcpy)
int SaveConfig(void)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    char fullPath[256];
    int i;
    FILE *stream;

    strcpy(fullPath, wobbleDir0);
    strcat(fullPath, "\\CONFIG.DAT");
    savedConfig.displayDriverCount = displayDriverCount;
    savedConfig.displayDriverConfigTag = displayDriverConfigTag;
    savedConfig.videoModeIndex = videoModeIndex;
    savedConfig.pixelFormatGroup = pixelFormatGroup;
    savedConfig.displayBits = displayBits;
    savedConfig.pixelFormatIndex = pixelFormatIndex;
    savedConfig.options.sfxMode = sfxMode;
    savedConfig.options.musicVolume = gameData->musicVolume;
    savedConfig.options.effectVolume = gameData->effectVolume;
    savedConfig.options.otherMusicVolume = gameData->otherMusicVolume;
    savedConfig.options.otherEffectVolume = gameData->otherEffectVolume;
    savedConfig.options.cdTrack = gameData->cdTrack;
    savedConfig.options.weaponsOn = gameData->weaponsOn;
    savedConfig.options.internalView = gameData->internalView;
    savedConfig.options.trailDetail = gameData->trailDetail;
    savedConfig.options.skyEnabled = gameData->skyEnabled;
    savedConfig.options.smoothTextures = gameData->smoothTextures;
    savedConfig.options.viewDetail = gameData->viewDetail;
    savedConfig.options.extraGraphicsOption = gameData->extraGraphicsOption;
    savedConfig.options.texturesOn = gameData->texturesOn;
    for (i = 0; i < 9; i++)
        savedConfig.options.name[i] = gameData->name[i];
    savedConfig.savedDeviceType = savedDeviceType;
    savedConfig.joystickPresent = joystickPresent;
    savedConfig.mouseEnabled = mouseEnabled;
    savedConfig.turnDivisor = turnDivisor;
    savedConfig.mouseSensitivity = mouseSensitivity;
    for (i = 0; i < 12; i++)
        savedConfig.gameKeys[i] = gameKeys[i];
    for (i = 0; i < 8; i++)
        savedConfig.keyBindings[i] = keyBindings[i];
    for (i = 0; i < 8; i++)
        savedConfig.padBindings[i] = padBindings[i];
    memcpy(savedConfig.hiScores, hiScoreTable, sizeof(savedConfig.hiScores));

    stream = fopen(fullPath, "wb");
    if (stream != 0) {
        fwrite(&savedConfig, sizeof(savedConfig), 1, stream);
        fclose(stream);
        return 0;
    } else {
        return 1;
    }
}
#pragma function(strcpy, strcat, memcpy)
#endif

#ifdef EQUIV
#ifndef NETPLAY
/* Proven equivalent rather than byte-exact: every store and copy agrees, but the register chosen
   for the fopen path address differs. */
int SaveConfig(void)
{
#ifdef POWERVR
    extern signed char fogEnabled;
#endif
    char path[256];
    FILE *file;
    int i;

    strcpy(path, wobbleDir0);
    strcat(path, "\\CONFIG.DAT");
    savedConfig.displayDriverCount = displayDriverCount;
    savedConfig.displayDriverConfigTag = displayDriverConfigTag;
    savedConfig.videoModeIndex = videoModeIndex;
    savedConfig.pixelFormatGroup = pixelFormatGroup;
    savedConfig.displayBits = displayBits;
    savedConfig.pixelFormatIndex = pixelFormatIndex;
    savedConfig.options.sfxMode = sfxMode;
    savedConfig.options.musicVolume = gameData->musicVolume;
    savedConfig.options.effectVolume = gameData->effectVolume;
    savedConfig.options.otherMusicVolume = gameData->otherMusicVolume;
    savedConfig.options.otherEffectVolume = gameData->otherEffectVolume;
    savedConfig.options.cdTrack = gameData->cdTrack;
    savedConfig.options.weaponsOn = gameData->weaponsOn;
    savedConfig.options.internalView = gameData->internalView;
#ifdef POWERVR
    savedConfig.options.fogEnabled = fogEnabled;
#endif
    savedConfig.options.trailDetail = gameData->trailDetail;
    savedConfig.options.skyEnabled = gameData->skyEnabled;
    savedConfig.options.smoothTextures = gameData->smoothTextures;
    savedConfig.options.viewDetail = gameData->viewDetail;
    savedConfig.options.extraGraphicsOption = gameData->extraGraphicsOption;
    savedConfig.options.texturesOn = gameData->texturesOn;
    for (i = 0; i < 9; i++)
        savedConfig.options.name[i] = gameData->name[i];
    savedConfig.savedDeviceType = savedDeviceType;
    savedConfig.joystickPresent = joystickPresent;
    savedConfig.mouseEnabled = mouseEnabled;
    savedConfig.turnDivisor = turnDivisor;
    savedConfig.mouseSensitivity = mouseSensitivity;
    memcpy(savedConfig.gameKeys, gameKeys, sizeof(savedConfig.gameKeys));
    memcpy(savedConfig.keyBindings, keyBindings, sizeof(savedConfig.keyBindings));
    memcpy(savedConfig.padBindings, padBindings, sizeof(savedConfig.padBindings));
    memcpy(savedConfig.hiScores, hiScoreTable, sizeof(savedConfig.hiScores));

    file = fopen(path, "wb");
    if (file != 0) {
        fwrite(&savedConfig, sizeof(savedConfig), 1, file);
        fclose(file);
        return 0;
    }
    return 1;
}
#endif
#endif

#ifdef NETPLAY
/* A small packet of 100 menu quads and the skeleton that carries them. */
typedef struct MenuQuadBatch {
    u_char header[16];
    POLY_FT4 quads[100];
    Skeleton *skeleton;
} MenuQuadBatch;

typedef struct MenuQuadFaceVertex {
    short flags;
    short texture;
    short index;
    short reserved;
} MenuQuadFaceVertex;

typedef struct MenuQuadFace {
    MenuQuadFaceVertex vertex;
    u_char reserved[12];
} MenuQuadFace;

extern Texture *textureTable[];
extern TrackData *track;

void InitMenuQuadBatch(MenuQuadBatch *batch, Skeleton *parent)
{
    short textureIndex;
    POLY_FT4 *quad;
    Texture *texture;

    textureIndex = 0x172;
    batch->skeleton = NewSkeleton();
    batch->skeleton->super = parent;
    quad = &batch->quads[0];
    quad->r0 = 0x7f;
    quad->g0 = 0x7f;
    quad->b0 = 0x7f;
    texture = textureTable[textureIndex];
    quad->u0 = texture->u0;
    quad->v0 = texture->v0;
    quad->u1 = texture->u1;
    quad->v1 = texture->v1;
    quad->u2 = texture->u2;
    quad->v2 = texture->v2;
    quad->u3 = texture->u3;
    quad->v3 = texture->v3;
    quad->tpage = texture->tsb;
    quad->clut = texture->cba;
    SetPolyFT4(quad);
}
#endif

#ifdef NETPLAY
extern int gstSessionConfigSelection;
extern signed char gstWeaponGridSelection;
extern char *gstWeaponGridNames[9];
extern int gstWeaponGridX[9];
extern char *gstSessionClassNames[4];
extern char *gstSessionLapOptionText[6];
extern int gstSessionLapOptionY[6];
extern char *gstSessionLifeOptionText[6];
extern int gstSessionLifeOptionY[6];
extern char *gstSessionPiranhaOptionText[2];
extern int gstSessionPiranhaOptionY[2];
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* Chooses the weapon grids, lap count, lives and Piranha ship rule for a hosted session. */
void doSessionConfigMenu(void)
{
    /* /Od keeps these original locals even though the code never reads them. */
    int menuY;
    int raceClass;
    int selected;
    int sessionOption;
    int labelX;
    char headerText[80];
    int i;

    Print("CREATE SESSION MODE", 30, 75, 0);
    AddText("RACE CUSTOMIZATION", 40, 50, 8, 0);
    if (gstRaceMode == 3)
        sprintf(headerText, "MULTIPLAYER TOURNAMENT");
    else
        sprintf(headerText, "%s: %s", gstSessionClassNames[gameData->shipClass], gstTrackNames[gameData->trackNum]);
    CentreText(headerText, 0, 90, 8, 0);
    if ((gstSessionConfigSelection == 0 || gstSessionConfigSelection == 1 || gstSessionConfigSelection == 2) && blinkFlag)
        Print("WEAPONS:", 30, 110, 1);
    else if (gstSessionConfigSelection != 0 && gstSessionConfigSelection != 1 && gstSessionConfigSelection != 2)
        Print("WEAPONS:", 30, 110, 0);
    for (i = 0; i < 9; i++) {
        if ((gstSessionConfigSelection == 0 || gstSessionConfigSelection == 1 || gstSessionConfigSelection == 2) && gstWeaponGridSelection == i) {
            if (blinkFlag && gstLevelWeapons[i])
                Print(gstWeaponGridNames[i], gstWeaponGridX[i], 110 + i / 3 * 9, 1);
            else if (blinkFlag && !gstLevelWeapons[i])
                Print(gstWeaponGridNames[i], gstWeaponGridX[i], 110 + i / 3 * 9, 0);
        } else {
            if (gstLevelWeapons[i])
                Print(gstWeaponGridNames[i], gstWeaponGridX[i], 110 + i / 3 * 9, 1);
            else
                Print(gstWeaponGridNames[i], gstWeaponGridX[i], 110 + i / 3 * 9, 0);
        }
    }
    if (gstSessionConfigSelection == 3 && blinkFlag)
        Print("NUMBER OF LAPS:", 30, 140, 1);
    else if (gstSessionConfigSelection != 3)
        Print("NUMBER OF LAPS:", 30, 140, 0);
    for (i = 0; i < 6; i++) {
        if (i == gstMenuLaps)
            Print(gstSessionLapOptionText[i], gstSessionLapOptionY[i], 140, 1);
        else
            Print(gstSessionLapOptionText[i], gstSessionLapOptionY[i], 140, 0);
    }
    if (gstSessionConfigSelection == 4 && blinkFlag)
        Print("LIVES PER PLAYER:", 30, 150, 1);
    else if (gstSessionConfigSelection != 4)
        Print("LIVES PER PLAYER:", 30, 150, 0);
    for (i = 0; i < 6; i++) {
        if (i == gstMenuLives)
            Print(gstSessionLifeOptionText[i], gstSessionLifeOptionY[i], 150, 1);
        else
            Print(gstSessionLifeOptionText[i], gstSessionLifeOptionY[i], 150, 0);
    }
    if (gstSessionConfigSelection == 5 && blinkFlag)
        Print("PIRANHA SHIPS:", 30, 160, 1);
    else if (gstSessionConfigSelection != 5)
        Print("PIRANHA SHIPS:", 30, 160, 0);
    for (i = 0; i < 2; i++) {
        if (i == gstMenuPiranha)
            Print(gstSessionPiranhaOptionText[i], gstSessionPiranhaOptionY[i], 160, 1);
        else
            Print(gstSessionPiranhaOptionText[i], gstSessionPiranhaOptionY[i], 160, 0);
    }
    if (gstSessionConfigSelection == 0 || gstSessionConfigSelection == 1 || gstSessionConfigSelection == 2)
        DoButton(30, 198, 2, "TOGGLE");
    else
        DoButton(30, 198, 2, "ACCEPT");
    DoButton(278, 198, 1, "BACK");

    switch (gstSessionConfigSelection) {
    case 0:
    case 1:
    case 2:
        Print("LIST OF ACTIVE WEAPONS ON WEAPON GRIDS.", 30, 182, 0);
        Print("YELLOW IS ACTIVATED, WHITE IS DEACTIVEATED.", 30, 189, 0);
        break;
    case 3:
        Print("NUMBER OF LAPS FOR SINGLE RACE OR TOURNAMENT.", 30, 182, 0);
        Print("DEFAULT NUMBER OF LAPS IS CLASS SPECIFIC.", 30, 189, 0);
        break;
    case 4:
        Print("NUMBER OF TIMES WEAK RACERS ARE ALLOWED", 30, 182, 0);
        Print("TO CONTINUE AFTER BEING DESTROYED.", 30, 189, 0);
        break;
    case 5:
        Print("ALLOW PIRANHA SHIPS TO BE ADMITTED TO GAME.", 30, 182, 0);
        Print("IF NO TEAM PIRANHA CAN NOT JOIN SESSION.", 30, 189, 0);
        break;
    }

    if (GetMenuKey(0)) {
        if (gstSessionConfigSelection > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstSessionConfigSelection--;
            if (gstSessionConfigSelection == 0 || gstSessionConfigSelection == 1)
                gstWeaponGridSelection -= 3;
            else if (gstSessionConfigSelection == 2)
                gstWeaponGridSelection = 6;
        }
        return;
    }
    if (GetMenuKey(1)) {
        if (gstSessionConfigSelection < 5) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstSessionConfigSelection++;
            if (gstSessionConfigSelection == 1 || gstSessionConfigSelection == 2)
                gstWeaponGridSelection += 3;
        }
        return;
    }
    if (GetMenuKey(2)) {
        if (gstSessionConfigSelection == 0 && gstWeaponGridSelection > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstWeaponGridSelection--;
        }
        if (gstSessionConfigSelection == 1 && gstWeaponGridSelection > 3) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstWeaponGridSelection--;
        }
        if (gstSessionConfigSelection == 2 && gstWeaponGridSelection > 6) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstWeaponGridSelection--;
        }
        if (gstSessionConfigSelection == 3 && gstMenuLaps > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstMenuLaps--;
        }
        if (gstSessionConfigSelection == 4 && gstMenuLives > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstMenuLives--;
        }
        if (gstSessionConfigSelection == 5 && gstMenuPiranha > 0) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstMenuPiranha--;
        }
        return;
    }
    if (GetMenuKey(3)) {
        if (gstSessionConfigSelection == 0 && gstWeaponGridSelection < 2) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstWeaponGridSelection++;
        }
        if (gstSessionConfigSelection == 1 && gstWeaponGridSelection < 5) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstWeaponGridSelection++;
        }
        if (gstSessionConfigSelection == 2 && gstWeaponGridSelection < 8) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstWeaponGridSelection++;
        }
        if (gstSessionConfigSelection == 3 && gstMenuLaps < 5) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstMenuLaps++;
        }
        if (gstSessionConfigSelection == 4 && gstMenuLives < 5) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstMenuLives++;
        }
        if (gstSessionConfigSelection == 5 && gstMenuPiranha < 1) {
            PlayNote(SAMPLE_MENU_MOVE, 255, 15, 15, 256);
            gstMenuPiranha++;
        }
        return;
    }
    if (GetMenuKey(4)) {
        if (gstLaunchMode == 0) {
            PlayNote(SAMPLE_MENU_SLIDE, 255, 15, 15, 256);
            gstPlayerNameLength = 0;
            gstSessionNameLength = 0;
            gstOptionsSelected = 0;
            currMenu = 17;
            InitSlider();
        } else {
            PlayNote(SAMPLE_MENU_SLIDE, 255, 15, 15, 256);
            currMenu = 0;
            InitSlider();
        }
        PollKeyboard();
        return;
    }
    if (GetMenuKey(5)) {
        PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
        if (gstSessionConfigSelection == 0 || gstSessionConfigSelection == 1 || gstSessionConfigSelection == 2) {
            if (!gstLevelWeapons[gstWeaponGridSelection])
                gstLevelWeapons[gstWeaponGridSelection] = 1;
            else
                gstLevelWeapons[gstWeaponGridSelection] = 0;
        } else {
            if (gstRaceMode != 3) {
                if (JoinSessionServer()) {
                    currMenu = 21;
                    CopyGameData(0, 0, 0);
                    InitSlider();
                    PollKeyboard();
                    return;
                } else {
                    gstDebugPrintf("doSessionConfigMenu: JoinSessionServer Failed! Doh!");
                }
            } else if (gstRaceMode == 3) {
                currMenu = 19;
                InitSlider();
                return;
            }
        }
    }
}
#endif
#endif

#ifdef NETPLAY
/* Constant test the original compiles away; its else branch is empty. */
#define MENU_QUAD_CORNER_SKEW 1
/* Positions the first quad over the selected section and updates its four screen corners. */
#include <math.h>
#pragma intrinsic(sin, cos)
void UpdateMenuQuadBatch(MenuQuadBatch *batch, TrackSection *section, short angle)
{
    short halfHeight;
    long depthTmp;
    long heightScaleLocal;
    long ySine;
    long xCosinePos;
    short trYCalc;
    short cornerTRXExt;
    short bottomLeftYAmt;
    short bottomLeftXTmp;
    long widthScaleExt;
    SVECTOR cornerLocal;
    DVECTOR screenPositionValue;
    short upperLeftYAmt;
    MenuQuadFace *faceRef;
    short topLeftXExt;
    short bottomRightY;
    long flagsTmp;
    long perspScale;
    short lowerRightX;
    short halfWidthSc;
    long yCosineAmt;
    long centerY;
    long xAxisSin;
    long screenXAmt;

    faceRef = (MenuQuadFace *)((char *)(track->faces + section->faceStart) + sizeof(Face));
    SetSkeletonPosition(batch->skeleton,
                        track->vertices[faceRef->vertex.index].vx,
                        track->vertices[faceRef->vertex.index].vy,
                        track->vertices[faceRef->vertex.index].vz);
    SetSkeletonDirection(batch->skeleton, 0, 0, 0);
    batch->skeleton->update = 1;
    SetSkeletonTransform(batch->skeleton);

    cornerLocal.vx = 0;
    cornerLocal.vy = 0;
    cornerLocal.vz = 0;
    perspScale = RotTransPers(&cornerLocal, (long *)&screenPositionValue, &depthTmp, &flagsTmp);
    screenXAmt = screenPositionValue.vx;
    centerY = screenPositionValue.vy;
    if (!(flagsTmp & 0x80460000)) {
        if (MENU_QUAD_CORNER_SKEW) {
            widthScaleExt = 0xA000 / perspScale;
            heightScaleLocal = 0xA000 / perspScale;
            halfWidthSc = (short)(widthScaleExt >> 1);
            halfHeight = (short)(heightScaleLocal >> 1);
        } else {
        }

        if (MENU_QUAD_CORNER_SKEW) {
            xCosinePos = (long)(cos((double)angle / 4096.0 * 6.28318530718) * 4096.0) * halfWidthSc >> 12;
        } else {
        }
        if (MENU_QUAD_CORNER_SKEW) {
            ySine = (long)(sin((double)angle / 4096.0 * 6.28318530718) * 4096.0) * halfWidthSc >> 12;
        } else {
        }
        if (MENU_QUAD_CORNER_SKEW) {
            xAxisSin = (long)(cos((double)(0x400 - angle) / 4096.0 * 6.28318530718) * 4096.0) * heightScaleLocal >> 12;
        } else {
        }
        yCosineAmt = (long)(sin((double)(0x400 - angle) / 4096.0 * 6.28318530718) * 4096.0) * heightScaleLocal >> 12;

        lowerRightX = (short)(screenXAmt - xCosinePos);
        bottomRightY = (short)(centerY - ySine);
        bottomLeftXTmp = (short)(screenXAmt + xCosinePos);
        bottomLeftYAmt = (short)(centerY + ySine);
        topLeftXExt = (short)(lowerRightX + xAxisSin);
        upperLeftYAmt = (short)(bottomRightY - yCosineAmt);
        cornerTRXExt = (short)(bottomLeftXTmp + xAxisSin);
        trYCalc = (short)(bottomLeftYAmt - yCosineAmt);

        batch->quads[0].x0 = topLeftXExt;
        batch->quads[0].y0 = upperLeftYAmt;
        batch->quads[0].x1 = cornerTRXExt;
        batch->quads[0].y1 = trYCalc;
        batch->quads[0].x2 = lowerRightX;
        batch->quads[0].y2 = bottomRightY;
        batch->quads[0].x3 = bottomLeftXTmp;
        batch->quads[0].y3 = bottomLeftYAmt;
    }
}
#endif
