#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include "pc_window.h"
#include "pc_display.h"
#include "d3denum.h"

#ifdef NETPLAY
/* The NET build expands these inline (repne scasb, rep movsd). */
#pragma intrinsic(strlen, strcpy, memcpy)
#endif

/* Adds an enumerated device to the selected driver. The GUID is copied through a local pointer to the
   device's guidValue, which keeps the exe's load/store order. */
#ifndef NETPLAY
BOOL __stdcall EnumDeviceCallback(GUID *guid, char *description, char *name, DWORD *hardwareDesc, DWORD *softwareDesc, void *context)
{
    int deviceIndex;

    OutputDebugStringA(description);
    OutputDebugStringA("\n\t");
    OutputDebugStringA(name);
    OutputDebugStringA("\n");
    deviceIndex = displayDrivers[videoModeIndex].deviceCount;
    if (guid == NULL) {
        OutputDebugStringA("NULL Guid\n");
    }
    if (guid != NULL) {
        GUID *dst = &displayDrivers[videoModeIndex].devices[deviceIndex].guidValue;
        displayDrivers[videoModeIndex].devices[deviceIndex].guid = dst;
        *dst = *guid;
    } else {
        displayDrivers[videoModeIndex].devices[deviceIndex].guid = NULL;
    }
    displayDrivers[videoModeIndex].devices[deviceIndex].description = GlobalAlloc(GPTR, strlen(description) + 1);
    if (displayDrivers[videoModeIndex].devices[deviceIndex].description != NULL) {
        strcpy(displayDrivers[videoModeIndex].devices[deviceIndex].description, description);
    }
    displayDrivers[videoModeIndex].devices[deviceIndex].name = GlobalAlloc(GPTR, strlen(name) + 1);
    if (displayDrivers[videoModeIndex].devices[deviceIndex].name != NULL) {
        strcpy(displayDrivers[videoModeIndex].devices[deviceIndex].name, name);
    }
    memcpy(displayDrivers[videoModeIndex].devices[deviceIndex].hardwareCaps, hardwareDesc, sizeof(displayDrivers[0].devices[0].hardwareCaps));
    memcpy(displayDrivers[videoModeIndex].devices[deviceIndex].softwareCaps, softwareDesc, sizeof(displayDrivers[0].devices[0].softwareCaps));
    if (displayDrivers[videoModeIndex].devices[deviceIndex].hardwareCaps[1] != 0) {
        accelDriverIndex = videoModeIndex;
        acceleratorAvailable = 1;
        accelDeviceIndex = deviceIndex;
    }
    displayDrivers[videoModeIndex].deviceCount++;
    return 1;
}
#endif
#ifdef NETPLAY
/* Copies the GUID through the device pointer stored by the NET build. */
BOOL __stdcall EnumDeviceCallback(GUID *guid, char *description, char *name, DWORD *hardwareDesc, DWORD *softwareDesc, void *context)
{
    int deviceIndex;

    OutputDebugStringA(description);
    OutputDebugStringA("\n\t");
    OutputDebugStringA(name);
    OutputDebugStringA("\n");
    deviceIndex = displayDrivers[videoModeIndex].deviceCount;
    if (guid == NULL) {
        OutputDebugStringA("NULL Guid\n");
    }
    if (guid != NULL) {
        displayDrivers[videoModeIndex].devices[deviceIndex].guid = &displayDrivers[videoModeIndex].devices[deviceIndex].guidValue;
        *displayDrivers[videoModeIndex].devices[deviceIndex].guid = *guid;
    } else {
        displayDrivers[videoModeIndex].devices[deviceIndex].guid = NULL;
    }
    displayDrivers[videoModeIndex].devices[deviceIndex].description = GlobalAlloc(GPTR, strlen(description) + 1);
    if (displayDrivers[videoModeIndex].devices[deviceIndex].description != NULL) {
        strcpy(displayDrivers[videoModeIndex].devices[deviceIndex].description, description);
    }
    displayDrivers[videoModeIndex].devices[deviceIndex].name = GlobalAlloc(GPTR, strlen(name) + 1);
    if (displayDrivers[videoModeIndex].devices[deviceIndex].name != NULL) {
        strcpy(displayDrivers[videoModeIndex].devices[deviceIndex].name, name);
    }
    memcpy(displayDrivers[videoModeIndex].devices[deviceIndex].hardwareCaps, hardwareDesc, sizeof(displayDrivers[0].devices[0].hardwareCaps));
    memcpy(displayDrivers[videoModeIndex].devices[deviceIndex].softwareCaps, softwareDesc, sizeof(displayDrivers[0].devices[0].softwareCaps));
    if (displayDrivers[videoModeIndex].devices[deviceIndex].hardwareCaps[1] != 0) {
        acceleratorAvailable = 1;
        accelDriverIndex = videoModeIndex;
        accelDeviceIndex = deviceIndex;
    }
    displayDrivers[videoModeIndex].deviceCount++;
    return 1;
}
#endif

