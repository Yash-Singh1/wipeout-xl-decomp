#include <windows.h>
#include <string.h>
#include "gst.h"

/* The serial driver of the SDK: a Win95 VxD (GSTCTL2D.VXD) opened with CreateFile and driven
   with DeviceIoControl, plus a modem dial routine that reads its settings from C:\GOL.INI.
   The VxD reports its result code through the "bytes returned" argument. Errors carry the
   call ids 0x10000-0x10016. */

extern HANDLE gstVxdHandle;
extern HANDLE *gstVxdEvents;
extern char gstVxdLockReady;
extern HANDLE gstVxdStopEvent;
extern HANDLE gstVxdThread;
extern DWORD gstVxdThreadId;
extern GstIni *gstSettings;
extern CRITICAL_SECTION gstVxdLock;
extern GstVxdConfig gstVxdConfig;
extern char gstUserId[0x20];
extern char gstGameName[0x40];

int gstVxdCreateEvents(int eventCount);
int gstVxdStartThread(void);
DWORD __stdcall gstVxdThreadProc(void *param);
int gstVxdFreeEvents(void);
int gstVxdStopThread(void);
void gstVxdInstallDriver(void);
int gstVxdPutChar(char c);
int gstVxdRelease(char mode);
int gstVxdDial(void);
int gstVxdHangup(void);
int gstVxdAllocUpPacket(GstCodec **packet, int port, unsigned size);
int gstVxdSendUpPacket(GstCodec *packet, int port);
int gstVxdFreeUpPacket(GstCodec *packet, int port);
int gstVxdGetDownPacket(GstCodec **packet, unsigned timeout);
int gstVxdRequestAll(char mode);
#include "gst_vxd.h"
int gstVxdUnsupported(void);
int gstVxdSetDefaultHandler(GstPacketHandler handler);
int gstVxdWaitPacket(int timeout);

/* Sends `text` to the modem, then a carriage return when `sendReturn` is set. When a list of
   replies is given, waits for one of them: the result is its index plus one. */
int gstModemCommand(const char *text, char sendReturn, const char **replies)
{
    char received[256];
    int ret;
    int pos;
    int index;
    const char **answer;

    if (text) {
        while (*text) {
            ret = gpiDriverSlot00(*text);
            if (ret < 1)
                return gstReportError(ret, 0xffff);
            text++;
            gstDelayHandler(25);
        }
    }
    if (sendReturn) {
        ret = gpiDriverSlot00(0xd);
        if (ret < 1)
            return gstReportError(ret, 0xffff);
    }
    if (replies) {
        gstDelayHandler(100);
        pos = 0;
        for (;;) {
            if (gstCancelCallback && gstCancelCallback())
                return gstReportError(0x8000000b, 0xffff);
            ret = gpiDriverSlot04(&received[pos]);
            if (ret == 0x8fff000b)
                continue;
            else if (ret < 1)
                return gstReportError(ret, 0xffff);
            if (received[pos] == 0xd || received[pos] == 0xa) {
                pos = 0;
                continue;
            }
            pos++;
            received[pos] = 0;
            index = 0;
            answer = replies;
            while (*answer) {
                if (strcmp(*answer, received) == 0) {
                    gstDelayHandler(100);
                    for (;;) {
                        if (gstCancelCallback && gstCancelCallback())
                            return gstReportError(0x8000000b, 0xffff);
                        ret = gpiDriverSlot04(received);
                        if (ret == 0x8fff000b)
                            break;
                        else if (ret < 1)
                            return gstReportError(ret, 0xffff);
                    }
                    return gstReportError(index + 1, 0xffff);
                }
                answer++;
                index++;
            }
            if (pos >= 0xff)
                return gstReportError(0x8fff0016, 0xffff);
        }
    } else {
        gstDelayHandler(400);
    }
    return gstReportError(1, 0xffff);
}

