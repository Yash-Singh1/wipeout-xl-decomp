#include <stdarg.h>
#include <stdio.h>
#include "gst.h"
#include "gstserver.h"
#include "gpi.h"

typedef struct GstQueueAcquireLocals {
    unsigned *readOffset;
    unsigned *writeOffset;
    unsigned nextIndex;
    unsigned nextOffset;
} GstQueueAcquireLocals;

void gstClearQueue(GstReliableQueue *queue);
void gstInitQueue(GstReliableQueue *queue);
void gstPeerReset(GstPeer *peer);
unsigned __stdcall gstPeerThread(void *parameter);
int gstPeerReceive(GstPeer *peer);
int gstPeerDeliver(GstPeer *peer, unsigned *unused1, unsigned *unused2);
int gstPeerResend(GstPeer *peer, unsigned index, unsigned now);
int gstPeerReleaseRecv(GstPeer *peer, GstCodec *packet);
int gstPeerReleaseSend(GstPeer *peer, GstCodec *packet);
int gstPeerAcknowledge(GstPeer *peer, unsigned long from, unsigned sequence);


/* Formats a message and hands it to the log callback, tagged with the peer it concerns. */
void gstPeerLog(GstPeer *peer, const char *format, ...)
{
    char text[240];
    va_list ap;

    if (gstDebugPrintCallback) {
        va_start(ap, format);
        vsprintf(text, format, ap);
        va_end(ap);
        if (peer == &gstPlayer)
            gstDebugPrintCallback("Player %d: %s", peer->id, text);
        else
            gstDebugPrintCallback("Server %d: %s", peer->id, text);
    }
}

int gstReleaseQueue(GstReliableQueue *queue)
{
    if (!queue)
        return GST_ERR_INVALID_PARAM;

    if (queue->entryOffsets) {
        gstFreeHandler(queue->entryOffsets);
        queue->entryOffsets = 0;
    }
    if (queue->buffer) {
        gstFreeHandler(queue->buffer);
        queue->buffer = 0;
    }
    gstClearQueue(queue);
    return 1;
}

void gstClearQueue(GstReliableQueue *queue)
{
    queue->writeIndex = 0;
    queue->readIndex = 0;
    queue->count = 0;
    queue->entryOffsets = 0;
    queue->bufferSize = 0;
    queue->buffer = 0;
}

int gstCreateQueue(GstReliableQueue *queue, unsigned count, unsigned bufferSize)
{
    if (!queue || count < 0x20 || bufferSize < 0x1000)
        return GST_ERR_INVALID_PARAM;

    bufferSize = (bufferSize + 3) & ~3;
    queue->entryOffsets = gstDefaultAlloc(count * sizeof(unsigned));
    if (!queue->entryOffsets)
        return 0x80000002;

    queue->count = count;
    queue->buffer = gstDefaultAlloc(bufferSize);
    if (!queue->buffer)
        return 0x80000002;

    queue->bufferSize = bufferSize;
    gstInitQueue(queue);
    return 1;
}

void gstInitQueue(GstReliableQueue *queue)
{
    queue->writeIndex = 0;
    queue->readIndex = 0;
    *queue->entryOffsets = 0;
}

#ifdef EQUIV
/* Reserves `size` bytes at the write end of the queue: returns the entry index and the buffer address.
   Proven equivalent, not byte-exact: the executable evaluates the index before the table base in the last store. */
