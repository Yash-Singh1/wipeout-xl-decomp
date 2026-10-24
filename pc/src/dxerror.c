#include "functions.h"
#include <string.h>
#include <direct.h>
#include <stdio.h>
#include "pc.h"

#define DIRECTX_SUCCESS_RESULT 0

extern char wobbleDir1[];
extern char wobbleDir0[];
extern char g_cdMessage[];
extern long ddResult;
extern int dxErrorLine;
extern short debugLogStarted;
extern short fileLogStarted;
extern int g_cdValidating;
char *DDErrorString(long code);
void ShutdownGame(void);
void exit(int status);

/* Writes the last DirectX error to the debug log on the install drive, then shuts the game down. */
void LogDXError(void)
{
    char fileName[64] = "\\debug.txt";
    char path[256];
    FILE *log;
    SYSTEMTIME created;

    if (ddResult != DIRECTX_SUCCESS_RESULT) {
        strcpy(path, wobbleDir0);
        strcat(path, fileName);
        if (!debugLogStarted) {
            remove(path);
        }
        log = fopen(path, "a+");
        if (log != NULL) {
            if (!debugLogStarted) {
                GetSystemTime(&created);
                fprintf(log, "Wipeout 2 PC Direct X Debug Log\n\nFile created at %d:%d:%d on %d/%d/%d\n\n", created.wHour, created.wMinute, created.wSecond, created.wDay, created.wMonth, created.wYear);
                debugLogStarted = 1;
            }
            wsprintfA(g_cdMessage, "Direct X ERROR:Line %05d \t%s", dxErrorLine, DDErrorString(ddResult));
            fprintf(log, g_cdMessage);
            fclose(log);
        }
        ShutdownGame();
        exit(-1);
    }
}

/* Makes the DirectX folder under the install directory the current directory. */
short ChangeToDirectXDir(void)
{
    char path[256];

    strcpy(path, wobbleDir1);
    strcat(path, "directx");
    return _chdir(path) == 0;
}

/* Logs a file that could not be opened to the file access debug log, tells the player and shuts the game down. */
void FileAccessError(char *fileName)
{
    SYSTEMTIME systemTime;
    char fullPath[256];
    char logFile[64] = "\\debug.txt";
    char text[512];
    FILE *log;

    strcpy(fullPath, wobbleDir0);
    strcat(fullPath, logFile);
    if (!fileLogStarted) {
        remove(fullPath);
    }
    log = fopen(fullPath, "a+");
    if (log != NULL) {
        if (!fileLogStarted) {
            GetSystemTime(&systemTime);
            fprintf(log, "Wipeout 2 PC File Access Debug Log\n\nFile created at %d:%d:%d on %d/%d/%d\n\n", systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wDay, systemTime.wMonth, systemTime.wYear);
            fileLogStarted = 1;
        }
        wsprintfA(g_cdMessage, "File could not be found %s", fileName);
        fprintf(log, g_cdMessage);
        fclose(log);
    }
    ShutdownGame();
    wsprintfA(text, "The following file could not be found : %s\n\nMake sure the Wipeout XL CD is in the drive and re-run the game.\n\n", fileName);
    strcat(text, "If this error reoccurs with the CD in the drive, re-install the game.");
    g_cdValidating = 1;
    MessageBoxA(0, text, "File Access Error", 0);
    g_cdValidating = 0;
    exit(-1);
}
