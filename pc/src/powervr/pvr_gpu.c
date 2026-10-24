#include "ajs.h"
#include "pc.h"

/* Import declarations here only provide linker names to the assembly thunks below. */
WINAPI_IMPORT void __cdecl ShowCursor(void);
WINAPI_IMPORT void __cdecl SetWindowLongA(void);
WINAPI_IMPORT void __cdecl DestroyWindow(void);
WINAPI_IMPORT void __cdecl DispatchMessageA(void);
WINAPI_IMPORT void __cdecl TranslateMessage(void);
WINAPI_IMPORT void __cdecl PeekMessageA(void);
WINAPI_IMPORT void __cdecl UpdateWindow(void);
WINAPI_IMPORT void __cdecl ShowWindow(void);
WINAPI_IMPORT void __cdecl LoadBitmapA(void);
WINAPI_IMPORT void __cdecl SetWindowPos(void);
WINAPI_IMPORT void __cdecl CreateWindowExA(void);
WINAPI_IMPORT void __cdecl GetWindowRect(void);
WINAPI_IMPORT void __cdecl GetDesktopWindow(void);
WINAPI_IMPORT void __cdecl RegisterClassA(void);
WINAPI_IMPORT void __cdecl LoadCursorA(void);
WINAPI_IMPORT void __cdecl LoadIconA(void);
WINAPI_IMPORT void __cdecl BringWindowToTop(void);
WINAPI_IMPORT void __cdecl GetLastActivePopup(void);
WINAPI_IMPORT void __cdecl FindWindowA(void);
WINAPI_IMPORT void __cdecl GetMessageA(void);
WINAPI_IMPORT void __cdecl wvsprintfA(void);
WINAPI_IMPORT void __cdecl ReleaseDC(void);
WINAPI_IMPORT void __cdecl GetDC(void);
WINAPI_IMPORT void __cdecl SetWindowsHookExA(void);
WINAPI_IMPORT void __cdecl UnhookWindowsHookEx(void);
WINAPI_IMPORT void __cdecl SetWindowTextA(void);
WINAPI_IMPORT void __cdecl CallNextHookEx(void);
WINAPI_IMPORT void __cdecl DeleteDC(void);
WINAPI_IMPORT void __cdecl BitBlt(void);
WINAPI_IMPORT void __cdecl DPtoLP(void);
WINAPI_IMPORT void __cdecl GetObjectA(void);
WINAPI_IMPORT void __cdecl SetMapMode(void);
WINAPI_IMPORT void __cdecl GetMapMode(void);
WINAPI_IMPORT void __cdecl SelectObject(void);
WINAPI_IMPORT void __cdecl CreateCompatibleDC(void);
WINAPI_IMPORT void __cdecl CreateSolidBrush(void);
WINAPI_IMPORT void __cdecl DeleteObject(void);
WINAPI_IMPORT void __cdecl GetStockObject(void);
WINAPI_IMPORT void __cdecl GetSystemPaletteEntries(void);
WINAPI_IMPORT void __cdecl RegQueryValueExA(void);
WINAPI_IMPORT void __cdecl RegOpenKeyExA(void);
WINAPI_IMPORT void __cdecl timeGetTime(void);
WINAPI_IMPORT void __cdecl auxGetDevCapsA(void);
WINAPI_IMPORT void __cdecl auxGetNumDevs(void);
WINAPI_IMPORT void __cdecl timeSetEvent(void);
WINAPI_IMPORT void __cdecl timeGetDevCaps(void);
WINAPI_IMPORT void __cdecl auxSetVolume(void);
WINAPI_IMPORT void __cdecl auxGetVolume(void);
WINAPI_IMPORT void __cdecl timeKillEvent(void);
WINAPI_IMPORT void __cdecl DirectDrawCreate(void);
WINAPI_IMPORT void __cdecl DirectDrawEnumerateA(void);
WINAPI_IMPORT void __cdecl lstrcmpA(void);
WINAPI_IMPORT void __cdecl GlobalFree(void);
WINAPI_IMPORT void __cdecl GlobalAlloc(void);
WINAPI_IMPORT void __cdecl VirtualProtect(void);
WINAPI_IMPORT void __cdecl GetProcAddress(void);
WINAPI_IMPORT void __cdecl GetVersion(void);
WINAPI_IMPORT void __cdecl GetModuleHandleA(void);
WINAPI_IMPORT void __cdecl GetSystemInfo(void);
WINAPI_IMPORT void __cdecl GetCurrentThreadId(void);
WINAPI_IMPORT void __cdecl GlobalSize(void);
WINAPI_IMPORT void __cdecl SetRect(void);
WINAPI_IMPORT void __cdecl EndDialog(void);
WINAPI_IMPORT void __cdecl SendMessageA(void);
WINAPI_IMPORT void __cdecl EndPaint(void);
WINAPI_IMPORT void __cdecl BeginPaint(void);
WINAPI_IMPORT void __cdecl GetSysColor(void);
WINAPI_IMPORT void __cdecl FillRect(void);
WINAPI_IMPORT void __cdecl GetClientRect(void);
WINAPI_IMPORT void __cdecl DefWindowProcA(void);