/* Reads from the modem until the last `length` characters received are the start of `prompt`. */
int gstModemWaitFor(const char *prompt, int length)
{
    char buffer[256];
    int result;
    int count;

    if (prompt == 0 || length > 0x100)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    count = 0;
    for (;;) {
        if (gstCancelCallback && gstCancelCallback())
            return gstReportError(0x8000000b, 0xffff);
        result = gpiDriverSlot04(&buffer[count]);
        if (result == 0x8fff000b)
            continue;
        else if (result < 1)
            return gstReportError(result, 0xffff);
        count++;
        if (strncmp(buffer, prompt, count) != 0) {
            while (count > 0) {
                count--;
                memcpy(buffer, buffer + 1, count);
                if (count > 0 && strncmp(buffer, prompt, count) == 0)
                    break;
            }
        } else if (count >= length) {
            break;
        }
    }
    return gstReportError(1, 0xffff);
}

char *gstGetUserId(void)
{
    return gstUserId;
}

char *gstGetGameName(void)
{
    return gstGameName;
}

void *gstDefaultAlloc(unsigned size)
{
    return GlobalAlloc(0, size);
}

void gstDefaultFree(void *block)
{
    GlobalFree(block);
}

void gstDefaultSleep(unsigned milliseconds)
{
    Sleep(milliseconds);
}

unsigned long gstDefaultGetTime(void)
{
    return GetTickCount();
}

/* Opens the driver in the given mode. `query` supplies the settings (id 1 is the kind of
   connection, ids 2-6 fill gstVxdConfig). */
