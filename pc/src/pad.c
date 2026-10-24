#include "functions.h"
#include "input_state.h"
#include "cheats.h"
#include "weapon.h"
#include "pc.h"
#include "sound_samples.h"
#include "gst.h"
#include <string.h>

#ifdef NETPLAY
extern int proximity;
extern const char *weaponNames[];
void gstPostLockstepAsynch(void *data, int size, u_char port);
typedef union OutgoingAsynch {
    NetAsynch message;
    struct {
        u_char eventType;
        u_char reserved1;
        u_char rearFire;
        u_char targetShip;
        u_char rearTarget;
        u_char reserved5[3];
        int proximity;
        u_char weaponType;
        u_char reserved13[3];
    } fire;
} OutgoingAsynch;
#endif

/* DirectInput key codes (DIK_*), as the keyboard driver reports them. */
enum {
    DIK_ESCAPE = 0x01,
    DIK_RETURN = 0x1C,
    DIK_A = 0x1E,
    DIK_S = 0x1F,
    DIK_D = 0x20,
    DIK_LMENU = 0x38,
    DIK_SPACE = 0x39,
    DIK_F1 = 0x3B,
    DIK_F2 = 0x3C,
    DIK_F3 = 0x3D,
    DIK_F4 = 0x3E,
    DIK_F5 = 0x3F,
    DIK_F6 = 0x40,
    DIK_F7 = 0x41,
    DIK_F9 = 0x43,
    DIK_UP = 0xC8,
    DIK_LEFT = 0xCB,
    DIK_RIGHT = 0xCD,
    DIK_DOWN = 0xD0
};

enum {
    DIRECTINPUT_VERSION_3 = 0x0300,
    DIMOFS_X = 0x00,
    DIMOFS_BUTTON0 = 0x0C,
    DIMOFS_BUTTON1 = 0x0D,
    DIMOFS_BUTTON2 = 0x0E,
    DISCL_EXCLUSIVE = 0x01,
    DISCL_NONEXCLUSIVE = 0x02,
    DISCL_FOREGROUND = 0x04,
    DI_EVENT_BUTTON_DOWN = 0x80,
    JOY_RETURN_X = 0x001,
    JOY_RETURN_Y = 0x002,
    JOY_RETURN_Z = 0x004,
    JOY_RETURN_R = 0x008,
    JOY_RETURN_BUTTONS = 0x080,
    JOY_RETURN_CENTERED = 0x400,
    JOY_USE_DEAD_ZONE = 0x800,
    JOY_CAP_HAS_Z = 0x001,
    JOY_CAP_HAS_R = 0x002,
    JOY_BUTTON_10 = 0x200
};

extern u_short menuKeys;
extern short ownShip;
extern ShipData *shipIndex;
/* DirectInput (declared by hand: the game sources don't include the DirectX headers). */
typedef struct GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;

typedef struct IDirectInputDevice IDirectInputDevice;
typedef struct IDirectInput IDirectInput;

typedef struct IDirectInputDeviceVtbl {
    long (__stdcall *QueryInterface)(IDirectInputDevice *, const GUID *, void **);
    unsigned long (__stdcall *AddRef)(IDirectInputDevice *);
    unsigned long (__stdcall *Release)(IDirectInputDevice *);
    long (__stdcall *GetCapabilities)(IDirectInputDevice *, void *);
    long (__stdcall *EnumObjects)(IDirectInputDevice *, void *, void *, unsigned long);
    long (__stdcall *GetProperty)(IDirectInputDevice *, const GUID *, void *);
    long (__stdcall *SetProperty)(IDirectInputDevice *, const GUID *, const void *);
    long (__stdcall *Acquire)(IDirectInputDevice *);
    long (__stdcall *Unacquire)(IDirectInputDevice *);
    long (__stdcall *GetDeviceState)(IDirectInputDevice *, unsigned long, void *);
    long (__stdcall *GetDeviceData)(IDirectInputDevice *, unsigned long, void *, unsigned long *, unsigned long);
    long (__stdcall *SetDataFormat)(IDirectInputDevice *, const void *);
    long (__stdcall *SetEventNotification)(IDirectInputDevice *, void *);
    long (__stdcall *SetCooperativeLevel)(IDirectInputDevice *, void *, unsigned long);
} IDirectInputDeviceVtbl;

struct IDirectInputDevice {
    const IDirectInputDeviceVtbl *lpVtbl;
};

typedef struct IDirectInputVtbl {
    long (__stdcall *QueryInterface)(IDirectInput *, const GUID *, void **);
    unsigned long (__stdcall *AddRef)(IDirectInput *);
    unsigned long (__stdcall *Release)(IDirectInput *);
    long (__stdcall *CreateDevice)(IDirectInput *, const GUID *, IDirectInputDevice **, void *);
} IDirectInputVtbl;

struct IDirectInput {
    const IDirectInputVtbl *lpVtbl;
};

typedef struct DIPROPDWORD {
    unsigned long dwSize;
    unsigned long dwHeaderSize;
    unsigned long dwObj;
    unsigned long dwHow;
    unsigned long dwData;
} DIPROPDWORD;
#ifdef NETPLAY
typedef struct DIMOUSEPROP {
    unsigned long dwObj;
    unsigned long dwHow;
    unsigned long dwData;
} DIMOUSEPROP;
#endif

long __stdcall DirectInputCreate(void *hinst, unsigned long version, IDirectInput **out, void *outer);

extern const GUID GUID_SysKeyboard;
extern const GUID GUID_SysMouse;
extern const char c_dfDIKeyboard[];
extern const char c_dfDIMouse[];
extern void *g_hwnd;
extern void *g_hInstance;
#include "directinput_state.h"
extern IDirectInputDevice *keyboardDevice;
extern IDirectInputDevice *mouseDevice;
extern int keyboardAcquired;
extern int mouseAcquired;
extern DIPROPDWORD kbdBufferProp;
#ifdef NETPLAY
extern DIMOUSEPROP mouseBufferProp;
#else
extern DIPROPDWORD mouseBufferProp;
#endif
#ifdef NETPLAY
extern unsigned long mouseBufferSize;
extern unsigned long mouseBufferHeaderSize;
#endif
extern int joystickPresent;
extern int joyButtonNow;
extern int joyButtonPrev;
extern unsigned long joyButtons;
extern UINT joyNumDevs;
extern UINT joyResult;
extern JOYCAPS joyCaps;
extern int keyBindings[];
extern JOYINFOEX joyInfo;
extern char joystickName[];
extern short g_keyboardCode;

extern int gameKeys[];
extern int padBindings[];

/* One buffered DirectInput event. */
typedef struct DIDEVICEOBJECTDATA {
    unsigned long dwOfs;
    unsigned long dwData;
    unsigned long dwTimeStamp;
    unsigned long dwSequence;
} DIDEVICEOBJECTDATA;

/* DirectInput buffered-data return codes. */
#define DI_BUFFEROVERFLOW 1
#define DIERR_INPUTLOST   0x8007001e
#define DIPROP_BUFFERSIZE ((const GUID *)1)

extern DIDEVICEOBJECTDATA keyboardEvents[];
extern DIDEVICEOBJECTDATA mouseEvents[];
extern u_short keysHeld;
extern u_short prevKeysHeld;
extern u_short menuLatch;
extern u_short prevPadButtons;
extern unsigned long prevJoyButtons;
extern u_short keyboardPad;
extern u_short mousePad;
extern u_short steerAnalog;
extern int leftKeyHeld;
extern int rightKeyHeld;
extern int lastSteerKey;
extern int mouseButton0Held;
extern int mouseButton1Held;
extern int mouseButton2Held;
extern int mouseSensitivity;
extern int mouseDivisor[];
extern short mouseX;
extern int mouseDelta;
extern char cheatBuffer[];
extern char g_keyCharTable[];
extern int debugTextFlag;
#include "race_mode.h"
extern int cheatfade;
extern int checkTimer;
#include "ship_state.h"
extern ConfigData *gameData;
extern MenuEntry *menu;
extern ConfigData *tempGameData;
extern int mouseEnabled;

/* Cheat codes as typed, newest letter first. */
#ifdef NETPLAY
extern char *cheatCodes[];
#else
char *cheatCodes[] = {
    "AGEMYSP", "TCETORPYSP", "REKCITYSP", "DIPARYSP", "MAETX", "SSALCX", "KCARTX", "HSUR", "ETAREMARF"
};
#endif

#include "hud_state.h"
extern int messageTimer;
extern int pauseRequested;
extern void (*CameraUpdate)();

/* Function keys that change the detail settings during a race; each shows a message on the screen.
   The sky and filtering cases toggle their flags before clearing the key code. */

