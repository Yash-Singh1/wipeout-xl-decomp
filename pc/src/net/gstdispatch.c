#include "gst.h"

/* Packet dispatch: each tag byte can have a handler, be queued for polling, or fall through
   to the default handler. The queue holds packets waiting to be polled. Errors carry call ids
   0x30000-0x30008. */

GstPacketHandler gstDefaultPacketHandler = 0;

/* The one error text (0x80020000) and the names of the call ids 0x30000 + n. */
const char *gstDispatchErrorNames[1] = {
    "Passed a NULL handler pointer",
};

const char *gstDispatchFunctionNames[9] = {
    "Initialize packet handlers",
    "Exit packet handlers",
    "Register User Default Handler",
    "Register User Handler List",
    "Register User Poll List",
    "Register System Handler List",
    "Register System Poll List",
    "Process Packet",
    "Dequeue Polled Packet",
};

int gstDispatchInit(unsigned queueSize)
{
    int i;
    int result;

    result = gstQueueCreate(&gstDownQueue, queueSize);
    if (result < 1)
        return gstReportError(result, 0x30000);
    gstDefaultPacketHandler = 0;
    for (i = 0; i < 256; i++) {
        gstTagHandler[i] = 0;
        gstTagPollHandler[i] = 0;
        gstTagFlags[i] = 0;
    }
    return gstReportError(1, 0x30000);
}

int gstQueueCreate(GstPacketQueue *queue, unsigned size)
{
    int status;

    status = gstQueueDestroy(queue);
    if (status < 1)
        return status;
    queue->items = gstDefaultAlloc(size * 4);
    if (queue->items == 0)
        return 0x80000002;
    queue->size = size;
    return 1;
}

int gstQueueDestroy(GstPacketQueue *queue)
{
    if (queue->items) {
        gstFreeHandler(queue->items);
        queue->items = 0;
    }
    gstQueueReset(queue);
    return 1;
}

void gstQueueReset(GstPacketQueue *queue)
{
    queue->items = 0;
    queue->head = 0;
    queue->tail = 0;
}

/* Frees every packet still queued for polling. */
int gstDispatchFlush(void)
{
    int status;
    GstCodec *packet;

    for (;;) {
        status = gstQueuePop(&gstDownQueue, &packet);
        if (status < 1)
            break;
        if (packet == 0)
            break;
        status = gstFreeDownPacket(packet);
        if (status < 1)
            break;
    }
    gstQueueClear(&gstDownQueue);
    return gstReportError(status, 0xffff);
}

void gstQueueClear(GstPacketQueue *queue)
{
    queue->head = 0;
    queue->tail = 0;
}

int gstQueuePop(GstPacketQueue *queue, GstCodec **packet)
{
    if (queue->tail != queue->head) {
        *packet = queue->items[queue->tail];
        queue->tail++;
        if (queue->tail >= queue->size)
            queue->tail = 0;
    } else {
        *packet = 0;
    }
    return 1;
}

int gstDispatchShutdown(void)
{
    int i;

    gstDefaultPacketHandler = 0;
    for (i = 0; i < 256; i++) {
        gstTagHandler[i] = 0;
        gstTagPollHandler[i] = 0;
        gstTagFlags[i] = 0;
    }
    gstQueueDestroy(&gstDownQueue);
    return gstReportError(1, 0x30001);
}

int gstDispatchSetDefault(GstPacketHandler handler)
{
    gstDefaultPacketHandler = handler;
    return gstReportError(1, 0x30002);
}

/* Sets the handler of every tag in the zero-terminated list. */
int gstDispatchSetHandler(GstPacketHandler handler, const unsigned char *tags)
{
    if (tags) {
        while (*tags) {
            gstTagHandler[*tags] = handler;
            if (gstTagHandler[*tags])
                gstTagFlags[*tags] |= GST_TAG_HANDLER;
            else
                gstTagFlags[*tags] &= ~GST_TAG_HANDLER;
            tags++;
        }
    }
    return gstReportError(1, 0x30003);
}

int gstDispatchSetPolled(char enable, const unsigned char *tags)
{
    if (tags) {
        while (*tags) {
            if (enable)
                gstTagFlags[*tags] |= GST_TAG_POLLED;
            else
                gstTagFlags[*tags] &= ~GST_TAG_POLLED;
            tags++;
        }
    }
    return gstReportError(1, 0x30004);
}

int gstDispatchSetPollHandler(GstPacketHandler handler, const unsigned char *tags)
{
    if (tags) {
        while (*tags) {
            gstTagPollHandler[*tags] = handler;
            if (gstTagPollHandler[*tags])
                gstTagFlags[*tags] |= GST_TAG_POLL_HANDLER;
            else
                gstTagFlags[*tags] &= ~GST_TAG_POLL_HANDLER;
            tags++;
        }
    }
    return gstReportError(1, 0x30005);
}

int gstDispatchSetEnabled(char enable, const unsigned char *tags)
{
    if (tags) {
        while (*tags) {
            if (enable)
                gstTagFlags[*tags] |= GST_TAG_ENABLED;
            else
                gstTagFlags[*tags] &= ~GST_TAG_ENABLED;
            tags++;
        }
    }
    return gstReportError(1, 0x30006);
}

/* Decides what happens to a packet: it is queued for polling (both handlers come back zero),
   or the handlers to call are returned. `unhandled` is set when nothing wants it. */
int gstDispatchLookup(GstCodec *packet, GstPacketHandler *first, GstPacketHandler *second, char *unhandled)
{
    unsigned char tag;
    int status;

    *unhandled = 0;
    if (packet == 0 || first == 0 || second == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0x30007);
    tag = packet->data[0];
    if (gstTagFlags[tag] & (GST_TAG_POLLED | GST_TAG_ENABLED)) {
        status = gstQueuePush(&gstDownQueue, packet);
        if (status < 1)
            return gstReportError(status, 0x30007);
        *first = 0;
        *second = 0;
    } else if (gstTagFlags[tag] & GST_TAG_POLL_HANDLER) {
        *first = gstTagPollHandler[tag];
        if (*first == 0)
            return gstReportError(0x80020000, 0x30007);
        if (gstTagFlags[tag] & GST_TAG_HANDLER) {
            *second = gstTagHandler[tag];
            if (*second == 0)
                return gstReportError(0x80020000, 0x30007);
        }
    } else if (gstTagFlags[tag] & GST_TAG_HANDLER) {
        *first = 0;
        *second = gstTagHandler[tag];
        if (*second == 0)
            return gstReportError(0x80020000, 0x30007);
    } else if (gstDefaultPacketHandler) {
        *first = 0;
        *second = gstDefaultPacketHandler;
        if (*second == 0)
            return gstReportError(0x80020000, 0x30007);
    } else {
        *first = 0;
        *second = 0;
        *unhandled = 1;
    }
    return gstReportError(1, 0x30007);
}

int gstQueuePush(GstPacketQueue *queue, GstCodec *packet)
{
    unsigned next;

    if (queue->tail == queue->head)
        gstQueueClear(queue);
    next = queue->head + 1;
    if (queue->size <= next)
        next = 0;
    if (queue->tail == next)
        return 0x8000000e;
    queue->items[queue->head] = packet;
    queue->head = next;
    return 1;
}

/* Takes the oldest polled packet off the queue (zero when there is none). */
int gstDispatchGetPolled(GstCodec **packet)
{
    int status;

    status = gstQueuePop(&gstDownQueue, packet);
    if (status < 1)
        return gstReportError(status, 0x30008);
    return gstReportError(1, 0x30008);
}
