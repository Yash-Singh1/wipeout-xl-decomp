#include <windows.h>
#include <ddraw.h>
#include "pc_window.h"
#include "pc_textures.h"
#include "pc_render.h"
#include "d3dsetup.h"
#include "d3denum.h"

enum {
    SETUP_MAIN_BUTTON_COUNT = 4,
#ifdef NETPLAY
    SETUP_MAIN_COMBO_COUNT = 3,
    SETUP_MAIN_LABEL_COUNT = 7,
#else
    SETUP_MAIN_COMBO_COUNT = 2,
    SETUP_MAIN_LABEL_COUNT = 6,
#endif
    SETUP_DIALOG_CONTROL_COUNT = 2,
    SETUP_WINDOW_WIDTH = 0x1a8,
    SETUP_WINDOW_HEIGHT = 0xc8,
    SETUP_ARROW_CURSOR_RESOURCE_ID = 0x7f00
};

#ifdef EQUIV
/* Same computation as the exe, with different registers. Picks the default display format
   (accelerated device, else MMX-capable Pentium, else 8-bit), then
   creates the main setup window's buttons, combo boxes and labels and fills the sound mode list. */
int CreateSetupControls(void)
{
#ifndef NETPLAY
    SYSTEM_INFO info;
    int mmx;
    int cpuHasMmx;
#endif
    int n;
    int id = SETUP_START_BUTTON_ID;
#ifdef NETPLAY
    int result;
#else
    int soundMode;
#endif

    if (!videoSettingsLoaded) {
        if (acceleratorAvailable) {
            videoModeIndex = accelDriverIndex;
            pixelFormatGroup = accelDeviceIndex;
            displayBits = 16;
            formatOverride = 1;
            rgbTextures = 1;
            texture16Bit = 1;
        } else {
#ifdef NETPLAY
            if (HasMMX()) {
                videoModeIndex = 1;
                pixelFormatGroup = 0;
                displayBits = 16;
                formatOverride = 1;
                rgbTextures = 1;
                texture16Bit = 1;
            } else {
                videoModeIndex = 0;
                pixelFormatGroup = 0;
                displayBits = 8;
                formatOverride = 0;
                rgbTextures = 0;
                texture16Bit = 0;
            }
#else
            mmx = 0;
            GetSystemInfo(&info);
            if (info.dwProcessorType == PROCESSOR_INTEL_PENTIUM) {
                __asm {
                    mov eax, 1
                    _emit 0x0f
                    _emit 0xa2
                    test edx, 0800000h
                    jnz mmx_detected
                    jmp no_mmx_detected
                mmx_detected:
                    mov mmx, 1
                no_mmx_detected:
                }
                cpuHasMmx = mmx;
            } else {
                cpuHasMmx = 0;
            }
            if (cpuHasMmx) {
                pixelFormatGroup = 0;
                displayBits = 16;
                videoModeIndex = 1;
                formatOverride = 1;
                rgbTextures = 1;
                texture16Bit = 1;
            } else {
                displayBits = 8;
                videoModeIndex = 0;
                pixelFormatGroup = 0;
                formatOverride = 0;
                rgbTextures = 0;
                texture16Bit = 0;
            }
#endif
        }
    } else {
        if (displayBits == 16) {
            formatOverride = 1;
            rgbTextures = 1;
            texture16Bit = 1;
        } else {
            formatOverride = 0;
            rgbTextures = 0;
            texture16Bit = 0;
        }
    }
    for (n = 0; n < SETUP_MAIN_BUTTON_COUNT; n++) {
        setupButtons[n] = CreateWindowExA(0, "BUTTON", mainButtonDefs[n].text, mainButtonDefs[n].style | WS_CHILD | WS_VISIBLE,
                                          mainButtonDefs[n].x, mainButtonDefs[n].y, mainButtonDefs[n].width, mainButtonDefs[n].height,
                                          g_hwnd, (HMENU)id, NULL, NULL);
        if (setupButtons[n] == NULL) {
            ShowMessage("CreateWindowEx failed");
            return 0;
        }
        ShowWindow(setupButtons[n], SW_SHOWNORMAL);
        id++;
    }
    for (n = 0; n < SETUP_MAIN_COMBO_COUNT; n++) {
        setupCombos[n] = CreateWindowExA(0, "COMBOBOX", mainComboDefs[n].text, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                         mainComboDefs[n].x, mainComboDefs[n].y, mainComboDefs[n].width, mainComboDefs[n].height,
                                         g_hwnd, (HMENU)id, NULL, NULL);
        if (setupCombos[n] == NULL) {
            ShowMessage("CreateWindowEx failed");
            return 0;
        }
        ShowWindow(setupCombos[n], SW_SHOWNORMAL);
        id++;
    }
    for (n = 0; n < SETUP_MAIN_LABEL_COUNT; n++) {
        setupLabels[n] = CreateWindowExA(0, "STATIC", mainLabelDefs[n].text, mainLabelDefs[n].style | WS_CHILD | WS_VISIBLE,
                                         mainLabelDefs[n].x, mainLabelDefs[n].y, mainLabelDefs[n].width, mainLabelDefs[n].height,
                                         g_hwnd, (HMENU)id, NULL, NULL);
        if (setupLabels[n] == NULL) {
            ShowMessage("CreateWindowEx failed");
            return 0;
        }
        ShowWindow(setupLabels[n], SW_SHOWNORMAL);
        id++;
    }
#ifdef NETPLAY
    result = SendMessageA(setupCombos[1], CB_RESETCONTENT, 0, 0);
    result = SendMessageA(setupCombos[1], CB_ADDSTRING, 0, (LPARAM)"Mono");
    result = SendMessageA(setupCombos[1], CB_ADDSTRING, 0, (LPARAM)"Stereo");
    result = SendMessageA(setupCombos[1], CB_ADDSTRING, 0, (LPARAM)"Dolby Surround");
    switch (sfxMode) {
    case 1:
        result = SendMessageA(setupCombos[1], CB_SETCURSEL, 0, 0);
        break;
    case 2:
        result = SendMessageA(setupCombos[1], CB_SETCURSEL, 1, 0);
        break;
    case 5:
        result = SendMessageA(setupCombos[1], CB_SETCURSEL, 2, 0);
        break;
    }
    UpdateSetupDisplayOptions();
    return 1;
#else
    SendMessageA(setupCombos[1], CB_RESETCONTENT, 0, 0);
    SendMessageA(setupCombos[1], CB_ADDSTRING, 0, (LPARAM)"Mono");
    SendMessageA(setupCombos[1], CB_ADDSTRING, 0, (LPARAM)"Stereo");
    SendMessageA(setupCombos[1], CB_ADDSTRING, 0, (LPARAM)"Dolby Surround");
    soundMode = -1;
    switch (sfxMode) {
    case 1:
        soundMode = 0;
        break;
    case 2:
        soundMode = 1;
        break;
    case 5:
        soundMode = 2;
        break;
    }
    if (soundMode >= 0) {
        SendMessageA(setupCombos[1], CB_SETCURSEL, soundMode, 0);
    }
    UpdateSetupDisplayOptions();
    return 1;
#endif
}
#endif