#ifdef NETPLAY
#pragma intrinsic(strcpy)
#endif
void DoDetailKeys(void)
{
#ifdef POWERVR
    extern signed char fogEnabled;
#endif
    if (g_keyboardCode == DIK_F1) {
        if ((shipIndex[ownShip].attr & RACING) && !pauseRequested) {
            if (!(shipIndex[ownShip].attr & VIEW_IN)) {
                CameraUpdate = UpdateCameraRaceInternal;
                shipIndex[ownShip].attr |= VIEW_IN;
                strcpy(hudMessage, "INTERNAL VIEW");
#ifdef NETPLAY
                messageTimer = 30;
#endif
            } else {
                CameraUpdate = UpdateCameraRaceExternal;
                shipIndex[ownShip].attr &= ~VIEW_IN;
                strcpy(hudMessage, "EXTERNAL VIEW");
#ifdef NETPLAY
                messageTimer = 30;
#endif
            }
#ifndef NETPLAY
            messageTimer = 30;
#endif
        }
        g_keyboardCode = -1;
    } else if (g_keyboardCode == DIK_F2) {
        if (mouseEnabled) {
            mouseSensitivity++;
            if (mouseSensitivity > 8)
                mouseSensitivity = 1;
            wsprintf(hudMessage, "MOUSE SENSITIVITY %d", mouseSensitivity);
            messageTimer = 30;
        }
        g_keyboardCode = -1;
    } else if (g_keyboardCode == DIK_F3) {
        gameData->viewDetail++;
        if (gameData->viewDetail >= 8)
            gameData->viewDetail = 0;
        g_keyboardCode = -1;
        wsprintf(hudMessage, "DRAW DISTANCE %d", gameData->viewDetail + 1);
        messageTimer = 30;
    } else if (g_keyboardCode == DIK_F4) {
        gameData->trailDetail++;
        if (gameData->trailDetail > 2)
            gameData->trailDetail = 0;
        g_keyboardCode = -1;
        switch (gameData->trailDetail) {
        case 0:
            strcpy(hudMessage, "ENGINE TRAILS OFF");
            break;
        case 1:
            strcpy(hudMessage, "ENGINE TRAILS ON");
            break;
        case 2:
            strcpy(hudMessage, "ENGINE TRAILS FULL");
            break;
        }
        messageTimer = 30;
    } else if (g_keyboardCode == DIK_F5) {
#ifdef POWERVR
        if (fogEnabled == 0)
            gameData->skyEnabled ^= 1;
#else
#ifdef NETPLAY
        gameData->skyEnabledSigned ^= 1;
#else
        gameData->skyEnabled ^= 1;
#endif
#endif
        g_keyboardCode = -1;
#ifdef POWERVR
        if (gameData->skyEnabled)
            strcpy(hudMessage, "SKY ON");
        else if (fogEnabled)
            strcpy(hudMessage, "SKY DISABLED");
        else
            strcpy(hudMessage, "SKY OFF");
#else
#ifdef NETPLAY
        if (gameData->skyEnabledSigned)
#else
        if (gameData->skyEnabled)
#endif
            strcpy(hudMessage, "SKY ON");
        else
            strcpy(hudMessage, "SKY OFF");
#endif
        messageTimer = 30;
#ifdef POWERVR
    } else if (g_keyboardCode == DIK_F6) {
        fogEnabled ^= 1;
        switch (fogEnabled) {
        case 0:
            strcpy(hudMessage, "FOG OFF");
            gameData->skyEnabled = 1;
            break;
        case 1:
            strcpy(hudMessage, "FOG ON");
            gameData->skyEnabled = 0;
            break;
        }
        messageTimer = 30;
        g_keyboardCode = -1;
#endif
    } else if (g_keyboardCode == DIK_F7) {
#ifdef POWERVR
        g_keyboardCode = -1;
#endif
#ifdef NETPLAY
        gameData->smoothTexturesSigned ^= 1;
#else
        gameData->smoothTextures ^= 1;
#endif
#ifndef POWERVR
        g_keyboardCode = -1;
#endif
#ifdef NETPLAY
        if (gameData->smoothTexturesSigned)
#else
        if (gameData->smoothTextures)
#endif
            strcpy(hudMessage, "FILTERING ON");
        else
            strcpy(hudMessage, "FILTERING OFF");
        messageTimer = 30;
    }
}


void SetBrake(ShipData *ship)
{
    if (padButtons & PAD_BUTTON_RIGHT_BRAKE)
        ship->r_brake_rot += 0x20;
    else if (ship->r_brake_rot > 0)
        ship->r_brake_rot -= 0x20;
    if (ship->r_brake_rot < 0)
        ship->r_brake_rot = 0;
    if (ship->r_brake_rot > 0x100)
        ship->r_brake_rot = 0x100;
    if (padButtons & PAD_BUTTON_LEFT_BRAKE)
        ship->l_brake_rot += 0x20;
    else if (ship->l_brake_rot > 0)
        ship->l_brake_rot -= 0x20;
    if (ship->l_brake_rot < 0)
        ship->l_brake_rot = 0;
    if (ship->l_brake_rot > 0x100)
        ship->l_brake_rot = 0x100;
}

#ifndef NETPLAY
void SetFire(ShipData *ship, WeaponData *weapons, short shipNo)
{
    if (((padPressed & PAD_BUTTON_FIRE) && ship->weapon != 0) ||
        ((padButtons & PAD_BUTTON_FIRE) && ship->weapon == 13)) {
        if (padButtons & PAD_BUTTON_REAR_FIRE)
            ship->rearFire = 1;
        else
            ship->rearFire = 0;
        if (!(ship->attr & SHIELDED)) {
            FireWeapon(ship, weapons, shipNo, ship->weapon);
            if (cheatWeapon != 0)
                GetCheatWeapon(shipIndex, ownShip);
        } else {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
        }
    }
}
#endif

#ifdef NETPLAY
void SetFire(ShipData *ship, WeaponData *weapons, short shipNo)
{
    OutgoingAsynch *eventPtr;
    OutgoingAsynch messageBuffer;

    while (shipIndex[shipNo].asynchIn != shipIndex[shipNo].asynchOut) {
        if (shipIndex[shipNo].asynchTime[shipIndex[shipNo].asynchOut] == localRandomSeed) {
            eventPtr = (OutgoingAsynch *)&shipIndex[shipNo].asynchQueue[shipIndex[shipNo].asynchOut];

            if (gstDebugOutputEnabled) {
                printf("SetFire: shipNo %d: asynchOut %d\n", shipNo, shipIndex[shipNo].asynchOut);
                printf("SetFire: Got weaponType %d [%s]\n", eventPtr->message.data[12], weaponNames[eventPtr->message.data[12]]);
            }

            if (eventPtr->message.data[12] == 12) {
                ship->netProximity = eventPtr->fire.proximity;
                if (ship->weapon != 0)
                    FireWeapon(ship, weapons, shipNo, ship->weapon);
            } else {
                ship->rearFire = eventPtr->message.data[2];
                ship->targetShip = eventPtr->message.data[3];
                ship->rearTarget = eventPtr->message.data[4];
                FireWeapon(ship, weapons, shipNo, ship->weapon);
                if (ship->weapon != 0)
                    FireWeapon(ship, weapons, shipNo, ship->weapon);
            }

            shipIndex[shipNo].asynchTime[shipIndex[shipNo].asynchOut] = 0;
            shipIndex[shipNo].asynchOut = (shipIndex[shipNo].asynchOut + 1) & 0x1f;
        } else {
            break;
        }
    }

    if ((padPressed & PAD_BUTTON_FIRE) && ship->weapon != 0 ||
        (padButtons & PAD_BUTTON_FIRE) && ship->weapon == 13) {
        if (padButtons & PAD_BUTTON_REAR_FIRE)
            ship->rearFire = 1;
        else
            ship->rearFire = 0;

        if (!(ship->attr & SHIELDED)) {
            if ((ship->weapon == 2 || ship->weapon == 5 || ship->weapon == 12) && networkGame) {
                if (ownShip == shipNo) {
                    /* The asynchronous payload deliberately leaves unrelated bytes untouched. */
                    messageBuffer.fire.eventType = 2;
                    messageBuffer.fire.reserved1 = 0;
                    messageBuffer.fire.targetShip = ship->targetShip;
                    messageBuffer.fire.rearFire = ship->rearFire;
                    messageBuffer.fire.rearTarget = ship->rearTarget;
                    messageBuffer.fire.weaponType = ship->weapon;
                    messageBuffer.fire.proximity = proximity;
                    gstPostLockstepAsynch(&messageBuffer.message, 0x10, 1);
                }
            } else {
                if (gstDebugOutputEnabled)
                    printf("SetFire: Firing [%s]\n", weaponNames[ship->weapon]);
                FireWeapon(ship, weapons, shipNo, ship->weapon);
                if (cheatWeapon != 0)
                    GetCheatWeapon(shipIndex, ownShip);
            }
        } else if (ownShip == shipNo) {
            PlayNote(SAMPLE_MENU_MOVE, 0xFF, 0xF, 0xF, 0x100);
        }
    }
}
#endif

#ifdef NETPLAY
/* The network build takes the shipNo as well, and only the ship that is ours plays the ditch sound.
   The weapons argument is not read. */
void SetDitch(ShipData *ship, WeaponData *weapons, int shipNo)
#else
void SetDitch(ShipData *ship)
#endif
{
    if (padPressed & PAD_BUTTON_DITCH_WEAPON) {
        if (ship->weapon != 0) {
#ifdef NETPLAY
            if (ownShip == shipNo)
#endif
                PlayNote(SAMPLE_DITCH_WEAPON, 0xFF, 0xF, 0xF, 0x100);
            ship->weapon = 0;
        }
        if (cheatWeapon != 0)
            GetCheatWeapon(shipIndex, ownShip);
    }
}

#ifndef NETPLAY
short GetMenuKey(short key)
{
    switch (key) {
    case 0:
        return menuKeys & MENU_KEY_UP;
    case 1:
        return menuKeys & MENU_KEY_DOWN;
    case 2:
        return menuKeys & MENU_KEY_LEFT;
    case 3:
        return menuKeys & MENU_KEY_RIGHT;
    case 4:
        return menuKeys & MENU_KEY_BACK;
    case 5:
        return menuKeys & MENU_KEY_ACCEPT;
    case 6:
        return menuKeys & MENU_KEY_PAGE_UP;
    case 7:
        return menuKeys & MENU_KEY_PAGE_DOWN;
    }
    return 0;
}
#endif

