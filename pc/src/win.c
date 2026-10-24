#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <mmsystem.h>

#include "pc_window.h"
#include "pc_input.h"
#include "pc_config.h"
#include "d3dsetup.h"
#ifdef NETPLAY
#include "gst.h"
#include "cd_tracks.h"
#endif
#ifdef POWERVR
#include "pc_textures.h"
#include "pc_render.h"
#endif

/* Dialog procedure: accepts the initial focus and closes the dialog when OK is pressed. */
BOOL __stdcall SetupDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hwnd, 1);
        }
        break;
    default:
        return 0;
    }
    return 0;
}

#if defined(NON_MATCHING) || defined(EQUIV)
static __inline void DrawSetupBitmap(HDC dc, HBITMAP image, short x, short y)
{
    BITMAP bitmap;
    POINT size;
    POINT origin;
    HDC memoryDC;

    memoryDC = CreateCompatibleDC(dc);
    SelectObject(memoryDC, image);
    SetMapMode(memoryDC, GetMapMode(dc));
    GetObjectA(image, sizeof(BITMAP), &bitmap);
    size.x = bitmap.bmWidth;
    size.y = bitmap.bmHeight;
    DPtoLP(dc, &size, 1);
    origin.x = 0;
    origin.y = 0;
    DPtoLP(memoryDC, &origin, 1);
    BitBlt(dc, x, y, size.x, size.y, memoryDC, origin.x, origin.y, SRCCOPY);
    DeleteDC(memoryDC);
}

static __inline void SelectSetupResolution(short selection)
{
    int resolution;
    int available;
    int i;

    resolutionSelection = selection;
    resolution = 0;
    i = 0;
    available = 0;
    for (; i < 6; i++) {
        if (displayBits == 16) {
            if (displayDrivers[videoModeIndex].supported16[pixelFormatGroup][i]) {
                if (resolutionSelection == available) {
                    pixelFormatIndex = resolution;
                    break;
                }
                available++;
            }
        } else {
            if (displayDrivers[videoModeIndex].supported8[pixelFormatGroup][i]) {
                if (resolutionSelection == available) {
                    pixelFormatIndex = resolution;
                    break;
                }
                available++;
            }
        }
        resolution++;
    }
}

#endif

#ifdef EQUIV
#ifdef NETPLAY
extern int gstProviderChoice;
extern signed char networkGame;
extern HWND connectionListBox;
extern HWND soundModeComboBox;
extern short netCdTrackError;
int InitNetwork(void);

