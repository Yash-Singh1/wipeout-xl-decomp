#include "ayrace.h"
#include "menus.h"
#include "pad.h"
#include "pc_particles.h"
#include <string.h>

#ifdef NETPLAY
extern signed char networkGame;
#endif

/* Starts the selected CD track during a race when playback changes. */
void SetInGameCDTrack(void *unused1, void *unused2, RaceConfig *config, int *state)
{
    MciStatus status;
    int result;
    int trackNumber = 0;
    status.item = 8;
    result = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_NOTIFY, (DWORD)&status);
    if (result != 0)
        return;
    trackNumber = status.returnValue;
    if (config->cdTrack == 1) {
        if (cdMusicTrack != trackNumber && *state == 0) {
            cdMusicTrack = cdMusicTable[cdMusicIndex];
            trackNameScroll = 480;
            OutputDebugString(msgRandomPlay);
            CdMusicPlay(cdMusicTrack, 0);
            *state = 1;
        } else if (cdMusicTrack == trackNumber && *state == 1) {
            *state = 0;
        }
    } else {
        if (trackNumber != config->cdTrack && *state == 0) {
            cdMusicTrack = config->cdTrack;
            OutputDebugString(msgStaticPlay);
            CdMusicPlay(cdMusicTrack, 0);
            *state = 1;
        } else if (trackNumber == config->cdTrack && *state == 1) {
            *state = 0;
        }
    }
}


/* Starts the selected CD track while the game is in a menu. */
void SetMenuCDTrack(void *unused1, void *unused2, RaceConfig *config)
{
    MciStatus status;
    int result;
    int trackNumber = 0;
    status.item = 8;
    result = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_NOTIFY, (DWORD)&status);
    if (result != 0)
        return;
    trackNumber = status.returnValue;
    if (config->cdTrack == 1) {
        if (cdMusicTrack != trackNumber) {
            cdMusicTrack = cdMusicTable[cdMusicIndex];
            trackNameScroll = 480;
            OutputDebugString(msgRandomPlayMenu);
            CdMusicPlay(cdMusicTrack, 0);
        }
    } else {
        if (config->cdTrack != trackNumber) {
            cdMusicTrack = config->cdTrack;
            OutputDebugString(msgStaticPlayMenu);
            CdMusicPlay(cdMusicTrack, 0);
        }
    }
}

/* Switches the player ship to attract mode after inactivity or destruction. */
void CheckShipDestroyed(ShipData *ships, RaceConfig *config, int *unused, int idleLimit)
{
    if (idleTimer >= idleLimit) {
        if (linkSendFlag != 0 && raceOutcome == 2 && gameType != 2) {
            linkSendFlag = 0;
            config->complete = 0;
        } else if ((ships[ownShip].attr & RACING) &&
                   !(ships[ownShip].combatAttr & DESTROYED)) {
            ships[ownShip].update = UpdateDroneShip;
            ships[ownShip].attr &= ~VIEW_IN;
            ships[ownShip].attr &= ~RACING;
            CameraUpdate = UpdateCameraAttractMode;
            humanToDroneSpeed(ships, ownShip);
            attractDelayFrames = 900;
            raceContext = 1;
            idleTimer = 0;
        }
    }
    if ((ships[ownShip].combatAttr & DESTROYED) && (ships[ownShip].attr & RACING)) {
        ships[ownShip].attr &= ~RACING;
        ships[ownShip].attr &= ~VIEW_IN;
        if (!(ships[ownShip].combatAttr & 4)) CameraUpdate = UpdateCameraAttractDestroyed;
        config->raceInfo = 1;
        linkSendFlag = 1;
        attractDelayFrames = 1;
        raceOutcome = 2;
        finishPosition = 15;
        copyInTempLapTimes(ships);
    }
    if (droneSwitchStep == 1) {
        droneSwitchStep++;
        linkSendFlag = 1;
        config->complete = 1;
        attractDelayFrames = 1;
        raceOutcome = 2;
        finishPosition = 15;
        if (ships[ownShip].update == UpdatePlayerShipStart)
            ships[ownShip].update = UpdateDroneShipStart;
        else
            ships[ownShip].update = UpdateDroneShip;
        ships[ownShip].attr &= ~VIEW_IN;
        ships[ownShip].attr &= ~RACING;
        CameraUpdate = UpdateCameraAttractMode;
        humanToDroneSpeed(ships, ownShip);
    }
}


/* Updates the idle timer while the player leaves the controls untouched. */
void CheckForButtonPress(void)
{
    UpdateNotes(1);
    if (raceContext != 1) {
        if (menuHeld != 0 || steerAnalog > 135 || steerAnalog < 122) idleTimer = 0;
        else idleTimer++;
    }
}

/* Shows the attract mode credits or the demo mode notice. */
void PlayCredits(int attract)
{
    if (attract == 1) {
        if (attractModeFlag == 1) ShowCredits(attractDelayFrames);
        else if (!(shipIndex[ownShip].combatAttr & DESTROYED))
            CentreText(msgDemoMode, 60, 20, 8, 1);
    }
}