int gstVxdOpen(unsigned long mode, GstQueryFn query)
{
    int status;
    int kind;
    GstVxdRequest msg;
    unsigned long version;

    gstMode = 0;
    gstFlags = 1;
    status = query(1, &kind);
    if (status != 1)
        return gstReportError(status, 0x10006);
    status = query(2, &gstVxdConfig.flags);
    if (status != 1)
        return gstReportError(status, 0x10006);
    status = query(3, &gstVxdConfig.queueSize);
    if (status != 1)
        return gstReportError(status, 0x10006);
    status = query(4, &gstVxdConfig.eventCount);
    if (status != 1)
        return gstReportError(status, 0x10006);
    status = query(5, &gstVxdConfig.param3);
    if (status != 1)
        return gstReportError(status, 0x10006);
    status = query(6, &gstVxdConfig.param4);
    if (status != 1)
        return gstReportError(status, 0x10006);
    gstVxdConfig.vxdEvents = 0;
    InitializeCriticalSection(&gstVxdLock);
    gstVxdLockReady = 1;
    status = gstDispatchInit(gstVxdConfig.queueSize);
    if (status < 1) {
        gstVxdClose();
        return gstReportError(status, 0x10006);
    }
    switch (kind) {
    case 1:
    case 2:
        break;
    case 4:
    case 8:
        status = gstVxdCreateEvents(gstVxdConfig.eventCount);
        if (status < 1) {
            gstVxdClose();
            return gstReportError(status, 0x10006);
        }
        break;
    case 16:
        status = gstVxdCreateEvents(gstVxdConfig.eventCount);
        if (status < 1 || (status = gstVxdStartThread()) < 1) {
            gstVxdClose();
            return gstReportError(status, 0x10006);
        }
        break;
    default:
        gstVxdClose();
        return gstReportError(0x80000006, 0x10006);
    }
    gstVxdHandle = CreateFileA("\\\\.\\GSTCTL2D.VXD", 0, 0, 0, 0, FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (gstVxdHandle == INVALID_HANDLE_VALUE) {
        gstVxdClose();
        return gstReportError(0x8fff0000, 0x10006);
    }
    msg.command = 0;
    msg.arg1 = 0;
    msg.arg2 = 0;
    msg.arg3 = 0;
    if (!DeviceIoControl(gstVxdHandle, 0x80, &msg, 0, &version, 0, &status, 0)) {
        gstVxdClose();
        return gstReportError(status, 0x10006);
    }
    if (gstDebugPrintCallback)
        gstDebugPrintCallback("VXD: %02X.%02X", version >> 8, version & 0xff);
    if (((version >> 8) & 0xff) != 0xd2) {
        gstVxdClose();
        return gstReportError(0x8fff0002, 0x10006);
    }
    if (!DeviceIoControl(gstVxdHandle, 0x100, &gstVxdConfig, 0x18, 0, mode, &status, 0)) {
        gstVxdClose();
        return gstReportError(status, 0x10006);
    }
    gstMode = mode;
    gstFlags = kind;
    gstVxdInstallDriver();
    return gstReportError(1, 0x10006);
}

/* Makes an event and a VxD handle for it, for each of eventCount + 1 queues. */
int gstVxdCreateEvents(int eventCount)
{
    HANDLE (WINAPI *openHandle)(HANDLE);
    int i;

    openHandle = (HANDLE (WINAPI *)(HANDLE))GetProcAddress(GetModuleHandleA("KERNEL32"), "OpenVxDHandle");
    if (openHandle == 0)
        return gstReportError(0x80000003, 0x10000);
    gstVxdEvents = gstAllocHandler((eventCount + 1) * 4);
    if (gstVxdEvents == 0)
        return gstReportError(0x80000003, 0x10000);
    gstVxdConfig.vxdEvents = gstAllocHandler((eventCount + 1) * 4);
    if (gstVxdConfig.vxdEvents == 0)
        return gstReportError(0x80000003, 0x10000);
    for (i = 0; i < eventCount + 1; i++) {
        gstVxdEvents[i] = CreateEventA(0, 1, 0, 0);
        if (gstVxdEvents[i] == 0)
            return gstReportError(0x80000003, 0x10000);
        gstVxdConfig.vxdEvents[i] = openHandle(gstVxdEvents[i]);
        if (gstVxdConfig.vxdEvents[i] == 0)
            return gstReportError(0x80000003, 0x10000);
    }
    return gstReportError(1, 0x10000);
}

/* Starts the thread that collects incoming packets by itself. */
int gstVxdStartThread(void)
{
    gstVxdStopEvent = CreateEventA(0, 1, 0, 0);
    if (gstVxdStopEvent == 0)
        return gstReportError(0x80000005, 0x10003);
    gstVxdThread = CreateThread(0, 0, gstVxdThreadProc, 0, 0, &gstVxdThreadId);
    if (gstVxdThread == 0)
        return gstReportError(0x80000005, 0x10003);
    return gstReportError(1, 0x10003);
}

/* The automatic receive thread: waits for the VxD to signal a packet, fetches it and hands it
   to the dispatcher. It ends when gstVxdStopEvent is set. */
DWORD __stdcall gstVxdThreadProc(void *param)
{
    int finished = 0;       /* set here and never read, as in the original */
    HANDLE waitHandles[2];
    int count;
    DWORD waited;
    GstCodec *packet;
    int downStatus;

    waitHandles[0] = gstVxdStopEvent;
    if (gstVxdEvents) {
        waitHandles[1] = gstVxdEvents[0];
        count = 2;
    } else {
        count = 1;
    }
    for (;;) {
        waited = WaitForMultipleObjects(count, waitHandles, 0, 3000);
        switch (waited) {
        case 0:
            goto stopped;    /* the stop event: the only way out of the loop */
        case 1:
            EnterCriticalSection(&gstVxdLock);
            DeviceIoControl(gstVxdHandle, 0x804, &packet, 0, 0, gstMode, &downStatus, 0);
            LeaveCriticalSection(&gstVxdLock);
            if (downStatus >= 1 && packet != 0) {
                downStatus = gstDispatchDownPacket(packet);
                if (downStatus < 1)
                    gstVxdFreeDownPacket(packet);
            } else if (downStatus < 1) {
                if (gstDebugPrintCallback)
                    gstDebugPrintCallback("Auto: %s", gstSdkErrorString(downStatus));
            }
            break;
        }
    }
stopped:
    if (gstDebugPrintCallback)
        gstDebugPrintCallback("Auto Exit");
    gstVxdThreadId = 0;
    CloseHandle(gstVxdThread);
    gstVxdThread = 0;
    return 0;
}

