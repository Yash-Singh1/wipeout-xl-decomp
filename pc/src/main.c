#define PC_DISPLAY_IMPORTS
#include "functions.h"
#include "sfx_started.h"
#include "ship_state.h"
#include "race_mode.h"
#include "hud_state.h"
#include "pc.h"
#include "pc_blit.h"
#include "sound_samples.h"
#include <string.h>

extern char wobbleDir5[];
extern int doubleSourceRects;

extern short g_keyboardCode;
extern int copyrightShown;

extern long ddResult;
extern int titleSeen;
extern int titleResult;
extern int menuQuit;
extern int raceContext;
extern int blinkFlag;
extern int cdMusicCount;
extern signed char cdMusicIndex;
extern signed char cdMusicTable[];
extern int cdMusicTrack;
extern int quadIndex;
extern short CurrentScreen;
extern int pressLoaded;
extern int titleLoaded;
extern HeapBlock *mainheapcopy;

extern ConfigData *gameData;
void UpdateNotes(int arg);
void SetDefaultPalette(void);
void PollKeyboard(void);
void AddBlit320(int layer, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int tex);

extern int quitRequested;
extern short TextureTableCount;
extern int menuTimeout;
extern int savedTextureTableCount;

void SfxReverbOff(void);
void SfxReverbOn(void);
int RaceFrameStart(void);



extern char wobbleDir4[], wobbleDir7[];
extern short ownShip;
extern int gameType, trackNo, linkMyState, texture16Bit;
extern signed char raceTracks[];
void PlayMovie(char *name);
#include "texture_pages.h"
void TitleScreen(void);
void StartRace(void);
void ShowCopyrightScreen(void);

#include "heap_state.h"
extern HeapBlock *reserveHeapCopy;
extern ConfigData initialGameData;
#include "movie_sound.h"
extern unsigned long previousFrameTime, currentFrameTime;
extern int savedAttractShip, savedAttractActTrack, savedAttractTrack, attractCycles;
extern int flipView, mmxAvailable;
extern char wobbleDir3[];
void CreateSineTable(void);
void InitJoystick(void);
void InitGameData(ConfigData *config);
void InitMenuFills(void);
void InitDefaultSlot(void);
#include "pc_backbuffer.h"

#ifdef NETPLAY
#include "gst.h"
#include "lobby.h"
extern int playerPoints[];
extern int attractCycles;
extern int rgbTextures;
void ClearScreens(void);
void RunMenus(void);
void ClearBackBufferToBlack(int unused);
void EnterNetworkMenu(void);
#pragma intrinsic(strcpy, strcat, memset)
#endif

#ifdef EQUIV
#ifdef NETPLAY
/* The network build: the same start-up and texture loading, but the title/menu/race loop also handles tournaments
   (the host picks the class and track of each race) and disconnects from the network at the end. */
