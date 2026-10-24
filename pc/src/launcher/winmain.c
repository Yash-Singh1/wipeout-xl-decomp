typedef unsigned long DWORD;
typedef void *HKEY;

extern __declspec(dllimport) long __stdcall RegOpenKeyExA(HKEY key, const char *subkey, DWORD options, DWORD access, HKEY *result);
extern __declspec(dllimport) long __stdcall RegQueryValueExA(HKEY key, const char *name, DWORD *reserved, DWORD *type, unsigned char *data, DWORD *size);
extern __declspec(dllimport) void *__stdcall ShellExecuteA(void *window, const char *operation, const char *file, const char *parameters, const char *directory, int show);
extern void __cdecl func_401280(char *destination, const char *source);
extern void __cdecl func_401288(char *destination, const char *source);

typedef struct WinMainLocals {
    HKEY gameKey;
    DWORD size;
    const char *publisherPath;
    HKEY publisherKey;
    const char *softwarePath;
    int failed;
    const char *sourceValue;
    const char *directoryValue;
    HKEY softwareKey;
    const char *studioPath;
    const char *gamePath;
    char gameDirectory[272];
    unsigned char sourceDirectory[256];
    HKEY studioKey;
} WinMainLocals;

/* Equivalent; the target and draft differ only in stack and register scheduling. */
#ifdef EQUIV
int __stdcall WinMain(void *instance, void *previous, char *commandLine, int show)
{
    WinMainLocals locals;

    locals.failed = 1;
    locals.softwarePath = "SOFTWARE";
    locals.publisherPath = "Psygnosis";
    locals.studioPath = "Studios";
    locals.gamePath = "WipeoutXL";
    locals.directoryValue = "DirectoryName";
    locals.sourceValue = "Source Directory";
    locals.size = 0x100;

    if (RegOpenKeyExA((HKEY)0x80000002, locals.softwarePath, 0, 0x20019, &locals.softwareKey) == 0) {
        if (RegOpenKeyExA(locals.softwareKey, locals.publisherPath, 0, 0x20019, &locals.publisherKey) == 0) {
            if (RegOpenKeyExA(locals.publisherKey, locals.studioPath, 0, 0x20019, &locals.studioKey) == 0) {
                if (RegOpenKeyExA(locals.studioKey, locals.gamePath, 0, 0x20019, &locals.gameKey) == 0) {
                    if (RegQueryValueExA(locals.gameKey, locals.directoryValue, 0, 0, locals.sourceDirectory, &locals.size) == 0) {
                        func_401280(locals.gameDirectory, (const char *)locals.sourceDirectory);
                        if (RegQueryValueExA(locals.gameKey, locals.sourceValue, 0, 0, locals.sourceDirectory, &locals.size) == 0) {
                            locals.failed = 0;
                        }
                    }
                }
            }
        }
    }

    if (locals.failed) {
        ShellExecuteA(0, "open", "setup.exe", 0, 0, 1);
        return 1;
    }

    func_401288(locals.gameDirectory, "\\wipeout2.exe");
    return (int)ShellExecuteA(0, "open", locals.gameDirectory, 0, 0, 1);
}
#endif