#pragma optimize("a", off)
#pragma optimize("w", on)
/* Resets race state and reinitializes the race objects. */
void Restart(struct ResetData *reset)
{
    int i;
    unsigned char mode[8];
    checkTimer = 0;
    for (i = 0; i < 6; i++) reset->checkPoints[i].reached = -1;
    TerminateSound();
    StartControlledSound();
    SetShieldHum(0);
    DroidSound(0);
    SetPitAmbient(0);
    SetAmbientLoop3(0);
    CrowdSound(0);
    ScrapeSound(0);
    FlattenTrack();
    initShipData(track->sections, reset->ships, reset->config);
    InitRescueDroid(reset->rescue, reset->shipShapes, &reset->ships[ownShip]);
    cameraPtr->section = reset->ships[ownShip].nearTrkSect;
    ResetStartBoom();
    if (reset->config->actTrackNum == 2) ResetAnimatedObjects();
    ResetLightStart();
    for (i = 0; i < 20; i++) {
        reset->weapons[i].count = 0;
        reset->weapons[i].type = 0;
    }
    for (i = 0; i < 29; i++) reset->weaponShapes[i].used = 0;
    InitCDAudio(gameData, mode);
    for (i = 0; i < 15; i++) {
        shipDamage[i] = 0;
        fracdamage[i] = 0;
        shipDeathCount[i] = -1;
    }
    InitElectric();
    InitPanelQuads();
    CameraUpdate = UpdateCameraRaceStart;
    linkRaceDone = 0;
    raceEndFlag = 0;
    raceCompleteFlag = 0;
    linkSendFlag = 0;
    raceEndState = 0;
    raceOutcome = 0;
    hudDisabled = 0;
    plasmaShotBusy = 0;
    perfectLapCount = 0;
    perfectLap = 0;
    droneSwitchStep = 0;
    continueAnimFrame = 0;
    idleTimer = 0;
    trackNameScroll = 480;
}
#pragma optimize("w", off)
#pragma optimize("a", on)

/* Stops race audio and clears the race state before leaving. */
void QuitGame(void)
{
#ifdef NETPLAY
    if (!networkGame && gameType != 2) gameType = 99;
#else
    if (gameType != 2) gameType = 99;
#endif
    SfxSetVolume(0);
    SfxSetCdVolume(0);
    KillControlledSound();
    fadeAmount = 0;
    cheatfade = 0;
    SetShieldHum(0);
    DroidSound(0);
    SetPitAmbient(0);
    SetAmbientLoop3(0);
    CrowdSound(0);
    ScrapeSound(0);
#ifdef NETPLAY
    if (!networkGame)
        CdStop();
#else
    CdStop();
#endif
#ifdef NETPLAY
    ClearBackBufferRect(0x61);
#else
    ClearBackBuffer(0x61);
#endif
}