/* The local names are chosen so that the frame has the executable's stack layout. */
int main(void)
{
    int curStatus;
    char tracks[8];
    int newStep;
    unsigned long curBytes;
    char file[256];
    ConfigData latestDefaults;
    PcWaveFormat soundFormat;

    gameData = &initialGameData;
    CreateSineTable();
    heap = GlobalAlloc(GMEM_ZEROINIT, 0x95000);
    curBytes = GlobalSize(heap);
    reserveHeap = GlobalAlloc(GMEM_ZEROINIT, 0x1CE800);
    InitDynamicMem(heap, curBytes);
    InitDynamicMem(reserveHeap, 0x1C2000);
    reserveHeapCopy = reserveHeap;
    mainheapcopy = heap;
    InitSystem();
    ClearVram();
    InitJoystick();
    PollKeyboard();
    InitialiseAmbientColour();
    InitialiseColourMatrix();
    InitialiseLightMatrix();
    ClearVram();
    blitCount[0][0] = 0;
    blitCount[0][1] = 0;
    blitCount[1][0] = 0;
    blitCount[1][1] = 0;
    InitSonyDisplay();
    previousFrameTime = timeGetTime();
    currentFrameTime = previousFrameTime;
    ClearScreens();
    gameData = &latestDefaults;
    TextureTableCount = 0x113;
    SetDispMask(0);
    InitGameDisplay(1);
    SetDispMask(1);
    raceInProgress = 3;
    ShowCopyrightScreen();
    InitSpuSound();
    InitSpuSamples();
    StartControlledSound();
    if (sfxStarted) {
        memset(&soundFormat, 0, sizeof(soundFormat));
        soundFormat.wFormatTag = WAVE_FORMAT_PCM;
        soundFormat.nChannels = 2;
        soundFormat.nSamplesPerSec = 44100;
        soundFormat.wBitsPerSample = 16;
        soundFormat.nBlockAlign = soundFormat.nChannels * soundFormat.wBitsPerSample / 8;
        soundFormat.nAvgBytesPerSec = soundFormat.nBlockAlign * soundFormat.nSamplesPerSec;
        SfxCreateBuffer(&introSoundBuffer, 0x10000, &soundFormat);
    }
    strcpy(file, wobbleDir7);
    strcat(file, "wxlintr.mpg");
    PlayMovie(file);
    InitText();
    InitGameData(gameData);
    if (noSavedConfig) SaveConfig();
    ownShip = 9;
    savedAttractShip = ownShip;
    savedAttractActTrack = gameData->actTrackNum;
    savedAttractTrack = gameData->trackNum;
    gameData->shipClass = 0;
    gameType = 99;
    continuesLeft = 0;
    menuTimeout = 3600;
    raceContext = 1;
    InitResident();
    InitMenuFills();
    strcpy(file, wobbleDir3);
    strcat(file, "chrome.tim");
    LoadTexture(file, 1);
    DrawSync(0);
    InitShufflePlay();
    cdMusicIndex = 0;
    cdMusicTrack = cdMusicTable[cdMusicIndex];
    raceInProgress = 0;
    InitDefaultSlot();
    menuQuit = 1;
    ClearScreens();
    strcpy(file, wobbleDir4);
    if (texture16Bit == 0) {
        strcat(file, "menuhf.bmp");
        LoadBitmapTextures(0, file, 11, 11);
        if (sillyShips) {
            strcpy(file, wobbleDir4);
            strcat(file, "sillyshp.bmp");
            LoadBitmapTextures(0, file, 13, 2);
            strcpy(file, wobbleDir4);
            strcat(file, "sillshad.bmp");
            LoadBitmapTextures(0, file, 15, 2);
        }
        strcpy(file, wobbleDir4);
        strcat(file, "menukey.bmp");
        LoadBitmapTextures(0, file, 6, 2);
        strcpy(file, wobbleDir4);
        strcat(file, "testmenu.bmp");
        LoadBitmapTextures(0, file, 1, 3);
        if (mmxAvailable) {
            strcpy(file, wobbleDir4);
            strcat(file, "hud2.ppm");
            textureSurfaces[3] = LoadPPMSurface(file, 3);
            strcpy(file, wobbleDir4);
            strcat(file, "xllogo.ppm");
            textureSurfaces[4] = LoadPPMSurface(file, 4);
            strcpy(file, wobbleDir4);
            strcat(file, "yfont.ppm");
            textureSurfaces[5] = LoadPPMSurface(file, 5);
            strcpy(file, wobbleDir4);
            strcat(file, "testmenu.ppm");
            textureSurfaces[6] = LoadPPMSurface(file, 6);
        } else {
            strcpy(file, wobbleDir4);
            strcat(file, "hud2.bmp");
            LoadBitmapTextures(0, file, 22, 3);
            strcpy(file, wobbleDir4);
            strcat(file, "xllogo.bmp");
            LoadBitmapTextures(0, file, 25, 1);
            strcpy(file, wobbleDir4);
            strcat(file, "yfont.bmp");
            LoadBitmapTextures(0, file, 26, 1);
        }
    } else {
        strcpy(file, wobbleDir4);
        strcat(file, "menuhf.tga");
        Load16BitBitmapTextures(0, file, 11, 11);
        if (sillyShips) {
            strcpy(file, wobbleDir4);
            strcat(file, "sillyshp.tga");
            Load16BitBitmapTextures(0, file, 13, 2);
            strcpy(file, wobbleDir4);
            strcat(file, "sillshad.tga");
            Load16BitBitmapTextures(0, file, 15, 2);
        }
        strcpy(file, wobbleDir4);
        strcat(file, "menukey.tga");
        Load16BitBitmapTextures(0, file, 6, 2);
        strcpy(file, wobbleDir4);
        strcat(file, "backgrnd.tga");
        Load16BitBitmapTextures(0, file, 0, 1);
        strcpy(file, wobbleDir4);
        strcat(file, "testmenu.tga");
        Load16BitBitmapTextures(0, file, 1, 3);
        strcpy(file, wobbleDir4);
        strcat(file, "hud2.tga");
        Load16BitBitmapTextures(0, file, 22, 3);
        strcpy(file, wobbleDir4);
        strcat(file, "xllogo.tga");
        Load16BitBitmapTextures(0, file, 25, 1);
    }
    do {
        while (titleResult != 1 && quitRequested != 1 && linkMyState != 1) {
            if (menuQuit) {
                TitleScreen();
                if (titleResult == 3) {
                    attractCycles++;
                    if (attractCycles > 2) {
                        strcpy(file, wobbleDir7);
                        strcat(file, "wxlintr.mpg");
                        PlayMovie(file);
                        attractCycles = 0;
                    } else {
                        raceContext = 1;
                        gameType = 0;
                        flipView = 1;
                        if (gameData->piranhaChallenge) ownShip = newRand() % 14;
                        else ownShip = newRand() % 11;
                        trackNo = newRand() % 6;
                        gameData->trackNum = trackNo;
                        gameData->actTrackNum = raceTracks[gameData->trackNum];
                        StartRace();
                    }
                }
            } else {
                RunMenus();
                if (quitRequested != 3 && quitRequested != 1) {
                    for (newStep = 0; newStep < 15; newStep++) {
                        killerTotal[newStep] = 0;
                        playerPoints[newStep] = 0;
                    }
                    tournamentTrack = 0;
                    raceContext = 0;
                    for (;;) {
                        if (networkGame && gstRaceMode == 3) {
                            tracks[0] = 1;
                            tracks[1] = 8;
                            tracks[2] = 13;
                            tracks[3] = 20;
                            tracks[4] = 2;
                            tracks[5] = 17;
                            tracks[6] = 6;
                            tracks[7] = 7;
                            gameData->shipClass = gstLevelClasses[tournamentTrack];
                            gameData->trackNum = gstLevelTracks[tournamentTrack];
                            gameData->actTrackNum = tracks[gstLevelTracks[tournamentTrack]];
                        }
                        StartRace();
                        if (networkGame) EnterNetworkMenu();
                        if (linkMyState != 4) break;
                        if (networkGame) {
                            tournamentTrack++;
                            if (tournamentTrack >= 8) break;
                        }
                    }
                }
            }
        }
        quadIndex = CurrentScreen * 300;
    } while (!RaceFrameStart());
    curStatus = gstDisconnectGPI();
    if (curStatus < 1)
        gstDebugPrintf("gstDisconnectGPI: error %d [%s]", curStatus, gstErrorString(curStatus));
    curStatus = gstReleaseBuffers();
    if (curStatus < 1)
        gstDebugPrintf("gstExitGPIi: error %d [%s]", curStatus, gstErrorString(curStatus));
    SaveConfig();
    SfxOff();
    if (!rgbTextures) ClearBackBufferToBlack(0);
    return 0;
}
#else
/* PC startup, texture loading and the title/menu/race loop. Matches the executable up to
   scheduling (EQUIV). */
