#ifndef D3DSETUP_H
#define D3DSETUP_H

#include <windows.h>

enum {
    SETUP_START_BUTTON_ID = 0x64,
    SETUP_QUIT_BUTTON_ID = 0x65,
    SETUP_RESOLUTION_COMBO_ID = 0x68,
    SETUP_SOUND_COMBO_ID = 0x69,
    SETUP_PROVIDER_COMBO_ID = 0x6a,
    SETUP_DRIVER_COMBO_RESOURCE_ID = 0x3f8,
    SETUP_DEVICE_COMBO_RESOURCE_ID = 0x3f9,
#ifdef NETPLAY
    SETUP_DIALOG_FIRST_CONTROL_ID = 0x71,
#else
    SETUP_DIALOG_FIRST_CONTROL_ID = 0x70,
#endif
    SETUP_ADVANCED_BUTTON_ID = 0x400,
    SETUP_DRIVER_NOTE_ID = 0x401,
#ifdef NETPLAY
    SETUP_DIALOG_ID = 0x77,
#else
    SETUP_DIALOG_ID = 0x76,
#endif
    SETUP_ADVANCED_DIALOG_ID = 9
};

/* A control in the display options window, 0x18 bytes: position and size in dialog units, the text, and the
   window style without WS_CHILD. */
typedef struct SetupControl {
    int x, y, width, height;
    char *text;
    DWORD style;
} SetupControl;

extern SetupControl mainButtonDefs[4];
extern SetupControl mainComboDefs[2];
extern SetupControl mainLabelDefs[6];
extern SetupControl setupButtonDefs[2];
extern SetupControl setupComboDefs[2];
extern SetupControl setupGroupDefs[2];
extern HWND dialogButtons[2];
extern HWND dialogGroups[2];

void FillDisplayComboBoxes(void);
int CreateSetupControls(void);
int CreateDisplayOptionsWindow(void);

#endif
