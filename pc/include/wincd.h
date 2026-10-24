#ifndef WINCD_H
#define WINCD_H

#include "pc.h"

/* CD audio through MCI. The device is opened once at start-up and every
   track's length is read from the disc. */

extern UINT g_cdDeviceID;
extern char g_cdMessage[];

void CdOpen(void);
#include "cd_tracks.h"
int CdControlB(unsigned char command, unsigned char *param, unsigned char *result);
int CdSetDebug(int level);

MCIERROR CdPlayTrack(void *notifyWnd, unsigned char track);
MCIERROR CdPause(void);
int CdPlay(void);
MCIERROR CdStop(void);

#endif