int main(void)
{
    BLK_FILL *fill[2];
    PcWaveFormat format;
    char path[256];
    ConfigData config;
    unsigned long heapSize;
    short i;

    gameData = &initialGameData;
    CreateSineTable();
    heap = GlobalAlloc(GMEM_ZEROINIT, 0x95000);
    heapSize = GlobalSize(heap);
    reserveHeap = GlobalAlloc(GMEM_ZEROINIT, 0x1CE800);
    InitDynamicMem(heap, heapSize);
    InitDynamicMem(reserveHeap, 0x1C2000);
    reserveHeapCopy = reserveHeap;
    mainheapcopy = heap;
    InitSystem();
    ClearVram();
    InitJoystick();
    PollKeyboard();
    InitialiseAmbientColour();
    InitialiseColourMatrix();
    InitialiseLightMatrix();
    ClearVram();
    blitCount[0][0] = 0;
    blitCount[0][1] = 0;
    blitCount[1][0] = 0;
    blitCount[1][1] = 0;
    InitSonyDisplay();
    previousFrameTime = timeGetTime();
    currentFrameTime = previousFrameTime;
    fill[0] = BlockFill(0, 0, 320, 240, 0, 0, 46);
    fill[1] = BlockFill(0, 256, 320, 240, 0, 0, 46);
    for (i = 0; i < 2; i++) {
        AddFill(0, (Tile *)fill[CurrentScreen]);
        RaceFrameStart();
    }
    gameData = &config;
    TextureTableCount = 0x113;
    SetDispMask(0);
    InitGameDisplay(1);
    SetDispMask(1);
    raceInProgress = 3;
    ShowCopyrightScreen();
    InitSpuSound();
    InitSpuSamples();
    StartControlledSound();
    if (sfxStarted) {
        memset(&format, 0, sizeof(format));
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2;
        format.nSamplesPerSec = 44100;
        format.wBitsPerSample = 16;
        format.nBlockAlign = format.wBitsPerSample / 8 * format.nChannels;
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
        SfxCreateBuffer(&introSoundBuffer, 0x10000, &format);
    }
    strcpy(path, wobbleDir7);
    strcat(path, "wxlintr.mpg");
    PlayMovie(path);
    InitText();
    InitGameData(gameData);
    if (noSavedConfig) SaveConfig();
    ownShip = 9;
    savedAttractShip = 9;
    gameData->shipClass = 0;
    gameType = 99;
    menuTimeout = 3600;
    raceContext = 1;
    savedAttractActTrack = gameData->actTrackNum;
    savedAttractTrack = gameData->trackNum;
    continuesLeft = 0;
    InitResident();
    InitMenuFills();
    strcpy(path, wobbleDir3);
    strcat(path, "chrome.tim");
    LoadTexture(path, 1);
    DrawSync(0);
    InitShufflePlay();
    cdMusicIndex = 0;
    cdMusicTrack = cdMusicTable[0];
    raceInProgress = 0;
    InitDefaultSlot();
    menuQuit = 1;
    fill[0] = BlockFill(0, 0, 320, 240, 0, 0, 46);
    fill[1] = BlockFill(0, 256, 320, 240, 0, 0, 46);
    for (i = 0; i < 2; i++) {
        AddFill(0, (Tile *)fill[CurrentScreen]);
        RaceFrameStart();
    }
    strcpy(path, wobbleDir4);
    if (texture16Bit == 0) {
        strcat(path, "menuhf.bmp");
        LoadBitmapTextures(0, path, 11, 11);
        if (sillyShips) {
            strcpy(path, wobbleDir4);
            strcat(path, "sillyshp.bmp");
            LoadBitmapTextures(0, path, 13, 2);
            strcpy(path, wobbleDir4);
            strcat(path, "sillshad.bmp");
            LoadBitmapTextures(0, path, 15, 2);
        }
        strcpy(path, wobbleDir4);
        strcat(path, "menukey.bmp");
        LoadBitmapTextures(0, path, 6, 2);
        strcpy(path, wobbleDir4);
        strcat(path, "testmenu.bmp");
        LoadBitmapTextures(0, path, 1, 3);
        if (mmxAvailable) {
            strcpy(path, wobbleDir4);
            strcat(path, "hud2.ppm");
            textureSurfaces[3] = LoadPPMSurface(path, 3);
            strcpy(path, wobbleDir4);
            strcat(path, "xllogo.ppm");
            textureSurfaces[4] = LoadPPMSurface(path, 4);
            strcpy(path, wobbleDir4);
            strcat(path, "yfont.ppm");
            textureSurfaces[5] = LoadPPMSurface(path, 5);
            strcpy(path, wobbleDir4);
            strcat(path, "testmenu.ppm");
            textureSurfaces[6] = LoadPPMSurface(path, 6);
        } else {
            strcpy(path, wobbleDir4);
            strcat(path, "hud2.bmp");
            LoadBitmapTextures(0, path, 22, 3);
            strcpy(path, wobbleDir4);
            strcat(path, "xllogo.bmp");
            LoadBitmapTextures(0, path, 25, 1);
            strcpy(path, wobbleDir4);
            strcat(path, "yfont.bmp");
            LoadBitmapTextures(0, path, 26, 1);
        }
    } else {
        strcpy(path, wobbleDir4);
        strcat(path, "menuhf.tga");
        Load16BitBitmapTextures(0, path, 11, 11);
        if (sillyShips) {
            strcpy(path, wobbleDir4);
            strcat(path, "sillyshp.tga");
            Load16BitBitmapTextures(0, path, 13, 2);
            strcpy(path, wobbleDir4);
            strcat(path, "sillshad.tga");
            Load16BitBitmapTextures(0, path, 15, 2);
        }
        strcpy(path, wobbleDir4);
        strcat(path, "menukey.tga");
        Load16BitBitmapTextures(0, path, 6, 2);
        strcpy(path, wobbleDir4);
        strcat(path, "backgrnd.tga");
        Load16BitBitmapTextures(0, path, 0, 1);
        strcpy(path, wobbleDir4);
        strcat(path, "testmenu.tga");
        Load16BitBitmapTextures(0, path, 1, 3);
        strcpy(path, wobbleDir4);
        strcat(path, "hud2.tga");
        Load16BitBitmapTextures(0, path, 22, 3);
        strcpy(path, wobbleDir4);
        strcat(path, "xllogo.tga");
        Load16BitBitmapTextures(0, path, 25, 1);
    }
    do {
        while (titleResult != 1 && quitRequested != 1 && linkMyState != 1) {
            if (menuQuit) {
                TitleScreen();
                if (titleResult == 3) {
                    attractCycles++;
                    if (attractCycles > 2) {
                        strcpy(path, wobbleDir7);
                        strcat(path, "wxlintr.mpg");
                        PlayMovie(path);
                        attractCycles = 0;
                    } else {
                        raceContext = 1;
                        gameType = 0;
                        flipView = 1;
                        if (gameData->rapierComplete) ownShip = newRand() % 14;
                        else ownShip = newRand() % 11;
                        trackNo = newRand() % 6;
                        gameData->actTrackNum = raceTracks[(signed char)trackNo];
                        gameData->trackNum = trackNo;
                        StartRace();
                    }
                }
            } else {
                savedTextureTableCount = TextureTableCount;
                quitRequested = 0;
                SfxReverbOff();
                MenuControl(gameData, menuTimeout);
                SfxReverbOn();
                TextureTableCount = savedTextureTableCount;
                menuQuit = 1;
                if (quitRequested != 3) menuQuit = 0;
                if (quitRequested != 3 && quitRequested != 1) {
                    raceContext = 0;
                    do {
                        StartRace();
                    } while (linkMyState == 4);
                }
            }
        }
        quadIndex = CurrentScreen * 300;
    } while (!RaceFrameStart());
    SaveConfig();
    SfxOff();
    if (!rgbTextures) ClearBackBufferRect(0);
    return 0;
}
#endif
#endif