#ifdef EQUIV
/* Not byte-exact, but equivalent: the literal order matches the original (d3dsetup.c is built without string pooling, as
   the original was: /O2 would merge the repeated "PSYGWO22" and "BUTTON" literals), but the window class stores and the
   loop registers differ. Registers the display options window class and creates the Advanced Options window centred on
   the desktop, with its OK and Cancel buttons, the two group boxes, and the device and display driver combo boxes. Returns 0
   when registration or any creation fails. */
int CreateDisplayOptionsWindow(void)
{
    WNDCLASSA wc;
    RECT rect;
    int x, y;
    int id;
    int i;

    id = SETUP_DIALOG_FIRST_CONTROL_ID;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DisplayOptionsProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIconA(g_hInstance, "AppIcon");
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)SETUP_ARROW_CURSOR_RESOURCE_ID);
    wc.hbrBackground = windowBrush;
    wc.lpszMenuName = "AppMenu2";
    wc.lpszClassName = "PSYGWO22";
    if (RegisterClassA(&wc) == 0) {
        return 0;
    }
    GetWindowRect(GetDesktopWindow(), &rect);
    x = (rect.right - rect.left - SETUP_WINDOW_WIDTH) / 2;
    y = (rect.bottom - rect.top - SETUP_WINDOW_HEIGHT) / 2;
    setupWindow = CreateWindowExA(WS_EX_APPWINDOW, "PSYGWO22", "Advanced Options",
                                  WS_POPUP | WS_CHILD | WS_VISIBLE | WS_CAPTION,
                                  x, y, SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT, g_hwnd,
                                  (HMENU)SETUP_DIALOG_ID, g_hInstance, NULL);
    if (setupWindow == NULL) {
        ShowMessage("CreateWindowEx failed");
        return 0;
    }
    for (i = 0; i < SETUP_DIALOG_CONTROL_COUNT; i++) {
        dialogButtons[i] = CreateWindowExA(0, "BUTTON", setupButtonDefs[i].text, setupButtonDefs[i].style | WS_CHILD | WS_VISIBLE,
                                           setupButtonDefs[i].x, setupButtonDefs[i].y, setupButtonDefs[i].width, setupButtonDefs[i].height,
                                           setupWindow, (HMENU)id, NULL, NULL);
        /* The original tests dialogButtons[0] on every pass, not the button it just created. */
        if (dialogButtons[0] == NULL) {
            ShowMessage("CreateWindowEx failed");
            return 0;
        }
        ShowWindow(dialogButtons[i], SW_SHOWNORMAL);
        id++;
    }
    for (i = 0; i < SETUP_DIALOG_CONTROL_COUNT; i++) {
        dialogGroups[i] = CreateWindowExA(0, "BUTTON", setupGroupDefs[i].text, setupGroupDefs[i].style | WS_CHILD | WS_VISIBLE,
                                          setupGroupDefs[i].x, setupGroupDefs[i].y, setupGroupDefs[i].width, setupGroupDefs[i].height,
                                          setupWindow, (HMENU)id, NULL, NULL);
        if (dialogGroups[i] == NULL) {
            ShowMessage("CreateWindowEx failed");
            return 0;
        }
        ShowWindow(dialogGroups[i], SW_SHOWNORMAL);
        id++;
    }
    for (i = 0; i < SETUP_DIALOG_CONTROL_COUNT; i++) {
        (&formatComboBox)[i] = CreateWindowExA(0, "COMBOBOX", setupComboDefs[i].text, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                               setupComboDefs[i].x, setupComboDefs[i].y, setupComboDefs[i].width, setupComboDefs[i].height,
                                               setupWindow, (HMENU)id, NULL, NULL);
        if ((&formatComboBox)[i] == NULL) {
            ShowMessage("CreateWindowEx failed");
            return 0;
        }
        ShowWindow((&formatComboBox)[i], SW_SHOWNORMAL);
        id++;
    }
    FillDisplayComboBoxes();
    return 1;
}
#endif

#ifdef EQUIV
/* Equivalent; the compiler schedules the mode index reset before the combo-box call. */
void FillDisplayComboBoxes(void)
{
    int i;
    int textured;

    if (formatOverride) {
        displayBits = 16;
        textured = 1;
    } else {
        displayBits = 8;
        textured = 0;
    }
    rgbTextures = textured;
    texture16Bit = textured;
    SendMessageA(formatComboBox, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < displayDrivers[videoModeIndex].deviceCount; i++) {
        SendMessageA(formatComboBox, CB_ADDSTRING, 0, (LPARAM)displayDrivers[videoModeIndex].devices[i].name);
    }
    SendMessageA(formatComboBox, CB_SETCURSEL, pixelFormatGroup, 0);
    SendMessageA(modeComboBox, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < displayDriverCount; i++) {
        SendMessageA(modeComboBox, CB_ADDSTRING, 0, (LPARAM)displayDrivers[i].name);
    }
    SendMessageA(modeComboBox, CB_SETCURSEL, videoModeIndex, 0);
}
#endif
