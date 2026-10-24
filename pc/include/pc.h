#ifndef PC_H
#define PC_H

/* Win32 calls used by the game code. The game sources don't include
   <windows.h> (its RECT clashes with libgpu's), so the imports they need are
   declared here. */

#define WINAPI_IMPORT __declspec(dllimport)

typedef unsigned long DWORD;

typedef struct SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} SYSTEMTIME;
typedef unsigned int UINT;
typedef DWORD MCIERROR;

WINAPI_IMPORT void __stdcall OutputDebugStringA(const char *string);
#define OutputDebugString OutputDebugStringA
WINAPI_IMPORT void __stdcall PostQuitMessage(int exitCode);
WINAPI_IMPORT void __stdcall GetSystemTime(SYSTEMTIME *time);

WINAPI_IMPORT char *__stdcall lstrcatA(char *string1, const char *string2);
WINAPI_IMPORT int __stdcall MessageBoxA(void *hWnd, const char *text, const char *caption, UINT type);
WINAPI_IMPORT int __cdecl wsprintfA(char *output, const char *format, ...);
#define wsprintf wsprintfA

typedef struct MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORD dwTotalPhys;
    DWORD dwAvailPhys;
    DWORD dwTotalPageFile;
    DWORD dwAvailPageFile;
    DWORD dwTotalVirtual;
    DWORD dwAvailVirtual;
} MEMORYSTATUS;

WINAPI_IMPORT void __stdcall GlobalMemoryStatus(MEMORYSTATUS *buffer);

/* Joystick (mmsystem.h) */

typedef struct JOYCAPS {
    unsigned short wMid;
    unsigned short wPid;
    char szPname[32];
    UINT wXmin, wXmax, wYmin, wYmax, wZmin, wZmax;
    UINT wNumButtons;
    UINT wPeriodMin, wPeriodMax;
    UINT wRmin, wRmax, wUmin, wUmax, wVmin, wVmax;
    UINT wCaps;
    UINT wMaxAxes, wNumAxes, wMaxButtons;
    char szRegKey[32];
    char szOEMVxD[260];
} JOYCAPS;

typedef struct JOYINFOEX {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwXpos, dwYpos, dwZpos, dwRpos, dwUpos, dwVpos;
    DWORD dwButtons;
    DWORD dwButtonNumber;
    DWORD dwPOV;
    DWORD dwReserved1, dwReserved2;
} JOYINFOEX;

WINAPI_IMPORT UINT __stdcall joyGetNumDevs(void);
WINAPI_IMPORT UINT __stdcall joyGetDevCapsA(UINT id, JOYCAPS *caps, UINT size);
WINAPI_IMPORT UINT __stdcall joyGetPosEx(UINT id, JOYINFOEX *info);

/* Media Control Interface (mmsystem.h) */

WINAPI_IMPORT MCIERROR __stdcall mciSendCommandA(UINT deviceID, UINT message, DWORD flags, DWORD param);

#define MCI_OPEN          0x803
#define MCI_CLOSE         0x804
#define MCI_PLAY          0x806
#define MCI_STOP          0x808
#define MCI_PAUSE         0x809
#define MCI_SET           0x80d
#define MCI_STATUS        0x814

#define MCI_NOTIFY        0x0001
#define MCI_WAIT          0x0002
#define MCI_FROM          0x0004
#define MCI_TO            0x0008
#define MCI_TRACK         0x0010
#define MCI_OPEN_SHAREABLE 0x0100
#define MCI_OPEN_TYPE_ID  0x1000
#define MCI_OPEN_TYPE     0x2000
#define MCI_SET_TIME_FORMAT 0x0400
#define MCI_STATUS_ITEM   0x0100

#define MCI_FORMAT_MSF    2
#define MCI_FORMAT_TMSF   10
#define MCI_DEVTYPE_CD_AUDIO 0x0204

typedef struct {
    DWORD dwCallback;
    UINT  wDeviceID;
    char *lpstrDeviceType;
    char *lpstrElementName;
    char *lpstrAlias;
} MCI_OPEN_PARMS;

typedef struct {
    DWORD dwCallback;
    DWORD dwTimeFormat;
    DWORD dwAudio;
} MCI_SET_PARMS;

#define MCI_STATUS_LENGTH           0x0001
#define MCI_STATUS_POSITION         0x0002
#define MCI_STATUS_NUMBER_OF_TRACKS 0x0003
#define MCI_STATUS_MODE             0x0004
#define MCI_STATUS_READY            0x0007
#define MCI_CDA_STATUS_TYPE_TRACK   0x4001
#define MCI_CDA_TRACK_AUDIO         0x0440

#define MCI_MODE_NOT_READY          0x020c
#define MCI_MODE_STOP               0x020d
#define MCI_MODE_PLAY               0x020e
#define MCI_MODE_RECORD             0x020f
#define MCI_MODE_SEEK               0x0210
#define MCI_MODE_PAUSE              0x0211
#define MCI_MODE_OPEN               0x0212

typedef struct {
    DWORD dwCallback;
    DWORD dwReturn;
    DWORD dwItem;
    DWORD dwTrack;
} MCI_STATUS_PARMS;

typedef struct {
    DWORD dwCallback;
} MCI_GENERIC_PARMS;

typedef struct {
    DWORD dwCallback;
    DWORD dwFrom;
    DWORD dwTo;
} MCI_PLAY_PARMS;

/* Win32 constants used by the game sources (windows.h itself clashes with libgpu). */
#define GMEM_ZEROINIT 0x0040
#define WAVE_FORMAT_PCM 1

/* Additional imports used by the display module. */
#ifdef PC_DISPLAY_IMPORTS
WINAPI_IMPORT unsigned long __stdcall timeGetTime(void);
WINAPI_IMPORT void * __stdcall GlobalAlloc(unsigned int flags, unsigned long size);
WINAPI_IMPORT unsigned long __stdcall GlobalSize(void *memory);
WINAPI_IMPORT int __cdecl wsprintfA(char *buffer, const char *format, ...);
#endif

#ifdef NETPLAY
WINAPI_IMPORT void __stdcall Sleep(unsigned long milliseconds);
#endif

#endif