#ifdef NETPLAY
short GetMenuKey(short key)
{
    switch (key) {
    case 0:
        return menuKeys & MENU_KEY_UP;
    case 1:
        return menuKeys & MENU_KEY_DOWN;
    case 2:
        return menuKeys & MENU_KEY_LEFT;
    case 3:
        return menuKeys & MENU_KEY_RIGHT;
    case 4:
        return menuKeys & MENU_KEY_BACK;
    case 5:
        return menuKeys & MENU_KEY_ACCEPT;
    case 6:
        return menuKeys & MENU_KEY_PAGE_UP;
    case 7:
        return menuKeys & MENU_KEY_PAGE_DOWN;
    default:
        return 0;
        /* The NET executable retains this unreachable break. */
        break;
    }
}
#endif

void InitJoystick(void)
{
    joyNumDevs = joyGetNumDevs();
    if ((int)joyNumDevs > 0) {
        joyResult = joyGetDevCapsA(0, &joyCaps, sizeof(JOYCAPS));
        strcpy(joystickName, joyCaps.szPname);
        joyInfo.dwSize = sizeof(JOYINFOEX);
        joyInfo.dwFlags = JOY_RETURN_X | JOY_RETURN_Y | JOY_RETURN_BUTTONS |
                          JOY_RETURN_CENTERED | JOY_USE_DEAD_ZONE;
        if (joyCaps.wCaps & JOY_CAP_HAS_Z)
            joyInfo.dwFlags |= JOY_RETURN_Z;
        if (joyCaps.wCaps & JOY_CAP_HAS_R)
            joyInfo.dwFlags |= JOY_RETURN_R;
    }
}


#ifndef NETPLAY
void InitPadSettings(void)
{
    gameKeys[0] = DIK_S;
    gameKeys[2] = DIK_LMENU;
    gameKeys[1] = DIK_SPACE;
    gameKeys[3] = DIK_A;
    gameKeys[4] = DIK_D;
    gameKeys[7] = DIK_LEFT;
    gameKeys[6] = DIK_DOWN;
    gameKeys[8] = DIK_RIGHT;
    gameKeys[5] = DIK_UP;
    gameKeys[9] = DIK_RETURN;
    gameKeys[11] = DIK_F9;
    gameKeys[10] = DIK_F1;
    keyBindings[1] = 4;
    keyBindings[0] = 2;
    keyBindings[2] = 1;
    keyBindings[6] = 8;
    keyBindings[3] = 0x40;

    keyBindings[4] = 0x80;
    keyBindings[5] = 0x200;
    keyBindings[7] = 0x100;
    padBindings[0] = 1;
    padBindings[1] = 2;
    padBindings[2] = 3;
    padBindings[6] = 0;
    padBindings[3] = 0;
    padBindings[4] = 0;
    padBindings[5] = 0;
    padBindings[7] = 0;
}
#endif

#ifdef NETPLAY
void InitPadSettings(void)
{
    gameKeys[0] = DIK_S;
    gameKeys[2] = DIK_LMENU;
    gameKeys[1] = DIK_SPACE;
    gameKeys[3] = DIK_A;
    gameKeys[4] = DIK_D;
    gameKeys[7] = DIK_LEFT;
    gameKeys[6] = DIK_DOWN;
    gameKeys[8] = DIK_RIGHT;
    gameKeys[5] = DIK_UP;
    gameKeys[9] = DIK_RETURN;
    gameKeys[11] = DIK_F9;
    gameKeys[10] = DIK_F1;
    keyBindings[0] = 2;
    keyBindings[1] = 4;
    keyBindings[2] = 1;
    keyBindings[6] = 8;
    keyBindings[3] = 0x40;

    keyBindings[4] = 0x80;
    keyBindings[5] = 0x200;
    keyBindings[7] = 0x100;
    padBindings[0] = 1;
    padBindings[1] = 2;
    padBindings[2] = 3;
    padBindings[6] = 0;
    padBindings[3] = 0;
    padBindings[4] = 0;
    padBindings[5] = 0;
    padBindings[7] = 0;
}
#endif


