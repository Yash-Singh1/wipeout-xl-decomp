#include "wincd.h"
#include "cd_track_buffer.h"

#ifdef NETPLAY
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#pragma intrinsic(strlen)
#endif

/* Win32 message box constants (windows.h is not included here). */
#define MB_OKCANCEL 0x00000001
#define MB_ICONINFORMATION 0x00000040
#define MB_TASKMODAL 0x00002000
#define IDOK 1

extern int g_cdValidating;
extern short g_cdTrackError;
extern void *g_hwnd;
extern int cdMusicCount;
#ifdef NETPLAY
extern signed char networkGame;
extern unsigned char gstDebugOutputEnabled;
void gstDebugPrintf(const char *format, ...);
#pragma function(memset)
extern void (__cdecl *gstDebugPrintCallback)(const char *format, ...);
extern int (__cdecl *gstCancelCallback)(void);
#endif

void exit(int status);

/* Opens the CD audio device through MCI. */
#ifndef NETPLAY
void CdOpen(void)
{
    MCIERROR err;
    MCI_OPEN_PARMS parms;

    do {
        parms.lpstrDeviceType = (char *)MCI_DEVTYPE_CD_AUDIO;
        err = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE, (DWORD)&parms);
        if (err != 0)
            continue;
        break;
    } while (1);
    g_cdDeviceID = parms.wDeviceID;
}
#endif

#ifdef NETPLAY
void CdOpen(void)
{
    MCI_OPEN_PARMS parms;
    MCIERROR result;

    do {
        parms.lpstrDeviceType = (char *)MCI_DEVTYPE_CD_AUDIO;
        result = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE, (DWORD)&parms);
        if (result != 0)
            continue;
        break;
    } while (1);
    g_cdDeviceID = parms.wDeviceID;
}
#endif

#ifdef NETPLAY
/* Empty NET CD control stub retained for PSYQ callers. */
int CdControl(void) { return 0; }
int CdControlF(void) { return 0; }
int CdReadSync(void) { return 0; }
int CdControlB(unsigned char command, unsigned char *param, unsigned char *result) { return 0; }
#else
/* Empty compatibility stubs for the PS1 CD control API. */
int CdControl(void) { return 0; }
int CdControlF(void) { return 0; }
int CdReadSync(void) { return 0; }
int CdControlB(unsigned char command, unsigned char *param, unsigned char *result) { return 0; }
#endif

/* Counts audio tracks reported by the CD drive. */
#ifndef NETPLAY
int CdCountTracks(int unused, CdlLOC *unusedBuffer)
{
    MCI_SET_PARMS set;
    MCI_STATUS_PARMS parms;
    MCIERROR error;
    int audioFound;
    int trackTotal;
    int i;

    audioFound = 0;
    set.dwTimeFormat = MCI_FORMAT_MSF;
    error = mciSendCommandA(g_cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
    if (error != 0) {
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        return 0;
    }
    parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    error = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&parms);
    if (error != 0) {
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        return 0;
    }
    trackTotal = parms.dwReturn;
    for (i = 0; i < trackTotal; i++) {
        parms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
        parms.dwTrack = i;
        error = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&parms);
        if (error != 0) {
            /* a track whose type can't be read is skipped */
        } else if (parms.dwReturn == MCI_CDA_TRACK_AUDIO) {
            audioFound = 1;
        }
    }
    if (!audioFound)
        trackTotal = 0;
    wsprintfA(g_cdMessage, "%d cd tracks\n", trackTotal);
    OutputDebugString(g_cdMessage);
    return trackTotal;
}
#endif

#ifdef NETPLAY
int CdCountTracks(int unused, CdlLOC *unusedBuffer)
{
    MCIERROR result;
    int audioPresent;
    int numTracks;
    int i;
    MCI_STATUS_PARMS status;
    MCI_SET_PARMS formatParams;

    audioPresent = 0;
    if (networkGame)
        return 0;
    formatParams.dwTimeFormat = MCI_FORMAT_MSF;
    result = mciSendCommandA(g_cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&formatParams);
    if (result != 0) {
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        return 0;
    }
    status.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    result = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&status);
    if (result != 0) {
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        return 0;
    }
    numTracks = status.dwReturn;
    for (i = 0; i < numTracks; i++) {
        status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
        status.dwTrack = i;
        result = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&status);
        if (result != 0) {
            /* a track whose type can't be read is skipped */
        } else if (status.dwReturn == MCI_CDA_TRACK_AUDIO) {
            audioPresent = 1;
        }
    }
    if (!audioPresent)
        numTracks = 0;
    wsprintfA(g_cdMessage, "%d cd tracks\n", numTracks);
    OutputDebugString(g_cdMessage);
    return numTracks;
}
#endif