/* Keep aliased race-state reloads across the input callbacks. */
#pragma optimize("a", off)
#pragma optimize("w", on)
/* Updates challenge progress and race completion state. */
/* NET adds networkGame handling to challenge and race completion. */
#ifdef NETPLAY
void UpdateRaceComplete(void *screen, void *unused, RaceConfig *config, short *choice, ScoreData *score, ShipData *ships)
{
    char text[32];

    if (config->complete) {
        if (((gameType == 2 && config->phantomChallenge != 0) ||
             (gameType == 2 && config->piranhaChallenge != 0)) &&
            (finishPosition == 1 || finishPosition >= 4)) {
            if (finishPosition == 1) {
                if (config->phantomChallenge == 1 && config->phantomCircuit == 5) {
                    config->phantomUnlocked = 1;
                    phantomDone = 1;
                    raceEndState = 2;
                    linkMyState = 3;
                    gameType = 99;
                } else if (config->piranhaChallenge == 1 && config->piranhaCircuit == 7) {
                    config->piranhaUnlocked = 1;
                    piranhaDone = 1;
                    raceEndState = 2;
                    linkMyState = 3;
                    gameType = 99;
                } else {
                    if (config->phantomChallenge == 1) circuitCount = config->phantomCircuit;
                    else if (config->piranhaChallenge == 1) circuitCount = config->piranhaCircuit;
                    strcpy(text, circuitCountText[circuitCount]);
                    if (GetMenuKey(0)) {
                        PlayNote(8, 255, 15, 15, 256);
                        (*choice)--;
                        if (*choice < 0) *choice = 0;
                    }
                    if (GetMenuKey(1)) {
                        PlayNote(8, 255, 15, 15, 256);
                        (*choice)++;
                        if (*choice > 1) *choice = 1;
                    }
                    if (GetMenuKey(5)) {
                        PlayNote(9, 255, 15, 15, 256);
                        config->complete = 0;
                        linkSendFlag = 0;
                        if (*choice == 0) {
                            if (config->phantomChallenge == 1) config->phantomCircuit++;
                            else config->piranhaCircuit++;
                            raceEndState = 2;
                            linkMyState = 4;
                        } else if (*choice == 1) {
                            EndChallenge(config);
                            raceEndState = 2;
                            linkMyState = 3;
                        }
                    }
                }
            } else {
                if (continuesLeft > 0) {
                    if (continueAnimFrame == 0) {
                        if (GetMenuKey(0)) {
                            PlayNote(8, 255, 15, 15, 256);
                            (*choice)--;
                            if (*choice < 0) *choice = 0;
                        }
                        if (GetMenuKey(1)) {
                            PlayNote(8, 255, 15, 15, 256);
                            (*choice)++;
                            if (*choice > 1) *choice = 1;
                        }
                    }
                } else {
                }
                if (continueAnimFrame == 0) {
                    if (GetMenuKey(5)) {
                        PlayNote(9, 255, 15, 15, 256);
                        if (*choice == 0 && continuesLeft > 0) {
                            continueAnimFrame = 1;
                            PlayNote(36, 255, 15, 15, 320);
                        } else if (*choice == 1 || continuesLeft == 0) {
                            config->complete = 0;
                            linkSendFlag = 0;
                            raceEndState = 2;
                            linkMyState = 3;
                            EndChallenge(config);
                        }
                    }
                } else if (continueAnimFrame == 30) {
                    config->complete = 0;
                    linkSendFlag = 0;
                    linkRaceDone = 1;
                    continuesLeft--;
                } else continueAnimFrame++;
            }
        } else if ((sixTracks == 1 && sixXPlayed == 0) || (eightTracks == 1 && eightXPlayed == 0)) {
            raceEndState = 1;
            linkSendFlag = 0;
            linkMyState = 3;
        } else {
            if (GetMenuKey(0)) {
                PlayNote(8, 255, 15, 15, 256);
                (*choice)--;
                if (*choice < 0) *choice = 0;
            }
            if (GetMenuKey(1)) {
                PlayNote(8, 255, 15, 15, 256);
                (*choice)++;
                if (*choice > 1) *choice = 1;
            }
            if (GetMenuKey(5)) {
                PlayNote(9, 255, 15, 15, 256);
                config->complete = 0;
                linkSendFlag = 0;
                if (*choice == 0) {
                    if (!networkGame)
                        linkRaceDone = 1;
                } else if (*choice == 1) {
                    raceEndState = 2;
                    gameType = 99;
                    linkMyState = 3;
                }
            }
        }
    } else if (config->raceInfo) {
        if (GetMenuKey(5)) {
            PlayNote(9, 255, 15, 15, 256);
            config->raceInfo = 0;
            if (raceOutcome == 2) {
                config->complete = 1;
                raceCompleteFlag = 1;
            } else config->highScore = 1;
        }
    } else {
        if (config->highScore) {
            config->highScore = 0;
            CheckForHiScore(config, (struct HiScoreData *)score, ships, ownShip, 0);
            InitDefaultName(config, (struct HiScoreData *)score, 0);
            if (!score->gotHiScore) {
                raceCompleteFlag = 1;
                config->complete = 1;
            }
        }
        if (score->gotHiScore) score->displayScore = 1;
        if (score->displayScore) EnterHighScoreIfNeeded(config, (struct HiScoreData *)score);
    }
    if (config->complete) InitScoreTabData((struct HiScoreData *)score);
}
#else
void UpdateRaceComplete(void *screen, void *unused, RaceConfig *config, short *choice, ScoreData *score, ShipData *ships)
{
    char text[32];

    do {
        if (config->complete) {
            if (((gameType == 2 && config->phantomChallenge != 0) ||
                 (gameType == 2 && config->piranhaChallenge != 0)) &&
                (finishPosition == 1 || finishPosition >= 4)) {
                if (finishPosition == 1) {
                    if (config->phantomChallenge == 1 && config->phantomCircuit == 5) {
                        phantomDone = 1;
                        config->phantomUnlocked = 1;
                        raceEndState = 2;
                        linkMyState = 3;
                        gameType = 99;
                        break;
                    }
                    if (config->piranhaChallenge == 1 && config->piranhaCircuit == 7) {
                        piranhaDone = 1;
                        config->piranhaUnlocked = 1;
                        raceEndState = 2;
                        linkMyState = 3;
                        gameType = 99;
                        break;
                    }
                    if (config->phantomChallenge == 1) circuitCount = config->phantomCircuit;
                    else if (config->piranhaChallenge == 1) circuitCount = config->piranhaCircuit;
                    strcpy(text, circuitCountText[circuitCount]);
                    if (GetMenuKey(0)) {
                        PlayNote(8, 255, 15, 15, 256);
                        (*choice)--;
                        if (*choice < 0) *choice = 0;
                    }
                    if (GetMenuKey(1)) {
                        PlayNote(8, 255, 15, 15, 256);
                        (*choice)++;
                        if (*choice > 1) *choice = 1;
                    }
                    if (GetMenuKey(5)) {
                        PlayNote(9, 255, 15, 15, 256);
                        config->complete = 0;
                        linkSendFlag = 0;
                        if (*choice == 0) {
                            if (config->phantomChallenge == 1) config->phantomCircuit++;
                            else config->piranhaCircuit++;
                            raceEndState = 2;
                            linkMyState = 4;
                        } else if (*choice == 1) {
                            EndChallenge(config);
                            raceEndState = 2;
                            linkMyState = 3;
                        }
                    }
                    break;
                } else {
                    if (continuesLeft > 0 && continueAnimFrame == 0) {
                        if (GetMenuKey(0)) {
                            PlayNote(8, 255, 15, 15, 256);
                            (*choice)--;
                            if (*choice < 0) *choice = 0;
                        }
                        if (GetMenuKey(1)) {
                            PlayNote(8, 255, 15, 15, 256);
                            (*choice)++;
                            if (*choice > 1) *choice = 1;
                        }
                    }
                    if (continueAnimFrame == 0) {
                        if (GetMenuKey(5)) {
                            PlayNote(9, 255, 15, 15, 256);
                            if (*choice == 0 && continuesLeft > 0) {
                                continueAnimFrame = 1;
                                PlayNote(36, 255, 15, 15, 320);
                            } else if (*choice == 1 || continuesLeft == 0) {
                                linkSendFlag = 0;
                                raceEndState = 2;
                                linkMyState = 3;
                                config->complete = 0;
                                EndChallenge(config);
                            }
                        }
                    } else if (continueAnimFrame == 30) {
                        linkSendFlag = 0;
                        linkRaceDone = 1;
                        config->complete = 0;
                        continuesLeft--;
                    } else continueAnimFrame++;
                    break;
                }
            }
            if ((sixTracks == 1 && sixXPlayed == 0) || (eightTracks == 1 && eightXPlayed == 0)) {
                raceEndState = 1;
                linkSendFlag = 0;
                linkMyState = 3;
                break;
            }
            if (GetMenuKey(0)) {
                PlayNote(8, 255, 15, 15, 256);
                (*choice)--;
                if (*choice < 0) *choice = 0;
            }
            if (GetMenuKey(1)) {
                PlayNote(8, 255, 15, 15, 256);
                (*choice)++;
                if (*choice > 1) *choice = 1;
            }
            if (GetMenuKey(5)) {
                PlayNote(9, 255, 15, 15, 256);
                config->complete = 0;
                linkSendFlag = 0;
                if (*choice == 0) linkRaceDone = 1;
                else if (*choice == 1) {
                    raceEndState = 2;
                    gameType = 99;
                    linkMyState = 3;
                }
            }
        } else if (config->raceInfo) {
            if (GetMenuKey(5)) {
                PlayNote(9, 255, 15, 15, 256);
                config->raceInfo = 0;
                if (raceOutcome == 2) {
                    raceCompleteFlag = 1;
                    config->complete = 1;
                } else config->highScore = 1;
            }
        } else {
            if (config->highScore) {
                config->highScore = 0;
                CheckForHiScore(config, (struct HiScoreData *)score, ships, ownShip, 0);
                InitDefaultName(config, (struct HiScoreData *)score, 0);
                if (!score->gotHiScore) {
                    raceCompleteFlag = 1;
                    config->complete = 1;
                }
            }
            if (score->gotHiScore) score->displayScore = 1;
            if (score->displayScore) EnterHighScoreIfNeeded(config, (struct HiScoreData *)score);
        }
    } while (0);
    if (config->complete) InitScoreTabData((struct HiScoreData *)score);
}
#endif
#pragma optimize("w", off)
#pragma optimize("a", on)



