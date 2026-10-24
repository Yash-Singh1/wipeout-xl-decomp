#include <stdarg.h>
#include "gst.h"

/* SDK calls that go straight to a driver routine once the SDK is initialised. */

int gstDial(const char *number)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xc);
    return gstDriverSlot08();
}

int gstHangup(void)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xd);
    return gstDriverSlot0c();
}

int gstAllocUpPacket(GstCodec **a, int b, unsigned c)
{
    if (a == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xe);
    *a = 0;
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xe);
    return gstDriverSlot10(a, b, c);
}

int gstSendEncodedUpPacket(GstCodec *a, int b)
{
    if (a == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xf);
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xf);
    return gstDriverCall(a, b);
}

int gstFreeUpPacket(GstCodec *a, int b)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0xffff);
    if (a == 0)
        return gstReportError(1, 0xffff);
    return gstDriverSlot18(a, b);
}

int gstGetDownPacket(GstCodec **a, int b)
{
    if (a == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x10);
    *a = 0;
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x10);
    return gstDriverSlot1c(a, b);
}

int gstFreeDownPacket(GstCodec *a)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x11);
    if (a == 0)
        return gstReportError(1, 0x11);
    return gstDriverSlot20(a);
}

int gstRequestAllPackets(int a)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x12);
    if (gstTestFlag(0x1a))
        return gstReportError(GST_ERR_UNSUPPORTED_MODE_FUNCTION, 0x12);
    return gstDriverSlot24(a);
}

/* Call 0x13: only these packet types are wanted from now on: tag characters (the first one is
   `firstTag`), ending in a zero. */
int gstRequestSpecificPackets(int mode, unsigned char firstTag, ...)
{
    unsigned char list[256];
    va_list arg;
    unsigned char *dst;

    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x13);
    if (gstTestFlag(0x1a))
        return gstReportError(GST_ERR_UNSUPPORTED_MODE_FUNCTION, 0x13);
    list[0] = firstTag;
    if (list[0]) {
        dst = &list[1];
        va_start(arg, firstTag);
        while ((*dst = va_arg(arg, char)) != 0)
            dst++;
        va_end(arg);
    }
    return gpiDriverSlot28(mode, list);
}

int gstTriggerHandler(int a)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x14);
    if (!gstTestFlag(0xa))
        return gstReportError(GST_ERR_UNSUPPORTED_MODE_FUNCTION, 0x14);
    return gstDriverSlot44(a);
}

int gstRegisterDebugPacketHandler(void)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x15);
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0x15);
}

int gstRegisterDefaultPacketHandler(GstPacketHandler handler)
{
    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x16);
    if (!gstTestFlag(0x1a))
        return gstReportError(GST_ERR_UNSUPPORTED_MODE_FUNCTION, 0x16);
    return gstDriverSlot30(handler);
}

/* Call 0x17: packets with these tags go to `handler`. */
int gstRegisterSpecificPacketHandler(GstPacketHandler handler, unsigned char firstTag, ...)
{
    unsigned char list[256];
    va_list arg;
    unsigned char *dst;

    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x17);
    if (!gstTestFlag(0x1a))
        return gstReportError(GST_ERR_UNSUPPORTED_MODE_FUNCTION, 0x17);
    list[0] = firstTag;
    if (list[0]) {
        dst = &list[1];
        va_start(arg, firstTag);
        while ((*dst = va_arg(arg, char)) != 0)
            dst++;
        va_end(arg);
    }
    return gpiDriverSlot34(handler, list);
}

/* Call 0x18: packets with these tags are only delivered when polled. */
int gstRegisterPolledPackets(int mode, unsigned char firstTag, ...)
{
    unsigned char list[256];
    va_list arg;
    unsigned char *dst;

    if (!gstIsInitialised())
        return gstReportError(GST_ERR_SDK_UNINITIALIZED, 0x18);
    if (!gstTestFlag(0x1a))
        return gstReportError(GST_ERR_UNSUPPORTED_MODE_FUNCTION, 0x18);
    list[0] = firstTag;
    if (list[0]) {
        dst = &list[1];
        va_start(arg, firstTag);
        while ((*dst = va_arg(arg, char)) != 0)
            dst++;
        va_end(arg);
    }
    return gpiDriverSlot38(mode, list);
}