#ifdef NETPLAY
/* Empty NET CD read stubs retained for PSYQ callers. */
int CdRead(void) { return 0; }
int CdGetToc2(void) { return 0; }
int CdGetToc(void) { return 0; }
int CdSetDebug(int level) { return 0; }
#else
/* Empty compatibility stubs for the PS1 CD API. */
int CdRead(void) { return 0; }
int CdGetToc2(void) { return 0; }
int CdGetToc(void) { return 0; }
int CdSetDebug(int level) { return 0; }
#endif

/* Starts CD playback for the requested track. */
#ifndef NETPLAY
MCIERROR CdPlayTrack(void *notifyWnd, unsigned char track)
{
    MCI_SET_PARMS set;
    unsigned char playTrack = 0;
    MCI_PLAY_PARMS play;

    {
        MCIERROR error;

        playTrack = track;
        wsprintfA(g_cdMessage, "Playing track = %d\n", track);
        OutputDebugString(g_cdMessage);
        set.dwTimeFormat = MCI_FORMAT_TMSF;
        error = mciSendCommandA(g_cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
        if (error != 0) {
            mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
            return error;
        }
        play.dwFrom = 0;
        play.dwTo = 0;
        play.dwFrom = track;
        play.dwCallback = (DWORD)notifyWnd;
        if (playTrack != cdMusicCount) {
            play.dwTo = (unsigned char)(track + 1);
            error = mciSendCommandA(g_cdDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD)&play);
            if (error != 0)
                return error;
        } else {
            error = mciSendCommandA(g_cdDeviceID, MCI_PLAY, MCI_FROM | MCI_NOTIFY, (DWORD)&play);
            if (error != 0)
                return error;
        }
    }
    return 0;
}
#endif

#ifdef NETPLAY
MCIERROR CdPlayTrack(void *notifyWnd, unsigned char track)
{
    MCIERROR result;
    unsigned char trackNum = 0;
    MCI_PLAY_PARMS playParams;
    MCI_SET_PARMS set;

    if (networkGame)
        return 0;
    trackNum = track;
    wsprintfA(g_cdMessage, "Playing track = %d\n", track);
    OutputDebugString(g_cdMessage);
    set.dwTimeFormat = MCI_FORMAT_TMSF;
    result = mciSendCommandA(g_cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
    if (result != 0) {
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        return result;
    }
    playParams.dwFrom = 0;
    playParams.dwTo = 0;
    playParams.dwFrom = track;
    playParams.dwCallback = (DWORD)notifyWnd;
    if (trackNum != cdMusicCount) {
        playParams.dwTo = (unsigned char)(track + 1);
        result = mciSendCommandA(g_cdDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD)&playParams);
        if (result != 0)
            return result;
    } else {
        result = mciSendCommandA(g_cdDeviceID, MCI_PLAY, MCI_FROM | MCI_NOTIFY, (DWORD)&playParams);
        if (result != 0)
            return result;
    }
    return 0;
}
#endif

/* Reports whether the CD drive door is open (MCI reports the mode as open). */
#ifndef NETPLAY
int CdIsDoorOpen(void)
{
    MCI_STATUS_PARMS status;

    status.dwItem = MCI_STATUS_MODE;
    {
        MCIERROR err;

        err = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&status);
        if (err != 0)
            return err;
    }
    if (status.dwReturn == MCI_MODE_OPEN)
        return 1;
    else
        return 0;
}
#endif

#ifdef NETPLAY
int CdIsDoorOpen(void)
{
    MCI_STATUS_PARMS parms;
    MCIERROR result;

    if (networkGame)
        return 0;
    parms.dwItem = MCI_STATUS_MODE;
    result = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&parms);
    if (result != 0)
        return result;
    if (parms.dwReturn == MCI_MODE_OPEN)
        return 1;
    else
        return 0;
}
#endif