#ifndef NETPLAY
int InitDirectInput(void)
{
    GUID keyboardGuid = GUID_SysKeyboard;
    GUID mouseGuid = GUID_SysMouse;
    long hr;
    if (DirectInputCreate(g_hInstance, DIRECTINPUT_VERSION_3, &directInput, 0)) {
        OutputDebugString("IDS_DINPUT_ERROR_DIC");
        return 0;
    }
    if (directInput->lpVtbl->CreateDevice(directInput, &keyboardGuid, &keyboardDevice, 0)) {
        OutputDebugString("IDS_DINPUT_ERROR_CD");
        return 0;
    }
    if (keyboardDevice->lpVtbl->SetDataFormat(keyboardDevice, c_dfDIKeyboard)) {
        OutputDebugString("IDS_DINPUT_ERROR_DF");
        return 0;
    }
    if (keyboardDevice->lpVtbl->SetCooperativeLevel(keyboardDevice, g_hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) {
        OutputDebugString("IDS_DINPUT_ERROR_SP");
        return 0;
    }
    kbdBufferProp.dwObj = 0;
    kbdBufferProp.dwHow = 0;
    kbdBufferProp.dwSize = sizeof(DIPROPDWORD);
    kbdBufferProp.dwHeaderSize = 0x10;
    kbdBufferProp.dwData = 0x20;
    if (keyboardDevice->lpVtbl->SetProperty(keyboardDevice, DIPROP_BUFFERSIZE, &kbdBufferProp)) {
        OutputDebugString("IDS_DINPUT_ERROR_SP");
        return 0;
    }
    hr = keyboardDevice->lpVtbl->Acquire(keyboardDevice);
    keyboardAcquired = 1;
    if (hr < 0)
        keyboardAcquired = 0;
    if (directInput->lpVtbl->CreateDevice(directInput, &mouseGuid, &mouseDevice, 0)) {
        OutputDebugString("CreateDevice(SysMouse)");
        return 0;
    }
    if (mouseDevice->lpVtbl->SetDataFormat(mouseDevice, c_dfDIMouse)) {
        OutputDebugString("SetDataFormat(SysMouse, dfDIMouse)");
        return 0;
    }
    if (mouseDevice->lpVtbl->SetCooperativeLevel(mouseDevice, g_hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)) {
        OutputDebugString("SetCooperativeLevel(SysMouse)");
        return 0;
    }
    mouseBufferProp.dwObj = 0;
    mouseBufferProp.dwHow = 0;
    mouseBufferProp.dwSize = sizeof(DIPROPDWORD);
    mouseBufferProp.dwHeaderSize = 0x10;
    mouseBufferProp.dwData = 0x20;
    /* The original passes kbdBufferProp here, not mouseBufferProp. */
    if (mouseDevice->lpVtbl->SetProperty(mouseDevice, DIPROP_BUFFERSIZE, &kbdBufferProp)) {
        OutputDebugString("Set buffer size(SysMouse)");
        return 0;
    }
    if (mouseDevice->lpVtbl->Acquire(mouseDevice)) {
        mouseAcquired = 0;
        return 1;
    }
    mouseAcquired = 1;
    return 1;
}
#endif

#ifdef NETPLAY
int InitDirectInput(void)
{
    long result;
    GUID mouseDeviceId;
    GUID keyboardGuid;

    keyboardGuid = GUID_SysKeyboard;
    mouseDeviceId = GUID_SysMouse;
    if (DirectInputCreate(g_hInstance, DIRECTINPUT_VERSION_3, &directInput, 0)) {
        OutputDebugString("IDS_DINPUT_ERROR_DIC");
        return 0;
    }
    if (directInput->lpVtbl->CreateDevice(directInput, &keyboardGuid, &keyboardDevice, 0)) {
        OutputDebugString("IDS_DINPUT_ERROR_CD");
        return 0;
    }
    if (keyboardDevice->lpVtbl->SetDataFormat(keyboardDevice, c_dfDIKeyboard)) {
        OutputDebugString("IDS_DINPUT_ERROR_DF");
        return 0;
    }
    if (keyboardDevice->lpVtbl->SetCooperativeLevel(keyboardDevice, g_hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) {
        OutputDebugString("IDS_DINPUT_ERROR_SP");
        return 0;
    }
    kbdBufferProp.dwSize = sizeof(DIPROPDWORD);
    kbdBufferProp.dwHeaderSize = 0x10;
    kbdBufferProp.dwObj = 0;
    kbdBufferProp.dwHow = 0;
    kbdBufferProp.dwData = 0x20;
    if (keyboardDevice->lpVtbl->SetProperty(keyboardDevice, DIPROP_BUFFERSIZE, &kbdBufferProp)) {
        OutputDebugString("IDS_DINPUT_ERROR_SP");
        return 0;
    }
    result = keyboardDevice->lpVtbl->Acquire(keyboardDevice);
    if (result >= 0)
        keyboardAcquired = 1;
    else
        keyboardAcquired = 0;
    if (directInput->lpVtbl->CreateDevice(directInput, &mouseDeviceId, &mouseDevice, 0)) {
        OutputDebugString("CreateDevice(SysMouse)");
        return 0;
    }
    if (mouseDevice->lpVtbl->SetDataFormat(mouseDevice, c_dfDIMouse)) {
        OutputDebugString("SetDataFormat(SysMouse, dfDIMouse)");
        return 0;
    }
    if (mouseDevice->lpVtbl->SetCooperativeLevel(mouseDevice, g_hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)) {
        OutputDebugString("SetCooperativeLevel(SysMouse)");
        return 0;
    }
    mouseBufferSize = sizeof(DIPROPDWORD);
    mouseBufferHeaderSize = 0x10;
    mouseBufferProp.dwObj = 0;
    mouseBufferProp.dwHow = 0;
    mouseBufferProp.dwData = 0x20;
    /* The original passes kbdBufferProp here, not mouseBufferProp. */
    if (mouseDevice->lpVtbl->SetProperty(mouseDevice, DIPROP_BUFFERSIZE, &kbdBufferProp)) {
        OutputDebugString("Set buffer size(SysMouse)");
        return 0;
    }
    if (mouseDevice->lpVtbl->Acquire(mouseDevice))
        mouseAcquired = 0;
    else
        mouseAcquired = 1;
    return 1;
}
#endif

void ReleaseDirectInput(void)
{
    if (mouseAcquired) {
        mouseDevice->lpVtbl->Unacquire(mouseDevice);
        mouseAcquired = 0;
    }
    if (keyboardAcquired) {
        keyboardDevice->lpVtbl->Unacquire(keyboardDevice);
        keyboardAcquired = 0;
    }
    if (mouseDevice)
        mouseDevice->lpVtbl->Release(mouseDevice);
    if (keyboardDevice)
        keyboardDevice->lpVtbl->Release(keyboardDevice);
    if (directInput)
        directInput->lpVtbl->Release(directInput);
}

#ifdef NETPLAY
/* Clears the network keyboard and mouse pad words. */
extern u_short D_abbf94, D_ab45cc, D_ab4e14, D_ab4e1c;

void func_43dd04(void)
{
    keyboardPad = 0;
    mousePad = 0;
    g_keyboardCode = -1;
    steerAnalog = 0x80;
    D_abbf94 = 0x80;
}
#endif

void ResetKeyboardState(void)
{
    g_keyboardCode = -1;
    steerAnalog = 0x80;
    keyboardPad = 0;
    mousePad = 0;
}

#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(strlen)
/* Reads keyboard, joystick and mouse input for the network build. */
void PollKeyboard(void)
{
    u_short keyInput;
    unsigned long codeIndex;
    int count;
    u_short menuBits;
    unsigned long ofs;
    short mouseButton;
    u_short joystickPadBits;
    long deviceCode;
    u_short menuButtons;
    unsigned long i;

    menuBits = 0;
    keyInput = 0;
    menuButtons = 0;
    joystickPadBits = 0;
    prevMenuHeld = menuHeld;
    prevKeysHeld = keysHeld;
    prevJoyButtons = joyButtons;
    prevPadButtons = padButtons;
    menuHeld = 0;
    keysHeld = 0;
    padButtons = 0;
    if (keyboardDevice != 0) {
    count = 0x20;
    deviceCode = keyboardDevice->lpVtbl->GetDeviceData(keyboardDevice, sizeof(DIDEVICEOBJECTDATA), keyboardEvents, &count, 0);
    if (deviceCode == DIERR_INPUTLOST) {
        deviceCode = keyboardDevice->lpVtbl->Acquire(keyboardDevice);
        deviceCode = keyboardDevice->lpVtbl->GetDeviceData(keyboardDevice, sizeof(DIDEVICEOBJECTDATA), keyboardEvents, &count, 0);
    }
    if (deviceCode == 0 || deviceCode == DI_BUFFEROVERFLOW) {
        if (deviceCode == DI_BUFFEROVERFLOW)
            OutputDebugString("*** DIRECT INPUT *** Buffer Overflow\n");
        for (i = 0; i < count; i++) {
            ofs = keyboardEvents[i].dwOfs;
            if (keyboardEvents[i].dwData & DI_EVENT_BUTTON_DOWN) {
                if (ofs == DIK_LEFT) {
                    menuBits |= MENU_KEY_LEFT;
                    keyInput |= MENU_KEY_LEFT;
                } else if (ofs == DIK_RIGHT) {
                    menuBits |= MENU_KEY_RIGHT;
                    keyInput |= MENU_KEY_RIGHT;
                }
                if (ofs == DIK_DOWN) {
                    menuBits |= MENU_KEY_DOWN;
                    keyInput |= MENU_KEY_DOWN;
                } else if (ofs == DIK_UP) {
                    menuBits |= MENU_KEY_UP;
                    keyInput |= MENU_KEY_UP;
                }
                if (ofs == DIK_RETURN) {
                    menuBits |= MENU_KEY_ACCEPT;
                    keyInput |= MENU_KEY_ACCEPT;
                }
                if (ofs == DIK_ESCAPE) {
                    menuBits |= MENU_KEY_BACK;
                    keyInput |= MENU_KEY_BACK;
                }
                g_keyboardCode = ofs;
                if (ofs == gameKeys[2])
                    keyboardPad |= PAD_BUTTON_DITCH_WEAPON;
                else if (ofs == gameKeys[0])
                    keyboardPad |= PAD_BUTTON_THRUST;
                else if (ofs == gameKeys[1])
                    keyboardPad |= PAD_BUTTON_FIRE;
                else if (ofs == gameKeys[10])
                    keyboardPad |= PadTriangle0;
                else if (ofs == gameKeys[3])
                    keyboardPad |= PAD_BUTTON_RIGHT_BRAKE;
                else if (ofs == gameKeys[4])
                    keyboardPad |= PAD_BUTTON_LEFT_BRAKE;
                else if (ofs == gameKeys[7]) {
                    leftKeyHeld = 1;
                    lastSteerKey = 1;
                } else if (ofs == gameKeys[8]) {
                    rightKeyHeld = 1;
                    lastSteerKey = 2;
                } else if (ofs == gameKeys[6])
                    keyboardPad |= PAD_BUTTON_REAR_FIRE;
                else if (ofs == gameKeys[5])
                    keyboardPad |= 0x10;
                else if (ofs == gameKeys[9])
                    keyboardPad |= 8;
                else if (ofs == gameKeys[11])
                    keyboardPad |= 1;
            } else {
                if (ofs == gameKeys[2])
                    keyboardPad &= ~PAD_BUTTON_DITCH_WEAPON;
                else if (ofs == gameKeys[0])
                    keyboardPad &= ~PAD_BUTTON_THRUST;
                else if (ofs == gameKeys[1])
                    keyboardPad &= ~PAD_BUTTON_FIRE;
                else if (ofs == gameKeys[10])
                    keyboardPad &= ~PadTriangle0;
                else if (ofs == gameKeys[3])
                    keyboardPad &= ~PAD_BUTTON_RIGHT_BRAKE;
                else if (ofs == gameKeys[4])
                    keyboardPad &= ~PAD_BUTTON_LEFT_BRAKE;
                else if (ofs == gameKeys[7])
                    leftKeyHeld = 0;
                else if (ofs == gameKeys[8])
                    rightKeyHeld = 0;
                else if (ofs == gameKeys[6])
                    keyboardPad &= ~PAD_BUTTON_REAR_FIRE;
                else if (ofs == gameKeys[5])
                    keyboardPad &= ~0x10;
                else if (ofs == gameKeys[9])
                    keyboardPad &= ~8;
                else if (ofs == gameKeys[11])
                    keyboardPad &= ~1;
                for (codeIndex = 9; codeIndex > 0; codeIndex--)
                    cheatBuffer[codeIndex] = cheatBuffer[codeIndex - 1];
                cheatBuffer[0] = g_keyCharTable[ofs];
                for (codeIndex = 0; (unsigned)codeIndex < 9; codeIndex++) {
                    if (strncmp(cheatBuffer, cheatCodes[codeIndex], strlen(cheatCodes[codeIndex])) == 0) {
                    if (codeIndex == 8) {
                        if (debugTextFlag)
                            debugTextFlag = 0;
                        else
                            debugTextFlag = 1;
                    } else if (raceInProgress == 1) {
                        switch (codeIndex) {
                        case 0:
                            if (gameData->weaponsOnSigned) {
                                cheatWeapon ^= 1;
                                if (cheatWeapon)
                                    GetNewWeapon(shipIndex, ownShip);
                                cheatfade = 255;
                            }
                            break;
                        case 1:
                            cheatDamage = cheatDamage ^ 1;
                            cheatfade = 255;
                            break;
                        case 2:
                            cheatTime = cheatTime ^ 1;
                            if (cheatTime)
                                checkTimer = 999;
                            cheatfade = 255;
                            break;
                        case 3:
                            cheatDrone = cheatDrone ^ 1;
                            cheatfade = 255;
                            break;
                        }
                    } else if (raceInProgress == 2) {
                        switch (codeIndex) {
                        case 4:
                            OutputDebugString("Piranha ship cheat:\n");
                            tempGameData->teamCheat = 1;
                            break;
                        case 5:
                            tempGameData->phantomCheat = 1;
                            menu[2].options[3].enabled = 1;
                            OutputDebugString("Phantom class cheat:\n");
                            break;
                        case 6:
                            OutputDebugString("Track cheat:\n");
                            tempGameData->trackCheat = 1;
                            menu[2].options[3].enabled = 1;
                            break;
                        }
                    } else if (raceInProgress == 3) {
                        if (codeIndex == 7)
                            sillyShips = 1;
                    }
                    }
                    }
                }
            if (leftKeyHeld && rightKeyHeld) {
                if (lastSteerKey == 1) {
                    steerAnalog = 0;
                    keyboardPad |= 0x80;
                } else {
                    steerAnalog = 0xff;
                    keyboardPad |= 0x20;
                }
            } else if (leftKeyHeld) {
                steerAnalog = 0;
                keyboardPad |= 0x80;
            } else if (rightKeyHeld) {
                steerAnalog = 0xff;
                keyboardPad |= 0x20;
            } else {
                steerAnalog = 0x80;
                keyboardPad &= ~0x80;
                keyboardPad &= ~0x20;
            }
        }
    }
    if (joystickPresent) {
        joyResult = joyGetPosEx(0, &joyInfo);
        joyButtons = joyInfo.dwButtons;
        if (joyResult == 0) {
            if (joyInfo.dwFlags & JOY_RETURN_X) {
                if (joyInfo.dwXpos < 0x4000) {
                    menuButtons |= MENU_KEY_LEFT;
                    keyInput |= MENU_KEY_LEFT;
                } else if (joyInfo.dwXpos > 0xc000) {
                    menuButtons |= MENU_KEY_RIGHT;
                    keyInput |= MENU_KEY_RIGHT;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_Y) {
                if (joyInfo.dwYpos < 0x4000) {
                    menuButtons |= MENU_KEY_UP;
                    keyInput |= MENU_KEY_UP;
                } else if (joyInfo.dwYpos > 0xc000) {
                    menuButtons |= MENU_KEY_DOWN;
                    keyInput |= MENU_KEY_DOWN;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_BUTTONS) {
                if (joyInfo.dwButtons & 4)
                    menuButtons |= 0x2000;
                if (joyInfo.dwButtons & 2) {
                    menuButtons |= MENU_KEY_ACCEPT;
                    keyInput |= MENU_KEY_ACCEPT;
                }
                if (joyInfo.dwButtons & 1)
                    menuButtons |= 0x8000;
                if (joyInfo.dwButtons & 8) {
                    menuButtons |= MENU_KEY_BACK;
                    keyInput |= MENU_KEY_BACK;
                }
                if (joyInfo.dwButtons & JOY_BUTTON_10) {
                    menuButtons |= MENU_KEY_BACK;
                    keyInput |= MENU_KEY_BACK;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_X) {
                if (!rightKeyHeld && !leftKeyHeld)
                    steerAnalog = joyInfo.dwXpos >> 8;
                if (joyInfo.dwXpos < 0x4000)
                    joystickPadBits |= 0x80;
                else if (joyInfo.dwXpos > 0xc000)
                    joystickPadBits |= 0x20;
            }
            if (joyInfo.dwFlags & JOY_RETURN_Y) {
                if (joyInfo.dwYpos < 0x4000)
                    joystickPadBits |= 0x10;
                else if (joyInfo.dwYpos > 0xc000)
                    joystickPadBits |= 0x40;
            }
            if (joyInfo.dwFlags & JOY_RETURN_Z) {
                if (joyInfo.dwZpos > 0x4000)
                    joystickPadBits |= PAD_BUTTON_THRUST;
            }
            if (joyInfo.dwFlags & JOY_RETURN_R) {
                if (joyInfo.dwRpos < 0x4000)
                    joystickPadBits |= PAD_BUTTON_RIGHT_BRAKE;
                else if (joyInfo.dwRpos > 0xc000)
                    joystickPadBits |= PAD_BUTTON_LEFT_BRAKE;
            }
            if (joyInfo.dwFlags & JOY_RETURN_BUTTONS) {
                if (keyBindings[3] & joyInfo.dwButtons)
                    joystickPadBits |= PAD_BUTTON_RIGHT_BRAKE;
                if (keyBindings[4] & joyInfo.dwButtons)
                    joystickPadBits |= PAD_BUTTON_LEFT_BRAKE;
                if (keyBindings[1] & joyInfo.dwButtons)
                    joystickPadBits |= PAD_BUTTON_FIRE;
                if (joyInfo.dwButtons & keyBindings[0])
                    joystickPadBits |= PAD_BUTTON_THRUST;
                if (keyBindings[6] & joyInfo.dwButtons)
                    joystickPadBits |= PadTriangle0;
                if (keyBindings[2] & joyInfo.dwButtons)
                    joystickPadBits |= PAD_BUTTON_DITCH_WEAPON;
                if (keyBindings[5] & joyInfo.dwButtons)
                    joystickPadBits |= 8;
                if (keyBindings[7] & joyInfo.dwButtons)
                    joystickPadBits |= 1;
            }
        }
    }
    if (mouseEnabled && mouseDevice) {
        count = 0x20;
        deviceCode = mouseDevice->lpVtbl->GetDeviceData(mouseDevice, sizeof(DIDEVICEOBJECTDATA), mouseEvents, &count, 0);
        if (deviceCode == DIERR_INPUTLOST)
            OutputDebugString("*** DIRECT INPUT *** Mouse Input Lost\n");
        if (deviceCode == 0 || deviceCode == DI_BUFFEROVERFLOW) {
            for (codeIndex = 0; codeIndex < count; codeIndex++) {
                switch (mouseEvents[codeIndex].dwOfs) {
                case DIMOFS_X:
                    mouseDelta = (int)mouseEvents[codeIndex].dwData / mouseDivisor[mouseSensitivity];
                    mouseX = steerAnalog + mouseDelta;
                    if (mouseX > 0xff)
                        mouseX = 0xff;
                    if (mouseX < 0)
                        mouseX = 0;
                    steerAnalog = mouseX;
                    break;
                case DIMOFS_BUTTON0:
                    if (padBindings[0] == 1)
                        mouseButton = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 1)
                        mouseButton = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 1)
                        mouseButton = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 1)
                        mouseButton = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 1)
                        mouseButton = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[codeIndex].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton0Held = 1;
                        mousePad |= mouseButton;
                    } else {
                        mouseButton0Held = 0;
                        mousePad &= ~mouseButton;
                    }
                    break;
                case DIMOFS_BUTTON1:
                    if (padBindings[0] == 2)
                        mouseButton = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 2)
                        mouseButton = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 2)
                        mouseButton = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 2)
                        mouseButton = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 2)
                        mouseButton = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[codeIndex].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton1Held = 1;
                        mousePad |= mouseButton;
                    } else {
                        mouseButton1Held = 0;
                        mousePad &= ~mouseButton;
                    }
                    break;
                case DIMOFS_BUTTON2:
                    if (padBindings[0] == 3)
                        mouseButton = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 3)
                        mouseButton = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 3)
                        mouseButton = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 3)
                        mouseButton = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 3)
                        mouseButton = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[codeIndex].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton2Held = 1;
                        mousePad |= mouseButton;
                    } else {
                        mouseButton2Held = 0;
                        mousePad &= ~mouseButton;
                    }
                    break;
                }
            }
        }
    }
    if (!pauseRequested) {
        menuHeld |= menuBits;
        menuHeld |= menuButtons;
        menuLatch = menuHeld;
    }
    keysHeld = keyInput;
    menuKeys = (prevMenuHeld ^ menuHeld) & menuHeld;
    keysPressed = (prevKeysHeld ^ keysHeld) & keysHeld;
    joyPressed = (prevJoyButtons ^ joyButtons) & joyButtons;
    padButtons = keyboardPad | padButtons;
    padButtons = mousePad | padButtons;
    padButtons = joystickPadBits | padButtons;
    padPressed = (prevPadButtons ^ padButtons) & padButtons;
    }
}