#ifdef NETPLAY
void LoadTitleScreen(void);
/* The title screen: shows the title picture and waits for Enter, or for the attract timeout. The network
   build keeps it up for as long as it likes and does not start the CD music. */
void TitleScreen(void)
{
    int frame;
    int delay;
    int k;

    frame = 0;
    if (titleSeen == 0)
        delay = 0x1E;
    else
        delay = 5;
    heap = mainheapcopy;
    LoadTitleScreen();
    if (!rgbTextures) {
        ddResult = primarySurface->lpVtbl->SetPalette(primarySurface, gamePalette);
        ddResult = backBuffer->lpVtbl->SetPalette(backBuffer, gamePalette);
    }
    if (!networkGame) {
        if (titleSeen) {
            if (cdMusicCount > 1)
                cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
            else
                cdMusicIndex = 0;
            cdMusicTrack = cdMusicTable[cdMusicIndex];
        } else {
            if (cdMusicCount > 1)
                cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
            else
                cdMusicIndex = 0;
            cdMusicTrack = cdMusicTable[cdMusicIndex];
            if (gameData->cdTrack == 1)
                CdMusicPlay(cdMusicTrack, 0);
            else
                CdMusicPlay(gameData->cdTrack, 0);
        }
    }
    DrawSync(0);
    titleResult = 0;
    while (titleResult == 0) {
        PollKeyboard();
        quadIndex = CurrentScreen * 300;
        if (delay * 2 * 15 > frame || networkGame) {
            frame++;
            if (frame > 10 && GetMenuKey(5)) {
                titleResult = 2;
                menuQuit = 0;
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                for (k = 0; k < 10; k++) {
                    ((long (*)(long))iso_read)(0);
                    UpdateNotes(0);
                    raceContext = 0;
                }
            }
        } else if (!networkGame) {
            titleResult = 3;
            raceContext = 1;
            menuQuit = 1;
        }
        AddBlit320(1, 0, 0, 160, 239, 0, 0, 160, 239, 0);
        AddBlit320(1, 160, 0, 319, 219, 160, 0, 319, 219, 0);
        if (blinkFlag) {
            if (!rgbTextures)
                AddBlit320(0, 160, 220, 287, 228, 0, 0, 127, 8, 2);
            else
                AddText("PRESS ENTER", 160, 220, 8, 0);
        }
        RaceFrameStart();
    }
    if (!rgbTextures) {
        ClearScreens();
        SetDefaultPalette();
        pressSurface->lpVtbl->Release(pressSurface);
        pressLoaded = 0;
    }
    textureSurfaces[0]->lpVtbl->Release(textureSurfaces[0]);
    titleLoaded = 0;
    titleSeen = 1;
}

