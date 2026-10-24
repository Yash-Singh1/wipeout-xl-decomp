#ifndef AYRACE_H
#define AYRACE_H

#include "hud.h"
#include "sound.h"
#include "pc.h"
#include "weapon.h"
#include "raceinit.h"
#include "pause.h"
#include "drone.h"

typedef RaceConfig RaceState;

/* Race-completion view of the name-entry and high-score state. */
typedef struct ScoreData {
    char nameEntered[9];
    unsigned char currChar;
    short charVal;
    short typed;
    short numChars;
    unsigned char dispChar[2];
    unsigned char pad12[2];
    int toggleDisplay;
    unsigned char userEntry;
    unsigned char checkVal;
    unsigned char enterHs;
    unsigned char pad1B;
    int count;
    int padDelay;
    int totalTime;
#ifdef NETPLAY
    signed char displayScore;
#else
    unsigned char displayScore;
#endif
    unsigned char displayHiScore;
    short charX;
    unsigned char confirm;
    unsigned char lConfirm;
    unsigned char pad2E[2];
    int raceTime;
    int lapTime;
    unsigned char pad38[8];
    unsigned char racePilot;
    unsigned char lapPilot;
    unsigned char pad42[2];
#ifdef NETPLAY
    signed char gotHiScore;
#else
    unsigned char gotHiScore;
#endif
    unsigned char displayFlag;
} ScoreData;

/* A CD position as the drive reports it: track and time in BCD. */
struct WipeCdStatus {
    u_char track;
    u_char reserved[3];
    u_char time;
};


typedef struct MciStatus {
    unsigned long callback;
    unsigned long returnValue;
    unsigned long item;
    unsigned long track;
} MciStatus;

extern unsigned int g_cdDeviceID;
extern int cdMusicTrack, trackNameScroll;
extern signed char cdMusicIndex, cdMusicTable[];
extern char msgRandomPlay[], msgStaticPlay[], msgRandomPlayMenu[], msgStaticPlayMenu[];
void SetInGameCDTrack(void *unused1, void *unused2, RaceConfig *config, int *state);
void SetMenuCDTrack(void *unused1, void *unused2, RaceConfig *config);

extern short idleTimer, droneSwitchStep;
#include "input_state.h"
extern unsigned short steerAnalog;
extern int linkSendFlag, attractDelayFrames, raceContext, attractModeFlag;
#ifdef NETPLAY
extern signed char raceOutcome;
#else
extern unsigned char raceOutcome;
#endif
extern char msgDemoMode[];
#include "ship_start.h"
void UpdateCameraAttractMode(TrackCamera *camera, ShipData *ship, RescueData *rescue);
void UpdateCameraAttractDestroyed(TrackCamera *camera, ShipData *ship, RescueData *rescue);
void ShowCredits(int count);
void CentreText(unsigned char *text, long x, long y, long size, long color);
void CheckShipDestroyed(ShipData *ships, RaceConfig *config, int *unused, int idleLimit);
void CheckForButtonPress(void);
void PlayCredits(int attract);

extern int fadeAmount, cheatfade;
#include "pc_backbuffer.h"
void QuitGame(void);

/* Destruction countdown: -1 is intact; zero marks the ship destroyed. */
extern int checkTimer, shipDeathCount[15], fracdamage[15], shipDamage[15];
extern int raceEndFlag, raceEndState;
extern unsigned char raceCompleteFlag, hudDisabled;
#include "weapon_state.h"
extern signed char perfectLapCount;
extern short continueAnimFrame;
extern TrackData *track;
extern ConfigData *gameData;
void FlattenTrack(void);
void initShipData(TrackSection *sections, ShipData *ships, RaceConfig *config);
void InitRescueDroid(RescueData *rescue, Object **shapes, ShipData *ship);
/* Resets the start boom polygons before a race (empty in the PC build). */
void ResetStartBoom(void);
void ResetAnimatedObjects(void);
void ResetLightStart(void);
void InitCDAudio(ConfigData *config, unsigned char *status);
void InitElectric(void);
void InitPanelQuads(void);
void UpdateCameraRaceStart(TrackCamera *camera, ShipData *ship, RescueData *rescue);
void Restart(struct ResetData *reset);

extern short sixXPlayed, eightXPlayed;
extern int circuitCount;
extern char *circuitCountText[];
/* Deselects the select box on screen after the race-complete panels are drawn (empty in the PC build). */
void DeSelectScreen(void *screen);
void DisplayRaceInfo(RaceConfig *config, ScoreData *score, ShipData *ships);
void DisplayScoreTab(ConfigData *config, struct HiScoreData *score);
void RaceComplete(void *screen, void *unused, RaceConfig *config, short *choice, ScoreData *score, ShipData *ships);

void AddText(unsigned char *text, long x, long y, long size, long color);
void AddNumber(int number, int x, int y, int size, int color);
#ifdef NETPLAY
void AddTime(int frames, int x, int y, int size, int color, signed char flags);
#else
void AddTime(int frames, int x, int y, int size, int color, unsigned char flags);
#endif
#include "menu_rotation.h"
extern Object *teamShip;
void DrawTeamShips(int x, int y, int z, short direction, short scale, Object *object);

#ifdef NON_MATCHING
extern short lapTimeRows[4];
extern signed char rearLockCycle;
extern int pauseRequested;
extern char trackNameText[];
void AddJustifyNumber(long number, long x, long y, long size, long color);
#include "center_text.h"
char *GetCdTrackName(int track);
char *GetCdTrackArtist(int track);
#ifdef NETPLAY
void DrawRaceHud(ShipData *ships, int choice, RaceConfig *config, struct HiScoreData *score);
#else
void DrawRaceHud(ShipData *ships, int choice, RaceConfig *config);
#endif
#endif

extern short phantomDone, piranhaDone;
extern int linkMyState;
short GetMenuKey(short key);
void InGameConfirm(void);
void InitDefaultName(ConfigData *config, struct HiScoreData *score, int unused);
void EnterHighScoreIfNeeded(ConfigData *config, struct HiScoreData *score);
void InitScoreTabData(struct HiScoreData *score);
void UpdateRaceComplete(void *screen, void *unused, RaceConfig *config, short *choice, ScoreData *score, ShipData *ships);

#ifdef NON_MATCHING
extern int hudThrust;
void UpdateCameraAttractInternal(TrackCamera *camera, ShipData *ships, RescueData *rescue);
#include "speedo_draw.h"
#endif

#endif
