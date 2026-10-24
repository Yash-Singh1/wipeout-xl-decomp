#include "functions.h"
#include "pc.h"

void exit(int status);

/* Reports an error. A non-fatal one holds the frame for two seconds, a fatal one logs and quits. */
void Error(char *string, short errorLevel)
{
#ifdef NETPLAY
    int error;
    short i;

    error = errorLevel;
    switch (error) {
    case 0:
        break;
    case NonFatal:
        for (i = 0; i < 60 * 2; i++)
            RaceFrameStart();
        break;
    case Fatal:
        OutputDebugString("Fatal Error!!!  ");
        for (i = 0; i < 2; i++)
            RaceFrameStart();
        exit(0);
        break;
    }
#else
    short i;

    switch (errorLevel) {
    case NonFatal:
        for (i = 0; i < 60 * 2; i++) {
            RaceFrameStart();
        }
        break;
    case Fatal:
        OutputDebugString("Fatal Error!!!  ");
        for (i = 0; i < 2; i++) {
            RaceFrameStart();
        }
        exit(0);
        break;
    }
#endif
}