int gstQueueAcquire(GstReliableQueue *queue, unsigned size, unsigned *index, unsigned char **buffer)
{
    unsigned *writeOffset;
    unsigned *oldest;
    unsigned nextIndex;
    unsigned endOffset;

    *buffer = 0;
    writeOffset = &queue->entryOffsets[queue->writeIndex];
    oldest = &queue->entryOffsets[queue->readIndex];
    if (queue->readIndex == queue->writeIndex) {
        gstInitQueue(queue);
        writeOffset = queue->entryOffsets;
        oldest = queue->entryOffsets;
        if (queue->bufferSize < size)
            return 0x8000000d;
    } else if (*oldest < *writeOffset) {
        if (queue->bufferSize - *writeOffset < size) {
            *writeOffset = 0;
            if (*oldest < size)
                return 0x8000000e;
        }
    } else {
        if (*oldest - *writeOffset < size)
            return 0x8000000e;
    }

    nextIndex = queue->writeIndex + 1;
    if (queue->count <= nextIndex)
        nextIndex = 0;
    if (queue->readIndex == nextIndex)
        return 0x8000000e;

    *index = queue->writeIndex;
    *buffer = queue->buffer + *writeOffset;
    endOffset = (*writeOffset + size + 3) & ~3;
    if (queue->bufferSize < endOffset)
        return 0x8000000e;
    else if (queue->bufferSize == endOffset)
        endOffset = 0;
    queue->writeIndex = nextIndex;
    queue->entryOffsets[nextIndex] = endOffset;
    return 1;
}
#endif

int gstPeerClose(GstPeer *peer)
{
    int result;
    unsigned index;

    if (!peer)
        return GST_ERR_INVALID_PARAM;

    if (peer->threadHandle) {
        if (peer->status >= 1)
            peer->status = 0x8f000001;
        if (peer->stopEvent)
            SetEvent(peer->stopEvent);
        if (WaitForSingleObject(peer->threadHandle, 4000) == WAIT_TIMEOUT) {
            TerminateThread(peer->threadHandle, 0);
            CloseHandle(peer->threadHandle);
            peer->threadHandle = 0;
            peer->threadId = 0;
        }
    }

    if (peer->stopEvent) {
        CloseHandle(peer->stopEvent);
        peer->stopEvent = 0;
    }
    if (peer->status >= 1)
        peer->status = 0x8f000001;

    if (peer->sendRecords) {
        for (index = 0; index < peer->sendQueue.count; index++) {
            if (peer->sendRecords[index].dests)
                gstFreeHandler(peer->sendRecords[index].dests);
        }
        gstFreeHandler(peer->sendRecords);
        peer->sendRecords = 0;
    }

    result = gstReleaseQueue(&peer->sendQueue);
    if (result < 1)
        return result;
    if (peer->sendEvent) {
        CloseHandle(peer->sendEvent);
        peer->sendEvent = 0;
    }
    if (peer->recvStates) {
        gstFreeHandler(peer->recvStates);
        peer->recvStates = 0;
    }
    result = gstReleaseQueue(&peer->receiveQueue);
    if (result < 1)
        return result;
    if (peer->slots) {
        gstFreeHandler(peer->slots);
        peer->slots = 0;
    }
    gstPeerReset(peer);
    return 1;
}

void gstPeerReset(GstPeer *peer)
{
    peer->slotCount = 0;
    peer->slots = 0;
    peer->id = 0;
    peer->threadHandle = 0;
    peer->threadId = 0;
    peer->stopEvent = 0;
    peer->onPrepare = 0;
    peer->onSystem = 0;
    peer->onPacket = 0;
    gstClearQueue(&peer->sendQueue);
    peer->sendRecords = 0;
    peer->userEvent = 0;
    peer->sendEvent = 0;
    gstClearQueue(&peer->receiveQueue);
    peer->recvStates = 0;
}

/* Sets up a peer: the send and receive queues, the per-destination records, the events, and the worker thread
   that resends unacknowledged packets; waits until the thread reports itself running. */