/* Dispatches the setup window's paint, device, media and network control messages. */
LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    PAINTSTRUCT paintData;
    LRESULT procResult;
    WORD ctrlId;
    int messageHandled;
    HDC deviceContext;
    int button;
    int choiceType;
    WORD code;

    if (!ClearResultAndReturnOne(&messageHandled, &procResult, hwnd, msg, wParam, lParam)) {
        PostQuitMessage(0);
        return 0;
    }
    if (messageHandled != 0) {
        return procResult;
    }

    switch (msg) {
    case WM_GETMINMAXINFO:
    case WM_INITMENU:
        break;
    case WM_ERASEBKGND:
        deviceContext = (HDC)wParam;
        GetClientRect(hwnd, &rect);
        if (gameWindowActive) {
            FillRect(deviceContext, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
        } else {
            FillRect(deviceContext, &rect, windowBrush);
        }
        return 1;
    case WM_SYSCOLORCHANGE:
        DeleteObject(windowBrush);
        windowBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        break;
    case WM_NCACTIVATE:
        if (!wParam && !g_cdValidating) {
            return 0;
        }
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        break;
    case WM_LBUTTONDBLCLK:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
        break;
    case WM_DEVICECHANGE:
        if (!CdIsDoorOpen() && netCdTrackError == 0 && !networkGame) {
            cdMusicTrack = 0;
            SetMenuCDTrack(menuTextureA, menuTextureB, gameData);
            cdTrackChanged = 1;
        }
        break;
    case MM_MCINOTIFY:
        switch (wParam & 0xffff) {
        case MCI_NOTIFY_ABORTED:
            break;
        case MCI_NOTIFY_SUPERSEDED:
            break;
        case MCI_NOTIFY_FAILURE:
            break;
        case MCI_NOTIFY_SUCCESSFUL:
            if (!networkGame) {
                SetMenuCDTrack(menuTextureA, menuTextureB, gameData);
            }
            break;
        }
        break;
    case WM_MBUTTONUP:
    case WM_ENTERMENULOOP:
    case WM_EXITMENULOOP:
        break;
    case WM_COMMAND:
        code = HIWORD(wParam);
        ctrlId = LOWORD(wParam);
        if (code == 1) {
            if (ctrlId == 0x68) {
                resolutionSelection = (short)SendMessageA(setupCombos[0], CB_GETCURSEL, 0, 0);
                pixelFormatIndex = SelectedResolutionIndex();
                UpdateSetupDisplayOptions();
            } else if (ctrlId == 0x69) {
                switch (SendMessageA(soundModeComboBox, 0x147, 0, 0)) {
                case 0:
                    sfxMode = 1;
                    break;
                case 1:
                    sfxMode = 2;
                    break;
                case 2:
                    sfxMode = 5;
                    break;
                }
            } else if (ctrlId == 0x6a) {
                choiceType = SendMessageA(connectionListBox, CB_GETCURSEL, 0, 0);
                gstProviderChoice = (unsigned char)choiceType;
            }
        } else if (code == 0) {
            if (ctrlId == 0x64) {
                if (gstProviderChoice == 0 && netCdTrackError == 1) {
                    while (netCdTrackError == 1) {
                        g_cdValidating = 1;
                        button = MessageBoxA(g_hwnd,
                                             "Make sure Wipeout XL CD is in the CD drive.\n",
                                             "Wipeout XL CD Validator",
                                             MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL);
                        g_cdValidating = 0;
                        if (button == IDOK) {
                            CdReadTrackTable();
                        } else {
                            return 0;
                        }
                    }
                }
                if (!InitNetwork()) {
                    gstDebugPrintf("GPI Initialization Failed");
                    break;
                } else {
                    startGameRequested = 1;
                }
            } else if (ctrlId == 0x65) {
                PostQuitMessage(0);
            }
            return 0;
        } else {
            switch (wParam & 0xffff) {
            case 2:
                PostQuitMessage(0);
                break;
            }
            return 0;
        }
        /* Selection changes redraw the setup window. */
    case WM_PAINT:
        deviceContext = BeginPaint(hwnd, &paintData);
        if (setupWindowActive) {
            DrawWindowBitmap(deviceContext, setupLogo, 13, 14);
            DrawWindowBitmap(deviceContext, setupBanner, 18, 120);
        }
        EndPaint(hwnd, &paintData);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
#else
/* Handles setup controls and media notifications; resolution selection schedules differently. */
LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC dc;
    WORD command;
    WORD notification;

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_ERASEBKGND:
        dc = (HDC)wParam;
        GetClientRect(hwnd, &rect);
        if (gameWindowActive) {
            FillRect(dc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return 1;
        } else {
            FillRect(dc, &rect, windowBrush);
            return 1;
        }
    case WM_SYSCOLORCHANGE:
        DeleteObject(windowBrush);
        windowBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        break;
    case WM_NCACTIVATE:
        if (!wParam && !g_cdValidating) {
            return 0;
        }
        break;
    case WM_COMMAND:
        command = LOWORD(wParam);
        notification = HIWORD(wParam);
        if (notification == CBN_SELCHANGE) {
            if (command == SETUP_RESOLUTION_COMBO_ID) {
                SelectSetupResolution((short)SendMessageA(setupCombos[0], CB_GETCURSEL, 0, 0));
                UpdateSetupDisplayOptions();
            } else if (command == SETUP_SOUND_COMBO_ID) {
                switch (SendMessageA(setupCombos[1], CB_GETCURSEL, 0, 0)) {
                case 0: sfxMode = 1; break;
                case 1: sfxMode = 2; break;
                case 2: sfxMode = 5; break;
                }
            }
        } else if (notification == 0) {
            if (command == SETUP_START_BUTTON_ID) {
                startGameRequested = 1;
                return 0;
            } else if (command == SETUP_QUIT_BUTTON_ID) {
                PostQuitMessage(0);
            }
            return 0;
        } else {
            wParam = LOWORD(wParam);
            if (wParam == IDCANCEL) {
                PostQuitMessage(0);
            }
            return 0;
        }
        /* Selection changes redraw the setup window. */
    case WM_PAINT:
        dc = BeginPaint(hwnd, &ps);
        if (setupWindowActive) {
            DrawSetupBitmap(dc, setupLogo, 13, 14);
            DrawSetupBitmap(dc, setupBanner, 18, 98);
        }
        EndPaint(hwnd, &ps);
        return 0;
    case WM_INITMENU:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_ENTERMENULOOP:
    case WM_EXITMENULOOP:
        break;
    case WM_DEVICECHANGE:
        if (!CdIsDoorOpen() && g_cdTrackError == 0) {
            cdMusicTrack = 0;
            SetMenuCDTrack(menuTextureA, menuTextureB, gameData);
            cdTrackChanged = 1;
        }
        break;
    case MM_MCINOTIFY:
        if ((wParam & 0xffff) == MCI_NOTIFY_SUCCESSFUL) {
            SetMenuCDTrack(menuTextureA, menuTextureB, gameData);
        }
        break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
#endif
#endif

#ifdef NETPLAY
/* Display options dialog: fills the mode and format lists, takes their selections and opens the advanced dialog. */
BOOL __stdcall DisplayOptionsProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    WORD notify;
    WORD command;
    int answer;
    DLGPROC proc;

    switch (msg) {
    case WM_INITDIALOG:
        modeComboBox = GetDlgItem(hDlg, SETUP_DRIVER_COMBO_RESOURCE_ID);
        formatComboBox = GetDlgItem(hDlg, SETUP_DEVICE_COMBO_RESOURCE_ID);
        FillDisplayComboBoxes();
        return 1;
    case WM_COMMAND:
        notify = HIWORD(wParam);
        command = LOWORD(wParam);
        if (notify == CBN_SELCHANGE) {
            if (command == SETUP_DRIVER_COMBO_RESOURCE_ID) {
                videoModeIndex = SendMessageA(modeComboBox, CB_GETCURSEL, 0, 0);
                pixelFormatGroup = 0;
                formatOverride = 0;
                FillDisplayComboBoxes();
            } else if (command == SETUP_DEVICE_COMBO_RESOURCE_ID) {
                pixelFormatGroup = SendMessageA(formatComboBox, CB_GETCURSEL, 0, 0);
                if (pixelFormatGroup == 0)
                    formatOverride = 0;
                else
                    formatOverride = 1;
                FillDisplayComboBoxes();
            }
        } else if (!notify) {
            if (command == IDOK) {
                EndDialog(hDlg, 0);
            } else if (command == IDCANCEL) {
                EndDialog(hDlg, 1);
            } else if (command == SETUP_ADVANCED_BUTTON_ID) {
                proc = AdvancedDisplayProc;
                answer = DialogBoxParamA(g_hInstance, MAKEINTRESOURCEA(SETUP_ADVANCED_DIALOG_ID), hDlg, proc, 0);
            }
        }
    }
    return 0;
}

#else
/* Display options dialog: fills the mode and format lists, takes their selections and opens the advanced dialog. */
BOOL __stdcall DisplayOptionsProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        modeComboBox = GetDlgItem(hDlg, SETUP_DRIVER_COMBO_RESOURCE_ID);
        formatComboBox = GetDlgItem(hDlg, SETUP_DEVICE_COMBO_RESOURCE_ID);
        FillDisplayComboBoxes();
        return 1;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            if (LOWORD(wParam) == SETUP_DRIVER_COMBO_RESOURCE_ID) {
                videoModeIndex = SendMessageA(modeComboBox, CB_GETCURSEL, 0, 0);
                pixelFormatGroup = 0;
                formatOverride = 0;
                FillDisplayComboBoxes();
                return 0;
            }
            if (LOWORD(wParam) == SETUP_DEVICE_COMBO_RESOURCE_ID) {
                pixelFormatGroup = SendMessageA(formatComboBox, CB_GETCURSEL, 0, 0);
                if (pixelFormatGroup == 0) {
                    formatOverride = 0;
                    FillDisplayComboBoxes();
                    return 0;
                } else {
                    formatOverride = 1;
                    FillDisplayComboBoxes();
                    return 0;
                }
            }
        } else if (HIWORD(wParam) == 0) {
            if (LOWORD(wParam) == IDOK) {
                EndDialog(hDlg, 0);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, 1);
            } else if (LOWORD(wParam) == SETUP_ADVANCED_BUTTON_ID) {
                DialogBoxParamA(g_hInstance, MAKEINTRESOURCEA(SETUP_ADVANCED_DIALOG_ID), hDlg, AdvancedDisplayProc, 0);
            }
        }
        return 0;
    }
    return 0;
}