/* Displays the race results and handles the result menu. */
void RaceComplete(void *screen, void *unused, RaceConfig *config, short *choice, ScoreData *score, ShipData *ships)
{
    char text[32];
    if (config->complete) {
        if ((gameType == 2 && config->phantomChallenge) ||
            (gameType == 2 && config->piranhaChallenge)) {
            if (finishPosition == 1 || finishPosition >= 4) {
                if (finishPosition == 1) {
                    if (config->phantomChallenge == 1 && config->phantomCircuit == 5) goto challengeDone;
                    if (config->piranhaChallenge == 1 && config->piranhaCircuit == 7) goto challengeDone;
                    if (config->phantomChallenge == 1)
                        CentreText("PHANTOM CHALLENGE", 60, 36, 8, 0);
                    else if (config->piranhaChallenge == 1)
                        CentreText("PIRANHA CHALLENGE", 60, 36, 8, 0);
                    strcpy(text, circuitCountText[circuitCount]);
                    if (circuitCount == 0) strcat(text, " CIRCUIT COMPLETE");
                    else strcat(text, " CIRCUITS COMPLETED");
                    CentreText(text, 60, 50, 8, 0);
                    if (*choice == 0) {
                        CentreText("NEXT CIRCUIT", 60, 110, 8, 1);
                        CentreText("END CHALLENGE", 60, 124, 8, 0);
                    } else if (*choice == 1) {
                        CentreText("NEXT CIRCUIT", 60, 110, 8, 0);
                        CentreText("END CHALLENGE", 60, 124, 8, 1);
                    }
                    DoButton(133, 210, 2, "SELECT");
                    DeSelectScreen(screen);
                challengeDone:;
                } else {
                    CentreText("YOU DID NOT QUALIFY", 60, 36, 8, 0);
                    DoButton(133, 210, 2, "SELECT");
                    if (continuesLeft > 0) {
                        DisplayContinues(176, 168);
                        if (*choice == 0) {
                            CentreText("CONTINUE GAME", 60, 110, 8, 1);
                            CentreText("GO TO MENU", 60, 124, 8, 0);
                        } else if (*choice == 1) {
                            CentreText("CONTINUE GAME", 60, 110, 8, 0);
                            CentreText("GO TO MENU", 60, 124, 8, 1);
                        }
                    } else CentreText("GO TO MENU", 60, 124, 8, 1);
                    DeSelectScreen(screen);
                }
                goto completeDone;
            }
        }
        if ((sixTracks == 1 && sixXPlayed == 0) || (eightTracks == 1 && eightXPlayed == 0)) goto completeDone;
        if (gameType == 2 || 0) {
            CentreText("MEDAL ACHIEVED", 60, 36, 8, 0);
            CentreText("FREE CONTINUE AWARDED", 60, 50, 8, 0);
        }
        if (*choice == 0) {
            CentreText("REPLAY GAME", 60, 110, 8, 1);
            CentreText("GO TO MENU", 60, 124, 8, 0);
        } else if (*choice == 1) {
            CentreText("REPLAY GAME", 60, 110, 8, 0);
            CentreText("GO TO MENU", 60, 124, 8, 1);
        }
        DoButton(133, 210, 2, "SELECT");
        DeSelectScreen(screen);
    completeDone:;
    } else if (config->raceInfo) {
        DisplayRaceInfo(config, score, ships);
        if (!(raceOutcome == 2 && shipDeathCount[ownShip] > 4))
            DeSelectScreen(screen);
    } else if (score->displayScore) {
        DisplayScoreTab(config, score);
        DeSelectScreen(screen);
    }
}
#include "pc.h"
#include "psyq.h"
MATRIX *GetSkeletonMatrix(Skeleton *skeleton);
#include <string.h>
#include "d3d.h"
#include "render.h"



extern int pauseRequested;


extern u_short menuKeys;
extern short savedLoop0, savedLoop1, savedLoop2, savedLoop3, savedLoop4, savedLoop5;
extern int oneMoreTime;
extern signed char audioRow;



extern int quitRequested;










extern RenderEntry renderQueue[];
extern int renderCount;
extern int rgbTextures;
extern int mmxAvailable;
extern float textDepth;

extern short loop3;


void UpdateNotes(int arg);
void UpdateNotes(int arg);
void CdMusicPlay(int track, int flags);
void InGameReset(PcGameData *config, struct SelectBox *box);
void DrawPauseOptions(PcGameData *config, struct SelectBox *box);
void DrawMenuBox(short x0, short y0, short x1, short y1, float depthFill, float depthBorder);
void UpdateMusicBar(PcGameData *config);
void ConvertMatrix(float *out, MATRIX *m);
void UpdateSfxBar(PcGameData *config);
extern char *cdTrackNames[];
extern char *cdTrackArtists[];

void DrawScaledTexturedQuad(float depth, POLY_FT4 *poly, int flags);