#else
/* The title screen: shows the title picture and waits for Enter, or for the attract timeout. */
void TitleScreen(void)
{
    char path[256];
    int limit;
    int k;
    int delay;
    short i;
    int frame;

    frame = 0;
    delay = (titleSeen == 0 ? 0x19 : 0) + 5;
    heap = mainheapcopy;
    strcpy(path, wobbleDir5);
    if (doubleSourceRects)
        strcat(path, "HI-RES\\");
    else
        strcat(path, "LO-RES\\");
    if (rgbTextures) {
        strcat(path, "RSTARTXL.ppm");
        textureSurfaces[0] = LoadPPMSurface(path, 0);
    } else {
        char pressPath[256];

        strcpy(pressPath, path);
        strcat(path, "RSTARTXL.bmp");
        strcat(pressPath, "PRESSXL.bmp");
        pressSurface = LoadBMPSurface(pressPath, 2, 1);
        textureSurfaces[0] = LoadBMPSurface(path, 0, 1);
    }
    strcat(path, "\n");
    if (!rgbTextures) {
        ddResult = primarySurface->lpVtbl->SetPalette(primarySurface, gamePalette);
        ddResult = backBuffer->lpVtbl->SetPalette(backBuffer, gamePalette);
    }
    if (titleSeen) {
        if (cdMusicCount > 1)
            cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
        else
            cdMusicIndex = 0;
        cdMusicTrack = cdMusicTable[cdMusicIndex];
    } else {
        if (cdMusicCount > 1)
            cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
        else
            cdMusicIndex = 0;
        cdMusicTrack = cdMusicTable[cdMusicIndex];
        if (gameData->cdTrack == 1)
            CdMusicPlay(cdMusicTrack, 0);
        else
            CdMusicPlay(gameData->cdTrack, 0);
    }
    DrawSync(0);
    limit = delay * 2 * 15;
    titleResult = 0;
    do {
        PollKeyboard();
        quadIndex = CurrentScreen * 300;
        if (frame < limit) {
            frame++;
            if (frame > 10 && GetMenuKey(5)) {
                titleResult = 2;
                menuQuit = 0;
                PlayNote(SAMPLE_MENU_SELECT, 255, 15, 15, 256);
                for (k = 10; k != 0; k--) {
                    ((long (*)(long))iso_read)(0);
                    UpdateNotes(0);
                    raceContext = 0;
                }
            }
        } else {
            titleResult = 3;
            raceContext = 1;
            menuQuit = 1;
        }
        AddBlit320(1, 0, 0, 160, 239, 0, 0, 160, 239, 0);
        AddBlit320(1, 160, 0, 319, 219, 160, 0, 319, 219, 0);
        if (blinkFlag) {
            if (!rgbTextures)
                AddBlit320(0, 160, 220, 287, 228, 0, 0, 127, 8, 2);
            else
                AddText("PRESS ENTER", 160, 220, 8, 0);
        }
        RaceFrameStart();
    } while (titleResult == 0);
    if (!rgbTextures) {
        BLK_FILL *fill[2];

        fill[0] = BlockFill(0, 0, 320, 240, 0, 0, 46);
        fill[1] = BlockFill(0, 256, 320, 240, 0, 0, 46);
        for (i = 2; i != 0; i--) {
            AddFill(0, (Tile *)fill[CurrentScreen]);
            RaceFrameStart();
        }
        SetDefaultPalette();
        pressSurface->lpVtbl->Release(pressSurface);
        pressLoaded = 0;
    }
    textureSurfaces[0]->lpVtbl->Release(textureSurfaces[0]);
    titleLoaded = 0;
    titleSeen = 1;
}

