#include <windows.h>
#include <string.h>
#include "gst.h"

/* The serial driver of the SDK (second half): a Win95 VxD (GSTCTL2D.VXD) opened with CreateFile and driven
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

/* Runs a packet through the dispatcher and calls whatever handlers it names; the packet is
   freed unless it was queued for polling. */
int gstDispatchDownPacket(GstCodec *packet)
{
    int result;
    char unhandled;
    GstPacketHandler normalHandler;
    GstPacketHandler poll;

    EnterCriticalSection(&gstVxdLock);
    result = gstDispatchLookup(packet, &poll, &normalHandler, &unhandled);
    if (result >= 1) {
        if (poll)
            poll(packet);
        if (normalHandler)
            normalHandler(packet);
        if (unhandled || poll || normalHandler)
            result = gstVxdFreeDownPacket(packet);
    }
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(result, 0x10002);
}

/* Closes the driver and everything that goes with it (also the error exit of gstVxdOpen). */
int gstVxdClose(void)
{
    int oldMode;

    oldMode = gstMode;
    gstMode = 0;
    gstFlags = 1;
    if (gstVxdHandle) {
        DeviceIoControl(gstVxdHandle, 0x101, 0, 0, 0, oldMode, 0, 0);
        CloseHandle(gstVxdHandle);
        gstVxdHandle = 0;
    }
    gstVxdStopThread();
    gstVxdFreeEvents();
    gstDispatchShutdown();
    if (gstVxdLockReady) {
        DeleteCriticalSection(&gstVxdLock);
        gstVxdLockReady = 0;
    }
    memset(&gstVxdConfig, 0, sizeof(gstVxdConfig));
    return gstReportError(1, 0x10007);
}

int gstVxdFreeEvents(void)
{
    unsigned i;

    if (gstVxdEvents) {
        for (i = 0; i < gstVxdConfig.eventCount + 1; i++)
            CloseHandle(gstVxdEvents[i]);
        gstFreeHandler(gstVxdEvents);
        gstVxdEvents = 0;
    }
    if (gstVxdConfig.vxdEvents) {
        gstFreeHandler(gstVxdConfig.vxdEvents);
        gstVxdConfig.vxdEvents = 0;
    }
    return gstReportError(1, 0x10001);
}

int gstVxdStopThread(void)
{
    if (gstVxdThread) {
        if (gstVxdStopEvent)
            SetEvent(gstVxdStopEvent);
        if (WaitForSingleObject(gstVxdThread, 4000) == WAIT_TIMEOUT) {
            TerminateThread(gstVxdThread, 0);
            CloseHandle(gstVxdThread);
            gstVxdThread = 0;
            gstVxdThreadId = 0;
        }
    }
    if (gstVxdStopEvent) {
        CloseHandle(gstVxdStopEvent);
        gstVxdStopEvent = 0;
    }
    return gstReportError(1, 0x10004);
}

/* Hands a received packet back to the VxD. */
int gstVxdFreeDownPacket(GstCodec *packet)
{
    int status;

    if (packet == 0)
        return gstReportError(1, 0x1000d);
    EnterCriticalSection(&gstVxdLock);
    DeviceIoControl(gstVxdHandle, 0x805, packet, 0, 0, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x1000d);
}

/* Points the SDK's driver table at the VxD routines. */
void gstVxdInstallDriver(void)
{
    gpiDriverSlot00 = gstVxdPutChar;
    gpiDriverSlot04 = gstVxdGetChar;
    gstDriverFlush = gstVxdClose;
    gstInitHook = gstVxdRelease;
    gstDriverSlot08 = gstVxdDial;
    gstDriverSlot0c = gstVxdHangup;
    gstDriverSlot10 = gstVxdAllocUpPacket;
    gstDriverCall = gstVxdSendUpPacket;
    gstDriverSlot18 = gstVxdFreeUpPacket;
    gstDriverSlot1c = gstVxdGetDownPacket;
    gstDriverSlot20 = gstVxdFreeDownPacket;
    gstDriverSlot24 = gstVxdRequestAll;
    gpiDriverSlot28 = gstVxdRequestSpecific;
    gpiDriverSlot2c = gstVxdUnsupported;
    gstDriverSlot30 = gstVxdSetDefaultHandler;
    gpiDriverSlot34 = gstVxdSetHandler;
    gpiDriverSlot38 = gstVxdSetPolled;
    gpiDriverSlot3c = gstVxdSetPollHandler;
    gpiDriverSlot40 = gstVxdSetEnabled;
    gstDriverSlot44 = gstVxdWaitPacket;
}

