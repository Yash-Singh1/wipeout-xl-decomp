extern int __stdcall func_10001030(void *module, unsigned int reason, void *reserved);
extern int __stdcall func_10002030(void *module, unsigned int reason, void *reserved);
extern void __stdcall func_10001420(void);
extern void __stdcall func_100015b0(void);
extern int g_dxAttachCount;
extern int (__stdcall *g_dxCallback)(void *module, unsigned int reason, void *reserved);

/* Equivalent; MSVC schedules the common callback path differently. */
#ifdef EQUIV
int __stdcall DllMain(void *module, unsigned int reason, void *reserved)
{
    int result = 1;

    if (reason == 1) {
        ++g_dxAttachCount;
    } else if (reason == 0) {
        if (g_dxAttachCount <= 0) {
            return 0;
        }
        --g_dxAttachCount;
    }

    if (reason == 1 || reason == 2) {
        if (g_dxCallback != 0) {
            result = g_dxCallback(module, reason, reserved);
        }
        if (result != 0) {
            result = func_10001030(module, reason, reserved);
        }
    }
    if (result != 0) {
        result = func_10002030(module, reason, reserved);
    }
    if (result == 0 && reason == 1) {
        func_10001420();
        func_100015b0();
    }

    if (reason == 0 || reason == 3) {
        if (func_10001030(module, reason, reserved) == 0) {
            result = 0;
        }
        if (result != 0 && g_dxCallback != 0) {
            result = g_dxCallback(module, reason, reserved);
        }
    }
    return result;
}
#endif