int gstPeerOpen(GstPeer *peer, GstServerConfig *config, unsigned long local, void *event)
{
    unsigned index;
    int status;
    unsigned startTick;

    if (!peer || !config || !event || !config->playerCount || config->sendSlots < 0x20 ||
        config->sendBytes < 0x1000 || !config->onPrepare || config->resendMs < 10 ||
        config->resendMs > 2000 || config->recvSlots < 0x20 || config->recvBytes < 0x1000 ||
        !config->onSystem || !config->onPacket)
        return GST_ERR_INVALID_PARAM;

    status = gstPeerClose(peer);
    if (status < 1)
        return status;

    peer->slotCount = config->playerCount;
    peer->slots = gstDefaultAlloc(config->playerCount * sizeof(GstPeerSlot));
    if (!peer->slots)
        return 0x80000002;
    memset(peer->slots, 0, config->playerCount * sizeof(GstPeerSlot));
    peer->id = local;
    peer->status = 0x8f000001;
    peer->onPrepare = config->onPrepare;
    peer->onSystem = config->onSystem;
    peer->onPacket = config->onPacket;

    status = gstCreateQueue(&peer->sendQueue, config->sendSlots, config->sendBytes);
    if (status < 1)
        return status;
    peer->sendRecords = gstDefaultAlloc(config->sendSlots * sizeof(GstSendRecord));
    if (!peer->sendRecords)
        return 0x80000002;
    memset(peer->sendRecords, 0, config->sendSlots * sizeof(GstSendRecord));
    for (index = 0; config->sendSlots > index; index++) {
        peer->sendRecords[index].dests = gstDefaultAlloc(config->playerCount * sizeof(GstSendDest));
        if (!peer->sendRecords[index].dests)
            return 0x80000002;
        memset(peer->sendRecords[index].dests, 0, config->playerCount * sizeof(GstSendDest));
    }
    peer->resendMs = config->resendMs;
    peer->userEvent = event;
    peer->sendEvent = CreateEventA(0, 1, 0, 0);
    if (!peer->sendEvent)
        return 0x80000003;

    status = gstCreateQueue(&peer->receiveQueue, config->recvSlots, config->recvBytes);
    if (status > 1)
        return status;
    peer->recvStates = gstDefaultAlloc(config->recvSlots * sizeof(unsigned));
    if (!peer->recvStates)
        return 0x80000002;
    memset(peer->recvStates, 0, config->recvSlots * sizeof(unsigned));

    peer->spareEvent = CreateEventA(0, 1, 0, 0);
    if (!peer->spareEvent)
        return 0x80000005;
    peer->stopEvent = CreateEventA(0, 1, 0, 0);
    if (!peer->stopEvent)
        return 0x80000005;
    peer->threadHandle = CreateThread(0, 0, gstPeerThread, peer, 0, &peer->threadId);
    if (!peer->threadHandle)
        return 0x80000005;

    startTick = gstClockHandler();
    while (peer->status != 1) {
        if (gstCancelCallback && gstCancelCallback())
            return 0x8000000b;
        if (gstClockHandler() - startTick >= 2000)
            return 0x8000000a;
    }
    return 1;
}

#ifdef EQUIV
/* Proven equivalent, not byte-exact: the order of the operands of the resend-interval comparison differs. */
/* The peer's worker thread: receives what DirectPlay has queued, delivers the packets that are in order, and
   resends the ones nobody has acknowledged within the resend interval. */