/* DirectDrawEnumerate callback: creates a DirectDraw object for each driver, reads its hardware and software caps
   into the next free driver slot and records its guid, description and name. Returns 1 whether or not the driver
   could be used. */
BOOL __stdcall EnumDriverCallback(GUID *guid, char *description, char *name, void *context)
{
    LPDIRECTDRAW directDraw;
    LPDIRECTDRAW2 directDraw2;

    OutputDebugStringA(description);
    OutputDebugStringA("\n");
    OutputDebugStringA(name);
    OutputDebugStringA("\n");
#ifdef NETPLAY
    ddResult = DirectDrawCreate(guid, &directDraw, NULL);
#else
    ddResult = DirectDrawCreateThunk(guid, &directDraw, NULL);
#endif
#ifdef NETPLAY
    dxErrorLine = 0x156a;
#else
    dxErrorLine = 0x126c;
#endif
    LogDXError();
    if (ddCreateFailed != 0) {
        OutputDebugStringA("Failed to create DD object\n");
        return 1;
    }
    ddResult = directDraw->lpVtbl->QueryInterface(directDraw, &IID_IDirectDraw2, (void **)&directDraw2);
#ifdef NETPLAY
    dxErrorLine = 0x1571;
#else
    dxErrorLine = 0x1273;
#endif
    LogDXError();
    if (ddResult != 0) {
        directDraw->lpVtbl->Release(directDraw);
        OutputDebugStringA("Fail 2\n");
        return 1;
    }
    displayDrivers[displayDriverCount].hardwareCaps[0] = sizeof(DDCAPS);
    displayDrivers[displayDriverCount].softwareCaps[0] = sizeof(DDCAPS);
    ddResult = directDraw2->lpVtbl->GetCaps(directDraw2, (LPDDCAPS)displayDrivers[displayDriverCount].hardwareCaps, (LPDDCAPS)displayDrivers[displayDriverCount].softwareCaps);
#ifdef NETPLAY
    dxErrorLine = 0x1589;
#else
    dxErrorLine = 0x128b;
#endif
    LogDXError();
    if (ddResult != 0) {
        directDraw2->lpVtbl->Release(directDraw2);
        directDraw->lpVtbl->Release(directDraw);
        OutputDebugStringA("Failed to read caps\n");
        return 1;
    }
    if (guid != NULL) {
#ifdef NETPLAY
        displayDrivers[displayDriverCount].guid = &displayDrivers[displayDriverCount].guidValue;
        *displayDrivers[displayDriverCount].guid = *guid;
#else
        GUID *dst = &displayDrivers[displayDriverCount].guidValue;
        displayDrivers[displayDriverCount].guid = dst;
        *dst = *guid;
#endif
    } else {
        displayDrivers[displayDriverCount].guid = NULL;
    }
    displayDrivers[displayDriverCount].description = GlobalAlloc(GPTR, strlen(description) + 1);
    if (displayDrivers[displayDriverCount].description != NULL) {
        strcpy(displayDrivers[displayDriverCount].description, description);
    }
    displayDrivers[displayDriverCount].name = GlobalAlloc(GPTR, strlen(name) + 1);
    if (displayDrivers[displayDriverCount].name != NULL) {
        strcpy(displayDrivers[displayDriverCount].name, name);
    }
    directDraw2->lpVtbl->Release(directDraw2);
    directDraw->lpVtbl->Release(directDraw);
    displayDriverCount++;
    return 1;
}
