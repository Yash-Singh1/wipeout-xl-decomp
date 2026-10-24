#include "gst.h"


/* The "Register User ..." calls of the SDK. Each reports success with its call id as detail
   (see gstSdkFunctionName for the id names). */

/* Call 2: the table of user error strings. */
int gstRegisterUserErrors(void *buffer, unsigned short size)
{
    if (buffer == 0 || (size & 0xffff) == 0) {
        gstUserErrorCount = 0;
        gstUserErrors = 0;
        return gstReportError(GST_ERR_INVALID_PARAM, 2);
    }
    gstUserErrorCount = size;
    gstUserErrors = buffer;
    return gstReportError(1, 2);
}

/* Installs the error callback; without one the SDK is never verbose. */
int gstRegisterErrorCallback(void (*callback)(int error, int detail), char verbose)
{
    gstErrorCallback = callback;
    if (gstErrorCallback == 0)
        gstVerbose = 0;
    else
        gstVerbose = verbose;
    return gstReportError(1, 0xb);
}

/* Call 3: the table of user function names. */
int gstRegisterUserFunctions(void *buffer, unsigned short size)
{
    if (buffer == 0 || (size & 0xffff) == 0) {
        gstUserFunctionCount = 0;
        gstUserFunctions = 0;
        return gstReportError(GST_ERR_INVALID_PARAM, 3);
    }
    gstUserFunctionCount = size;
    gstUserFunctions = buffer;
    return gstReportError(1, 3);
}

/* Call 4: the allocator; the system one when none is given. */
int gstRegisterUserAlloc(void *(*handler)(unsigned size))
{
    if (handler)
        gstAllocHandler = handler;
    else
        gstAllocHandler = gstDefaultAlloc;
    return gstReportError(1, 4);
}

/* Call 5: the deallocator; the system one when none is given. */
int gstRegisterUserFree(void (*handler)(void *block))
{
    if (handler)
        gstFreeHandler = handler;
    else
        gstFreeHandler = gstDefaultFree;
    return gstReportError(1, 5);
}

/* Call 6 */
int gstRegisterStatusCallback(GstHandler handler)
{
    gstStatusCallback = handler;
    return gstReportError(1, 6);
}

/* Call 7: keeps a delay handler once one is set, else installs Sleep. */
int gstRegisterUserDelay(void)
{
    if (gstDelayHandler) {
    } else
        gstDelayHandler = gstDefaultSleep;
    return gstReportError(1, 7);
}

/* Call 8: the clock; GetTickCount when none is given. */
int gstRegisterUserClock(unsigned long (*handler)(void))
{
    if (handler)
        gstClockHandler = handler;
    else
        gstClockHandler = gstDefaultGetTime;
    return gstReportError(1, 8);
}

/* Call 9 */
int gstRegisterCancelCallback(GstHandler handler)
{
    gstCancelCallback = handler;
    return gstReportError(1, 9);
}

/* Call 10 */
int gstRegisterDebugPrint(void (*callback)(const char *format, ...))
{
    gstDebugPrintCallback = callback;
    return gstReportError(1, 0xa);
}

/* Call 0: the game names the mode to start in. */
int gstInitSDK(GstQueryFn query)
{
    int result;
    unsigned long mode;

    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0);
    if (gstIsInitialised())
        return gstReportError(0x80000001, 0);
    result = query(0, &mode);
    if (result != 1)
        return gstReportError(result, 0);
    return gstVxdOpen(mode, query);
}

/* Call 1: shuts the driver down. Quietly succeeds when the SDK was never started. */
int gstExitSDK(void)
{
    if (!gstIsInitialised())
        return gstReportError(1, 1);
    return gstDriverFlush();
}

/* Starts the SDK in the given mode; it can only be initialised once. */
int gstInitialise(int mode)
{
    if (gstIsInitialised())
        return gstReportError(0x80000001, 1);
    return gstInitHook(mode);
}
