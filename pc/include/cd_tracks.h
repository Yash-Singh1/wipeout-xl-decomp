#ifndef CD_TRACKS_H
#define CD_TRACKS_H

#include "cd.h"

int CdCountTracks(int device, CdlLOC *trackData);

/* Table-of-contents entry point shared by game and Win32 setup code. */
#ifdef NETPLAY
int CdReadTrackTable(void);
#endif

#endif