/* Pauses the active CD audio track. */
#ifndef NETPLAY
MCIERROR CdPause(void)
{
    MCI_GENERIC_PARMS parms;
    MCIERROR err;

    err = mciSendCommandA(g_cdDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD)&parms);
    if (err != 0)
        return err;
    return 0;
}
#endif

#ifdef NETPLAY
MCIERROR CdPause(void)
{
    MCI_GENERIC_PARMS parms;
    MCIERROR result;

    if (networkGame)
        return 0;
    result = mciSendCommandA(g_cdDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD)&parms);
    if (result != 0)
        return result;
    return 0;
}
#endif

/* Resumes the paused CD audio track. */
#ifndef NETPLAY
int CdPlay(void)
{
    MCIERROR err;
    MCI_PLAY_PARMS parms;

    parms.dwCallback = 0;
    err = mciSendCommandA(g_cdDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&parms);
    if (err != 0)
        return 1;
    return 0;
}
#endif

#ifdef NETPLAY
int CdPlay(void)
{
    MCI_PLAY_PARMS parms;
    MCIERROR result;

    if (networkGame)
        return 0;
    parms.dwCallback = 0;
    result = mciSendCommandA(g_cdDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&parms);
    if (result != 0)
        return 1;
    return 0;
}
#endif

/* Reports whether the CD device is playing or paused. */
#ifndef NETPLAY
int CdIsActive(void)
{
    MCI_STATUS_PARMS status;

    status.dwItem = MCI_STATUS_MODE;
    {
        MCIERROR err;

        err = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&status);
        if (err != 0)
            return 1;
    }
    if (status.dwReturn == MCI_MODE_PAUSE || status.dwReturn == MCI_MODE_PLAY) {
        return 1;
        /* Unreachable, but the executable still contains this call. */
        OutputDebugString("CD PAUSED");
    }
    return 0;
}
#endif

#ifdef NETPLAY
int CdIsActive(void)
{
    MCI_STATUS_PARMS parms;
    MCIERROR result;

    if (networkGame)
        return 0;
    parms.dwItem = MCI_STATUS_MODE;
    result = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&parms);
    if (result != 0)
        return 1;
    if (parms.dwReturn == MCI_MODE_PAUSE || parms.dwReturn == MCI_MODE_PLAY) {
        return 1;
        /* Unreachable, but the executable still contains this call. */
        OutputDebugString("CD PAUSED");
    }
    return 0;
}
#endif

/* Closes the MCI CD audio device. */
#ifdef NETPLAY
void CdClose(void)
{
    if (networkGame)
        return;
    mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
}
#else
void CdClose(void)
{
    mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
}
#endif

/* Length of each track of the original disc in frames (75 per second) less the 150 frame
   lead-in, used to tell the game's own CD from any other audio disc. */
extern int cdTrackLengths[];

/* One track of the disc as read from the table of contents. */
typedef struct CdTrack {
    int minutes;
    int seconds;
    int frames;
    int start;      /* position in frames */
} CdTrack;

extern CdTrack cdTrackTable[];
extern int cdTrackFrames[];
#ifdef NETPLAY
extern short netCdTrackError;
#endif

/* Reads the table of contents and checks every track length against the original disc;
   g_cdTrackError is set when they differ. */