int gstVxdPutChar(char c)
{
    int status;
    int written;

    if (!DeviceIoControl(gstVxdHandle, 0x201, &c, 1, &written, gstMode, &status, 0))
        return gstReportError(status, 0xffff);
    if (written == 0)
        return gstReportError(0x8fff000b, 0xffff);
    return gstReportError(status, 0xffff);
}

int gstVxdGetChar(char *c)
{
    int status;
    int received;

    if (c == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    if (!DeviceIoControl(gstVxdHandle, 0x200, c, 1, &received, gstMode, &status, 0))
        return gstReportError(status, 0xffff);
    if (received == 0)
        return gstReportError(0x8fff000b, 0xffff);
    return gstReportError(status, 0xffff);
}

/* Initialises the VxD for the given mode and drops the handle again. */
int gstVxdRelease(char mode)
{
    if (!mode && gstVxdHandle) {
        if (!DeviceIoControl(gstVxdHandle, 0x10, 0, 0, 0, 0, 0, 0))
            return gstReportError(0x8fff0001, 0x10007);
    }
    if (gstVxdHandle) {
        CloseHandle(gstVxdHandle);
        gstVxdHandle = 0;
    }
    return gstReportError(1, 0x10007);
}

static const char *okReplies[3] = { "OK", "ERROR" };
static const char *dialReplies[6] = { "CONNECT", "ERROR", "NO DIAL TONE", "NO CARRIER", "BUSY" };

/* Dials the number from C:\GOL.INI: sets up the serial port from the file's settings, resets
   the modem, dials and waits for the answer, then logs in with the user id and password. */
int gstVxdDial(void)
{
    char vxdPort[32];
    char baudStr[12];
    char dialString[64];
    char userPassword[64];
    struct {
        unsigned baud;
        char *port;
    } serial;
    char command[256];
    int reason;
    char garbage;
    int outLen;

    if (!gstFileExists("C:\\GOL.INI"))
        return gstReportError(0x80030000, 0x10008);
    gstSettings = gstIniLoadFile("C:\\GOL.INI");
    if (gstSettings == 0)
        return gstReportError(0x80030001, 0x10008);
    strncpy(vxdPort, gstIniGet(gstSettings, "VXDCOM"), 0x20);
    strncpy(baudStr, gstIniGet(gstSettings, "WINBAUD"), 0xc);
    strncpy(gstUserId, gstIniGet(gstSettings, "UID"), 0x20);
    strncpy(gstGameName, gstIniGet(gstSettings, "GAMENAME"), 0x40);
    strncpy(userPassword, gstIniGet(gstSettings, "PASSWORD"), 0x40);
    strncpy(dialString, gstIniGet(gstSettings, "DIALSTRING"), 0x40);
    if (!gstIniFree(gstSettings))
        return gstReportError(0x80030000, 0x10008);
    if (strcmp(baudStr, "9600") == 0)
        serial.baud = 9600;
    else if (strcmp(baudStr, "14400") == 0)
        serial.baud = 14400;
    else if (strcmp(baudStr, "19200") == 0)
        serial.baud = 19200;
    else if (strcmp(baudStr, "38400") == 0)
        serial.baud = 38400;
    else if (strcmp(baudStr, "56000") == 0)
        serial.baud = 56000;
    else
        return gstReportError(0x8fff0010, 0x10008);
    serial.port = vxdPort;
    if (!DeviceIoControl(gstVxdHandle, 0x102, &serial, 8, 0, gstMode, &reason, 0)) {
        if (reason != 0x8fff000f)
            return gstReportError(reason, 0x10008);
        return gstReportError(1, 0x10008);
    }
    reason = gstModemCommand(0, 1, 0);
    if (reason < 1
        || (reason = gstModemCommand("AT", 1, okReplies)) < 1 || reason != 1
        || (reason = gstModemCommand("ATX0E1V1Q0", 1, okReplies)) < 1 || reason != 1
        || (reason = gstModemCommand(dialString, 1, dialReplies)) < 1 || reason != 1) {
        gstVxdHangup();
        if (reason == 2)
            return gstReportError(0x8fff0016, 0x10008);
        else if (reason == 3)
            return gstReportError(0x8fff0012, 0x10008);
        else if (reason == 4)
            return gstReportError(0x8fff0017, 0x10008);
        else if (reason == 5)
            return gstReportError(0x8fff0014, 0x10008);
        return gstReportError(reason, 0x10008);
    }
    reason = gstModemWaitFor("--> ", 4);
    if (reason < 1) {
        gstVxdHangup();
        return gstReportError(0x80030002, 0x10008);
    }
    gstDefaultSleep(500);
    reason = gstVxdGetChar(&garbage);
    if (reason != 0x8fff000b) {
        if (reason < 1) {
            gstVxdHangup();
            return gstReportError(reason, 0x10008);
        }
        gstDefaultSleep(2000);
        while ((reason = gstVxdGetChar(&garbage)) != 0x8fff000b) {
            if (reason < 1) {
                gstVxdHangup();
                return gstReportError(reason, 0x10008);
            }
        }
        reason = gstVxdPutChar(0xd);
        if (reason < 1) {
            gstVxdHangup();
            return gstReportError(reason, 0x10008);
        }
        reason = gstModemWaitFor("--> ", 4);
        if (reason < 1) {
            gstVxdHangup();
            return gstReportError(0x80030002, 0x10008);
        }
    }
    wsprintfA(command, "%c%c%c%c%c%c%c%s%c%c%s%c%c%s%c%c", 0x13, 0x13, 0x13, 7, 0x13, 0x14, 2,
              "0001", 1, 2, gstUserId, 1, 2, userPassword, 3, 0);
    if (!DeviceIoControl(gstVxdHandle, 0x201, command, strlen(command), &outLen, gstMode, &reason, 0)) {
        gstVxdHangup();
        return gstReportError(0x80030003, 0x10008);
    }
    reason = gstModemWaitFor("\001#\001#", 4);
    if (reason < 1) {
        gstVxdHangup();
        return gstReportError(0x80030005, 0x10008);
    }
    if (!DeviceIoControl(gstVxdHandle, 0x800, &serial, 8, 0, gstMode, &reason, 0)) {
        gstVxdHangup();
        return gstReportError(0x80030007, 0x10008);
    }
    return gstReportError(1, 0x10008);
}

int gstVxdHangup(void)
{
    int status;

    if (!DeviceIoControl(gstVxdHandle, 0x103, 0, 0, 0, gstMode, &status, 0))
        return gstReportError(status, 0x10009);
    return gstReportError(1, 0x10009);
}

int gstVxdAllocUpPacket(GstCodec **packet, int port, unsigned size)
{
    int status;

    EnterCriticalSection(&gstVxdLock);
    DeviceIoControl(gstVxdHandle, 0x801, packet, port, &size, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x1000a);
}

int gstVxdSendUpPacket(GstCodec *packet, int port)
{
    int status;

    EnterCriticalSection(&gstVxdLock);
    DeviceIoControl(gstVxdHandle, 0x802, packet, port, 0, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x1000b);
}

int gstVxdFreeUpPacket(GstCodec *packet, int port)
{
    int status;

    EnterCriticalSection(&gstVxdLock);
    DeviceIoControl(gstVxdHandle, 0x803, packet, port, 0, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0xffff);
}

/* Fetches the next received packet: from the VxD when its event is signalled, else from the
   polled queue. With a timeout (zero polls once, -1 waits forever) it keeps trying. */
int gstVxdGetDownPacket(GstCodec **packet, unsigned timeout)
{
    int status;
    unsigned startTick;

    *packet = 0;
    startTick = gstClockHandler();
    status = 1;
    for (;;) {
        if (!gstTestFlag(0x1c) || WaitForSingleObject(gstVxdEvents[0], 0) == 0) {
            EnterCriticalSection(&gstVxdLock);
            DeviceIoControl(gstVxdHandle, 0x804, packet, 0, 0, gstMode, &status, 0);
            LeaveCriticalSection(&gstVxdLock);
            if (status < 1) {
                *packet = 0;
                break;
            }
            if (*packet) {
                status = gstDispatchDownPacket(*packet);
                if (status < 1)
                    break;
            }
            *packet = 0;
        }
        status = gstDispatchGetPolled(packet);
        if (status < 1)
            break;
        else if (*packet)
            break;
        else if (!gstTestFlag(0x1c) || timeout == 0)
            break;
        if (gstCancelCallback && gstCancelCallback()) {
            status = 0x8000000b;
            break;
        }
        if (timeout && timeout != -1 && gstClockHandler() - startTick >= timeout) {
            status = 0x8000000a;
            break;
        }
    }
    return gstReportError(status, 0x1000c);
}

int gstVxdRequestAll(char mode)
{
    int status;

    EnterCriticalSection(&gstVxdLock);
    DeviceIoControl(gstVxdHandle, 0x806, 0, mode, 0, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x1000e);
}

int gstVxdRequestSpecific(char mode, unsigned char *tags)
{
    int flags;
    int status;

    flags = (mode ? 0x80000000 : 0) | 1;
    EnterCriticalSection(&gstVxdLock);
    DeviceIoControl(gstVxdHandle, 0x807, 0, flags, tags, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x1000f);
}

int gstVxdUnsupported(void)
{
    return gstReportError(0x80000008, 0x10010);
}

int gstVxdSetDefaultHandler(GstPacketHandler handler)
{
    int status;

    EnterCriticalSection(&gstVxdLock);
    status = gstDispatchSetDefault(handler);
    if (status >= 1)
        DeviceIoControl(gstVxdHandle, 0x806, 0, handler != 0, 0, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x10011);
}

int gstVxdSetHandler(GstPacketHandler handler, unsigned char *tags)
{
    int flags;
    int status;

    flags = (handler ? 0x80000000 : 0) | 2;
    EnterCriticalSection(&gstVxdLock);
    status = gstDispatchSetHandler(handler, tags);
    if (status >= 1)
        DeviceIoControl(gstVxdHandle, 0x807, 0, flags, tags, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x10012);
}

int gstVxdSetPolled(char mode, unsigned char *tags)
{
    int flags;
    int status;

    flags = (mode ? 0x80000000 : 0) | 1;
    EnterCriticalSection(&gstVxdLock);
    status = gstDispatchSetPolled(mode, tags);
    if (status >= 1)
        DeviceIoControl(gstVxdHandle, 0x807, 0, flags, tags, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x10013);
}

int gstVxdSetPollHandler(GstPacketHandler handler, unsigned char *tags)
{
    int flags;
    int status;

    flags = (handler ? 0x80000000 : 0) | 0x20;
    EnterCriticalSection(&gstVxdLock);
    status = gstDispatchSetPollHandler(handler, tags);
    if (status >= 1)
        DeviceIoControl(gstVxdHandle, 0x807, 0, flags, tags, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x10014);
}

int gstVxdSetEnabled(char enable, unsigned char *tags)
{
    int flags;
    int status;

    flags = (enable ? 0x80000000 : 0) | 0x10;
    EnterCriticalSection(&gstVxdLock);
    status = gstDispatchSetEnabled(enable, tags);
    if (status >= 1)
        DeviceIoControl(gstVxdHandle, 0x807, 0, flags, tags, gstMode, &status, 0);
    LeaveCriticalSection(&gstVxdLock);
    return gstReportError(status, 0x10015);
}

/* Waits for a packet and drops it: an error when nothing arrived in time. */
int gstVxdWaitPacket(int timeout)
{
    GstCodec *packet;
    int status;

    status = gstVxdGetDownPacket(&packet, timeout);
    if (status >= 1 && packet == 0)
        status = 0x80000010;
    return gstReportError(status, 0x10016);
}

int gstVxdReset(void)
{
    int status;

    if (gstVxdHandle) {
        EnterCriticalSection(&gstVxdLock);
        if (!DeviceIoControl(gstVxdHandle, 0x40, 0, 0, 0, gstMode, &status, 0))
            return gstReportError(0x8fff0001, 0x10007);
        LeaveCriticalSection(&gstVxdLock);
    } else {
        return gstReportError(0x80000000, 0x10007);
    }
    return gstReportError(1, 0x10007);
}

/* The next four are diagnostics: they ask the VxD for internal state with DeviceIoControl 0x80. */
int gstVxdQueryCommand0(int *out)
{
    int result = 0x8fff0000;
    GstVxdRequest request;

    if (out == 0)
        return GST_ERR_INVALID_PARAM;
    request.command = 0;
    request.arg1 = 0;
    request.arg2 = 0;
    request.arg3 = 0;
    DeviceIoControl(gstVxdHandle, 0x80, &request, 0, out, 0, &result, 0);
    return result;
}

int gstVxdGetLastError(int *error)
{
    int result = 0x8fff0000;
    GstVxdRequest request;

    if (error == 0)
        return GST_ERR_INVALID_PARAM;
    request.command = 1;
    request.arg1 = 0;
    request.arg2 = 0;
    request.arg3 = 2;
    DeviceIoControl(gstVxdHandle, 0x80, &request, 0, error, 0, &result, 0);
    return result;
}

/* Collects four result flags from the VxD's registers. */
int gstVxdGetStatusBits(unsigned char *bits)
{
    int result = 0x8fff0000;
    int value;
    GstVxdRequest request;

    if (bits == 0)
        return GST_ERR_INVALID_PARAM;
    *bits = 0;
    request.command = 1;
    request.arg1 = 0;
    request.arg2 = 0;
    request.arg3 = 0;
    DeviceIoControl(gstVxdHandle, 0x80, &request, 0, &value, 0, &result, 0);
    if (result >= 1) {
        if (value)
            *bits |= 1;
        request.command = 3;
        request.arg1 = 1;
        request.arg2 = 0;
        request.arg3 = 0;
        DeviceIoControl(gstVxdHandle, 0x80, &request, 0, &value, 0, &result, 0);
    }
    if (result >= 1) {
        if (value)
            *bits |= 2;
        request.command = 3;
        request.arg1 = 1;
        request.arg2 = 0;
        request.arg3 = 0x48;
        DeviceIoControl(gstVxdHandle, 0x80, &request, 0, &value, 0, &result, 0);
    }
    if (result >= 1) {
        if (value)
            *bits |= 4;
        request.command = 3;
        request.arg1 = 1;
        request.arg2 = 0;
        request.arg3 = 0x49;
        DeviceIoControl(gstVxdHandle, 0x80, &request, 0, &value, 0, &result, 0);
    }
    if (result >= 1 && value)
        *bits |= 8;
    return result;
}

int gstVxdReadRegister(unsigned char *out)
{
    int result = 0x8fff0000;
    GstVxdRequest request;
    unsigned raw;

    if (out == 0)
        return GST_ERR_INVALID_PARAM;
    request.command = 3;
    request.arg1 = 1;
    request.arg2 = 0;
    request.arg3 = 6;
    DeviceIoControl(gstVxdHandle, 0x80, &request, 0, &raw, 0, &result, 0);
    if (result >= 1)
        *out = raw;
    return result;
}

/* Reads byte `offset` of the VxD's state, four bytes at a time. */
int gstVxdReadByte(unsigned char offset, unsigned char *out)
{
    int result = 0x8fff0000;
    GstVxdRequest request;
    unsigned raw;

    if (out == 0)
        return GST_ERR_INVALID_PARAM;
    request.command = 3;
    request.arg1 = 1;
    request.arg2 = 0;
    request.arg3 = offset / 4 + 7;
    DeviceIoControl(gstVxdHandle, 0x80, &request, 0, &raw, 0, &result, 0);
    if (result >= 1)
        *out = raw >> (offset % 4 * 8);
    return result;
}
