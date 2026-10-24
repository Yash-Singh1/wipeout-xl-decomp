#include "gst.h"

static const char *successUndefined = "Undefined success";
static const char *successOk = "Successful";
static const char *errorUndefined = "Undefined error";
static const char *errorUnknown = "Unknown error code";
/* 0x8fff0000 + n: the serial driver. */
static const char *driverErrors[25] = {
    "GST Driver uninitialized",
    "GST Driver not removed",
    "GST Driver version mismatch",
    "Invalid process type",
    "Process already running",
    "Invalid function call",
    "Invalid channel number",
    "Invalid function parameter",
    "Paired function mismatch",
    "Recurring failure",
    "No driver level memory",
    "Asynchronous I/O pending",
    "Invalid modem port",
    "Modem port in use",
    "Modem port initialization",
    "Modem port already open",
    "Invalid baud rate",
    "No modem found",
    "No dial tone",
    "Service did not answer",
    "Service line busy",
    "Voice answered",
    "Serial port error",
    "No carrier",
    "Serial BREAK received",
};
/* 0x80000000 + n: the SDK. */
static const char *sdkErrors[17] = {
    "SDK not initialized",
    "SDK already initialized",
    "Memory unallocated",
    "Process data uninitialized",
    "Packet handler uninitialized",
    "Packet automation uninitialized",
    "Invalid mode",
    "Unsupported mode",
    "Unsupported SDK function",
    "Unsupported mode function",
    "Timeout occurred",
    "User cancel",
    "Not in asynchronous mode",
    "Queue to small for packet",
    "Packet queue overflow",
    "Synchronization failed",
    "No packets are ready",
};
/* 0x80030000 + n: dialling in. */
static const char *loginErrors[8] = {
    "Configuration file missing",
    "Configuration file error",
    "No log in prompt",
    "Illegal log in message",
    "Did not pay service",
    "User name not found",
    "Duplicate user",
    "Incorrect password",
};
static const char *functionUnknown = "Unknown function code";
/* Call ids 0 - 0x18. */
static const char *sdkFunctions[25] = {
    "Initialize SDK",
    "Exit SDK",
    "Register User Errors",
    "Register User Functions",
    "Register User Memory Allocate",
    "Register User Memory Free",
    "Register User Status Callback",
    "Register User Delay",
    "Register User Clock",
    "Register User Cancel Callback",
    "Register User Debug Print",
    "Register User Error Callback",
    "Dial, Connect and Login",
    "Logout, Disconnect and Hangup",
    "Allocate New Upload Packet",
    "Send Encoded Upload Packet",
    "Get Next Download Packet",
    "Release Download Packet",
    "Request All Incoming Packets",
    "Request Specific Packet Types",
    "Trigger Next Handler Callback",
    "Register Debug Packet Handler",
    "Register Default Packet Handler",
    "Register Specific Packet Handler",
    "Register Polled Packets",
};
/* Call ids 0x10000 - 0x10017: the driver routines. */
static const char *osFunctions[24] = {
    "(OS) Initialize Synchronizer",
    "(OS) Exit Synchronizer",
    "(OS) Packet Processor",
    "(OS) Initialize Automation",
    "(OS) Exit Automation",
    "(OS) Send Modem Command",
    "(OS) Initialize SDK",
    "(OS) Exit SDK",
    "(OS) Dial, Connect and Login",
    "(OS) Logout, Disconnect and Hangup",
    "(OS) Allocate New Upload Packet",
    "(OS) Send Encoded Upload Packet",
    "(OS) Get Next Download Packet",
    "(OS) Release Download Packet",
    "(OS) Request All Packets",
    "(OS) Request Packet Types",
    "(OS) Register Debug Packet Handler",
    "(OS) Register Default Packet Handler",
    "(OS) Register Packet Handler",
    "(OS) Register Polled Packets",
    "(OS) Register System Packet Handler",
    "(OS) Register System Polled Packets",
    "(OS) Trigger Next Handler Callback",
    "(OS) Retrieve Debug Data",
};

/* The SDK's state: the mode it was started in, its flags, the driver routine table (filled in by
   gstVxdInstallDriver) and the user's tables. */