#endif
/* Advanced display dialog: describes the driver choices and closes on OK. */
BOOL __stdcall AdvancedDisplayProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WORD command;
    WORD notification;

    switch (msg) {
    case WM_INITDIALOG:
        driverNoteWnd = GetDlgItem(hDlg, SETUP_DRIVER_NOTE_ID);
        SendMessageA(driverNoteWnd, WM_SETTEXT, 0, (LPARAM)
        "Direct X Driver Driver.\r\r\n\r\r\n"
        "This option will normally be set to display and refers to the display card in your system. This is the default setting and supports systems without a 3D accelerator card and those with an integrated display/3D card.\r\r\n\r\r\n"
        "If you have both a display card and a separate 3D accelerator card, there will be another option in the list for the 3D accelerator card, which normally should be ignored.\r\r\n\r\r\n"
        "Device\r\r\nRamp Emulation\r\r\nSelect this option if you do not have a 3D accelerator card or an MMX processor.\r\r\n\r\r\n"
        "RGB Emulation\r\r\nSelect this option if you have an MMX processor but do not have a 3D accelerator card.\r\r\n\r\r\n"
        "Direct 3D HAL\r\r\nSelect this option if you have a 3D accelerator card. This option will only be available if a 3D accelerator card is installed correctly.\r\r\n");
        return 1;
    case WM_COMMAND:
        command = LOWORD(wParam);
        notification = HIWORD(wParam);
        if (notification == 0) {
            if (command == IDOK) {
                EndDialog(hDlg, 0);
            }
        }
        return 0;
    }
    return 0;
}

#ifdef EQUIV
/* The Win32s export, protection output and vertical position reuse one word,
   including the address passed to VirtualProtect. */
union SetupWindowScratch {
    DWORD (__stdcall *imteFromModule)(HMODULE);
    DWORD oldProtect;
    int y;
};

static __inline unsigned char *GetExecutableImageBase(HMODULE module, union SetupWindowScratch *scratch)
{
    HMODULE win32sKernel;
    unsigned char *(__stdcall *baseFromImte)(DWORD);
    unsigned char *imageBase;
    imageBase = NULL;
    if ((GetVersion() & 0xc0000000) == 0x80000000) {
        win32sKernel = GetModuleHandleA("W32SKRNL.DLL");
        scratch->imteFromModule = (DWORD (__stdcall *)(HMODULE))GetProcAddress(win32sKernel, "_ImteFromHModule@4");
        baseFromImte = (unsigned char *(__stdcall *)(DWORD))GetProcAddress(win32sKernel, "_BaseAddrFromImte@4");
        if (scratch->imteFromModule && baseFromImte) {
            imageBase = baseFromImte(scratch->imteFromModule(module));
        }
    } else {
        imageBase = (unsigned char *)module;
    }
    return imageBase;
}

