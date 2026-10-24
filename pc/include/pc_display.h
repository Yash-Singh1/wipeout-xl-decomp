#ifndef PC_DISPLAY_H
#define PC_DISPLAY_H

#include <windows.h>
#include <ddraw.h>

/* Enumerated Direct3D device, 0x178 bytes. */
typedef struct DisplayDevice {
    char *description;
    char *name;
    GUID *guid;
    GUID guidValue;
    DWORD hardwareCaps[43];
    DWORD softwareCaps[43];
    DWORD reserved;
} DisplayDevice;

/* Enumerated DirectDraw driver, 0x10c4 bytes. */
typedef struct DisplayDriver {
    char *description;
    char *name;
    GUID *guid;
    GUID guidValue;
    DWORD hardwareCaps[79];
    DWORD softwareCaps[79];
    DWORD reserved[3];
    int deviceCount;
    unsigned char supported8[4][8];
    unsigned char supported16[4][8];
    DDPIXELFORMAT formats8[4][8];
    DDPIXELFORMAT formats16[4][8];
    DisplayDevice devices[4];
} DisplayDriver;

extern DisplayDriver displayDrivers[];
extern const GUID IID_IDirect3D;

/* Executable thunk that jumps to the DirectDrawCreate import. */
HRESULT __stdcall DirectDrawCreateThunk(GUID *guid, LPDIRECTDRAW *dd, IUnknown *outer);

/* Executable thunk that jumps to the DirectDrawEnumerate import. */
HRESULT __stdcall DirectDrawEnumerateThunk(LPDDENUMCALLBACKA callback, LPVOID context);

/* A resolution's availability and pixel formats for one display driver, filled while modes are enumerated. */
typedef struct AvailableMode {
    char present8;
    char present16;
    DDPIXELFORMAT format8;
    DDPIXELFORMAT format16;
} AvailableMode;

extern AvailableMode availableModes[][6];

int __stdcall EnumDisplayModesCallback(LPDDSURFACEDESC surface, LPVOID context);
int SelectedResolutionIndex(void);
void BenchmarkMagnitude(void);

#endif