/* Restarts ambient loops after leaving the pause menu. */
void RestoreAmbientLoops(void)
{
    if (savedLoop0 == 1)
        SetShieldHum(1);
    else
        SetShieldHum(0);
    if (savedLoop1 == 1)
        DroidSound(1);
    else
        DroidSound(0);
    if (savedLoop2 == 1)
        SetPitAmbient(1);
    else
        SetPitAmbient(0);
    if (savedLoop3 == 1)
        SetAmbientLoop3(1);
    else
        SetAmbientLoop3(0);
    if (savedLoop4 == 1)
        CrowdSound(1);
    else
        CrowdSound(0);
    if (savedLoop5 == 1)
        ScrapeSound(1);
    else
        ScrapeSound(0);
}

/* Returns the display name for a CD track. */
char *GetCdTrackName(int track)
{
    return cdTrackNames[track];
}

/* Returns the display artist for a CD track. */
char *GetCdTrackArtist(int track)
{
    return cdTrackArtists[track];
}

/* Checks pause input and updates the pause state. */
void CheckGamePause(ShipData *ships, PcGameData *config)
{
    if ((ships[ownShip].attr & RACING) && pauseRequested && pause.selection == -1) {
        pause.showOptions = 1;
        config->paused = 0;
        pause.musicPaused = 0;
        pause.selection = 0;
        pause.confirmChoice = 0;
        menuKeys = 0;
        CdPause();
        savedLoop0 = shieldHumPlaying;
        savedLoop1 = droidSoundPlaying;
        savedLoop2 = pitAmbientPlaying;
        savedLoop3 = loop3;
        savedLoop4 = crowdSoundPlaying;
        savedLoop5 = scrapeSoundPlaying;
        SetShieldHum(0);
        DroidSound(0);
        SetPitAmbient(0);
        SetAmbientLoop3(0);
        CrowdSound(0);
        ScrapeSound(0);
        PlayNote(9, 255, 15, 15, 256);
    }
}

/* Applies the selected single-player pause menu command. */
void PerformPauseOperations(struct SelectBox *box, PcGameData *config)
{
    if (pause.selection > -1) {
        if (pauseRequested == 1) {
            UpdateNotes(0);
            SfxSetReverbDepth(0);
            MuteControlledSound();
            fadeAmount = 0;
            cheatfade = 0;
            if (pause.showOptions) {
                if (config->paused == 0) {
                    if (keysPressed & MENU_KEY_UP) {
                        pause.selection--;
                        if (pause.selection < 0)
                            pause.selection = 4;
                        PlayNote(8, 255, 15, 15, 256);
                    } else if (keysPressed & MENU_KEY_DOWN) {
                        pause.selection++;
                        if (pause.selection > 4)
                            pause.selection = 0;
                        PlayNote(8, 255, 15, 15, 256);
                    } else if (keysPressed & MENU_KEY_ACCEPT) {
                        if (pause.selection == 1 || pause.selection == 2 || pause.selection == 4) {
                            pause.showOptions = 0;
                            pause.confirmChoice = 0;
                        } else if (pause.selection == 0) {
                            pause.selection = -1;
                            pauseRequested = 0;
#ifdef NETPLAY
                            RestoreAmbientLoops();
#else
                            if (savedLoop0 == 1)
                                SetShieldHum(1);
                            else
                                SetShieldHum(0);
                            if (savedLoop1 == 1)
                                DroidSound(1);
                            else
                                DroidSound(0);
                            if (savedLoop2 == 1)
                                SetPitAmbient(1);
                            else
                                SetPitAmbient(0);
                            if (savedLoop3 == 1)
                                SetAmbientLoop3(1);
                            else
                                SetAmbientLoop3(0);
                            if (savedLoop4 == 1)
                                CrowdSound(1);
                            else
                                CrowdSound(0);
                            if (savedLoop5 == 1)
                                ScrapeSound(1);
                            else
                                ScrapeSound(0);
#endif
                            CdPlay();
                            oneMoreTime = 1;
                        } else {
                            config->paused = 1;
                            audioRow = 0;
                        }
                    }
                } else {
                    InGameReset(config, box);
                }
            } else {
                if ((keysPressed & MENU_KEY_LEFT) || (keysPressed & MENU_KEY_RIGHT)) {
                    pause.confirmChoice ^= 1;
                    PlayNote(8, 255, 15, 15, 256);
                }
                if (keysPressed & MENU_KEY_ACCEPT) {
                    PlayNote(9, 255, 15, 15, 256);
                    if (pause.confirmChoice == 0) {
                        pause.showOptions = 0;
                        pause.selection = -1;
                        pauseRequested = 0;
                    } else {
                        switch (pause.selection) {
                        case 1:
                            pause.selection = -1;
                            MuteControlledSound();
                            if (gameType == 2 || 0)
                                droneSwitchStep = 1;
                            else
                                linkRaceDone = 1;
                            break;
                        case 2:
                            pause.selection = -1;
                            gameType = 99;
                            raceEndState = 2;
                            linkMyState = 3;
                            SfxSetVolume(0);
                            break;
                        case 4:
#ifdef NETPLAY
                            pause.selection = -1;
                            gameType = 99;
                            raceEndState = 1;
                            quitRequested = 1;
#else
                            raceEndState = 1;
                            quitRequested = 1;
                            pause.selection = -1;
                            gameType = 99;
#endif
                            PostQuitMessage(0);
                            break;
                        }
                        pauseRequested = 0;
                        CdPlay();
                        oneMoreTime = 1;
                    }
                }
            }
        } else {
            pause.selection = -1;
#ifdef NETPLAY
            RestoreAmbientLoops();
#else
            if (savedLoop0 == 1)
                SetShieldHum(1);
            else
                SetShieldHum(0);
            if (savedLoop1 == 1)
                DroidSound(1);
            else
                DroidSound(0);
            if (savedLoop2 == 1)
                SetPitAmbient(1);
            else
                SetPitAmbient(0);
            if (savedLoop3 == 1)
                SetAmbientLoop3(1);
            else
                SetAmbientLoop3(0);
            if (savedLoop4 == 1)
                CrowdSound(1);
            else
                CrowdSound(0);
            if (savedLoop5 == 1)
                ScrapeSound(1);
            else
                ScrapeSound(0);
#endif
            CdPlay();
            oneMoreTime = 1;
        }
    }
}