#else
/* Reads the keyboard, joystick and mouse and updates the pad and menu key words. */
void PollKeyboard(void)
{
    u_short joyPad;
    u_short keyBits;
    u_short joyMenu;
    u_short menuBits;
    unsigned long count;
    long hr;
    unsigned long i;
    int delta;
    int j;
    unsigned long data;
    unsigned long ofs;
    int cheat;
    short buttonBit;

    menuBits = 0;
    keyBits = 0;
    joyMenu = 0;
    joyPad = 0;
    prevMenuHeld = menuHeld;
    prevKeysHeld = keysHeld;
    prevJoyButtons = joyButtons;
    prevPadButtons = padButtons;
    menuHeld = 0;
    keysHeld = 0;
    padButtons = 0;
    if (keyboardDevice == 0)
        return;
    count = 0x20;
    hr = keyboardDevice->lpVtbl->GetDeviceData(keyboardDevice, sizeof(DIDEVICEOBJECTDATA), keyboardEvents, &count, 0);
    if (hr == DIERR_INPUTLOST) {
        keyboardDevice->lpVtbl->Acquire(keyboardDevice);
        hr = keyboardDevice->lpVtbl->GetDeviceData(keyboardDevice, sizeof(DIDEVICEOBJECTDATA), keyboardEvents, &count, 0);
    }
    if (hr == 0 || hr == DI_BUFFEROVERFLOW) {
        if (hr == DI_BUFFEROVERFLOW)
            OutputDebugString("*** DIRECT INPUT *** Buffer Overflow\n");
        for (i = 0; i < count; i++) {
            data = keyboardEvents[i].dwData;
            ofs = keyboardEvents[i].dwOfs;
            if (data & DI_EVENT_BUTTON_DOWN) {
                if (ofs == DIK_LEFT) {
                    menuBits |= MENU_KEY_LEFT;
                    keyBits |= MENU_KEY_LEFT;
                } else if (ofs == DIK_RIGHT) {
                    menuBits |= MENU_KEY_RIGHT;
                    keyBits |= MENU_KEY_RIGHT;
                }
                if (ofs == DIK_DOWN) {
                    menuBits |= MENU_KEY_DOWN;
                    keyBits |= MENU_KEY_DOWN;
                } else if (ofs == DIK_UP) {
                    menuBits |= MENU_KEY_UP;
                    keyBits |= MENU_KEY_UP;
                }
                if (ofs == DIK_RETURN) {
                    menuBits |= MENU_KEY_ACCEPT;
                    keyBits |= MENU_KEY_ACCEPT;
                }
                if (ofs == DIK_ESCAPE) {
                    menuBits |= MENU_KEY_BACK;
                    keyBits |= MENU_KEY_BACK;
                }
                g_keyboardCode = ofs;
                if (ofs == gameKeys[2])
                    keyboardPad |= PAD_BUTTON_DITCH_WEAPON;
                else if (ofs == gameKeys[0])
                    keyboardPad |= PAD_BUTTON_THRUST;
                else if (ofs == gameKeys[1])
                    keyboardPad |= PAD_BUTTON_FIRE;
                else if (ofs == gameKeys[10])
                    keyboardPad |= PadTriangle0;
                else if (ofs == gameKeys[3])
                    keyboardPad |= PAD_BUTTON_RIGHT_BRAKE;
                else if (ofs == gameKeys[4])
                    keyboardPad |= PAD_BUTTON_LEFT_BRAKE;
                else if (ofs == gameKeys[7]) {
                    leftKeyHeld = 1;
                    lastSteerKey = 1;
                } else if (ofs == gameKeys[8]) {
                    rightKeyHeld = 1;
                    lastSteerKey = 2;
                } else if (ofs == gameKeys[6])
                    keyboardPad |= PAD_BUTTON_REAR_FIRE;
                else if (ofs == gameKeys[5])
                    keyboardPad |= 0x10;
                else if (ofs == gameKeys[9])
                    keyboardPad |= 8;
                else if (ofs == gameKeys[11])
                    keyboardPad |= 1;
            } else {
                if (ofs == gameKeys[2])
                    keyboardPad &= ~PAD_BUTTON_DITCH_WEAPON;
                else if (ofs == gameKeys[0])
                    keyboardPad &= ~PAD_BUTTON_THRUST;
                else if (ofs == gameKeys[1])
                    keyboardPad &= ~PAD_BUTTON_FIRE;
                else if (ofs == gameKeys[10])
                    keyboardPad &= ~PadTriangle0;
                else if (ofs == gameKeys[3])
                    keyboardPad &= ~PAD_BUTTON_RIGHT_BRAKE;
                else if (ofs == gameKeys[4])
                    keyboardPad &= ~PAD_BUTTON_LEFT_BRAKE;
                else if (ofs == gameKeys[7])
                    leftKeyHeld = 0;
                else if (ofs == gameKeys[8])
                    rightKeyHeld = 0;
                else if (ofs == gameKeys[6])
                    keyboardPad &= ~PAD_BUTTON_REAR_FIRE;
                else if (ofs == gameKeys[5])
                    keyboardPad &= ~0x10;
                else if (ofs == gameKeys[9])
                    keyboardPad &= ~8;
                else if (ofs == gameKeys[11])
                    keyboardPad &= ~1;
                for (cheat = 9; cheat > 0; cheat--)
                    cheatBuffer[cheat] = cheatBuffer[cheat - 1];
                cheatBuffer[0] = g_keyCharTable[ofs];
                for (cheat = 0; (unsigned)cheat < 9; cheat++) {
                    if (strncmp(cheatBuffer, cheatCodes[cheat], strlen(cheatCodes[cheat])) != 0)
                        continue;
                    if (cheat == 8) {
                        if (debugTextFlag)
                            debugTextFlag = 0;
                        else
                            debugTextFlag = 1;
                    } else if (raceInProgress == 1) {
                        switch (cheat) {
                        case 0:
                            if (gameData->weaponsOn) {
                                cheatWeapon ^= 1;
                                if (cheatWeapon)
                                    GetNewWeapon(shipIndex, ownShip);
                                cheatfade = 255;
                            }
                            break;
                        case 1:
                            cheatfade = 255;
                            cheatDamage ^= 1;
                            break;
                        case 2:
                            cheatTime ^= 1;
                            if (cheatTime)
                                checkTimer = 999;
                            cheatfade = 255;
                            break;
                        case 3:
                            cheatfade = 255;
                            cheatDrone ^= 1;
                            break;
                        }
                    } else if (raceInProgress == 2) {
                        switch (cheat) {
                        case 4:
                            OutputDebugString("Piranha ship cheat:\n");
                            tempGameData->teamCheat = 1;
                            break;
                        case 5:
                            menu[2].options[3].enabled = 1;
                            tempGameData->phantomCheat = 1;
                            OutputDebugString("Phantom class cheat:\n");
                            break;
                        case 6:
                            OutputDebugString("Track cheat:\n");
                            tempGameData->trackCheat = 1;
                            menu[2].options[3].enabled = 1;
                            break;
                        }
                    } else if (raceInProgress == 3) {
                        if (cheat == 7)
                            sillyShips = 1;
                    }
                }
            }
            if (leftKeyHeld && rightKeyHeld) {
                if (lastSteerKey == 1) {
                    steerAnalog = 0;
                    keyboardPad |= 0x80;
                } else {
                    steerAnalog = 0xff;
                    keyboardPad |= 0x20;
                }
            } else if (leftKeyHeld) {
                steerAnalog = 0;
                keyboardPad |= 0x80;
            } else if (rightKeyHeld) {
                steerAnalog = 0xff;
                keyboardPad |= 0x20;
            } else {
                steerAnalog = 0x80;
                keyboardPad &= 0xff5f;
            }
        }
    }
    if (joystickPresent) {
        joyResult = joyGetPosEx(0, &joyInfo);
        joyButtons = joyInfo.dwButtons;
        if (joyResult == 0) {
            if (joyInfo.dwFlags & JOY_RETURN_X) {
                if (joyInfo.dwXpos < 0x4000) {
                    keyBits |= MENU_KEY_LEFT;
                    joyMenu = MENU_KEY_LEFT;
                } else if (joyInfo.dwXpos > 0xc000) {
                    keyBits |= MENU_KEY_RIGHT;
                    joyMenu = MENU_KEY_RIGHT;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_Y) {
                if (joyInfo.dwYpos < 0x4000) {
                    joyMenu |= MENU_KEY_UP;
                    keyBits |= MENU_KEY_UP;
                } else if (joyInfo.dwYpos > 0xc000) {
                    joyMenu |= MENU_KEY_DOWN;
                    keyBits |= MENU_KEY_DOWN;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_BUTTONS) {
                if (joyInfo.dwButtons & 4)
                    joyMenu |= 0x2000;
                if (joyInfo.dwButtons & 2) {
                    joyMenu |= MENU_KEY_ACCEPT;
                    keyBits |= MENU_KEY_ACCEPT;
                }
                if (joyInfo.dwButtons & 1)
                    joyMenu |= 0x8000;
                if (joyInfo.dwButtons & 8) {
                    joyMenu |= MENU_KEY_BACK;
                    keyBits |= MENU_KEY_BACK;
                }
                if (joyInfo.dwButtons & JOY_BUTTON_10) {
                    joyMenu |= MENU_KEY_BACK;
                    keyBits |= MENU_KEY_BACK;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_X) {
                if (!rightKeyHeld && !leftKeyHeld)
                    steerAnalog = joyInfo.dwXpos >> 8;
                if (joyInfo.dwXpos < 0x4000)
                    joyPad = 0x80;
                else if (joyInfo.dwXpos > 0xc000)
                    joyPad = 0x20;
            }
            if (joyInfo.dwFlags & JOY_RETURN_Y) {
                if (joyInfo.dwYpos < 0x4000)
                    joyPad |= 0x10;
                else if (joyInfo.dwYpos > 0xc000)
                    joyPad |= 0x40;
            }
            if (joyInfo.dwFlags & JOY_RETURN_Z) {
                if (joyInfo.dwZpos > 0x4000)
                    joyPad |= PAD_BUTTON_THRUST;
            }
            if (joyInfo.dwFlags & JOY_RETURN_R) {
                if (joyInfo.dwRpos < 0x4000)
                    joyPad |= PAD_BUTTON_RIGHT_BRAKE;
                else if (joyInfo.dwRpos > 0xc000)
                    joyPad |= PAD_BUTTON_LEFT_BRAKE;
            }
            if (joyInfo.dwFlags & JOY_RETURN_BUTTONS) {
                if (keyBindings[3] & joyInfo.dwButtons)
                    joyPad |= PAD_BUTTON_RIGHT_BRAKE;
                if (keyBindings[4] & joyInfo.dwButtons)
                    joyPad |= PAD_BUTTON_LEFT_BRAKE;
                if (keyBindings[1] & joyInfo.dwButtons)
                    joyPad |= PAD_BUTTON_FIRE;
                if (joyInfo.dwButtons & keyBindings[0])
                    joyPad |= PAD_BUTTON_THRUST;
                if (keyBindings[6] & joyInfo.dwButtons)
                    joyPad |= PadTriangle0;
                if (keyBindings[2] & joyInfo.dwButtons)
                    joyPad |= PAD_BUTTON_DITCH_WEAPON;
                if (keyBindings[5] & joyInfo.dwButtons)
                    joyPad |= 8;
                if (keyBindings[7] & joyInfo.dwButtons)
                    joyPad |= 1;
            }
        }
    }
    if (mouseEnabled && mouseDevice) {
        count = 0x20;
        hr = mouseDevice->lpVtbl->GetDeviceData(mouseDevice, sizeof(DIDEVICEOBJECTDATA), mouseEvents, &count, 0);
        if (hr == DIERR_INPUTLOST)
            OutputDebugString("*** DIRECT INPUT *** Mouse Input Lost\n");
        if ((hr == 0 || hr == DI_BUFFEROVERFLOW) && count != 0) {
            i = count;
            j = 0;
            do {
                switch (mouseEvents[j].dwOfs) {
                case DIMOFS_X:
                    delta = (int)mouseEvents[j].dwData / mouseDivisor[mouseSensitivity];
                    mouseDelta = delta;
                    mouseX = steerAnalog;
                    mouseX += mouseDelta;
                    if (mouseX > 0xff)
                        mouseX = 0xff;
                    if (mouseX < 0)
                        mouseX = 0;
                    steerAnalog = mouseX;
                    break;
                case DIMOFS_BUTTON0:
                    if (padBindings[0] == 1)
                        buttonBit = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 1)
                        buttonBit = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 1)
                        buttonBit = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 1)
                        buttonBit = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 1)
                        buttonBit = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[j].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton0Held = 1;
                        mousePad |= buttonBit;
                    } else {
                        mouseButton0Held = 0;
                        mousePad &= ~buttonBit;
                    }
                    break;
                case DIMOFS_BUTTON1:
                    if (padBindings[0] == 2)
                        buttonBit = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 2)
                        buttonBit = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 2)
                        buttonBit = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 2)
                        buttonBit = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 2)
                        buttonBit = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[j].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton1Held = 1;
                        mousePad |= buttonBit;
                    } else {
                        mouseButton1Held = 0;
                        mousePad &= ~buttonBit;
                    }
                    break;
                case DIMOFS_BUTTON2:
                    if (padBindings[0] == 3)
                        buttonBit = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 3)
                        buttonBit = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 3)
                        buttonBit = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 3)
                        buttonBit = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 3)
                        buttonBit = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[j].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton2Held = 1;
                        mousePad |= buttonBit;
                    } else {
                        mouseButton2Held = 0;
                        mousePad &= ~buttonBit;
                    }
                    break;
                }
                j++;
            } while (--i != 0);
        }
    }
    if (!pauseRequested) {
        menuHeld |= menuBits;
        menuHeld |= joyMenu;
        menuLatch = menuHeld;
    }
    keysHeld = keyBits;
    menuKeys = ~prevMenuHeld & menuHeld;
    keysPressed = ~prevKeysHeld & keyBits;
    joyPressed = ~prevJoyButtons & joyButtons;
    padButtons |= keyboardPad;
    padButtons |= mousePad;
    padButtons |= joyPad;
    padPressed = ~prevPadButtons & padButtons;
}