void Stub_433F70(void)
{
}

void Stub_433F80(void)
{
}

/* These PowerVR libgpu compatibility calls are no-ops (the rest of the set is ajs.c's). */
u_long *ClearOTag(u_long *ot, int n)
{
}

u_long *ClearOTagR(u_long *ot, int n)
{
}

/* PowerVR's linker emitted these import thunks as indirect jumps. */
__declspec(naked) void Stub_466860(void) { __asm jmp dword ptr [OutputDebugStringA] }
__declspec(naked) void Stub_466866(void) { __asm jmp dword ptr [lstrcmpA] }
__declspec(naked) void Stub_46686C(void) { __asm jmp dword ptr [GlobalFree] }
__declspec(naked) void Stub_466872(void) { __asm jmp dword ptr [GlobalMemoryStatus] }
__declspec(naked) void Stub_466878(void) { __asm jmp dword ptr [GlobalAlloc] }
__declspec(naked) void Stub_46687E(void) { __asm jmp dword ptr [VirtualProtect] }
__declspec(naked) void Stub_466884(void) { __asm jmp dword ptr [GetProcAddress] }
__declspec(naked) void Stub_46688A(void) { __asm jmp dword ptr [GetVersion] }
__declspec(naked) void Stub_466890(void) { __asm jmp dword ptr [GetModuleHandleA] }
__declspec(naked) void Stub_466896(void) { __asm jmp dword ptr [lstrcatA] }
__declspec(naked) void Stub_46689C(void) { __asm jmp dword ptr [GetSystemInfo] }
__declspec(naked) void Stub_4668A2(void) { __asm jmp dword ptr [GetCurrentThreadId] }
__declspec(naked) void Stub_4668A8(void) { __asm jmp dword ptr [GetSystemTime] }
__declspec(naked) void Stub_4668AE(void) { __asm jmp dword ptr [GlobalSize] }
__declspec(naked) void Stub_4668B4(void) { __asm jmp dword ptr [wsprintfA] }
__declspec(naked) void Stub_4668BA(void) { __asm jmp dword ptr [SetRect] }
__declspec(naked) void Stub_4668C0(void) { __asm jmp dword ptr [EndDialog] }
__declspec(naked) void Stub_4668C6(void) { __asm jmp dword ptr [PostQuitMessage] }
__declspec(naked) void Stub_4668CC(void) { __asm jmp dword ptr [SendMessageA] }
__declspec(naked) void Stub_4668D2(void) { __asm jmp dword ptr [EndPaint] }
__declspec(naked) void Stub_4668D8(void) { __asm jmp dword ptr [BeginPaint] }
__declspec(naked) void Stub_4668DE(void) { __asm jmp dword ptr [GetSysColor] }
__declspec(naked) void Stub_4668E4(void) { __asm jmp dword ptr [FillRect] }
__declspec(naked) void Stub_4668EA(void) { __asm jmp dword ptr [GetClientRect] }
__declspec(naked) void Stub_4668F0(void) { __asm jmp dword ptr [DefWindowProcA] }

__declspec(naked) void Stub_4668F6(void)
{
    __asm jmp dword ptr [MessageBoxA]
}

__declspec(naked) void Stub_4668FC(void)
{
    __asm jmp dword ptr [ShowCursor]
}

__declspec(naked) void Stub_466902(void)
{
    __asm jmp dword ptr [SetWindowLongA]
}

__declspec(naked) void Stub_466908(void)
{
    __asm jmp dword ptr [DestroyWindow]
}

__declspec(naked) void Stub_46690E(void)
{
    __asm jmp dword ptr [DispatchMessageA]
}

__declspec(naked) void Stub_466914(void)
{
    __asm jmp dword ptr [TranslateMessage]
}

__declspec(naked) void Stub_46691A(void)
{
    __asm jmp dword ptr [PeekMessageA]
}

__declspec(naked) void Stub_466920(void)
{
    __asm jmp dword ptr [UpdateWindow]
}

__declspec(naked) void Stub_466926(void)
{
    __asm jmp dword ptr [ShowWindow]
}

__declspec(naked) void Stub_46692C(void)
{
    __asm jmp dword ptr [LoadBitmapA]
}

__declspec(naked) void Stub_466932(void)
{
    __asm jmp dword ptr [SetWindowPos]
}

__declspec(naked) void Stub_466938(void)
{
    __asm jmp dword ptr [CreateWindowExA]
}

__declspec(naked) void Stub_46693E(void)
{
    __asm jmp dword ptr [GetWindowRect]
}

__declspec(naked) void Stub_466944(void)
{
    __asm jmp dword ptr [GetDesktopWindow]
}

__declspec(naked) void Stub_46694A(void)
{
    __asm jmp dword ptr [RegisterClassA]
}