unsigned __stdcall gstPeerThread(void *parameter)
{
    unsigned *entryOffset;
    GstSendRecord *queued;
    unsigned index;
    unsigned lastIndex;
    unsigned num;
    unsigned ignored1;
    unsigned extra2;
    GstPeer *self;
    void *events[3];
    unsigned sleep;
    unsigned time;
    unsigned signal;
    unsigned lastTick;

    self = parameter;
    events[0] = self->stopEvent;
    events[1] = self->userEvent;
    events[2] = self->sendEvent;
    lastTick = gstClockHandler();
    sleep = self->resendMs;
    self->status = 1;
    ResetEvent(self->stopEvent);
    for (;;) {
        signal = WaitForMultipleObjects(3, events, 0, sleep);
        switch (signal) {
        case 0:
            goto closed;
        case 1:
            EnterCriticalSection(&g_gpiCriticalSection);
            self->status = gstPeerReceive(self);
            LeaveCriticalSection(&g_gpiCriticalSection);
            if (self->status < 1) {
                gstPeerLog(self, "Enqueue error %s", gstErrorString(self->status));
                goto closed;
            }
            if (WaitForSingleObject(events[2], 0) != 0)
                break;
        case 2:
            self->status = gstPeerDeliver(self, &ignored1, &extra2);
            if (self->status < 1) {
                gstPeerLog(self, "Dequeue error %s", gstErrorString(self->status));
                goto closed;
            }
            break;
        }
        time = gstClockHandler();
        if (time - lastTick >= self->resendMs) {
            EnterCriticalSection(&g_gpiCriticalSection);
            index = self->sendQueue.readIndex;
            queued = self->sendRecords + index;
            entryOffset = self->sendQueue.entryOffsets + index;
            lastIndex = self->sendQueue.writeIndex;
            num = self->sendQueue.count;
            while (lastIndex != index) {
                if (*entryOffset != 0xffffffff && queued->active &&
                    time - queued->sentTime >= self->resendMs) {
                    self->status = gstPeerResend(self, index, time);
                    if (self->status < 1) {
                        LeaveCriticalSection(&g_gpiCriticalSection);
                        gstPeerLog(self, "Resend error %s", gstErrorString(self->status));
                        goto closed;
                    }
                }
                index++;
                if (num <= index) {
                    index = 0;
                    queued = self->sendRecords;
                    entryOffset = self->sendQueue.entryOffsets;
                } else {
                    queued++;
                    entryOffset++;
                }
            }
            LeaveCriticalSection(&g_gpiCriticalSection);
            lastTick = time;
            sleep = self->resendMs;
        } else {
            sleep = self->resendMs - (time - lastTick);
        }
    }
closed:
    gstPeerLog(self, "Closed [%s]", gstErrorString(self->status));
    CloseHandle(self->threadHandle);
    self->threadHandle = 0;
    self->threadId = 0;
    return 0;
}
#endif

/* Takes the next message DirectPlay has for this peer: system messages go to the system handler, protocol
   commands and acknowledgements are handled here, and data packets are queued for delivery. */
int gstPeerReceive(GstPeer *peer)
{
    unsigned long packetSize;
    unsigned *queueIndex;
    unsigned short ackWord;
    unsigned short command;
    GstPeerSlot *slot;
    char delivered;
    GstCodec *buf;
    int err;
    unsigned long dpid;
    int rc;

    EnterCriticalSection(&gpiReceiveLock);
    packetSize = 0x800;
    rc = gpiInterface->vtable->receive(gpiInterface, &dpid, &peer->id, 2, 0, &packetSize);
    buf = 0;
    for (;;) {
        if (rc == 0x887700be) {
            ResetEvent(peer->userEvent);
            err = 1;
            break;
        } else if (rc != 0x8877001e && rc < 0) {
            err = 0x8f000006;
            break;
        } else {
            if (buf) {
                err = gstPeerReleaseRecv(peer, buf);
                if (err < 1)
                    break;
            }
            err = gstPeerAllocRecv(peer, packetSize, &queueIndex, &buf);
            if (err < 1) {
                gstPeerLog(peer, "Could not receive: %s", gstErrorString(err));
                break;
            }
            rc = gpiInterface->vtable->receive(gpiInterface, &dpid, &peer->id, 2, buf->data, &packetSize);
            if (rc >= 0) {
                err = 0x8fff000b;
                break;
            }
        }
    }
    LeaveCriticalSection(&gpiReceiveLock);
    if (err != 0x8fff000b) {
        if (buf) {
            if (err == 1)
                err = gstPeerReleaseRecv(peer, buf);
            else
                gstPeerReleaseRecv(peer, buf);
        }
        return err;
    }

    if (dpid == 0) {
        EnterCriticalSection(&g_gpiCriticalSection);
        err = peer->onSystem(dpid, (struct GstSysMessage *)buf->data);
        LeaveCriticalSection(&g_gpiCriticalSection);
        if (err < 1)
            return err;
        return gstPeerReleaseRecv(peer, buf);
    }

    command = *(unsigned short *)buf->data;
    buf->size -= 2;
    buf->data += 2;
    buf->maxSize -= 2;
    buf->pos = 0;
    if (command & 0x8000) {
        if (command == 0x8000) {
            gstPeerLog(peer, "RELCMD_NOHOST");
            gstPeerReleaseRecv(peer, buf);
            return 0x8f000002;
        }
        gstPeerLog(peer, "Unknown RELCMD %Xh from DPID %d", command, dpid);
        return gstPeerReleaseRecv(peer, buf);
    }

    slot = peer->slots;
    for (*queueIndex = 0; peer->slotCount > *queueIndex; (*queueIndex)++, slot++) {
        if (slot->dpid == dpid)
            break;
    }
    if (peer->slotCount <= *queueIndex || slot->recvSeq == 0) {
        gstPeerLog(peer, "Uninitialized: ID %Xh Type %d From %d", command, buf->data[0], dpid);
        return gstPeerReleaseRecv(peer, buf);
    }

    if (command & 0x4000) {
        err = gstPeerAcknowledge(peer, dpid, command & 0x3fff);
        if (err < 1)
            return gstPeerReleaseRecv(peer, buf);
        return gstPeerReleaseRecv(peer, buf);
    }

    if (command == 0) {
        EnterCriticalSection(&g_gpiCriticalSection);
        err = peer->onPacket(dpid, buf, &delivered);
        LeaveCriticalSection(&g_gpiCriticalSection);
        if (err < 1)
            return err;
        if (delivered)
            return gstPeerReleaseRecv(peer, buf);
        return 1;
    }

    ackWord = command | 0x4000;
    gpiInterface->vtable->send(gpiInterface, peer->id, dpid, 0, &ackWord, 2);
    if (slot->recvSeq == command)
        SetEvent(peer->sendEvent);
    else if (slot->recvSeq > command && slot->recvSeq - command < 0x1fff ||
             slot->recvSeq < command && command - slot->recvSeq > 0x1fff)
        return gstPeerReleaseRecv(peer, buf);
    return 1;
}