#endif

#ifdef EQUIV
#ifdef NETPLAY
#pragma intrinsic(strlen)
/* Three joystick tests differ only in operand load order. */
/* Polls keyboard, joystick and mouse input for race control. */
void PollRaceControlInput(void)
{
    unsigned long i;
    u_short keyInput;
    int count;
    unsigned long codeIndex;
    u_short menuBits;
    unsigned long ofs;
    u_short mouseButtonMask; /* Explicit word masks preserve the /Od load order. */
    u_short joystickPadBits;
    long deviceCode;
    u_short menuButtons;

    menuBits = 0;
    keyInput = 0;
    menuButtons = 0;
    joystickPadBits = 0;
    prevMenuHeld = menuHeld;
    prevKeysHeld = keysHeld;
    prevJoyButtons = joyButtons;
    D_ab45cc = D_ab4e14;
    menuHeld = 0;
    keysHeld = 0;
    D_ab4e14 = 0;
    if (keyboardDevice != 0) {
    count = 0x20;
    deviceCode = keyboardDevice->lpVtbl->GetDeviceData(keyboardDevice, sizeof(DIDEVICEOBJECTDATA), keyboardEvents, &count, 0);
    if (deviceCode == DIERR_INPUTLOST) {
        deviceCode = keyboardDevice->lpVtbl->Acquire(keyboardDevice);
        deviceCode = keyboardDevice->lpVtbl->GetDeviceData(keyboardDevice, sizeof(DIDEVICEOBJECTDATA), keyboardEvents, &count, 0);
    }
    if (deviceCode == 0 || deviceCode == DI_BUFFEROVERFLOW) {
        if (deviceCode == DI_BUFFEROVERFLOW)
            OutputDebugString("*** DIRECT INPUT *** Buffer Overflow\n");
        for (i = 0; i < count; i++) {
            ofs = keyboardEvents[i].dwOfs;
            if (keyboardEvents[i].dwData & DI_EVENT_BUTTON_DOWN) {
                if (ofs == DIK_LEFT) {
                    menuBits |= MENU_KEY_LEFT;
                    keyInput |= MENU_KEY_LEFT;
                } else if (ofs == DIK_RIGHT) {
                    menuBits |= MENU_KEY_RIGHT;
                    keyInput |= MENU_KEY_RIGHT;
                }
                if (ofs == DIK_DOWN) {
                    menuBits |= MENU_KEY_DOWN;
                    keyInput |= MENU_KEY_DOWN;
                } else if (ofs == DIK_UP) {
                    menuBits |= MENU_KEY_UP;
                    keyInput |= MENU_KEY_UP;
                }
                if (ofs == DIK_RETURN) {
                    menuBits |= MENU_KEY_ACCEPT;
                    keyInput |= MENU_KEY_ACCEPT;
                }
                if (ofs == DIK_ESCAPE) {
                    menuBits |= MENU_KEY_BACK;
                    keyInput |= MENU_KEY_BACK;
                }
                g_keyboardCode = ofs;
                if (ofs == gameKeys[2])
                    keyboardPad |= PAD_BUTTON_DITCH_WEAPON;
                else if (ofs == gameKeys[0])
                    keyboardPad |= PAD_BUTTON_THRUST;
                else if (ofs == gameKeys[1])
                    keyboardPad |= PAD_BUTTON_FIRE;
                else if (ofs == gameKeys[10])
                    keyboardPad |= PadTriangle0;
                else if (ofs == gameKeys[3])
                    keyboardPad |= PAD_BUTTON_RIGHT_BRAKE;
                else if (ofs == gameKeys[4])
                    keyboardPad |= PAD_BUTTON_LEFT_BRAKE;
                else if (ofs == gameKeys[7]) {
                    leftKeyHeld = 1;
                    lastSteerKey = 1;
                } else if (ofs == gameKeys[8]) {
                    rightKeyHeld = 1;
                    lastSteerKey = 2;
                } else if (ofs == gameKeys[6])
                    keyboardPad |= PAD_BUTTON_REAR_FIRE;
                else if (ofs == gameKeys[5])
                    keyboardPad |= 0x10;
                else if (ofs == gameKeys[9])
                    keyboardPad |= 8;
                else if (ofs == gameKeys[11])
                    keyboardPad |= 1;
            } else {
                if (ofs == gameKeys[2])
                    keyboardPad &= ~PAD_BUTTON_DITCH_WEAPON;
                else if (ofs == gameKeys[0])
                    keyboardPad &= ~PAD_BUTTON_THRUST;
                else if (ofs == gameKeys[1])
                    keyboardPad &= ~PAD_BUTTON_FIRE;
                else if (ofs == gameKeys[10])
                    keyboardPad &= ~PadTriangle0;
                else if (ofs == gameKeys[3])
                    keyboardPad &= ~PAD_BUTTON_RIGHT_BRAKE;
                else if (ofs == gameKeys[4])
                    keyboardPad &= ~PAD_BUTTON_LEFT_BRAKE;
                else if (ofs == gameKeys[7])
                    leftKeyHeld = 0;
                else if (ofs == gameKeys[8])
                    rightKeyHeld = 0;
                else if (ofs == gameKeys[6])
                    keyboardPad &= ~PAD_BUTTON_REAR_FIRE;
                else if (ofs == gameKeys[5])
                    keyboardPad &= ~0x10;
                else if (ofs == gameKeys[9])
                    keyboardPad &= ~8;
                else if (ofs == gameKeys[11])
                    keyboardPad &= ~1;
                for (codeIndex = 9; codeIndex > 0; codeIndex--)
                    cheatBuffer[codeIndex] = cheatBuffer[codeIndex - 1];
                cheatBuffer[0] = g_keyCharTable[ofs];
                for (codeIndex = 0; (unsigned)codeIndex < 9; codeIndex++) {
                    if (strncmp(cheatBuffer, cheatCodes[codeIndex], strlen(cheatCodes[codeIndex])) == 0) {
                    if (codeIndex == 8) {
                        if (debugTextFlag)
                            debugTextFlag = 0;
                        else
                            debugTextFlag = 1;
                    } else if (raceInProgress == 1) {
                        switch (codeIndex) {
                        case 0:
                            if (gameData->weaponsOnSigned) {
                                cheatWeapon ^= 1;
                                if (cheatWeapon)
                                    GetNewWeapon(shipIndex, ownShip);
                                cheatfade = 255;
                            }
                            break;
                        case 1:
                            cheatDamage = cheatDamage ^ 1;
                            cheatfade = 255;
                            break;
                        case 2:
                            cheatTime = cheatTime ^ 1;
                            if (cheatTime)
                                checkTimer = 999;
                            cheatfade = 255;
                            break;
                        case 3:
                            cheatDrone = cheatDrone ^ 1;
                            cheatfade = 255;
                            break;
                        }
                    } else if (raceInProgress == 2) {
                        switch (codeIndex) {
                        case 4:
                            OutputDebugString("Piranha ship cheat:\n");
                            tempGameData->teamCheat = 1;
                            break;
                        case 5:
                            tempGameData->phantomCheat = 1;
                            menu[2].options[3].enabled = 1;
                            OutputDebugString("Phantom class cheat:\n");
                            break;
                        case 6:
                            OutputDebugString("Track cheat:\n");
                            tempGameData->trackCheat = 1;
                            menu[2].options[3].enabled = 1;
                            break;
                        }
                    } else if (raceInProgress == 3) {
                        if (codeIndex == 7)
                            sillyShips = 1;
                    }
                    }
                    }
                }
            if (leftKeyHeld && rightKeyHeld) {
                if (lastSteerKey == 1) {
                    D_abbf94 = 0;
                    keyboardPad |= 0x80;
                } else {
                    D_abbf94 = 0xff;
                    keyboardPad |= 0x20;
                }
            } else if (leftKeyHeld) {
                D_abbf94 = 0;
                keyboardPad |= 0x80;
            } else if (rightKeyHeld) {
                D_abbf94 = 0xff;
                keyboardPad |= 0x20;
            } else {
                D_abbf94 = 0x80;
                keyboardPad &= ~0x80;
                keyboardPad &= ~0x20;
            }
        }
    }
    if (joystickPresent) {
        joyResult = joyGetPosEx(0, &joyInfo);
        joyButtons = joyInfo.dwButtons;
        if (joyResult == 0) {
            if (joyInfo.dwFlags & JOY_RETURN_X) {
                if (joyInfo.dwXpos < 0x4000) {
                    menuButtons |= MENU_KEY_LEFT;
                    keyInput |= MENU_KEY_LEFT;
                } else if (joyInfo.dwXpos > 0xc000) {
                    menuButtons |= MENU_KEY_RIGHT;
                    keyInput |= MENU_KEY_RIGHT;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_Y) {
                if (joyInfo.dwYpos < 0x4000) {
                    menuButtons |= MENU_KEY_UP;
                    keyInput |= MENU_KEY_UP;
                } else if (joyInfo.dwYpos > 0xc000) {
                    menuButtons |= MENU_KEY_DOWN;
                    keyInput |= MENU_KEY_DOWN;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_BUTTONS) {
                if (joyInfo.dwButtons & 4)
                    menuButtons |= 0x2000;
                if (joyInfo.dwButtons & 2) {
                    menuButtons |= MENU_KEY_ACCEPT;
                    keyInput |= MENU_KEY_ACCEPT;
                }
                if (joyInfo.dwButtons & 1)
                    menuButtons |= 0x8000;
                if (joyInfo.dwButtons & 8) {
                    menuButtons |= MENU_KEY_BACK;
                    keyInput |= MENU_KEY_BACK;
                }
                if (joyInfo.dwButtons & JOY_BUTTON_10) {
                    menuButtons |= MENU_KEY_BACK;
                    keyInput |= MENU_KEY_BACK;
                }
            }
            if (joyInfo.dwFlags & JOY_RETURN_X) {
                if (!rightKeyHeld && !leftKeyHeld)
                    D_abbf94 = joyInfo.dwXpos >> 8;
                if (joyInfo.dwXpos < 0x4000)
                    joystickPadBits |= 0x80;
                else if (joyInfo.dwXpos > 0xc000)
                    joystickPadBits |= 0x20;
            }
            if (joyInfo.dwFlags & JOY_RETURN_Y) {
                if (joyInfo.dwYpos < 0x4000)
                    joystickPadBits |= 0x10;
                else if (joyInfo.dwYpos > 0xc000)
                    joystickPadBits |= 0x40;
            }
            if (joyInfo.dwFlags & JOY_RETURN_Z) {
                if (joyInfo.dwZpos > 0x4000)
                    joystickPadBits |= PAD_BUTTON_THRUST;
            }
            if (joyInfo.dwFlags & JOY_RETURN_R) {
                if (joyInfo.dwRpos < 0x4000)
                    joystickPadBits |= PAD_BUTTON_RIGHT_BRAKE;
                else if (joyInfo.dwRpos > 0xc000)
                    joystickPadBits |= PAD_BUTTON_LEFT_BRAKE;
            }
            if (joyInfo.dwFlags & JOY_RETURN_BUTTONS) {
                if (joyInfo.dwButtons & keyBindings[3])
                    joystickPadBits |= PAD_BUTTON_RIGHT_BRAKE;
                if (keyBindings[4] & joyInfo.dwButtons)
                    joystickPadBits |= PAD_BUTTON_LEFT_BRAKE;
                if (keyBindings[1] & joyInfo.dwButtons)
                    joystickPadBits |= PAD_BUTTON_FIRE;
                if (joyInfo.dwButtons & keyBindings[0])
                    joystickPadBits |= PAD_BUTTON_THRUST;
                if (keyBindings[6] & joyInfo.dwButtons)
                    joystickPadBits |= PadTriangle0;
                if (joyInfo.dwButtons & keyBindings[2])
                    joystickPadBits |= PAD_BUTTON_DITCH_WEAPON;
                if (keyBindings[5] & joyInfo.dwButtons)
                    joystickPadBits |= 8;
                if (joyInfo.dwButtons & keyBindings[7])
                    joystickPadBits |= 1;
            }
        }
    }
    if (mouseEnabled && mouseDevice) {
        count = 0x20;
        deviceCode = mouseDevice->lpVtbl->GetDeviceData(mouseDevice, sizeof(DIDEVICEOBJECTDATA), mouseEvents, &count, 0);
        if (deviceCode == DIERR_INPUTLOST)
            OutputDebugString("*** DIRECT INPUT *** Mouse Input Lost\n");
        if (deviceCode == 0 || deviceCode == DI_BUFFEROVERFLOW) {
            for (codeIndex = 0; (unsigned long)count > codeIndex; codeIndex++) {
                switch (mouseEvents[codeIndex].dwOfs) {
                case DIMOFS_X:
                    mouseDelta = (int)mouseEvents[codeIndex].dwData / mouseDivisor[mouseSensitivity];
                    mouseX = D_abbf94 + mouseDelta;
                    if (mouseX > 0xff)
                        mouseX = 0xff;
                    if (mouseX < 0)
                        mouseX = 0;
                    D_abbf94 = mouseX;
                    break;
                case DIMOFS_BUTTON0:
                    if (padBindings[0] == 1)
                        mouseButtonMask = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 1)
                        mouseButtonMask = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 1)
                        mouseButtonMask = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 1)
                        mouseButtonMask = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 1)
                        mouseButtonMask = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[codeIndex].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton0Held = 1;
                        mousePad = (mouseButtonMask & 0xffff) | mousePad;
                    } else {
                        mouseButton0Held = 0;
                        mousePad &= ~mouseButtonMask;
                    }
                    break;
                case DIMOFS_BUTTON1:
                    if (padBindings[0] == 2)
                        mouseButtonMask = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 2)
                        mouseButtonMask = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 2)
                        mouseButtonMask = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 2)
                        mouseButtonMask = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 2)
                        mouseButtonMask = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[codeIndex].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton1Held = 1;
                        mousePad = (mouseButtonMask & 0xffff) | mousePad;
                    } else {
                        mouseButton1Held = 0;
                        mousePad &= ~mouseButtonMask;
                    }
                    break;
                case DIMOFS_BUTTON2:
                    if (padBindings[0] == 3)
                        mouseButtonMask = PAD_BUTTON_THRUST;
                    else if (padBindings[1] == 3)
                        mouseButtonMask = PAD_BUTTON_FIRE;
                    else if (padBindings[2] == 3)
                        mouseButtonMask = PAD_BUTTON_DITCH_WEAPON;
                    else if (padBindings[3] == 3)
                        mouseButtonMask = PAD_BUTTON_RIGHT_BRAKE;
                    else if (padBindings[4] == 3)
                        mouseButtonMask = PAD_BUTTON_LEFT_BRAKE;
                    if (mouseEvents[codeIndex].dwData & DI_EVENT_BUTTON_DOWN) {
                        mouseButton2Held = 1;
                        mousePad = (mouseButtonMask & 0xffff) | mousePad;
                    } else {
                        mouseButton2Held = 0;
                        mousePad &= ~mouseButtonMask;
                    }
                    break;
                }
            }
        }
    }
    if (!pauseRequested) {
        menuHeld |= menuBits;
        menuHeld |= menuButtons;
        menuLatch = menuHeld;
    }
    keysHeld = keyInput;
    menuKeys = (prevMenuHeld ^ menuHeld) & menuHeld;
    keysPressed = (prevKeysHeld ^ keysHeld) & keysHeld;
    joyPressed = (prevJoyButtons ^ joyButtons) & joyButtons;
    D_ab4e14 = keyboardPad | D_ab4e14;
    D_ab4e14 = mousePad | D_ab4e14;
    D_ab4e14 = joystickPadBits | D_ab4e14;
    /* The original reads the old pressed latch in this XOR. */
    D_ab4e1c = (D_ab45cc ^ D_ab4e1c) & D_ab4e14;
    }
}
#endif
#endif