#endif

/* Runs the front-end menus; menuQuit tells whether they were left with a quit request. */
void RunMenus(void)
{
    quitRequested = 0;
    savedTextureTableCount = TextureTableCount;
    SfxReverbOff();
    MenuControl(gameData, menuTimeout);
    SfxReverbOn();
    TextureTableCount = savedTextureTableCount;
    if (quitRequested == 3)
        menuQuit = 1;
    else
        menuQuit = 0;
}

extern char startPath[];
extern short textureX, textureY;
extern int attractDelayFrames;
extern int linkSendFlag;
extern short raceFinished;
extern short sixTracks, sixXPlayed, eightTracks, eightXPlayed, phantomDone, piranhaDone;

void CdMusicStop(void);


/* Matches the executable up to code layout (EQUIV). In this build MSVC merges the class-specific
   track lookup into a shared tail. */
#ifdef EQUIV
/* Starts the race, plays the movies for newly won championships and reloads the menu pictures. */
void StartRace(void)
{
#ifdef NETPLAY
    textureX = 320;
    textureY = 0;
    attractDelayFrames = 0;
    linkSendFlag = 0;
    savedLapTimes[0] = 0;
    savedLapTimes[1] = 0;
    savedLapTimes[2] = 0;
    savedLapTimes[3] = 0;
    savedLapTimes[4] = 0;
    if (gameType == 2 && gameData->piranhaChallenge) {
        gameData->actTrackNum = raceTracks[gameData->class2Race];
        gameData->trackNum = gameData->class2Race;
        gameData->shipClass = 3;
    } else if (gameType == 2 && gameData->phantomChallenge) {
        gameData->actTrackNum = raceTracks[gameData->class1Race];
        gameData->trackNum = gameData->class1Race;
        gameData->shipClass = 2;
    }
    trackNo = gameData->actTrackNum;
    if (!networkGame)
        CdMusicStop();
    linkMyState = 0;
    menuQuit = 0;
    race(gameData);
    raceFinished = 0;
#else
    int selectedTrack;

    textureY = 0;
    attractDelayFrames = 0;
    linkSendFlag = 0;
    savedLapTimes[0] = 0;
    savedLapTimes[1] = 0;
    savedLapTimes[2] = 0;
    savedLapTimes[3] = 0;
    savedLapTimes[4] = 0;
    textureX = 320;
#ifdef POWERVR
    if (gameType == 2 && gameData->piranhaChallenge != 0) {
        gameData->shipClass = 3;
        selectedTrack = gameData->class2Race;
        gameData->trackNum = selectedTrack;
        gameData->actTrackNum = raceTracks[selectedTrack];
    } else if (gameType == 2 && gameData->phantomChallenge != 0) {
        gameData->shipClass = 2;
        selectedTrack = gameData->class1Race;
        gameData->trackNum = selectedTrack;
        gameData->actTrackNum = raceTracks[selectedTrack];
    }
#else
    if (gameType == 2 && gameData->rapierComplete != 0) {
        gameData->shipClass = 3;
        selectedTrack = gameData->class2Race;
        gameData->trackNum = selectedTrack;
        gameData->actTrackNum = raceTracks[selectedTrack];
    } else if (gameType == 2 && gameData->venomComplete != 0) {
        gameData->shipClass = 2;
        selectedTrack = gameData->class1Race;
        gameData->trackNum = selectedTrack;
        gameData->actTrackNum = raceTracks[selectedTrack];
    }
#endif
    trackNo = gameData->actTrackNum;
    CdMusicStop();
    linkMyState = 0;
    menuQuit = 0;
    ((void (*)(ConfigData *))race)(gameData);
    raceFinished = 0;
#endif
    if (sixTracks != 0 && sixXPlayed == 0) {
        strcpy(startPath, wobbleDir7);
        strcat(startPath, "xtro1.mpg");
        PlayMovie(startPath);
        sixTracks = 0;
        sixXPlayed = 1;
    } else if (eightTracks != 0 && eightXPlayed == 0) {
        strcpy(startPath, wobbleDir7);
        strcat(startPath, "xtro3.mpg");
        PlayMovie(startPath);
        eightTracks = 0;
        eightXPlayed = 1;
    } else if (phantomDone != 0) {
        strcpy(startPath, wobbleDir7);
        strcat(startPath, "xtro2.mpg");
        PlayMovie(startPath);
        phantomDone = 0;
    } else if (piranhaDone != 0) {
        strcpy(startPath, wobbleDir7);
        strcat(startPath, "xtro4.mpg");
        PlayMovie(startPath);
        piranhaDone = 0;
    }
    strcpy(startPath, wobbleDir4);
    if (texture16Bit == 0) {
        strcat(startPath, "menuhf.bmp");
        LoadBitmapTextures(0, startPath, 11, 11);
        if (sillyShips) {
            strcpy(startPath, wobbleDir4);
            strcat(startPath, "sillyshp.bmp");
            LoadBitmapTextures(0, startPath, 13, 2);
            strcpy(startPath, wobbleDir4);
            strcat(startPath, "sillshad.bmp");
            LoadBitmapTextures(0, startPath, 15, 2);
        }
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "menukey.bmp");
        LoadBitmapTextures(0, startPath, 6, 2);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "testmenu.bmp");
        LoadBitmapTextures(0, startPath, 1, 3);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "hud2.bmp");
        LoadBitmapTextures(0, startPath, 22, 3);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "xllogo.bmp");
        LoadBitmapTextures(0, startPath, 25, 1);
    } else {
        strcat(startPath, "menukey.tga");
        Load16BitBitmapTextures(0, startPath, 6, 2);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "backgrnd.tga");
        Load16BitBitmapTextures(0, startPath, 0, 1);
        strcpy(startPath, wobbleDir4);
        strcat(startPath, "testmenu.tga");
        Load16BitBitmapTextures(0, startPath, 1, 3);
    }
    if (linkMyState == 2)
        menuQuit = 1;
    racetidy();
}
#endif