/* Draws the pause menu and its current selection. */
void DrawPauseMenu(struct SelectBox *box, PcGameData *config)
{
    if (pause.selection > -1) {
        textDepth = 0.05f;
        if (pause.showOptions) {
            DrawMenuBox(140, 120, 500, 320, 0.06f, 0.07f);
            DrawPauseOptions(config, box);
        } else {
            DrawMenuBox(120, 170, 520, 280, 0.06f, 0.07f);
            InGameConfirm();
        }
        textDepth = 0.1f;
    }
}

/* Resets race gameplay after a restart command. */
void InGameReset(PcGameData *config, struct SelectBox *box)
{
    short x = 95;
    char y = 12;

    if (pause.selection == 3) {
        if (keysPressed & MENU_KEY_ACCEPT)
            config->paused = 0;
        if (config->paused) {
            if (keysPressed & MENU_KEY_UP) {
                audioRow--;
                if (audioRow < 0)
                    audioRow = 0;
            } else if (keysPressed & MENU_KEY_DOWN) {
                audioRow++;
                if (audioRow > 2)
                    audioRow = 2;
            }
            if (audioRow == 0) {
                if (keysPressed & MENU_KEY_LEFT) {
                    PlayNote(8, 255, 15, 15, 256);
                    config->cdTrack--;
                    trackNameScroll = 480;
                    if (config->cdTrack < 1)
                        config->cdTrack = cdMusicCount;
                    if (config->cdTrack != 1) {
                        CdMusicPlay(config->cdTrack, 0);
                        pause.musicPaused = 1;
                    } else {
                        cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
                        trackNameScroll = 480;
                        cdMusicTrack = cdMusicTable[cdMusicIndex];
                        CdMusicPlay(cdMusicTrack, 0);
                        pause.musicPaused = 1;
                    }
                } else if (keysPressed & MENU_KEY_RIGHT) {
                    PlayNote(8, 255, 15, 15, 256);
                    config->cdTrack++;
                    trackNameScroll = 480;
                    if (config->cdTrack > cdMusicCount)
                        config->cdTrack = 1;
                    if (config->cdTrack != 1) {
                        CdMusicPlay(config->cdTrack, 0);
                        pause.musicPaused = 1;
                    } else {
                        cdMusicIndex = (cdMusicIndex + 1) % (cdMusicCount - 1);
                        trackNameScroll = 480;
                        cdMusicTrack = cdMusicTable[cdMusicIndex];
                        CdMusicPlay(cdMusicTrack, 0);
                        pause.musicPaused = 1;
                    }
                }
            }
            if (audioRow == 1) {
                if (padButtons & PAD_BUTTON_LEFT) {
                    config->otherMusicVolume = config->otherMusicVolume - 1;
                    if (config->otherMusicVolume < 0)
                        config->otherMusicVolume = 0;
                } else if (padButtons & PAD_BUTTON_RIGHT) {
                    config->otherMusicVolume = config->otherMusicVolume + 1;
                    if (config->otherMusicVolume > 100)
                        config->otherMusicVolume = 100;
                }
                config->musicVolume = config->otherMusicVolume;
                SfxSetCdVolume((config->musicVolume * 255) / 100);
            }
            if (audioRow == 2) {
                SetShieldHum(1);
                if (padButtons & PAD_BUTTON_LEFT) {
                    config->otherEffectVolume = config->otherEffectVolume - 1;
                    if (config->otherEffectVolume < 0)
                        config->otherEffectVolume = 0;
                } else if (padButtons & PAD_BUTTON_RIGHT) {
                    config->otherEffectVolume = config->otherEffectVolume + 1;
                    if (config->otherEffectVolume > 100)
                        config->otherEffectVolume = 100;
                }
                config->effectVolume = config->otherEffectVolume;
                SfxSetVolume((config->effectVolume * 255) / 100);
                SetNoteVolume(shieldHumVoice, 255);
                SfxEffectLRPan(shieldHumVoice, 15);
                SfxEffectFBPan(shieldHumVoice, 15);
            } else {
                SetShieldHum(0);
            }
            UpdateNotes(1);
        } else {
            SetShieldHum(0);
        }
    }
}