#ifndef NETPLAY
int CdReadTrackTable(void)
{
    char text[0x200] = "\0";
    MCI_SET_PARMS set;
    short firstTrackNum;
    char tmp[64];
    int trackLen;
    MCI_STATUS_PARMS parms;
    int i;
    MCIERROR error;
    int trackTotal;

    firstTrackNum = 1;
    g_cdTrackError = 0;
    for (i = 0; i < 25; i++) {
        cdTrackTable[i].minutes = 0;
        cdTrackTable[i].seconds = 0;
        cdTrackTable[i].frames = 0;
        cdTrackTable[i].start = 0;
        cdTrackFrames[i] = 0;
    }
    set.dwTimeFormat = MCI_FORMAT_MSF;
    error = mciSendCommandA(g_cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set);
    if (error != 0) {
        g_cdTrackError = 1;
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        if (error == 0x101)
            CdOpen();
    }
    parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    error = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&parms);
    if (error != 0) {
        trackTotal = 0;
        g_cdTrackError = 1;
    } else {
        trackTotal = parms.dwReturn;
    }
    for (i = firstTrackNum; i <= trackTotal; i++) {
        parms.dwItem = MCI_STATUS_POSITION;
        parms.dwTrack = i;
        error = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&parms);
        if (error != 0) {
            g_cdTrackError = 1;
            break;
        }
        trackLen = parms.dwReturn;
        wsprintfA(tmp, "Track %2d - %02d:%02d:%02d\n", i, (unsigned char)trackLen, (unsigned char)(trackLen >> 8), (unsigned char)(trackLen >> 16));
        lstrcatA(text, tmp);
        cdTrackTable[i - firstTrackNum].minutes = (unsigned char)trackLen;
        cdTrackTable[i - firstTrackNum].seconds = (unsigned char)(trackLen >> 8);
        cdTrackTable[i - firstTrackNum].frames = (unsigned char)(trackLen >> 16);
        cdTrackTable[i - firstTrackNum].start = cdTrackTable[i - firstTrackNum].minutes * 60 * 75 + cdTrackTable[i - firstTrackNum].seconds * 75 + cdTrackTable[i - firstTrackNum].frames;
    }
    for (i = 1; i < trackTotal - 1; i++) {
        cdTrackFrames[i] = cdTrackTable[i + 1].start - cdTrackTable[i].start - 150;
        if (cdTrackFrames[i] != cdTrackLengths[i]) {
            g_cdTrackError = 1;
        }
    }
    if (trackTotal < 3) {
        g_cdTrackError = 1;
    }
    return 0;
}
#endif

#ifdef NETPLAY
int CdReadTrackTable(void)
{
    int trackLen;
    MCIERROR mciResult;
    int i;
    int numberOfTracks;
    char tableText[0x200] = "\0";
    char formattedLine[64];
    MCI_SET_PARMS formatParams;
    MCI_STATUS_PARMS trackParams;
    short firstTrack;

    firstTrack = 1;
    netCdTrackError = 0;
    for (i = 0; i < 25; i++) {
        cdTrackTable[i].minutes = 0;
        cdTrackTable[i].seconds = 0;
        cdTrackTable[i].frames = 0;
        cdTrackTable[i].start = 0;
        cdTrackFrames[i] = 0;
    }
    formatParams.dwTimeFormat = MCI_FORMAT_MSF;
    mciResult = mciSendCommandA(g_cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&formatParams);
    if (mciResult != 0) {
        netCdTrackError = 1;
        if (gstDebugOutputEnabled)
            gstDebugPrintf("getCDTrackStartTimes: Failure #1");
        mciSendCommandA(g_cdDeviceID, MCI_CLOSE, 0, 0);
        if (mciResult == 0x101)
            CdOpen();
    }
    trackParams.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    mciResult = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&trackParams);
    if (mciResult != 0) {
        numberOfTracks = 0;
        netCdTrackError = 1;
        if (gstDebugOutputEnabled)
            gstDebugPrintf("getCDTrackStartTimes: Failure #2");
    } else {
        numberOfTracks = trackParams.dwReturn;
    }
    for (i = firstTrack; i <= numberOfTracks; i++) {
        trackParams.dwItem = MCI_STATUS_POSITION;
        trackParams.dwTrack = i;
        mciResult = mciSendCommandA(g_cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&trackParams);
        if (mciResult != 0) {
            if (gstDebugOutputEnabled)
                gstDebugPrintf("getCDTrackStartTimes: Failure #3");
            netCdTrackError = 1;
            break;
        }
        trackLen = trackParams.dwReturn;
        wsprintfA(formattedLine, "Track %2d - %02d:%02d:%02d\n", i, (unsigned char)trackLen, (unsigned char)(trackLen >> 8), (unsigned char)(trackLen >> 16));
        lstrcatA(tableText, formattedLine);
        cdTrackTable[i - firstTrack].minutes = (unsigned char)trackLen;
        cdTrackTable[i - firstTrack].seconds = (unsigned char)(trackLen >> 8);
        cdTrackTable[i - firstTrack].frames = (unsigned char)(trackLen >> 16);
        cdTrackTable[i - firstTrack].start = cdTrackTable[i - firstTrack].minutes * 60 * 75 + cdTrackTable[i - firstTrack].seconds * 75 + cdTrackTable[i - firstTrack].frames;
    }
    for (i = 1; i < numberOfTracks - 1; i++) {
        cdTrackFrames[i] = cdTrackTable[i + 1].start - cdTrackTable[i].start - 150;
        if (cdTrackFrames[i] != cdTrackLengths[i]) {
            if (gstDebugOutputEnabled)
                gstDebugPrintf("getCDTrackStartTimes: Failure #4");
            netCdTrackError = 1;
        }
    }
    if (numberOfTracks < 3) {
        if (gstDebugOutputEnabled)
            gstDebugPrintf("getCDTrackStartTimes: Failure #5");
        netCdTrackError = 1;
    }
    return 0;
}
#endif