static __inline void AllowCodeWrites(HMODULE module, union SetupWindowScratch *scratch)
{
    unsigned char *imageBase;

    imageBase = GetExecutableImageBase(module, scratch);
    if (imageBase) {
        IMAGE_OPTIONAL_HEADER *imageHeaders;

        imageHeaders = (IMAGE_OPTIONAL_HEADER *)(imageBase + ((IMAGE_DOS_HEADER *)imageBase)->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
        VirtualProtect(imageBase + imageHeaders->BaseOfCode,
                       imageHeaders->SizeOfCode, PAGE_READWRITE, &scratch->oldProtect);
    }
}

#ifdef NETPLAY
#include "setup_window.h"
#else
static __inline int CreateSetupWindow(HINSTANCE instance)
{
    RECT desktop;
    WNDCLASSA windowClass;
    union SetupWindowScratch scratch;
    int windowCreated;

    g_hInstance = instance;
    windowBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(instance, "AppIcon");
    windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    windowClass.hbrBackground = windowBrush;
    windowClass.lpszMenuName = "AppMenu";
    windowClass.lpszClassName = "PSYGWO2";

    AllowCodeWrites(GetModuleHandleA(NULL), &scratch);

    if (!RegisterClassA(&windowClass)) {
        windowCreated = 0;
    } else {
        int x;

        GetWindowRect(GetDesktopWindow(), &desktop);
        x = (desktop.right - desktop.left - 500) / 2;
        scratch.y = (desktop.bottom - desktop.top - 360) / 2;
        g_hwnd = CreateWindowExA(WS_EX_APPWINDOW, "PSYGWO2", "Wipeout XL",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU, x, scratch.y, 500, 360,
                                NULL, NULL, instance, NULL);
        if (!g_hwnd) {
            ShowMessage("CreateWindowEx failed");
            windowCreated = 0;
        } else {
            EnumerateDisplayDrivers();
            if (!acceleratorAvailable) {
                g_cdValidating = 1;
                MessageBoxA(NULL, "You must have a 3D accelerator card to run Wipeout XL",
                            "Accelerator card validation", 0);
                g_cdValidating = 0;
                windowCreated = 0;
            } else {
                SetWindowPos(g_hwnd, NULL, x, scratch.y, 500, 360, SWP_NOZORDER | SWP_NOACTIVATE);
                windowCreated = 1;
            }
        }
    }
    return windowCreated;
}
#endif

#ifdef NETPLAY
extern char *gstTitleName, *gstTitleVersion, *gstTitleDescription;
extern char gstPlayerName[], gstSessionName[];
extern signed char gstPlayerNameLength, gstSessionNameLength;
extern unsigned char gstLaunchMode;
extern int gstUseGPI;
extern int gstProviderChoice;
extern short netCdTrackError;
extern signed char networkGame;
int gstInitGPI_Callback(int what, GstQueryResult *result);
int InitNetwork(void);
int ReadGameCommandLineOptions(char *commandLine);
void SetAssetDirectories(void);
#endif

#ifdef NETPLAY
#ifdef EQUIV
#pragma intrinsic(strcat, strcpy, strlen)
/* Initializes the setup window, lets the player pick the network provider, reads the player and session names and
   starts the game. The original expands strcpy, strcat and strlen inline. */
int __stdcall WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int show)
{
    int quit;
    int resError;
    int pad;
    int lobbyResult;
    HWND existing;
    HWND popup;
    int installError;
    int i;
    int ddError;
    int confirm;
    int boxResult;

    installError = 0;
    resError = 0;
    pad = 0;
    quit = 0;
    setupQuitRequested = 0;
    setupTimeout = 1500;
    g_keyboardCode = -1;
    setupWindowActive = 1;
    existing = FindWindowA("PSYGWO2", NULL);
    if (existing) {
        popup = GetLastActivePopup(existing);
        BringWindowToTop(existing);
        if (popup != existing) {
            BringWindowToTop(popup);
        }
        return 0;
    }
    if (LoadInstallPaths()) {
        setupWindowActive = 0;
        quit = 1;
        installError = 1;
    }
    SetAssetDirectories();
    if (!ReadGameCommandLineOptions(commandLine)) {
        return 0;
    }
    if (!CreateSetupWindow(instance, commandLine)) {
        return 0;
    }
    gstDebugPrintf(0);
    gstDebugPrintf("gstTitleName:    %s", gstTitleName);
    gstDebugPrintf("gstVersionName:  %s", gstTitleVersion);
    gstDebugPrintf("gstRevisionName: %s", gstTitleDescription);
    setupLogo = LoadBitmapA(instance, MAKEINTRESOURCE(113));
    setupBanner = LoadBitmapA(instance, MAKEINTRESOURCE(114));
    noSavedConfig = LoadConfig();
    CreateSetupControls();
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    UpdateWindow(g_hwnd);
    gameData = &initialGameData;
    gameData->cdTrack = 1;
    if (!networkGame) {
        InitCD(g_cdTrackBuffer, 0);
    }
    CdValidate();
    InitSetupBackground();
    lobbyResult = gstLobbyGPI(gstInitGPI_Callback);
    switch (lobbyResult) {
    case 1:
        break;
    case 3:
        gstUseGPI = 0;
        setupWindowActive = 0;
        gstLaunchMode = 2;
        break;
    case 4:
        gstUseGPI = 0;
        setupWindowActive = 0;
        gstLaunchMode = 1;
        break;
    case 2:
        gstUseGPI = 0;
        setupWindowActive = 0;
        break;
    default:
        gstDebugPrintf("WinMain: gstLobbyGPI Failed: %d [%s]", lobbyResult, gstErrorString(lobbyResult));
        gstUseGPI = 1;
        setupWindowActive = 1;
        break;
    }
    while (setupWindowActive) {
        if (g_keyboardCode != -1) {
            if (g_keyboardCode == VK_RETURN) {
                if (gstProviderChoice == 0 && netCdTrackError == 1) {
                    while (netCdTrackError == 1) {
                        g_cdValidating = 1;
                        confirm = MessageBoxA(g_hwnd, "Make sure Wipeout XL CD is in the CD drive.\n", "Wipeout XL CD Validator", MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL);
                        g_cdValidating = 0;
                        if (confirm == IDOK) {
                            CdReadTrackTable();
                        } else {
                            g_keyboardCode = -1;
                            break;
                        }
                    }
                }
                if (gstProviderChoice == 0 && netCdTrackError == 1) {
                    continue;
                }
                if (!InitNetwork()) {
                    gstDebugPrintf("GPI Initialization Failed");
                    g_keyboardCode = -1;
                    continue;
                }
                startGameRequested = 1;
                g_keyboardCode = -1;
            }
            if (g_keyboardCode == VK_ESCAPE) {
                PostQuitMessage(0);
                g_keyboardCode = -1;
            }
        }
        if (startGameRequested) {
            if (!displayDrivers[videoModeIndex].supported8[pixelFormatGroup][pixelFormatIndex] &&
                !displayDrivers[videoModeIndex].supported16[pixelFormatGroup][pixelFormatIndex]) {
                startGameRequested = 0;
                g_cdValidating = 1;
                MessageBoxA(NULL, "Mode not supported . Try another one", "DirectX Error", 0);
                g_cdValidating = 0;
            } else {
                setupWindowActive = 0;
            }
        }
        quit = ProcessSetupMessages();
    }
    if (!gstUseGPI) {
        if (gstLaunchMode == 2) {
            while (netCdTrackError == 1) {
                g_cdValidating = 1;
                boxResult = MessageBoxA(g_hwnd, "Make sure Wipeout XL CD is in the CD drive.\n", "Wipeout XL CD Validator", MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL);
                g_cdValidating = 0;
                if (boxResult == IDOK) {
                    CdReadTrackTable();
                } else {
                    exit(-1);
                }
            }
        }
        if (!InitNetwork()) {
            gstDebugPrintf("WinMain: InitializeNetwork_GPI Failed");
            MessageBoxA(NULL, "Could Join or Host Session", "Network Initialization Failed", 0);
            return 0;
        }
    }
    ShowCursor(FALSE);
    {
        FILE *stream;
        char *newline;
        unsigned char pos;

        newline = 0;
        stream = fopen("pname.txt", "r");
        if (stream == 0) {
            sprintf(gstPlayerName, "PLAYER");
        } else {
            fgets(gstPlayerName, 10, stream);
            gstPlayerName[8] = 0;
            newline = strchr(gstPlayerName, '\n');
            if (newline) {
                *newline = 0;
            }
            newline = strchr(gstPlayerName, '\r');
            if (newline) {
                *newline = 0;
            }
            fclose(stream);
        }
        gstPlayerNameLength = strlen(gstPlayerName);
        for (pos = 0; pos < gstPlayerNameLength; pos++) {
            if (isalpha(gstPlayerName[pos])) {
                if (islower(gstPlayerName[pos])) {
                    gstPlayerName[pos] = toupper(gstPlayerName[pos]);
                }
            }
        }
        for (pos = gstPlayerNameLength; pos < 8; pos++) {
            gstPlayerName[pos] = ' ';
        }
        gstPlayerName[8] = 0;
        stream = fopen("sname.txt", "r");
        if (stream == 0) {
            sprintf(gstSessionName, "XL MULTI");
        } else {
            fgets(gstSessionName, 10, stream);
            gstSessionName[8] = 0;
            newline = strchr(gstSessionName, '\n');
            if (newline) {
                *newline = 0;
            }
            newline = strchr(gstSessionName, '\r');
            if (newline) {
                *newline = 0;
            }
            fclose(stream);
        }
        gstSessionNameLength = strlen(gstSessionName);
        for (pos = 0; pos < gstSessionNameLength; pos++) {
            if (isalpha(gstSessionName[pos])) {
                if (islower(gstSessionName[pos])) {
                    gstSessionName[pos] = toupper(gstSessionName[pos]);
                }
            }
        }
        for (pos = gstSessionNameLength; pos < 8; pos++) {
            gstSessionName[pos] = ' ';
        }
        gstSessionName[8] = 0;
    }
    if (setupLogo) {
        DeleteObject(setupLogo);
    }
    if (setupBanner) {
        DeleteObject(setupBanner);
    }
    if (quit == 0) {
        renderSize.cx = displayResolutions[pixelFormatIndex].width;
        screenWidth = renderSize.cx;
        renderSize.cy = displayResolutions[pixelFormatIndex].height;
        screenHeight = renderSize.cy;
        screenScaleX = (float)displayResolutions[pixelFormatIndex].width;
        screenScaleY = (float)displayResolutions[pixelFormatIndex].height;
        if (screenWidth > 320) {
            doubleSourceRects = 1;
            strcat(wobbleDir4, "HI-RES\\");
        } else {
            strcat(wobbleDir4, "LO-RES\\");
        }
        for (i = 0; i < 320; i++) {
            scaleTableX[i] = i * screenWidth / 320;
        }
        for (i = 0; i < 240; i++) {
            scaleTableY[i] = i * screenHeight / 240;
        }
        DestroySetupControls();
        gameWindowActive = 1;
        SetWindowLongA(g_hwnd, GWL_STYLE, WS_POPUP);
        ShowCursor(FALSE);
        CreateDirectDrawObjects();
        ddError = CreateDirect3DDevice();
        if (!ddError) {
            if (InitDirectInput() == 1) {
                main(0, 0);
                ShutdownGame();
            }
        } else {
            resError = 1;
            ShutdownGame();
        }
    }
    gameWindowActive = 0;
    if (!hardwareVerified) {
        g_cdValidating = 1;
        MessageBoxA(NULL, "This version of Wipeout XL requires a Videologic PowerVR card. This card could not be detected. \n\n Make sure the PowerVR card is correctly installed and try again.", "Hardware verification.", 0);
        g_cdValidating = 0;
    } else if (installError) {
        g_cdValidating = 1;
        MessageBoxA(NULL, "Wipeout XL has not been installed correctly. \n\n Re-install game from CD and try again.", "Installation error", 0);
        g_cdValidating = 0;
    } else if (resError == 1) {
        strcpy(g_cdMessage, "There was a problem running in that resolution.\n\n");
        strcat(g_cdMessage, "This may be because your video card does not have enough memory available.\n\n");
        strcat(g_cdMessage, "Restart the game and select another resolution\n");
        g_cdValidating = 1;
        MessageBoxA(NULL, g_cdMessage, "Mode Error", 0);
        g_cdValidating = 0;
    }
    return windowMessage.wParam;
}
#pragma function(strcat, strcpy, strlen)
#endif
#else
/* Initializes the setup window, handles its message loop, then starts the game. */
int __stdcall WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int show)
{
    HWND existing;
    HWND popup;
    int installError = 0;
    int displayError = 0;
    int quit = 0;
    int i;
#ifdef POWERVR
    int deviceError;
#endif

    setupQuitRequested = 0;
    setupTimeout = 1500;
    setupWindowActive = 1;
    g_keyboardCode = -1;
    existing = FindWindowA("PSYGWO2", NULL);
    if (existing) {
        popup = GetLastActivePopup(existing);
        BringWindowToTop(existing);
        if (popup != existing) {
            BringWindowToTop(popup);
        }
        return 0;
    }
    if (LoadInstallPaths()) {
        setupWindowActive = 0;
        quit = 1;
        installError = 1;
    }
    SetAssetDirectories();
    if (!CreateSetupWindow(instance)) {
        return 0;
    }
    setupLogo = LoadBitmapA(instance, MAKEINTRESOURCE(113));
    setupBanner = LoadBitmapA(instance, MAKEINTRESOURCE(114));
    noSavedConfig = LoadConfig();
    CreateSetupControls();
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    UpdateWindow(g_hwnd);
    gameData = &initialGameData;
    initialGameData.cdTrack = 1;
    InitCD(g_cdTrackBuffer, 0);
    CdValidate();
    SetPolyF4(&setupBackground);
    setupBackground.x0 = 0;
    setupBackground.y0 = 0;
    setupBackground.x1 = 320;
    setupBackground.y1 = 0;
    setupBackground.x2 = 0;
    setupBackground.y2 = 240;
    setupBackground.x3 = 320;
    setupBackground.y3 = 240;
    setupBackground.r0 = 0;
    setupBackground.g0 = 0;
    setupBackground.b0 = 0;
    while (setupWindowActive) {
        if (g_keyboardCode != -1) {
            if (g_keyboardCode == VK_RETURN) {
                startGameRequested = 1;
                g_keyboardCode = -1;
            }
            if (g_keyboardCode == VK_ESCAPE) {
                PostQuitMessage(0);
                g_keyboardCode = -1;
            }
        }
        if (startGameRequested) {
            if (!displayDrivers[videoModeIndex].supported8[pixelFormatGroup][pixelFormatIndex] &&
                !displayDrivers[videoModeIndex].supported16[pixelFormatGroup][pixelFormatIndex]) {
                startGameRequested = 0;
                g_cdValidating = 1;
                MessageBoxA(NULL, "Mode not supported . Try another one", "DirectX Error", 0);
                g_cdValidating = 0;
            } else {
                setupWindowActive = 0;
            }
        }
        setupLoopTick = timeGetTime();
        framesThisSecond++;
        if (time(NULL) != lastFpsSecond) {
            lastFpsSecond = time(NULL);
            framesThisSecond = 0;
        }
        quit = 1;
        if (!setupQuitRequested) {
            if (PeekMessageA(&windowMessage, NULL, 0, 0, PM_REMOVE) && g_hwnd) {
                if (windowMessage.message == WM_QUIT) {
                    setupQuitRequested = 1;
                    setupWindowActive = 0;
                    continue;
                }
                if (windowMessage.message == WM_KEYDOWN && !(windowMessage.lParam & KEY_PREVIOUSLY_DOWN)) {
                    g_keyboardCode = windowMessage.wParam;
                }
                TranslateMessage(&windowMessage);
                DispatchMessageA(&windowMessage);
            }
            quit = 0;
        }
    }
    if (setupLogo) {
        DeleteObject(setupLogo);
    }
    if (setupBanner) {
        DeleteObject(setupBanner);
    }
    if (quit == 0) {
        unsigned short width, height;

        width = displayResolutions[pixelFormatIndex].width;
        height = displayResolutions[pixelFormatIndex].height;
        screenWidth = width;
        screenHeight = height;
        screenScaleX = (float)width;
        screenScaleY = (float)height;
        if (screenWidth > 320) {
            doubleSourceRects = 1;
            strcat(wobbleDir4, "HI-RES\\");
        } else {
            strcat(wobbleDir4, "LO-RES\\");
        }
        {
            unsigned short *entry = scaleTableX;

            for (i = 0; entry < scaleTableX + 320; i++, entry++) {
                *entry = i * width / 320;
                renderSize.cx = width;
            }
        }
        {
            unsigned short *entry = scaleTableY;

            for (i = 0; entry < scaleTableY + 240; i++, entry++) {
                *entry = i * height / 240;
                renderSize.cy = height;
            }
        }
        for (i = 0; i < 4; i++) {
            DestroyWindow(setupButtons[i]);
        }
        for (i = 0; i < 2; i++) {
            DestroyWindow(setupCombos[i]);
        }
        for (i = 0; i < 6; i++) {
            DestroyWindow(setupLabels[i]);
        }
        gameWindowActive = 1;
        SetWindowLongA(g_hwnd, GWL_STYLE, WS_POPUP);
        ShowCursor(FALSE);
        CreateDirectDrawObjects();
#ifdef POWERVR
        deviceError = CreateDirect3DDevice();
        /* The command-texture probe accepts an unused device argument. */
        if (((int (*)(LPDIRECTDRAW, IDirect3DDevice *))CheckPVRTextureSupport)(lpDD, lpD3DDevice)) {
            hardwareVerified = 1;
        } else {
            hardwareVerified = 0;
            deviceError = 1;
        }
        if (!deviceError) {
#else
        if (!CreateDirect3DDevice()) {
#endif
            if (InitDirectInput() == 1) {
                /* The executable passes two unused arguments to the game entry point. */
                ((int (*)(int, char **))main)(0, 0);
                ShutdownGame();
            }
        } else {
            displayError = 1;
            ShutdownGame();
        }
    }
    gameWindowActive = 0;
    if (!hardwareVerified) {
        g_cdValidating = 1;
        MessageBoxA(NULL, "This version of Wipeout XL requires a Videologic PowerVR card. This card could not be detected. \n\n Make sure the PowerVR card is correctly installed and try again.", "Hardware verification.", 0);
        g_cdValidating = 0;
    } else if (installError) {
        g_cdValidating = 1;
        MessageBoxA(NULL, "Wipeout XL has not been installed correctly. \n\n Re-install game from CD and try again.", "Installation error", 0);
        g_cdValidating = 0;
    } else if (displayError == 1) {
        strcpy(g_cdMessage, "There was a problem running in that resolution.\n\n");
        strcat(g_cdMessage, "This may be because your video card does not have enough memory available.\n\n");
        strcat(g_cdMessage, "Restart the game and select another resolution\n");
        g_cdValidating = 1;
        MessageBoxA(NULL, g_cdMessage, "Mode Error", 0);
        g_cdValidating = 0;
    }
    return windowMessage.wParam;
}
#endif
#endif



#ifdef NETPLAY
extern int gstProviderChoice;
extern int gstUseGPI;
extern signed char networkGame;
int gstInitGPI_Callback(int what, GstQueryResult *result);
void ErrorFunctionHandler(int error, int code);
void DefaultPacketHandler(GstCodec *packet);
void func_48f84e(char *format, ...);
int func_48f8aa(void);
#include "race_packets.h"
int func_48f8e1(void);

/* Starts the network layer: picks the provider, initialises the GPI layer, installs the game's error, debug, cancel
   and packet handlers, and connects. Returns 1 when the network game can start. */
int InitNetwork(void)
{
    int result;

    switch (gstProviderChoice) {
    case 0:
        networkGame = 0;
        return 1;
    default:
        networkGame = 1;
        break;
    }
    if (gstUseGPI) {
        result = gstInitGPI(gstInitGPI_Callback);
        if (result < 1) {
            gstDebugPrintf("gstInitGPI Failed: %d [%s]", result, gstErrorString(result));
            return 0;
        }
    }
    gstRegisterErrorCallback(ErrorFunctionHandler, 0);
    gstRegisterDebugPrint(func_48f84e);
    gstRegisterCancelCallback(func_48f8aa);
    gstRegisterDefaultPacketHandler(DefaultPacketHandler);
    gstRegisterSpecificPacketHandler(HandleMakePlayer, 0x41, 0);
    gstRegisterSpecificPacketHandler(HandleJoinPkt, 0x44, 0);
    gstRegisterSpecificPacketHandler(gstHandleAsynch, 0x53, 0);
    gstRegisterSpecificPacketHandler(gstHandleRestart, 0x46, 0);
    gstRegisterSpecificPacketHandler(HandleEventPkt, 0x6a, 0);
    g_cdValidating = 1;
    if (!func_48f8e1())
        return 0;
    g_cdValidating = 0;
    return 1;
}
#endif

#ifdef NETPLAY
extern char D_4e1680[];

#ifdef NETPLAY
extern u_char gstDebugOutputEnabled;
int vsprintf(char *buffer, const char *format, va_list args);

/* Formats a debug message when D_4dc8c4 is set and sends it to the gst debug printer. */
void func_48f84e(char *format, ...)
{
    va_list args;
    char buffer[256];

    if (gstDebugOutputEnabled) {
        args = (va_list)&format + sizeof(format);
        vsprintf(buffer, format, args);
        args = 0;
        gstDebugPrintf(buffer);
    }
}
#endif

/* Logs an error reported by the gst layer and the function it came from. Driver error 0x8fff000b is not logged. */
void ErrorFunctionHandler(int error, int code)
{
    if (error == 0x8fff000b)
        return;
    gstDebugPrintf("ErrorFunctionHandler: error1 %d [%s]", error, gstErrorString(error));
    gstDebugPrintf("ErrorFunctionHandler: error2 %d [%s]", code, gstFunctionName(code));
}

/* Logs a gst packet that no handler took: its type, the name entry for that type, and its size. */
void DefaultPacketHandler(GstCodec *packet)
{
    gstDebugPrintf("DefaultPacketHandler %3d [%s] Size %d", packet->data[0], D_4e1680 + packet->data[0] * 30,
                   packet->size);
}
#endif

#ifdef NETPLAY
extern u_short keysPressed;
void PollRaceControlInput(void);
void DoDetailKeys(void);

/* Polls the pad helpers of the network build and reports whether the key bit 0x1000 was pressed. */
int func_48f8aa(void)
{
    PollRaceControlInput();
    DoDetailKeys();
    if (keysPressed & 0x1000)
        return 1;
    return 0;
}
#endif

#ifdef NETPLAY
extern char *gstTitleName, *gstTitleVersion, *gstTitleDescription;
extern GstTitle gstTitle;
extern GstCampaign gstCampaign;
extern GstScoreType gstShipScore;
#include "provider_state.h"
extern const GstGuid gstApplicationGuid;
int gstConnectGPI_Callback(int what, GstQueryResult *result);

/* Describes the game (title and the one score it keeps, the ship number) and connects the GPI layer. */
int func_48f8e1(void)
{
    int result;

    gstTitle.name = gstTitleName;
    gstTitle.version = gstTitleVersion;
    gstTitle.description = gstTitleDescription;
    gstCampaign.count = 1;
    gstShipScore.name = "Ship Number";
    gstShipScore.min = 0;
    gstShipScore.max = 14;
    gstShipScore.flags = 0;
    gstShipScore.sort = 0;
    gstShipScore.labelled = 0;
    gstShipScore.display = 0;
    gstShipScore.scope = 0;
    gstShipScore.labels = 0;
    gstCampaign.scores = &gstShipScore;
    result = gstConnectGPI(gstConnectGPI_Callback);
    if (result < 1) {
        gstDebugPrintf("gstConnectGPI failed %d [%s]\n", result, gstErrorString(result));
        return 0;
    }
    return 1;
}

/* Answers the GPI connect queries: the title description and the campaign. */
int gstConnectGPI_Callback(int what, GstQueryResult *result)
{
    switch (what) {
    case GST_QUERY_TITLE:
        result->pointer = &gstTitle;
        break;
    case GST_QUERY_CAMPAIGN:
        result->pointer = &gstCampaign;
        break;
    default:
        gstDebugPrintf("gstConnectGPI_Callback: Error: ulRequest %x", what);
        return GST_ERR_INVALID_PARAM;
    }
    return 1;
}

/* Answers the queries of gstInitGPI: the provider, the connection kind, queue sizes and the application guid. */
int gstInitGPI_Callback(int what, GstQueryResult *result)
{
    if (result == 0)
        return GST_ERR_INVALID_PARAM;
    switch (what) {
    case 0:
        result->pointer = gstProviderIds[gstProviderChoice];
        break;
    case 1:
        result->dword = 0x10;
        break;
    case 0x40:
        *(GstGuid *)result = gstApplicationGuid;
        break;
    case 0x41:
        result->dword = 0xf;
        break;
    case 0x42:
        result->dword = 0x8000;
        break;
    case 0x43:
        result->dword = 0x400;
        break;
    case 0x44:
        result->dword = 0x64;
        break;
    case 2:
        result->dword = 0x2800;
        break;
    case 3:
        result->dword = 0x100;
        break;
    case 4:
        result->dword = 2;
        break;
    case 5:
        result->dword = 0x2800;
        break;
    case 6:
        result->dword = 0x400;
        break;
    default:
        gstDebugPrintf("gstInitGPI_Callback: bad parameter");
        return GST_ERR_INVALID_PARAM;
    }
    return 1;
}
#endif

/* Shows a formatted message in a message box, followed by a line break. */
void ShowMessage(char *format, ...)
{
    char text[256];
#ifdef NETPLAY
    /* The network build formats from the first variable argument directly, with no va_list local. */
    wvsprintfA(text, format, (va_list)&format + sizeof(format));
#else
    va_list args;

    va_start(args, format);
    wvsprintfA(text, format, args);
#endif
    lstrcatA(text, "\r\n");
    g_cdValidating = 1;
    MessageBoxA(0, text, "D3D Example Message", 0);
    g_cdValidating = 0;
#ifndef NETPLAY
    va_end(args);
#endif
}