/* Draws the pause menu options. */
/* NET reworks the option layout and adds networkGame handling to the pause menu. */
#ifdef NETPLAY
void DrawPauseOptions(PcGameData *config, struct SelectBox *box)
{
    char names[5][18];
    short topY = 95;
    char h = 12;
    int i;

    if (pause.selection == 3 && config->paused) {
        DrawMenuBox(150, 300, 490, 440, 0.03f, 0.04f);
        textDepth = 0.01f;
        switch (audioRow) {
        case 0:
            if (blinkFlag)
                CentreText("CD TRACK", 0, h * 5 + topY, 8, 0);
            if (config->cdTrack == 0)
                PrintCX(GetCdTrackName(config->cdTrack), 160, topY + h * 3 * 2, 0);
            else
                PrintCX(GetCdTrackName(config->cdTrack - 1), 160, topY + h * 3 * 2, 0);
            AddText("MUSIC", 85, 185, 8, 0);
            AddText("SFX", 85, 200, 8, 0);
            break;
        case 1:
            CentreText("CD TRACK", 0, h * 5 + topY, 8, 0);
            if (config->cdTrack == 0)
                PrintCX(GetCdTrackName(config->cdTrack), 160, topY + h * 3 * 2, 0);
            else
                PrintCX(GetCdTrackName(config->cdTrack - 1), 160, topY + h * 3 * 2, 0);
            if (blinkFlag)
                AddText("MUSIC", 85, 185, 8, 0);
            AddText("SFX", 85, 200, 8, 0);
            break;
        case 2:
            CentreText("CD TRACK", 0, h * 5 + topY, 8, 0);
            if (config->cdTrack == 0)
                PrintCX(GetCdTrackName(config->cdTrack), 160, topY + h * 3 * 2, 0);
            else
                PrintCX(GetCdTrackName(config->cdTrack - 1), 160, topY + h * 3 * 2, 0);
            AddText("MUSIC", 85, 185, 8, 0);
            if (blinkFlag)
                AddText("SFX", 85, 200, 8, 0);
            break;
        }
        UpdateMusicBar(config);
        UpdateSfxBar(config);
    }
    textDepth = 0.05f;
    if (blinkFlag)
        CentreText("PAUSED", 107, 70, 8, 1);
    strcpy(names[0], "CONTINUE");
    strcpy(names[1], "RESTART");
    strcpy(names[2], "RETURN TO MENU");
    strcpy(names[3], "AUDIO CONFIG");
    strcpy(names[4], "QUIT TO WINDOWS");
    for (i = 0; i < 5; i++) {
        if (pause.selection == i)
            CentreText(names[i], 0, h * i + topY, 8, 1);
        else
            CentreText(names[i], 0, h * i + topY, 8, 0);
    }
}
#else
void DrawPauseOptions(PcGameData *config, struct SelectBox *box)
{
    char options[5][18];
    int i;
    int y;

    if (pause.selection == 3 && config->paused) {
        DrawMenuBox(150, 300, 490, 440, 0.03f, 0.04f);
        textDepth = 0.01f;
        switch (audioRow) {
        case 0:
            if (blinkFlag)
                CentreText("CD TRACK", 0, 155, 8, 0);
            if (config->cdTrack)
                PrintCX(cdTrackNames[config->cdTrack - 1], 160, 167, 0);
            else
                PrintCX(cdTrackNames[config->cdTrack], 160, 167, 0);
            AddText("MUSIC", 85, 185, 8, 0);
            AddText("SFX", 85, 200, 8, 0);
            break;
        case 1:
            CentreText("CD TRACK", 0, 155, 8, 0);
            if (config->cdTrack)
                PrintCX(cdTrackNames[config->cdTrack - 1], 160, 167, 0);
            else
                PrintCX(cdTrackNames[config->cdTrack], 160, 167, 0);
            if (blinkFlag)
                AddText("MUSIC", 85, 185, 8, 0);
            AddText("SFX", 85, 200, 8, 0);
            break;
        case 2:
            CentreText("CD TRACK", 0, 155, 8, 0);
            if (config->cdTrack)
                PrintCX(cdTrackNames[config->cdTrack - 1], 160, 167, 0);
            else
                PrintCX(cdTrackNames[config->cdTrack], 160, 167, 0);
            AddText("MUSIC", 85, 185, 8, 0);
            if (blinkFlag)
                AddText("SFX", 85, 200, 8, 0);
            break;
        }
        UpdateMusicBar(config);
        UpdateSfxBar(config);
    }
    textDepth = 0.05f;
    if (blinkFlag)
        CentreText("PAUSED", 107, 70, 8, 1);
    strcpy(options[0], "CONTINUE");
    strcpy(options[1], "RESTART");
    strcpy(options[2], "RETURN TO MENU");
    strcpy(options[3], "AUDIO CONFIG");
    strcpy(options[4], "QUIT TO WINDOWS");
    for (i = 0, y = 95; y < 155; i++, y += 12) {
        if (pause.selection == i)
            CentreText(options[i], 0, y, 8, 1);
        else
            CentreText(options[i], 0, y, 8, 0);
    }
}
#endif

/* Confirms the current pause menu selection. */
void InGameConfirm(void)
{
    if (pause.selection == 1) {
        CentreText("ARE YOU SURE YOU", 0x4d, 0x5f, 8, 0);
        CentreText("WANT TO RESTART ?", 0x5a, 0x69, 8, 0);
    } else {
        CentreText("ARE YOU SURE YOU", 0x4d, 0x5f, 8, 0);
        CentreText("WANT TO QUIT ?", 0x69, 0x69, 8, 0);
    }
    if (pause.confirmChoice) {
        if (blinkFlag)
            AddText("YES", 0x4b, 0x78, 8, 1);
        else
            AddText("YES", 0x4b, 0x78, 8, 0);
        AddText("NO", 0xdc, 0x78, 8, 0);
    } else {
        AddText("YES", 0x4b, 0x78, 8, 0);
        if (blinkFlag)
            AddText("NO", 0xdc, 0x78, 8, 1);
        else
            AddText("NO", 0xdc, 0x78, 8, 0);
    }
}

/* Adjusts and draws the music volume bar. */
void UpdateMusicBar(PcGameData *config)
{
    short y;
    short x;
    short level;

    level = (config->otherMusicVolume * 65) / 100;
    if (level > 0) {
        SetTextQuad(&menuBarPoly, 140, 185, 0, 240, level, 8, 32, 0);
        x = 280;
        y = 370;
        menuBarPoly.x0 = x;
        menuBarPoly.y0 = y;
        menuBarPoly.x1 = x + level * 2;
        menuBarPoly.y1 = y;
        menuBarPoly.x2 = x;
        menuBarPoly.y2 = y + 16;
        menuBarPoly.x3 = x + level * 2;
        menuBarPoly.y3 = y + 16;
        DrawScaledTexturedQuad(0.01f, &menuBarPoly, 0);
    }
    SetTextQuad(&menuBarPoly, 138, 183, 0, 74, 138, 24, 32, 0);
    DrawScaledTexturedQuad(0.02f, &menuBarPoly, 0);
}

