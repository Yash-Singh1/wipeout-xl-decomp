#include "gst.h"
#include "gpi_api.h"

/* Reports success. */
int gstReturnSuccess(void)
{
    return gstReportError(1, 0xffff);
}

/* Succeeds once the SDK is initialised, else reports why not. */
int gstCheckInitialised(void)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    return 1;
}

/* GPI entry points: they check their arguments and the SDK state, then call the serial
   (mode 0x40) or the network implementation. Errors carry the FN_GPI_ call id as detail
   (0x100001 is FN_GPI_CONNECT, ...); 0xffff means no particular call. */

extern int func_4b5171(void);
extern int func_4b5a8a(void);
extern int func_4b5cd9(void);

/* FN_GPI_PROVIDER_ENUM: lists "Single Player", "Gamers Online" and the DirectPlay providers. */
int gstEnumProvidersGPI(GstProviderFn callback)
{
    int result;

    if (callback == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100003);
    if (gstIsInitialised())
        return gstReportError(0x80000001, 0x100003);
    gstBufferList = 0;
    result = gstAddRequest(0x10000, "Single Player", 0, 0, callback);
    if (result < 1)
        return gstReportError(result, 0x100003);
    result = gstAddRequest(0x20000, "Gamers Online", 1, 0, callback);
    if (result < 1)
        return gstReportError(result, 0x100003);
    result = gpiEnumProviders(0x30000, callback);
    if (result < 1)
        return result;
    return gstReportError(1, 0x100003);
}

/* FN_GPI_INIT: the game picks one of the providers listed above. */
int gstInitGPI(GstQueryFn query)
{
    unsigned long choice;
    int status;
    GstBuffer *cur;

    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100000);
    if (gstIsInitialised())
        return gstReportError(0x80000001, 0x100000);
    status = query(0, &choice);
    if (status != 1)
        return gstReportError(status, 0x100000);
    if ((choice & 0xffff0000) == 0) {
        if (choice == 1)
            return gstVxdOpen(choice, query);
    } else if (choice == 0x10000) {
        return gstReportError(1, 0x100000);
    } else if ((choice & 0xffff0000) == 0x20000) {
        cur = gstBufferList;
        if (cur) {
            while (cur && cur->owner != choice)
                cur = cur->next;
        }
        if (cur)
            return gstVxdOpen(cur->id, query);
    } else if ((choice & 0xffff0000) == 0x30000) {
        cur = gstBufferList;
        if (cur) {
            while (cur && cur->owner != choice)
                cur = cur->next;
        }
        if (cur)
            return gpiInit(cur->guid, query);
    }
    return gstReportError(GST_ERR_INVALID_PARAM, 0x100000);
}

/* Frees every buffer the SDK handed out, then flushes the driver. */
int gstReleaseBuffers(void)
{
    GstBuffer *buffer;

    buffer = gstBufferList;
    if (buffer) {
        gstBufferList = 0;
        while (buffer) {
            GstBuffer *current;

            current = buffer;
            buffer = buffer->next;
            gstFreeHandler(current);
        }
    }
    return gstExitSDK();
}

int gstConnectGPI(GstQueryFn query)
{
    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100001);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100001);
    if (gstMode == 0x40)
        return gpiConnect(query);
    return gstConnectNet(query);
}

int gstDisconnectGPI(void)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100002);
    if (gstMode == 0x40)
        return func_4b5171();
    return gstDisconnectNet();
}

int gstEnumSessionsGPI(GstSessionEnumFn callback)
{
    if (callback == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100004);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100004);
    if (gstMode == 0x40)
        return gpiEnumSessions(callback);
    return gstEnumSessionsNet(callback);
}

int gstCreateSessionGPI(GstQueryFn query)
{
    int result;

    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100005);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100005);
    result = gstDispatchFlush();
    if (result < 1)
        return gstReportError(result, 0x100005);
    if (gstMode == 0x40) {
        result = gpiCreateSession(query);
        if (result < 1)
            return result;
    }
    return gstCreateSessionNet(query);
}

int gstJoinSessionGPI(GstQueryFn query)
{
    int result;

    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100006);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100006);
    result = gstDispatchFlush();
    if (result < 1)
        return gstReportError(result, 0x100006);
    if (gstMode == 0x40) {
        result = gpiJoinSession(query);
        if (result < 1)
            return result;
    }
    return gstJoinSessionNet(query);
}

int gstNewUpPacket(int port, char type, int destination, int size, void *data)
{
    if (size == 0 || data == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    return gstNewUpPacketNet(port, type, destination, size, data);
}

int gstExitSessionGPI(void)
{
    int result;

    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100007);
    result = gstExitSessionNet();
    if (result < 1 && gstMode != 0x40)
        return result;
    if (gstMode == 0x40)
        result = func_4b5a8a();
    return gstReportError(result, 0x100007);
}

int gstEnumLockstepGPI(unsigned long sessionId, unsigned char *kinds, GstLockstepEnumFn callback, unsigned timeout)
{
    if (callback == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    return gstEnumLockstepNet(sessionId, kinds, callback, timeout);
}

int gstJoinLockstepGPI(GstQueryFn query)
{
    int result;

    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100008);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100008);
    if (gstMode == 0x40) {
        result = gpiJoinLockstep(query);
        if (result < 1)
            return result;
    }
    return gstJoinLockstepNet(query);
}

int gstNewEventUpPacketGPI(int port, char type, int event, unsigned size, void *data)
{
    if (size == 0 || data == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    return gstNewEventUpPacketNet(port, type, event, size, data);
}

int gstNewLockstepUpPacket(int port, char type, int size, void *data)
{
    if (size == 0 || data == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xffff);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    return gstNewLockstepUpPacketNet(port, type, size, data);
}

int gstExitLockstepGPI(void)
{
    int result;

    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x100009);
    result = gstExitLockstepNet();
    if (result < 1 && gstMode != 0x40)
        return result;
    if (gstMode == 0x40)
        result = func_4b5cd9();
    return gstReportError(result, 0x100009);
}

int gstSendChatGPI(int port, int unused, unsigned long sender, const char *text)
{
    char defaultText = gstDefaultPort;

    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    if (text == 0)
        text = &defaultText;
    return gstSendChatNet(port, unused, sender, text);
}

int gstSendCommandGPI(int port, int unused, unsigned long target, unsigned long sender, int command, short argument)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    return gstSendCommandNet(port, unused, target, sender, command, argument);
}

int gstIDRequestGPI(unsigned char which, unsigned long *out)
{
    if (out == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x10000a);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x10000a);
    return gstIdRequestNet(which, out);
}

int gstSendUpPacketGPI(GstCodec *packet, int port)
{
    if (packet == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xf);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xf);
    if (gstMode == 0x40)
        return gpiSendUnreliable(packet, port);
    return gstDriverCall(packet, port);
}