/* Checks whether the inserted disc has the expected track lengths. */
#ifndef NETPLAY
void CdValidate(void)
{
    int result;

    CdReadTrackTable();
    while (g_cdTrackError == 1) {
        g_cdValidating = 1;
        result = MessageBoxA(g_hwnd, "Make sure Wipeout XL CD is in the CD drive.\n", "Wipeout XL CD Validator",
                             MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL);
        g_cdValidating = 0;
        if (result == IDOK)
            CdReadTrackTable();
        else
            exit(-1);
    }
    cdMusicCount = CdCountTracks(1, g_cdTrackBuffer);
}
#endif

#ifdef NETPLAY
/* Refreshes the CD track table and updates the audio track count. */
void CdValidate(void)
{
    CdReadTrackTable();
    cdMusicCount = CdCountTracks(1, g_cdTrackBuffer);
}
#endif

/* Stops CD playback and waits for the MCI command to finish. */
#ifndef NETPLAY
MCIERROR CdStop(void)
{
    MCI_GENERIC_PARMS parms;
    MCIERROR err;

    err = mciSendCommandA(g_cdDeviceID, MCI_STOP, MCI_WAIT, (DWORD)&parms);
    if (err != 0)
        return err;
    return 0;
}
#endif

#ifdef NETPLAY
MCIERROR CdStop(void)
{
    MCI_GENERIC_PARMS parms;
    MCIERROR result;

    if (networkGame)
        return 0;
    result = mciSendCommandA(g_cdDeviceID, MCI_STOP, MCI_WAIT, (DWORD)&parms);
    if (result != 0)
        return result;
    return 0;
}
#endif

#ifdef NETPLAY
extern char *gstDebugScreen;
extern char *gstDebugLogBuffer;
extern int gstDebugLogRow;
extern char gstDebugLineEnding[];

/* Writes text into the NET debug screen at a column and row, or clears the screen. */
void gstDebugWriteText(int column, int row, const char *text)
{
    int offset;
    unsigned int i;
    int j;

    if (gstDebugOutputEnabled) {
        if (!text) {
            for (i = 0; i < 25; i++) {
                for (j = 0; j < 80; j++) {
                    offset = i * 160 + j * 2;
                    gstDebugScreen[offset] = ' ';
                }
            }
        } else {
            for (i = 0; i < strlen(text); i++) {
                int position;

                position = row * 160 + (column + i) * 2;
                gstDebugScreen[position] = text[i];
            }
        }
    }
}

/* Formats a message, adds it to the NET debug log, and writes it to stdout. */
void gstDebugPrintf(const char *format, ...)
{
    int offset;
    unsigned int i;
    int j;
    char buffer[256];
    va_list arglist;

    if (gstDebugOutputEnabled) {
        if (!format) {
            for (i = 0; i < 25; i++) {
                for (j = 0; j < 80; j++) {
                    offset = i * 160 + j * 2;
                    gstDebugLogBuffer[offset] = ' ';
                }
            }
            gstDebugLogRow = 0;
        } else {
            va_start(arglist, format);
            vsprintf(buffer, format, arglist);
            va_end(arglist);
            for (i = 0; i < 80; i++) {
                offset = gstDebugLogRow * 160 + i * 2;
                gstDebugLogBuffer[offset] = ' ';
            }
            buffer[79] = 0;
            for (i = 0; i < strlen(buffer); i++) {
                offset = gstDebugLogRow * 160 + i * 2;
                gstDebugLogBuffer[offset] = buffer[i];
            }
            gstDebugLogRow++;
            if (gstDebugLogRow == 25)
                gstDebugLogRow = 0;
            for (i = 0; i < 80; i++) {
                offset = gstDebugLogRow * 160 + i * 2;
                gstDebugLogBuffer[offset] = ' ';
            }
            printf(buffer);
            printf(gstDebugLineEnding);
            fflush(stdout);
        }
    }
}
#endif
