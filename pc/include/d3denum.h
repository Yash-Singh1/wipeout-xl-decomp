#ifndef D3DENUM_H
#define D3DENUM_H

#include <windows.h>
#include <ddraw.h>

/* Driver and device that were chosen as the hardware-accelerated Direct3D device during enumeration. */
extern int accelDriverIndex;
extern int accelDeviceIndex;
/* Non-zero when the DirectDraw object for an enumerated driver could not be created: the
   enumeration then skips the driver instead of using the object. */
extern int ddCreateFailed;

BOOL __stdcall EnumDriverCallback(GUID *guid, char *description, char *name, void *context);
BOOL __stdcall EnumDeviceCallback(GUID *guid, char *description, char *name, DWORD *hardwareDesc, DWORD *softwareDesc, void *context);

#endif