/* Adjusts and draws the sound effect volume bar. */
void UpdateSfxBar(PcGameData *config)
{
    short y;
    short vol;
    short x;

    vol = (config->otherEffectVolume * 65) / 100;
    if (vol > 0) {
        SetTextQuad(&menuBarPoly, 140, 200, 82, 240, vol, 8, 32, 0);
        x = 280;
        y = 400;
        menuBarPoly.x0 = x;
        menuBarPoly.y0 = y;
        menuBarPoly.x1 = x + vol * 2;
        menuBarPoly.y1 = y;
        menuBarPoly.x2 = x;
        menuBarPoly.y2 = y + 16;
        menuBarPoly.x3 = x + vol * 2;
        menuBarPoly.y3 = y + 16;
        DrawScaledTexturedQuad(0.01f, &menuBarPoly, 0);
    }
    SetTextQuad(&menuBarPoly, 138, 198, 0, 74, 138, 24, 32, 0);
    DrawScaledTexturedQuad(0.02f, &menuBarPoly, 0);
}

/* Draws a menu box with fill and border depth values. */
void DrawMenuBox(short x0, short y0, short x1, short y1, float depthFill, float depthBorder)
{
    POLY_FT4 poly;

    SetTextQuad(&poly, 60, 80, 109, 56, 1, 1, 32, 0);
    poly.x0 = x0 + 2;
    poly.y0 = y0 + 2;
    poly.x1 = x1 - 2;
    poly.y1 = y0 + 2;
    poly.x2 = x0 + 2;
    poly.y2 = y1 - 2;
    poly.x3 = x1 - 2;
    poly.y3 = y1 - 2;
    DrawScaledTexturedQuad(depthFill, &poly, 0);
    SetTextQuad(&poly, 60, 80, 109, 62, 1, 1, 32, 0);
    poly.x0 = x0;
    poly.y0 = y0;
    poly.x1 = x1;
    poly.y1 = y0;
    poly.x2 = x0;
    poly.y2 = y1;
    poly.x3 = x1;
    poly.y3 = y1;
    DrawScaledTexturedQuad(depthBorder, &poly, 0);
}

/* Adds a model object and its render data to the queue. */
void AddRenderObject(Object *obj, int mode, u_char *rgb)
{
    if (obj) {
        if (obj->normalCount == 12345 || obj->normalCount == -12345) {
            ConvertMatrix(renderQueue[renderCount].matrix, GetSkeletonMatrix(obj->skeleton));
            renderQueue[renderCount].quadCount = obj->spriteQuadCount;
            renderQueue[renderCount].quads = obj->spriteQuads;
            renderQueue[renderCount].vertices = (D3DLVERTEX *)obj->bspTree;
            renderQueue[renderCount].vertexCount = obj->vertexCount;
            renderQueue[renderCount].triLists = (TriList *)obj->normals;
            renderQueue[renderCount].mode = mode;
            if (rgbTextures == 0 || mmxAvailable != 0)
                renderQueue[renderCount].mode = 1;
            if (obj->normalCount == -12345)
                renderQueue[renderCount].mode = 1;
            if (rgb) {
                renderQueue[renderCount].hasColour = 1;
                renderQueue[renderCount].r = rgb[0];
                renderQueue[renderCount].g = rgb[1];
                renderQueue[renderCount].b = rgb[2];
            } else {
                renderQueue[renderCount].hasColour = 0;
            }
            renderCount++;
        }
    }
}

void AddRenderSkeleton(Skeleton *skeleton, PCVertex *vertices, int vertexCount,
                       ParticleTriangleList *triangles, void *billboards,
                       int billboardCount, int mode)
{
    ConvertMatrix(renderQueue[renderCount].matrix, GetSkeletonMatrix(skeleton));
    renderQueue[renderCount].quadCount = billboardCount;
    renderQueue[renderCount].quads = billboards;
    renderQueue[renderCount].vertices = (D3DLVERTEX *)vertices;
    renderQueue[renderCount].vertexCount = vertexCount;
    renderQueue[renderCount].triLists = triangles;
    renderQueue[renderCount].mode = mode;
    renderQueue[renderCount].hasColour = 0;
    renderCount++;
}

/* Converts a fixed-point matrix to the renderer's float format. */
void ConvertMatrix(float *out, MATRIX *m)
{
    if (!out)
        return;
    if (!m)
        return;
    out[0] = m->m[0][0] / 4096.0f;
    out[1] = m->m[1][0] / 4096.0f;
    out[2] = m->m[2][0] / 4096.0f;
    out[3] = 0;
    out[4] = m->m[0][1] / 4096.0f;
    out[5] = m->m[1][1] / 4096.0f;
    out[6] = m->m[2][1] / 4096.0f;
    out[7] = 0;
    out[8] = m->m[0][2] / 4096.0f;
    out[9] = m->m[1][2] / 4096.0f;
    out[10] = m->m[2][2] / 4096.0f;
    out[11] = 0;
    out[12] = m->t[0] / 4096.0f;
    out[13] = m->t[1] / 4096.0f;
    out[14] = m->t[2] / 4096.0f;
    out[15] = 1.0f;
}

extern IDirect3DDevice *lpD3DDevice;
extern D3DMATRIXHANDLE matrixWorld;
extern D3DMATRIXHANDLE matrixView;
extern D3DMATRIXHANDLE matrixProj;
extern long ddResult;

/* Frees the three Direct3D matrices and empties the render queue. */
void ReleaseRenderMatrices(void)
{
    ddResult = lpD3DDevice->lpVtbl->DeleteMatrix(lpD3DDevice, matrixWorld);
    ddResult = lpD3DDevice->lpVtbl->DeleteMatrix(lpD3DDevice, matrixView);
    ddResult = lpD3DDevice->lpVtbl->DeleteMatrix(lpD3DDevice, matrixProj);
    renderCount = 0;
}