#endif

#ifndef NETPLAY
int PollJoystick(void)
{
    if (joystickPresent) {
        joyButtonPrev = joyButtonNow;
        joyResult = joyGetPosEx(0, &joyInfo);
        joyButtons = joyInfo.dwButtons;
        if (joyResult == 0 && (joyInfo.dwFlags & JOY_RETURN_BUTTONS)) {
            if (joyInfo.dwButtons & 4)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & 2)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & 1)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & 8)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & JOY_BUTTON_10)
                joyButtonNow = 1;
        }
    }
    return joyButtonPrev != joyButtonNow;
}
#endif

#ifdef NETPLAY
int PollJoystick(void)
{
    if (joystickPresent) {
        joyButtonPrev = joyButtonNow;
        joyResult = joyGetPosEx(0, &joyInfo);
        joyButtons = joyInfo.dwButtons;
        if (joyResult == 0 && (joyInfo.dwFlags & JOY_RETURN_BUTTONS)) {
            if (joyInfo.dwButtons & 4)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & 2)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & 1)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & 8)
                joyButtonNow = 1;
            if (joyInfo.dwButtons & JOY_BUTTON_10)
                joyButtonNow = 1;
        }
    }
    if (joyButtonPrev != joyButtonNow)
        return 1;
    else
        return 0;
}
#endif

#ifdef POWERVR
void Stub_4291B0(void)
{
}

void Stub_4291C0(void)
{
}

void Stub_42B6C0(void)
{
}

extern long gsxy[];
extern long gsz[];
extern long LCM[];

void InputOrAsset_42C870(long *destination)
{
    memcpy(destination, LCM, 32);
}

void InputOrAsset_42C930(long *x, long *y, long *z)
{
    *x = gsxy[0];
    *y = gsxy[1];
    *z = gsxy[2];
}

void InputOrAsset_42C960(long *x, long *y, long *z)
{
    *x = gsz[1];
    *y = gsz[2];
    *z = gsz[3];
}
#endif