/* Reserves room for a received packet: the codec header sits first in the reserved block and the data starts
   0x10 bytes in; the packet is ready to be decoded. */
int gstPeerAllocRecv(GstPeer *peer, int size, unsigned **indexOut, GstCodec **packet)
{
    int status;
    unsigned index;

    EnterCriticalSection(&g_gpiCriticalSection);
    status = gstQueueAcquire(&peer->receiveQueue, size + 0x10,
                             &index, (unsigned char **)packet);
    LeaveCriticalSection(&g_gpiCriticalSection);
    if (status < 1)
        return gstReportError(status, 0x0f000023);

    (*packet)->data = (unsigned char *)*packet + 0x10;
    (*packet)->size = size;
    (*packet)->pos = 0;
    (*packet)->maxSize = size;
    *indexOut = peer->recvStates + index;
    **indexOut = 0xffffffff;
    return gstReportError(1, 0x0f000023);
}

int gstPeerReleaseRecv(GstPeer *peer, GstCodec *packet)
{
    struct {
        int result;
        unsigned index;
    } locals;

    if (!packet)
        return 1;

    EnterCriticalSection(&g_gpiCriticalSection);
    locals.result = gstQueueRelease(&peer->receiveQueue, (unsigned char *)packet, &locals.index);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return locals.result;
}

/* Circular packet removal is equivalent; the binary uses a different branch layout. */
int gstQueueRelease(GstReliableQueue *queue, unsigned char *packet, unsigned *index)
{
    unsigned *offset;
    unsigned candidate;

    *index = queue->readIndex;
    offset = &queue->entryOffsets[*index];
    while (queue->writeIndex != *index) {
        if (queue->buffer + *offset == packet) {
            *offset = 0xffffffff;
            if (queue->readIndex == *index) {
                candidate = *index;
                while (queue->writeIndex != candidate) {
                    if (*offset != 0xffffffff)
                        break;
                    queue->readIndex++;
                    candidate = queue->readIndex;
                    if (queue->count <= candidate) {
                        queue->readIndex = 0;
                        candidate = queue->readIndex;
                        offset = queue->entryOffsets;
                    } else {
                        offset++;
                    }
                }
            }
            return 1;
        }

        (*index)++;
        if (queue->count <= *index) {
            *index = 0;
            offset = queue->entryOffsets;
        } else {
            offset++;
        }
    }
    return 0x8fff0008;
}

