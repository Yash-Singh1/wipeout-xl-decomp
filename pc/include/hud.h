#ifndef HUD_H
#define HUD_H

#include "wipeout.h"
#include "font.h"

/* One glyph of a bitmap font: its VRAM origin and advance width. */
typedef struct FontGlyph {
    u_short vramX;
    u_short vramY;
    u_short width;
    short advance;
} FontGlyph;

/* The eight quads of the speedo: frame, speed bar, shield bar and thrust marker, one per screen. */
typedef struct Speedo {
    POLY_FT4 frame[2];
    POLY_FT4 speed[2];
    POLY_FT4 shield[2];
    POLY_FT4 marker[2];
} Speedo;

/* One checkpoint of the race: the track section it sits on, the time allowance,
   and whether the player has reached it. The array is indexed directly. */
struct CheckPoints {
    short       section;
    short       time;
    signed char reached;
    char        padding;
};

/* The in-game menu polygons. Each tile is a Gouraud quad. */
typedef struct InGameMenu {
    u_char  pad0[0xC4];
    POLY_G4 footer;
    u_char  textureWindow[12];
    POLY_G4 red[20];
    POLY_G4 green[20];
} InGameMenuPolys;


/* Ship records are ShipData; the HUD and the race-start code use the same layout under this name. */
typedef ShipData RaceShip;



extern short ownShip, finishPosition, sixTracks, eightTracks;
extern int awardedMedal;
#include "hud_state.h"
extern unsigned char envmapr, envmapg, envmapb;
short CalcPosition(ShipData *ships);
void AssignMedals(RaceConfig *config);
void copyInTempLapTimes(ShipData *ships);
void EndChallenge(RaceConfig *config);
int GetAng(VECTOR a, VECTOR b);
int GetMagnitude(float x, float y, float z);

extern int gameType;
extern const double angleScale, fullCircle, fixedPointScale;

extern int typeSwitchesMode;
extern POLY_FT4 weaponHudPolys[2], menuBarPoly;
extern POLY_FT4 panelQuads[4];
extern short weaponX[], weaponY[];
extern unsigned char weaponU[], weaponV[], weaponWidth[], weaponHeight[];
void SetTextQuad(POLY_FT4 *poly, short x, short y, unsigned char u, unsigned char v, unsigned char width, unsigned char height, short tpage, int color);
void DrawWeaponIcons(ShipData *ship);

extern int hudFrame;
#ifdef NETPLAY
void AdvanceHudFrame(ShipData *ships, RaceConfig *config, int choice);
#else
void AdvanceHudFrame(void);
#endif

typedef struct ButtonIcon {
    unsigned char u, v, width, height;
} ButtonIcon;
extern ButtonIcon buttonIcons[];
/* Texture page that the HUD and menu bars, panels and text are drawn from. */
extern int hudTexturePage;
extern short continueAnimFrame;
void Print(u_char *text, int x, int y, int color);
void DoButton(short x, short y, short icon, char *text);
void DisplayContinues(short x, short y);

extern int blinkFlag, extendedTimeFlag;
extern signed char classLaps[];
#ifdef NETPLAY
extern signed char noCheckPoints;
#else
extern unsigned char noCheckPoints;
#endif
void UpdatePlayerShip(ShipData *ships, void *shapes, short shipNo, void *weapons);
void AlterCheckPoint(struct CheckPoints *checkpoints, ShipData *ships, int targetShip, RaceConfig *config);

extern struct MenuEntry *menu;
extern short savedTrackSelection;

extern RaceLine raceLines[];
extern short numTrackSections, playerPosition, currentRacePosition;
extern int trackNameOffset, messageTimer;
struct PauseMenu;
extern struct PauseMenu pause;
struct HiScoreData;
void UpdateRaceHud(ShipData *ships, int choice, RaceConfig *config, struct HiScoreData *score);

#ifdef NON_MATCHING
extern char hudLapTimesText[];
extern char hudLapNumberFormat[];
extern char hudAutopilotText[];
extern char hudDisengagingText[];
extern char hudLapText[];
extern char hudLapTotalText[];
extern char hudRearLockText[];
extern char hudPerfectLapText[];
extern char hudCdPlayingFormat[];
extern char hudCdStoppedFormat[];
extern char hudCheckpointText[];
extern char hudPositionText[];
extern char hudPositionTotalText[];
extern char hudWrongWayText[];
#endif
void TickHUDMessage(void);
void DrawHUDMessage(void);
void InitPanelQuads(void);
void SetTextQuad(POLY_FT4 *quad, short x, short y, u_char u, u_char v,
                 u_char width, u_char height, short tpage, int colour);

#endif
