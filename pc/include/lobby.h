/* Joining a session, the lockstep player list and the level block the host sends (NET build). */
#ifndef LOBBY_H
#define LOBBY_H

#include "gst.h"
#include "config.h"

#define LOBBY_MAX_PLAYERS 15

extern int gstLobbyCount;                               /* players listed in gstLobbyNames */
extern short gstLobbyIds[16];                           /* their lockstep ids */
extern char gstLobbyNames[LOBBY_MAX_PLAYERS][100];
extern short gstShipTaken[16];                          /* 1 for each ship another player flies */
extern char gstPlayerName[];                            /* the name this player entered */

/* The texts the level block is written and read with: "Race Type=Timed" and so on. Each key
   and value is looked up in gstSessionText; a received value only has to start with the text. */
enum {
    TEXT_RACE_TYPE, TEXT_TIMED, TEXT_DEATH_RACE, TEXT_TOURNAMENT,
    TEXT_CLASS, TEXT_VECTOR, TEXT_VENOM, TEXT_RAPIER, TEXT_PHANTOM,
    TEXT_TRACK_NAME, TEXT_TALONS_REACH, TEXT_SAGARMATHA, TEXT_VALPARAISO, TEXT_PHENITIA_PARK,
    TEXT_GARE_D_EUROPA, TEXT_ODESSA_KEYS, TEXT_VOSTOK, TEXT_SPILSKINANKE,
    TEXT_JOINABLE, TEXT_JUMP_IN, TEXT_LAPS, TEXT_ONE, TEXT_TWO, TEXT_THREE, TEXT_FOUR, TEXT_FIVE,
    TEXT_LIVES, TEXT_WEAPONS, TEXT_SMART_BOMB, TEXT_PLASMA, TEXT_MINE, TEXT_ELECTRIC_BOLT,
    TEXT_MISSILE, TEXT_ROCKET, TEXT_QUAKE, TEXT_TURBO, TEXT_SHIELD,
    TEXT_PIRANHA, TEXT_TRUE, TEXT_FALSE, TEXT_DEFAULT, TEXT_UNLIMITED, TEXT_NONE, TEXT_YES, TEXT_NO,
    TEXT_COUNT
};
/* An initialised table of string pointers in the exe, in this order (not reproduced as data). */
extern char *gstSessionText[TEXT_COUNT];

/* The settings of the race a session runs, as the host sent them (gstGetLevelBlock). */
extern unsigned long gstLevelBlockSize;                 /* bytes in gstLevelBlock */
extern char gstLevelBlock[];
extern unsigned char gstLevelJoinable;
extern unsigned char gstLevelJumpIn;
extern unsigned char gstLevelWeapons[9];                /* 1 for each weapon allowed */
extern unsigned char gstLevelLaps;
extern unsigned char gstLevelLives;
extern unsigned char gstLevelPiranha;
extern unsigned char gstLevelTracks[8];                 /* tournament: track of each race */
extern unsigned char gstLevelClasses[8];                /* tournament: class of each race */
extern int gameType;
extern ConfigData *gameData;

/* The answers gstJoinSessionGPI_Callback and gstCreateSessionGPI get. */
extern GstSession gstSessionRequest;                    /* name, description and the level block */
extern GstJoinInfo gstJoinInfo;
extern unsigned short gstJoinValues[];
extern char gstSessionName[];                           /* the name given to a created session */

extern unsigned char gstJoinError;                      /* why joining failed: 1 no ship, 2 piranha, 3 not joinable */
extern unsigned char gstTeamSlots[5];                   /* free ships left in each team */

/* What the host picked on the create-session screen. */
typedef struct GstMenuRace {
    unsigned char track;
    unsigned char shipClass;
} GstMenuRace;
extern int gstMenuLaps;                                 /* index into the lap options */
extern int gstMenuLives;
extern int gstMenuPiranha;
extern GstMenuRace gstMenuRaces[8];

int gstGetLevelBlock(unsigned long sessionId);
int gstBuildLevelBlock(char *buffer, unsigned long *size);
int JoinSessionServer(void);
void gstPostLevelBlock(char *block, unsigned long size, unsigned long sessionId);
int JoinSessionClient(unsigned long sessionId);
int gstChangePlayerName(const char *name, unsigned long id);
int gstSendCommand(unsigned long sender, unsigned long target, int command, short argument);
int gstEnumLS(unsigned long sessionId);
void gstPostSessionAsynch(int destination, void *data, int size, unsigned char port);
int gstJoinLockstep(void);
int gstEnumLS_Callback(unsigned long id, char *name, char *description, unsigned char count,
                       unsigned char *kinds, short *scores);
int gstLockstepGPI_Callback(int what, char *result);
int gstJoinSessionGPI_Callback(int what, GstQueryResult *result);

#endif