#ifdef EQUIV
/* Proven equivalent, not byte-exact: the executable scales the receive-state index with a shift and a add. */
/* Hands the received packets to the owner in sequence order, one slot (remote player) at a time; a packet that
   arrives out of order waits, and one that is far behind is dropped. */
int gstPeerDeliver(GstPeer *peer, unsigned *unused1, unsigned *unused2)
{
    unsigned *entryOffset;
    unsigned *who;
    unsigned short *sequence;
    unsigned until;
    unsigned writePos;
    GstPeerSlot *pslot;
    char done;
    GstCodec *codec;
    int res;
    unsigned first;
    unsigned cursor;
    unsigned char *buf;
    unsigned entries;

    *unused1 = 0;
    *unused2 = 0;
    EnterCriticalSection(&g_gpiCriticalSection);
    buf = peer->receiveQueue.buffer;
    entries = peer->receiveQueue.count;
    writePos = peer->receiveQueue.writeIndex;
    first = peer->receiveQueue.readIndex;
    res = 1;
    if (first == writePos) {
        LeaveCriticalSection(&g_gpiCriticalSection);
        ResetEvent(peer->sendEvent);
        return 1;
    }
    cursor = first;
    entryOffset = peer->receiveQueue.entryOffsets + cursor;
    who = &peer->recvStates[cursor];
    until = first;
    do {
        if (*entryOffset != 0xffffffff && *who != 0xffffffff) {
            codec = (GstCodec *)(*entryOffset + buf);
            sequence = (unsigned short *)(codec->data - 2);
            pslot = peer->slots + *who;
            if (pslot->recvSeq == 0) {
                res = gstPeerReleaseRecv(peer, codec);
                if (res < 1)
                    break;
            }
            if (*sequence != 0xffff) {
                if (pslot->recvSeq == *sequence) {
                    *sequence = 0xffff;
                    res = peer->onPacket(pslot->dpid, codec, &done);
                    if (res < 1)
                        break;
                    if (done) {
                        res = gstPeerReleaseRecv(peer, codec);
                        if (res < 1)
                            break;
                    }
                    pslot->recvSeq++;
                    if (pslot->recvSeq > 0x3fff)
                        pslot->recvSeq = 1;
                    else {
                    }
                    until = cursor;
                } else if (pslot->recvSeq > *sequence && pslot->recvSeq - *sequence < 0x1fff ||
                           pslot->recvSeq < *sequence && *sequence - pslot->recvSeq > 0x1fff) {
                    res = gstPeerReleaseRecv(peer, codec);
                    if (res < 1)
                        break;
                }
            }
        }
        cursor++;
        if (cursor >= entries) {
            cursor = 0;
            entryOffset = peer->receiveQueue.entryOffsets;
            who = peer->recvStates;
        } else {
            entryOffset++;
            who++;
        }
        if (peer->receiveQueue.writeIndex == cursor) {
            cursor = first;
            entryOffset = peer->receiveQueue.entryOffsets + cursor;
            who = &peer->recvStates[cursor];
        }
    } while (until != cursor);
    LeaveCriticalSection(&g_gpiCriticalSection);
    ResetEvent(peer->sendEvent);
    return res;
}
#endif

/* Sends one packet again to every destination that has not acknowledged it; a packet nobody can be sent to is
   given up. */
int gstPeerResend(GstPeer *peer, unsigned index, unsigned now)
{
    GstSendRecord *queued;
    unsigned destIndex;
    unsigned sent;
    GstSendDest *target;
    GstCodec *packet;

    packet = (GstCodec *)(peer->sendQueue.entryOffsets[index] + peer->sendQueue.buffer);
    queued = peer->sendRecords + index;
    if (queued->pending == 0)
        return gstPeerReleaseSend(peer, packet);

    queued->sentTime = now;
    sent = 0;
    target = queued->dests;
    for (destIndex = 0; queued->destCount > destIndex; destIndex++, target++) {
        if (target->seq != 0xffff) {
            memcpy(packet->data, target, 2);
            gpiInterface->vtable->send(gpiInterface, peer->id, target->dpid, 0, packet->data, packet->size);
            sent++;
        }
    }
    if (sent == 0) {
        gstPeerLog(peer, "Packet from DPID %d Type %d not sent", peer->id, packet->data[2]);
        return gstPeerReleaseSend(peer, packet);
    }
    return 1;
}

