#ifndef RACECTRL_H
#define RACECTRL_H

void FlushMessages(void);

#ifdef NETPLAY
#include "gst.h"

typedef CheckCell CheckCellClasses[4][7];
void RaceFrameEnd(void);

int gstEventGetCount(void);
GstEvent *gstEventDequeue(int kind);
int gstEnumLockstepPlayers(unsigned long lockstepId);
void gstPostLockstepEvent(short type, short value, short player, unsigned char data, unsigned char send);
void func_43dd04(void);
extern int raceFrameReady;
extern int savedCheckTimer;
extern TrackSection *raceStartSection;
extern unsigned char lockstepPosted[];
extern int playerPoints[];
extern unsigned char lastKiller, lastVictim;
extern unsigned char playerFinishedRace;
#endif

#endif