/* Shows the copyright picture for up to ten seconds, or until a key is pressed. */
void ShowCopyrightScreen(void)
{
    char fullPath[256];
    int i;

    strcpy(fullPath, wobbleDir5);
    if (doubleSourceRects)
        strcat(fullPath, "HI-RES\\");
    else
        strcat(fullPath, "LO-RES\\");
    if (rgbTextures) {
        strcat(fullPath, "woxlcopy.ppm");
        copyrightSurface = LoadPPMSurface(fullPath, 1);
    } else {
        strcat(fullPath, "woxlcopy.bmp");
        copyrightSurface = LoadBMPSurface(fullPath, 1, 0);
        SetDefaultPalette();
    }
    strcat(fullPath, "\n");
    for (i = 0; i < 300; i++) {
        PollKeyboard();
        AddBlit320(1, 0, 0, 319, 239, 0, 0, 319, 239, 1);
        RaceFrameStart();
        if (GetMenuKey(5) || g_keyboardCode == 0x39 /* DIK_SPACE */) {
            g_keyboardCode = -1;
            break;
        }
    }
    copyrightSurface->lpVtbl->Release(copyrightSurface);
    copyrightShown = 0;
}

/* Loads the title picture, and the press-start picture when the screen is not 16-bit. */
void LoadTitleScreen(void)
{
    char fullPath[256];
    char pressPath[256];

    strcpy(fullPath, wobbleDir5);
    if (doubleSourceRects)
        strcat(fullPath, "HI-RES\\");
    else
        strcat(fullPath, "LO-RES\\");
    if (rgbTextures) {
        strcat(fullPath, "RSTARTXL.ppm");
        textureSurfaces[0] = LoadPPMSurface(fullPath, 0);
    } else {
        strcpy(pressPath, fullPath);
        strcat(fullPath, "RSTARTXL.bmp");
        strcat(pressPath, "PRESSXL.bmp");
        pressSurface = LoadBMPSurface(pressPath, 2, 1);
        textureSurfaces[0] = LoadBMPSurface(fullPath, 0, 1);
    }
    strcat(fullPath, "\n");
}

/* Clears both frame buffers to black behind the loading screen. */
void ClearScreens(void)
{
    BLK_FILL *fill[2];
    short i;

    fill[0] = BlockFill(0, 0, 320, 240, 0, 0, 46);
    fill[1] = BlockFill(0, 256, 320, 240, 0, 0, 46);
    for (i = 0; i < 2; i++) {
        AddFill(0, (Tile *)fill[CurrentScreen]);
        RaceFrameStart();
    }
}