int gstPeerReleaseSend(GstPeer *peer, GstCodec *packet)
{
    struct {
        int result;
        unsigned index;
    } locals;

    if (!packet)
        return 1;

    EnterCriticalSection(&g_gpiCriticalSection);
    locals.result = gstQueueRelease(&peer->sendQueue, (unsigned char *)packet, &locals.index);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return locals.result;
}

/* Acknowledges the send record (destination, sequence) of every pending packet, and releases the packets whose
   records are all acknowledged. */
int gstPeerAcknowledge(GstPeer *peer, unsigned long dpid, unsigned sequence)
{
    unsigned char *pkt;
    unsigned *entryOffset;
    GstSendRecord *queued;
    unsigned pos;
    unsigned recNo;
    char found;
    unsigned writeIndex;
    int res;
    GstSendDest *record;
    unsigned entries;

    EnterCriticalSection(&g_gpiCriticalSection);
    pos = peer->sendQueue.readIndex;
    queued = peer->sendRecords + pos;
    entryOffset = peer->sendQueue.entryOffsets + pos;
    writeIndex = peer->sendQueue.writeIndex;
    entries = peer->sendQueue.count;
    found = 0;
    res = 1;
    while (writeIndex != pos) {
        if (*entryOffset != 0xffffffff && queued->active) {
            if (queued->pending > 0) {
                record = queued->dests;
                for (recNo = 0; queued->destCount > recNo; recNo++, record++) {
                    if (record->dpid == dpid &&
                        record->seq == (unsigned short)sequence) {
                        record->dpid = 0;
                        record->seq = 0xffff;
                        found = 1;
                        queued->pending--;
                        if (queued->pending == 0)
                            break;
                    }
                }
            }

            if (queued->pending == 0) {
                pkt = peer->sendQueue.buffer + *entryOffset;
                res = gstPeerReleaseSend(peer, (GstCodec *)pkt);
                if (res < 1)
                    break;
            }
            if (found)
                break;
        }

        pos++;
        if (entries <= pos) {
            pos = 0;
            queued = peer->sendRecords;
            entryOffset = peer->sendQueue.entryOffsets;
        } else {
            queued++;
            entryOffset++;
        }
    }
    LeaveCriticalSection(&g_gpiCriticalSection);
    return res;
}

/* Reserves room for an outgoing packet in the send queue and starts its codec with a two-byte header. */
int gstPeerAllocSend(GstPeer *peer, int size, GstCodec **packet)
{
    int status;
    unsigned index;

    EnterCriticalSection(&g_gpiCriticalSection);
    status = gstQueueAcquire(&peer->sendQueue, size + 0x12,
                             &index, (unsigned char **)packet);
    LeaveCriticalSection(&g_gpiCriticalSection);
    if (status < 1)
        return gstReportError(status, 0x0f000024);

    peer->sendRecords[index].active = 0;
    (*packet)->data = (unsigned char *)*packet + 0x10;
    (*packet)->size = 2;
    (*packet)->pos = 2;
    (*packet)->maxSize = size + 2;
    return gstReportError(1, 0x0f000024);
}

#ifdef EQUIV
/* Proven equivalent, not byte-exact: the executable scales the table index with a shift and an add. */
/* Finds the queued packet in the send queue, lets the owner prepare it for sending (onPrepare), and sends it
   to the destinations it chose; an unreliable packet is released as soon as it has gone out. */
