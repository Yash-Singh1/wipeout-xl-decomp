/* The multiplayer menu screens of the NET build (netmenu.c). */
#ifndef NETMENU_H
#define NETMENU_H

#include "lobby.h"
#include "wincd.h"

#define GST_MAX_SESSIONS 11

extern int currMenu;
extern float textDepth;
extern int blinkFlag;
extern int menuSlideActive;
extern int menuSlideFrame;
extern short menuSlideX, menuSlideY, menuSlideStep;

extern int gstSessionCount;                             /* sessions found by the last scan */
extern int gstSessionSelected;                          /* the highlighted one */
extern unsigned long gstSessionScanTime;                /* tick of the last scan */
extern char gstSessionNames[GST_MAX_SESSIONS][100];
extern unsigned long gstSessionIds[GST_MAX_SESSIONS];
extern unsigned char gstIsHost;                         /* this player created the session */
extern unsigned char gstLaunchMode;                     /* set by the start-up code: 0 when played from the menus */
extern int gstOptionsSelected;                          /* highlighted line of the network options screen */
extern signed char gstPlayerNameLength, gstSessionNameLength;  /* characters typed so far */
extern unsigned char gstCdError;                        /* "INSERT WIPEOUT XL CD" is showing */
extern short netCdTrackError;

void DrawMenuBox(short x0, short y0, short x1, short y1, float depthFill, float depthBorder);
void doNetworkOptionsMenu(void);
void doSessionJoinMenu(void);
char GetKeyboardChar(void);
extern volatile short g_keyboardCode;
int gstEnumSessions_Callback(unsigned long id, char *name, char *description, unsigned char a, unsigned char b);
void doLobbyMenu(void);
void InitSlider(void);

#endif