int gstMode = 0;
unsigned gstFlags = 1;
int (*gpiDriverSlot00)(char c) = 0;
int (*gpiDriverSlot04)(char *c) = 0;
int (*gstDriverFlush)(void) = 0;
int (*gstInitHook)(char keepOpen) = 0;
int (*gstDriverSlot08)(void) = 0;
int (*gstDriverSlot0c)(void) = 0;
int (*gstDriverSlot10)(GstCodec **packet, int port, unsigned size) = 0;
int (*gstDriverCall)(GstCodec *packet, int arg) = 0;
int (*gstDriverSlot18)(GstCodec *packet, int arg) = 0;
int (*gstDriverSlot1c)(GstCodec **packet, int arg) = 0;
int (*gstDriverSlot20)(GstCodec *packet) = 0;
int (*gstDriverSlot24)(char mode) = 0;
int (*gpiDriverSlot28)(char mode, char *list) = 0;
int (*gpiDriverSlot2c)(void) = 0;
int (*gstDriverSlot30)(GstPacketHandler handler) = 0;
int (*gpiDriverSlot34)(GstPacketHandler handler, char *list) = 0;
int (*gpiDriverSlot38)(char mode, char *list) = 0;
int (*gpiDriverSlot3c)(GstPacketHandler handler, char *tags) = 0;
int (*gpiDriverSlot40)(char enable, char *tags) = 0;
int (*gstDriverSlot44)(int timeout) = 0;
char gstVerbose = 0;
unsigned short gstUserErrorCount = 0;
const char **gstUserErrors = 0;
unsigned short gstUserFunctionCount = 0;
const char **gstUserFunctions = 0;

/* Passes an error to the registered callback (when quiet only real errors) and returns it. */
int gstReportError(int error, int detail)
{
    if (gstErrorCallback) {
        if (gstVerbose || error < 1)
            gstErrorCallback(error, detail);
    }
    return error;
}

char gstIsInitialised(void)
{
    return gstMode != 0;
}

char gstTestFlag(int mask)
{
    return (gstFlags & mask) != 0;
}


/* Text for an SDK error code; the high word selects the table. 0x80010000 + n are the packet
   codec errors and 0x80020000 + n the dispatcher's, 0xffff0000 + n the user's own. */
const char *gstSdkErrorString(int error)
{
    if (error > 1)
        return successUndefined;
    else if (error == 1)
        return successOk;
    else if (error == 0)
        return errorUndefined;
    else if ((error & 0xffff0000) == 0x8fff0000 && (error & 0xffff) < 0x19)
        return driverErrors[error & 0xffff];
    else if ((error & 0xffff0000) == 0x80000000 && (error & 0xffff) < 0x11)
        return sdkErrors[error & 0xffff];
    else if ((error & 0xffff0000) == 0x80010000 && (error & 0xffff) < 4)
        return gstCodecErrorNames[error & 0xffff];
    else if ((error & 0xffff0000) == 0x80020000 && (error & 0xffff) < 1)
        return gstDispatchErrorNames[error & 0xffff];
    else if ((error & 0xffff0000) == 0x80030000 && (error & 0xffff) < 8)
        return loginErrors[error & 0xffff];
    else if ((error & 0xffff0000) == 0xffff0000 && (error & 0xffff) < gstUserErrorCount && gstUserErrors)
        return gstUserErrors[error & 0xffff];
    else
        return errorUnknown;
}

/* Name of an SDK call id (the detail argument of gstReportError). */
const char *gstSdkFunctionName(int code)
{
    if ((code & 0xffff0000) == 0 && (code & 0xffff) < 0x19)
        return sdkFunctions[code & 0xffff];
    else if ((code & 0xffff0000) == 0x10000 && (code & 0xffff) < 0x18)
        return osFunctions[code & 0xffff];
    else if ((code & 0xffff0000) == 0x20000 && (code & 0xffff) < 0x18)
        return gstCodecFunctionNames[code & 0xffff];
    else if ((code & 0xffff0000) == 0x30000 && (code & 0xffff) < 9)
        return gstDispatchFunctionNames[code & 0xffff];
    else if ((code & 0xffff0000) == 0x7fff0000 && (code & 0xffff) < gstUserFunctionCount && gstUserFunctions)
        return gstUserFunctions[code & 0xffff];
    else
        return functionUnknown;
}