int gstPeerSend(char reliable, GstPeer *peer, GstCodec *packet, unsigned long target)
{
    unsigned *entryOffset;
    GstSendRecord *send;
    unsigned pos;
    unsigned lastIndex;
    GstPeerSlot *slots;
    char skipped;
    int res;
    unsigned count;
    unsigned char *queueBuffer;

    EnterCriticalSection(&g_gpiCriticalSection);
    slots = peer->slots;
    pos = peer->sendQueue.readIndex;
    send = peer->sendRecords + pos;
    entryOffset = peer->sendQueue.entryOffsets + pos;
    lastIndex = peer->sendQueue.writeIndex;
    count = peer->sendQueue.count;
    queueBuffer = peer->sendQueue.buffer;
    res = 0x8fff0008;
    while (lastIndex != pos) {
        if (*entryOffset != 0xffffffff && send->active == 0 && *entryOffset + queueBuffer == (unsigned char *)packet) {
            send->active = 1;
            res = peer->onPrepare(reliable, slots, send, target, &skipped);
            if (res < 1)
                break;
            if (skipped) {
                res = gstPeerReleaseSend(peer, packet);
            } else {
                res = gstPeerResend(peer, pos, gstClockHandler());
                if (res >= 1 && !reliable)
                    res = gstPeerReleaseSend(peer, packet);
            }
            break;
        }
        pos++;
        if (count <= pos) {
            pos = 0;
            send = peer->sendRecords;
            entryOffset = peer->sendQueue.entryOffsets;
        } else {
            send++;
            entryOffset++;
        }
    }
    LeaveCriticalSection(&g_gpiCriticalSection);
    if (res == 0x8fff0008)
        pos = 0;
    return res;
}
#endif

#ifdef EQUIV
/* Proven equivalent, not byte-exact: operand order and stack slots differ. */
/* Forgets a remote player: stops waiting for its acknowledgements (releasing the packets nobody else owes one
   for) and drops the packets received from it that have not been delivered. */
int gstPeerDropPlayer(GstPeer *peer, unsigned long dpid)
{
    unsigned *entryOffset;
    GstSendRecord *record;
    unsigned index;
    unsigned writeIndex;
    GstSendDest *target;
    unsigned destIndex;
    GstCodec *sendPacket;
    unsigned *recvState;
    GstCodec *recvPacket;
    int result;
    unsigned count;

    EnterCriticalSection(&g_gpiCriticalSection);
    index = peer->sendQueue.readIndex;
    record = peer->sendRecords + index;
    entryOffset = peer->sendQueue.entryOffsets + index;
    writeIndex = peer->sendQueue.writeIndex;
    count = peer->sendQueue.count;
    result = 1;
    while (writeIndex != index) {
        if (*entryOffset != 0xffffffff && record->active) {
            if (record->pending > 0) {
                target = record->dests;
                for (destIndex = 0; record->destCount > destIndex; destIndex++, target++) {
                    if (target->dpid == dpid) {
                        target->dpid = 0;
                        target->seq = 0xffff;
                        record->pending--;
                        if (record->pending == 0)
                            break;
                    }
                }
            }
            if (record->pending == 0) {
                sendPacket = (GstCodec *)(peer->sendQueue.buffer + *entryOffset);
                result = gstPeerReleaseSend(peer, sendPacket);
                if (result < 1)
                    break;
            }
        }
        index++;
        if (count <= index) {
            index = 0;
            record = peer->sendRecords;
            entryOffset = peer->sendQueue.entryOffsets;
        } else {
            record++;
            entryOffset++;
        }
    }

    index = peer->receiveQueue.readIndex;
    recvState = peer->recvStates + index;
    entryOffset = peer->receiveQueue.entryOffsets + index;
    writeIndex = peer->receiveQueue.writeIndex;
    count = peer->receiveQueue.count;
    result = 1;
    while (result >= 1 && writeIndex != index) {
        if (*entryOffset != 0xffffffff && *recvState != 0xffffffff && peer->slots[*recvState].dpid == dpid) {
            recvPacket = (GstCodec *)(peer->receiveQueue.buffer + *entryOffset);
            result = gstPeerReleaseRecv(peer, recvPacket);
            if (result < 1)
                break;
        }
        index++;
        if (count <= index) {
            index = 0;
            recvState = peer->recvStates;
            entryOffset = peer->receiveQueue.entryOffsets;
        } else {
            recvState++;
            entryOffset++;
        }
    }
    LeaveCriticalSection(&g_gpiCriticalSection);
    return result;
}
#endif
