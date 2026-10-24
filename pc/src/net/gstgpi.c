#include <string.h>
#include <ctype.h>
#include "gst.h"

/* The network (non-serial) implementation of the GPI calls: each one builds a request
   packet, sends it and waits up to four seconds for the matching reply. The packet tags
   are single characters ('q' title, 'A' session, 'B' session list, 'D' campaign, 'E' exit,
   's' disconnect, 'P' data). */


#ifdef EQUIV
int gstConnectNet(GstQueryFn query)
{
    unsigned long start;
    unsigned char type;
    char tags[4] = { 'q', 'A', 'D', 0 };
    int result;
    unsigned char subtype;
    GstCodec *down;
    GstCodec *up;
    GstCampaign *campaign;
    GstTitle *title;
    char gotId;
    unsigned char idSubtype;
    char gotTitle;

    result = gstDial(0);
    if (result < 1)
        return result;
    result = query(GST_QUERY_TITLE, &title);
    if (result < 1) {
        return gstReportError(result, 0x100001);
    } else if (title != 0) {
        result = query(GST_QUERY_CAMPAIGN, &campaign);
        if (result < 1) {
            return gstReportError(result, 0x100001);
        } else if (campaign == 0) {
            return gstReportError(0x80100001, 0x100001);
        }
        gstTitleId = 0;
        gstCampaignId = 0;
        gotTitle = 0;
        gotId = 0;
        result = gstAllocUpPacket(&up, 0, 0x300);
        if (result < 1)
            return result;
        if ((result = gstEncodeTitle(title, campaign, up)) < 1
            || (result = gpiDriverSlot40(1, tags)) < 1
            || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
            gstFreeUpPacket(up, 0);
            gpiDriverSlot40(0, tags);
            return result;
        }
        start = gstClockHandler();
        for (;;) {
            result = gstGetDownPacket(&down, 0);
            if (result < 1)
                break;
            if (down) {
                result = gstDecodeU8(down, &type);
                if (result < 1)
                    break;
                if (type == 'q') {
                    result = gstDecodeU8(down, &subtype);
                    if (result < 1)
                        break;
                    if (subtype != 1) {
                        result = 0x80100003;
                        break;
                    }
                    gotTitle = 1;
                } else if (type == 'A') {
                    result = gstDecodeU8(down, &idSubtype);
                    if (result < 1)
                        break;
                    if (idSubtype == 6) {
                        result = gstDecodePackedU24(down, &gstTitleId);
                        if (result < 1)
                            break;
                        if (gstTitleId == 0) {
                            result = 0x80100003;
                            break;
                        }
                        gotId = 1;
                    }
                }
                if (gotTitle && gotId)
                    break;
                result = gstFreeDownPacket(down);
                if (result < 1)
                    break;
                down = 0;
            }
            if (gstCancelCallback && gstCancelCallback()) {
                result = 0x8000000b;
                break;
            }
            if (gstClockHandler() - start >= 4000) {
                result = 0x8000000a;
                break;
            }
        }
        if (result < 1) {
            gstFreeDownPacket(down);
            gpiDriverSlot40(0, tags);
            return result;
        }
        result = gstFreeDownPacket(down);
        if (result < 1)
            return result;
        result = gstAllocUpPacket(&up, 0, 0x20);
        if (result < 1) {
            gpiDriverSlot40(0, tags);
            return result;
        }
        if ((result = gstEncodeCampaignRequest(up)) < 1
            || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
            gstFreeUpPacket(up, 0);
            gpiDriverSlot40(0, tags);
            return result;
        }
        start = gstClockHandler();
        for (;;) {
            result = gstGetDownPacket(&down, 0);
            if (result < 1)
                break;
            if (down) {
                result = gstDecodeU8(down, &type);
                if (result < 1)
                    break;
                if (type == 'D') {
                    result = gstDecodeU8(down, &idSubtype);
                    if (result < 1)
                        break;
                    if (idSubtype == 2) {
                        result = gstDecodePackedU24(down, &gstCampaignId);
                        if (result < 1)
                            break;
                        if (gstCampaignId == 0) {
                            result = 0x80100005;
                            break;
                        }
                        break;
                    }
                }
                result = gstFreeDownPacket(down);
                if (result < 1)
                    break;
                down = 0;
            }
            if (gstCancelCallback && gstCancelCallback()) {
                result = 0x8000000b;
                break;
            }
            if (gstClockHandler() - start >= 4000) {
                result = 0x8000000a;
                break;
            }
        }
        gpiDriverSlot40(0, tags);
        if (result < 1) {
            gstFreeDownPacket(down);
            return result;
        }
        result = gstFreeDownPacket(down);
        if (result < 1)
            return result;
    }
    return 1;
}
#endif

#ifdef NETPLAY
/* Packet 'q': the title and the score types of the campaign. */
int gstEncodeTitle(GstTitle *title, GstCampaign *campaign, GstCodec *codec)
{
    int result;
    int i;
    int j;

    result = gstEncodeU8(codec, 'q');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, strlen(title->name));
    if (result < 1)
        return result;
    result = gstEncodeBytes(codec, strlen(title->name), title->name);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, strlen(title->version));
    if (result < 1)
        return result;
    result = gstEncodeBytes(codec, strlen(title->version), title->version);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, strlen(title->description));
    if (result < 1)
        return result;
    result = gstEncodeBytes(codec, strlen(title->description), title->description);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 2);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 1);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, campaign->count);
    if (result < 1)
        return result;
    for (j = 0; j < campaign->count; j++) {
        result = gstEncodeU8(codec, strlen(campaign->scores[j].name));
        if (result < 1)
            return result;
        result = gstEncodeBytes(codec, strlen(campaign->scores[j].name), campaign->scores[j].name);
        if (result < 1)
            return result;
        result = gstEncodeS16(codec, campaign->scores[j].min);
        if (result < 1)
            return result;
        result = gstEncodeS16(codec, campaign->scores[j].max);
        if (result < 1)
            return result;
        result = gstEncodeU8(codec, campaign->scores[j].flags);
        if (result < 1)
            return result;
        result = gstEncodeU8(codec, campaign->scores[j].sort);
        if (result < 1)
            return result;
        result = gstEncodeU8(codec, campaign->scores[j].labelled);
        if (result < 1)
            return result;
        result = gstEncodeU8(codec, campaign->scores[j].display);
        if (result < 1)
            return result;
        result = gstEncodeU8(codec, campaign->scores[j].scope);
        if (result < 1)
            return result;
        if (campaign->scores[j].labelled) {
            for (i = 0; campaign->scores[j].max - campaign->scores[j].min >= i; i++) {
                result = gstEncodeU8(codec, strlen(campaign->scores[j].labels[i]));
                if (result < 1)
                    return result;
            }
            for (i = 0; campaign->scores[j].max - campaign->scores[j].min >= i; i++) {
                result = gstEncodeBytes(codec, strlen(campaign->scores[j].labels[i]), campaign->scores[j].labels[i]);
                if (result < 1)
                    return result;
            }
        }
    }
    result = gstEncodeU8(codec, 2);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 1);
    if (result < 1)
        return result;
    return 1;
}
#endif

/* Packet 'D': asks for the campaign id. */
int gstEncodeCampaignRequest(GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 'D');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 2);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, 0x81);
    if (result < 1)
        return result;
    return 1;
}

#ifdef EQUIV
int gstDisconnectNet(void)
{
    unsigned long start;
    unsigned char type;
    char tags[2] = { 's', 0 };
    int result;
    GstCodec *down;
    GstCodec *up;

    result = gstAllocUpPacket(&up, 0, 0x20);
    if (result < 1)
        return result;
    if ((result = gstEncodeDisconnect(up)) < 1
        || (result = gpiDriverSlot40(1, tags)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    start = gstClockHandler();
    for (;;) {
        result = gstGetDownPacket(&down, 0);
        if (result < 1)
            break;
        if (down) {
            result = gstDecodeU8(down, &type);
            if (result < 1)
                break;
            if (type == 's')
                break;
            result = gstFreeDownPacket(down);
            if (result < 1)
                break;
            down = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            result = 0x8000000b;
            break;
        }
        if (gstClockHandler() - start >= 4000) {
            result = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, tags);
    if (result < 1) {
        gstFreeDownPacket(down);
        return result;
    }
    result = gstFreeDownPacket(down);
    if (result < 1)
        return result;
    result = gstHangup();
    if (result < 1)
        return result;
    return gstReportError(1, 0x100002);
}
#endif

/* Packet 's': says goodbye. */
int gstEncodeDisconnect(GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 's');
    if (result < 1)
        return result;
    return 1;
}

int gstEnumSessionsNet(GstSessionEnumFn query)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    unsigned char lastNameBytes;
    char sessionName[100];
    unsigned char currentSubValue;
    unsigned char curDetail;
    unsigned char pendingPc;
    unsigned long lastIdEcho;
    unsigned short pendingValueEcho;
    unsigned char lastNumSessions;
    unsigned char localIndex;
    GstCodec *lastOut;
    unsigned char currentCh;
    GstCodec *latestIn;
    unsigned long nextId;
    int pendingResult;
    unsigned char latestDescLen;
    char currentTags[2] = { 'B', 0 };
    char curText[100];
    unsigned char newPacketType;
    unsigned long currentBegin;

    pendingResult = gstAllocUpPacket(&lastOut, 0, 0x20);
    if (pendingResult < 1)
        return pendingResult;
    if ((pendingResult = gstEncodeSessionQuery(2, 0x81, 3, 0x302, lastOut)) < 1
        || (pendingResult = gpiDriverSlot40(1, currentTags)) < 1
        || (pendingResult = gstSendEncodedUpPacket(lastOut, 0)) < 1) {
        gstFreeUpPacket(lastOut, 0);
        gpiDriverSlot40(0, currentTags);
        return pendingResult;
    }
    currentBegin = gstClockHandler();
    for (;;) {
        pendingResult = gstGetDownPacket(&latestIn, 0);
        if (pendingResult < 1)
            break;
        if (latestIn) {
            pendingResult = gstDecodeU8(latestIn, &newPacketType);
            if (pendingResult < 1)
                break;
            if (newPacketType == 'B') {
                pendingResult = gstDecodeU8(latestIn, &curDetail);
                if (pendingResult < 1)
                    break;
                if (curDetail == 2) {
                    pendingResult = gstDecodePackedU24(latestIn, &lastIdEcho);
                    if (pendingResult < 1)
                        break;
                    if (lastIdEcho != 0x81) {
                        pendingResult = 0x80100006;
                        break;
                    }
                    pendingResult = gstDecodeU8(latestIn, &currentSubValue);
                    if (pendingResult < 1)
                        break;
                    if (currentSubValue != 3) {
                        pendingResult = 0x80100006;
                        break;
                    }
                    pendingResult = gstDecodeU16(latestIn, &pendingValueEcho);
                    if (pendingResult < 1)
                        break;
                    if (pendingValueEcho != 0x302) {
                        pendingResult = 0x80100006;
                        break;
                    }
                    break;
                }
            }
            pendingResult = gstFreeDownPacket(latestIn);
            if (pendingResult < 1)
                break;
            latestIn = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            pendingResult = 0x8000000b;
            break;
        }
        if (gstClockHandler() - currentBegin >= 4000) {
            pendingResult = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, currentTags);
    if (pendingResult < 1) {
        gstFreeDownPacket(latestIn);
        return pendingResult;
    }
    pendingResult = gstDecodeU8(latestIn, &lastNumSessions);
    if (pendingResult < 1) {
        gstFreeDownPacket(latestIn);
        return gstReportError(0x80100006, 0x100004);
    }
    for (localIndex = 0; localIndex < lastNumSessions; localIndex++) {
        pendingResult = gstDecodePackedU24(latestIn, &nextId);
        if (pendingResult < 1)
            break;
        pendingResult = gstDecodeU8(latestIn, 0);
        if (pendingResult < 1)
            break;
        pendingResult = gstDecodeU8(latestIn, &lastNameBytes);
        if (pendingResult < 1)
            break;
        if (lastNameBytes > 99) {
            pendingResult = gstDecodeBytes(latestIn, 99, sessionName);
            if (pendingResult < 1)
                break;
            pendingResult = gstDecodeBytes(latestIn, lastNameBytes - 99, 0);
            if (pendingResult < 1)
                break;
            lastNameBytes = 99;
        } else {
            pendingResult = gstDecodeBytes(latestIn, lastNameBytes, sessionName);
            if (pendingResult < 1)
                break;
        }
        sessionName[lastNameBytes] = 0;
        for (currentCh = 0; currentCh < lastNameBytes; currentCh++)
            sessionName[currentCh] = toupper(sessionName[currentCh]);
        pendingResult = gstDecodeU8(latestIn, &latestDescLen);
        if (pendingResult < 1)
            break;
        if (latestDescLen > 99) {
            pendingResult = gstDecodeBytes(latestIn, 99, curText);
            if (pendingResult < 1)
                break;
            pendingResult = gstDecodeBytes(latestIn, latestDescLen - 99, 0);
            if (pendingResult < 1)
                break;
            latestDescLen = 99;
        } else {
            pendingResult = gstDecodeBytes(latestIn, latestDescLen, curText);
            if (pendingResult < 1)
                break;
        }
        curText[latestDescLen] = 0;
        for (currentCh = 0; currentCh < latestDescLen; currentCh++)
            curText[currentCh] = toupper(curText[currentCh]);
        pendingResult = gstDecodeU8(latestIn, &pendingPc);
        if (pendingResult < 1)
            break;
        pendingResult = query(nextId, sessionName, curText, pendingPc, pendingPc);
        if (pendingResult < 1)
            break;
    }
    if (pendingResult < 1) {
        gstFreeDownPacket(latestIn);
        return pendingResult;
    }
    return gstFreeDownPacket(latestIn);
}

/* Packet 'B': asks for the list of sessions matching two criteria. */
int gstEncodeSessionQuery(unsigned char kind1, unsigned long value1, unsigned char kind2, unsigned short value2, GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 'B');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, kind1);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, value1);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, kind2);
    if (result < 1)
        return result;
    result = gstEncodeU16(codec, value2);
    if (result < 1)
        return result;
    return 1;
}

#ifdef EQUIV
int gstCreateSessionNet(GstQueryFn query)
{
    unsigned long start;
    GstSession *session;
    unsigned char type;
    char tags[2] = { 'A', 0 };
    int result;
    unsigned long *out;
    GstCodec *down;
    GstCodec *up;
    unsigned long id;
    unsigned char subtype;

    result = query(GST_QUERY_SESSION_RESULT, &out);
    if (result < 1) {
        return gstReportError(result, 0x100005);
    } else if (out == 0) {
        return gstReportError(GST_ERR_INVALID_PARAM, 0x100005);
    }
    result = query(GST_QUERY_SESSION, &session);
    if (result < 1) {
        return gstReportError(result, 0x100005);
    } else if (session == 0) {
        return gstReportError(0x80100001, 0x100005);
    }
    result = gstAllocUpPacket(&up, 0, 0x400);
    if (result < 1)
        return result;
    if ((result = gstEncodeSessionCreate(session, up)) < 1
        || (result = gpiDriverSlot40(1, tags)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    start = gstClockHandler();
    for (;;) {
        result = gstGetDownPacket(&down, 0);
        if (result < 1)
            break;
        if (down) {
            result = gstDecodeU8(down, &type);
            if (result < 1)
                break;
            if (type == 'A') {
                result = gstDecodeU8(down, &subtype);
                if (result < 1)
                    break;
                if (subtype == 3) {
                    result = gstDecodePackedU24(down, &id);
                    if (result < 1)
                        break;
                    if (id == 0) {
                        result = 0x80100007;
                        break;
                    }
                    break;
                }
            }
            result = gstFreeDownPacket(down);
            if (result < 1)
                break;
            down = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            result = 0x8000000b;
            break;
        }
        if (gstClockHandler() - start >= 4000) {
            result = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, tags);
    if (result < 1) {
        gstFreeDownPacket(down);
        return result;
    }
    result = gstFreeDownPacket(down);
    if (result < 1)
        return result;
    gstGpiSessionId = id;
    *out = id;
    return gstJoinSessionNet(query);
}
#endif

/* Packet 'A' sub 3: creates a session. */
int gstEncodeSessionCreate(GstSession *session, GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 'A');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 3);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 0);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, strlen(session->name));
    if (result < 1)
        return result;
    result = gstEncodePackedU32(codec, strlen(session->description));
    if (result < 1)
        return result;
    result = gstEncodePackedU32(codec, 0);
    if (result < 1)
        return result;
    if (session->name) {
        result = gstEncodeBytes(codec, strlen(session->name), session->name);
        if (result < 1)
            return result;
    }
    if (session->description) {
        result = gstEncodeBytes(codec, strlen(session->description), session->description);
        if (result < 1)
            return result;
    }
    result = gstEncodePackedU32(codec, session->dataSize);
    if (result < 1)
        return result;
    if (session->data) {
        result = gstEncodeBytes(codec, session->dataSize, session->data);
        if (result < 1)
            return result;
    }
    return 1;
}

#ifdef EQUIV
int gstJoinSessionNet(GstQueryFn query)
{
    unsigned long start;
    GstJoinInfo *info;
    unsigned char type;
    unsigned long id;
    char tags[2] = { 'D', 0 };
    int result;
    GstCodec *down;
    GstCodec *up;
    unsigned char subtype;

    result = query(GST_QUERY_TITLE, &id);
    if (result < 1) {
        return gstReportError(result, 0x100006);
    }
    result = query(GST_QUERY_CAMPAIGN, &info);
    if (result < 1) {
        return gstReportError(result, 0x100006);
    } else if (info == 0) {
        return gstReportError(0x80100001, 0x100006);
    }
    result = gstAllocUpPacket(&up, 0, 0x80);
    if (result < 1)
        return result;
    if ((result = gstEncodeSessionJoin(id, info, up)) < 1
        || (result = gpiDriverSlot40(1, tags)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    start = gstClockHandler();
    for (;;) {
        result = gstGetDownPacket(&down, 0);
        if (result < 1)
            break;
        if (down) {
            result = gstDecodeU8(down, &type);
            if (result < 1)
                break;
            if (type == 'D') {
                result = gstDecodeU8(down, &subtype);
                if (result < 1)
                    break;
                if (subtype == 3) {
                    result = gstDecodePackedU24(down, &gstGpiSessionId);
                    if (result < 1)
                        break;
                    if (gstGpiSessionId != id) {
                        result = 0x80100007;
                        break;
                    }
                    break;
                }
            }
            result = gstFreeDownPacket(down);
            if (result < 1)
                break;
            down = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            result = 0x8000000b;
            break;
        }
        if (gstClockHandler() - start >= 4000) {
            result = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, tags);
    if (result < 1) {
        gstFreeDownPacket(down);
        return result;
    }
    result = gstFreeDownPacket(down);
    if (result < 1)
        return result;
    return 1;
}
#endif

/* Packet 'D' sub 3: joins a session. */
int gstEncodeSessionJoin(unsigned long id, GstJoinInfo *info, GstCodec *codec)
{
    int result;
    int i;

    result = gstEncodeU8(codec, 'D');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 3);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, id);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, info->count);
    if (result < 1)
        return result;
    result = gstEncodeS16(codec, info->flags);
    if (result < 1)
        return result;
    for (i = 0; i < info->count; i++) {
        result = gstEncodeS16(codec, info->values[i]);
        if (result < 1)
            return result;
    }
    return 1;
}

/* Sends a data packet ('P'); a `type` of 0 goes through the GPI send call. */
int gstNewUpPacketNet(int port, char type, unsigned long destination, unsigned size, void *data)
{
    int result;
    GstCodec *up;

    result = gstAllocUpPacket(&up, port, size + 0x20);
    if (result < 1)
        return result;
    if ((result = gstEncodeU8(up, 'P')) < 1
        || (result = gstEncodeU8(up, 3)) < 1
        || (result = gstEncodePackedU24(up, destination)) < 1
        || (result = gstEncodePackedU32(up, size)) < 1
        || (result = gstEncodeBytes(up, size, data)) < 1) {
        gstFreeUpPacket(up, port);
        return result;
    }
    if (type == 0)
        result = gstSendUpPacketGPI(up, port);
    else
        result = gstSendEncodedUpPacket(up, port);
    if (result < 1) {
        gstFreeUpPacket(up, port);
        return result;
    }
    return 1;
}

#ifdef EQUIV
int gstExitSessionNet(void)
{
    unsigned long start;
    unsigned char type;
    char tags[2] = { 'E', 0 };
    int result;
    GstCodec *down;
    GstCodec *up;
    unsigned char subtype;

    result = gstAllocUpPacket(&up, 0, 0x20);
    if (result < 1)
        return result;
    if ((result = gstEncodeSessionExit(3, gstGpiSessionId, up)) < 1
        || (result = gpiDriverSlot40(1, tags)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    start = gstClockHandler();
    for (;;) {
        result = gstGetDownPacket(&down, 0);
        if (result < 1)
            break;
        if (down) {
            result = gstDecodeU8(down, &type);
            if (result < 1)
                break;
            if (type == 'E') {
                result = gstDecodeU8(down, &subtype);
                if (result < 1)
                    break;
                if (subtype == 3)
                    break;
            }
            result = gstFreeDownPacket(down);
            if (result < 1)
                break;
            down = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            result = 0x8000000b;
            break;
        }
        if (gstClockHandler() - start >= 4000) {
            result = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, tags);
    if (result < 1) {
        gstFreeDownPacket(down);
        return result;
    }
    result = gstFreeDownPacket(down);
    if (result < 1)
        return result;
    return 1;
}
#endif

/* Packet 'E': leaves a session. */
int gstEncodeSessionExit(unsigned char kind, unsigned long id, GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 'E');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, kind);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, id);
    if (result < 1)
        return result;
    return 1;
}

/* Lists the lockstep games of a session; `callback` is called once per game. */
int gstEnumLockstepNet(unsigned long sessionId, unsigned char *kinds, GstLockstepEnumFn callback, unsigned timeout)
{
    /* Local names chosen to reproduce the exe's frame layout. */
    unsigned char lastNameBytes;
    unsigned char curGameSubtype;
    char gameName[256];
    unsigned char currentSubValue;
    unsigned char currentCategory;
    char points2;
    unsigned char latestN;
    unsigned long localAck;
    unsigned short newEchoedValue;
    unsigned char lastEntry;
    unsigned long lastGstid;
    unsigned char localKinds2;
    GstCodec *tx;
    GstCodec *nextRecv;
    unsigned long outAnswerId;
    unsigned char okSubtype;
    int lastResult;
    unsigned char descBytes;
    char outTags[2] = { 'B', 0 };
    char nextDescText[256];
    unsigned char curCode;
    unsigned char *pendingKindArray;
    unsigned short *newRaceScores;
    unsigned long newT0;
    unsigned char outPos;

    if (callback == 0)
        return GST_ERR_INVALID_PARAM;
    if (kinds) {
        pendingKindArray = kinds;
        localKinds2 = 0;
        while (*pendingKindArray) {
            pendingKindArray++;
            localKinds2++;
        }
    } else
        localKinds2 = 0;
    lastResult = gstAllocUpPacket(&tx, 0, 0x20);
    if (lastResult < 1)
        return lastResult;
    if ((lastResult = gstEncodeSessionQuery(5, 0x84, 6, 0x181, tx)) < 1
        || (lastResult = gstEncodeQueryTerm(3, sessionId, 1, tx)) < 1
        || (lastResult = gstEncodeByteList(localKinds2, kinds, tx)) < 1
        || (lastResult = gpiDriverSlot40(1, outTags)) < 1
        || (lastResult = gstSendEncodedUpPacket(tx, 0)) < 1) {
        gstFreeUpPacket(tx, 0);
        gpiDriverSlot40(0, outTags);
        return lastResult;
    }
    newT0 = gstClockHandler();
    for (;;) {
        lastResult = gstGetDownPacket(&nextRecv, 0);
        if (lastResult < 1)
            break;
        if (nextRecv) {
            lastResult = gstDecodeU8(nextRecv, &curCode);
            if (lastResult < 1)
                break;
            if (curCode == 'B') {
                lastResult = gstDecodeU8(nextRecv, &currentCategory);
                if (lastResult < 1)
                    break;
                if (currentCategory == 5) {
                    lastResult = gstDecodePackedU24(nextRecv, &localAck);
                    if (lastResult < 1)
                        break;
                    if (localAck != 0x84) {
                        lastResult = 0x80100006;
                        break;
                    }
                    lastResult = gstDecodeU8(nextRecv, &currentSubValue);
                    if (lastResult < 1)
                        break;
                    if (currentSubValue != 6) {
                        lastResult = 0x80100006;
                        break;
                    }
                    lastResult = gstDecodeU16(nextRecv, &newEchoedValue);
                    if (lastResult < 1)
                        break;
                    if (newEchoedValue != 0x181) {
                        lastResult = 0x80100006;
                        break;
                    }
                    lastResult = gstDecodeU8(nextRecv, &curGameSubtype);
                    if (lastResult < 1)
                        break;
                    if (curGameSubtype != 3) {
                        lastResult = 0x80100006;
                        break;
                    }
                    lastResult = gstDecodePackedU24(nextRecv, &outAnswerId);
                    if (lastResult < 1)
                        break;
                    lastResult = gstDecodeU8(nextRecv, &okSubtype);
                    if (lastResult < 1)
                        break;
                    if (okSubtype != 1) {
                        lastResult = 0x80100006;
                        break;
                    }
                    break;
                }
            }
            lastResult = gstFreeDownPacket(nextRecv);
            if (lastResult < 1)
                break;
            nextRecv = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            lastResult = 0x8000000b;
            break;
        }
        if (gstClockHandler() - newT0 >= timeout) {
            lastResult = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, outTags);
    if (lastResult < 1) {
        gstFreeDownPacket(nextRecv);
        return lastResult;
    }
    lastResult = gstDecodeU8(nextRecv, &localKinds2);
    if (lastResult < 1) {
        gstFreeDownPacket(nextRecv);
        return gstReportError(0x80100006, 0xffff);
    }
    if (localKinds2 > 0) {
        pendingKindArray = gstAllocHandler(localKinds2);
        if (pendingKindArray == 0) {
            gstFreeDownPacket(nextRecv);
            return gstReportError(0x80000002, 0xffff);
        }
        newRaceScores = gstAllocHandler(localKinds2 * 2);
        if (newRaceScores == 0) {
            gstFreeHandler(pendingKindArray);
            gstFreeDownPacket(nextRecv);
            return gstReportError(0x80000002, 0xffff);
        }
        for (lastEntry = 0; lastEntry < localKinds2; lastEntry++) {
            lastResult = gstDecodeU8(nextRecv, pendingKindArray + lastEntry);
            if (lastResult < 1) {
                gstFreeHandler(pendingKindArray);
                gstFreeHandler(newRaceScores);
                gstFreeDownPacket(nextRecv);
                return gstReportError(0x80100006, 0xffff);
            }
        }
    } else {
        pendingKindArray = 0;
        newRaceScores = 0;
    }
    lastResult = gstDecodeU8(nextRecv, &latestN);
    if (lastResult < 1) {
        if (localKinds2 > 0) {
            gstFreeHandler(pendingKindArray);
            gstFreeHandler(newRaceScores);
        }
        gstFreeDownPacket(nextRecv);
        return gstReportError(0x80100006, 0xffff);
    }
    for (lastEntry = 0; lastEntry < latestN; lastEntry++) {
        lastResult = gstDecodePackedU24(nextRecv, &lastGstid);
        if (lastResult < 1)
            break;
        lastResult = gstDecodeU8(nextRecv, 0);
        if (lastResult < 1)
            break;
        lastResult = gstDecodeU8(nextRecv, &lastNameBytes);
        if (lastResult < 1)
            break;
        lastResult = gstDecodeBytes(nextRecv, lastNameBytes, gameName);
        if (lastResult < 1)
            break;
        gameName[lastNameBytes] = 0;
        lastResult = gstDecodeU8(nextRecv, &descBytes);
        if (lastResult < 1)
            break;
        lastResult = gstDecodeBytes(nextRecv, descBytes, nextDescText);
        if (lastResult < 1)
            break;
        nextDescText[descBytes] = 0;
        lastResult = gstDecodeU8(nextRecv, &points2);
        if (lastResult < 1)
            break;
        if (points2) {
            for (outPos = 0; outPos < localKinds2; outPos++) {
                lastResult = gstDecodeS16(nextRecv, newRaceScores + outPos);
                if (lastResult < 1)
                    break;
            }
            if (lastResult < 1)
                break;
        }
        lastResult = callback(lastGstid, gameName, nextDescText, points2 ? localKinds2 : 0, pendingKindArray, newRaceScores);
        if (lastResult < 1)
            break;
    }
    if (localKinds2 > 0) {
        gstFreeHandler(pendingKindArray);
        gstFreeHandler(newRaceScores);
    }
    if (lastResult < 1) {
        gstFreeDownPacket(nextRecv);
        return lastResult;
    }
    return gstFreeDownPacket(nextRecv);
}

int gstEncodeQueryTerm(unsigned char kind, unsigned long id, unsigned char flag, GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, kind);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, id);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, flag);
    if (result < 1)
        return result;
    return 1;
}

#ifdef EQUIV
/* A length-prefixed list of bytes. */
int gstEncodeByteList(unsigned char count, const unsigned char *bytes, GstCodec *codec)
{
    unsigned char i;
    int result;

    result = gstEncodeU8(codec, count);
    if (result < 1)
        return result;
    for (i = 0; i < count; i++) {
        result = gstEncodeU8(codec, bytes[i]);
        if (result < 1)
            return result;
    }
    return 1;
}
#endif

#ifdef EQUIV
int gstJoinLockstepNet(GstQueryFn query)
{
    unsigned long start;
    unsigned char type;
    char tags[3] = { 'C', 'D', 0 };
    int result;
    unsigned long echoId;
    GstCodec *down;
    GstCodec *up;
    unsigned char subtype;
    int lockstepId;

    result = query(GST_QUERY_TITLE, &lockstepId);
    if (result < 1) {
        return gstReportError(result, 0x100008);
    }
    result = gstAllocUpPacket(&up, 0, 0x20);
    if (result < 1)
        return result;
    if ((result = gstEncodeLockstepJoin(lockstepId, up)) < 1
        || (result = gpiDriverSlot40(1, tags)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    start = gstClockHandler();
    for (;;) {
        result = gstGetDownPacket(&down, 0);
        if (result < 1)
            break;
        if (down) {
            result = gstDecodeU8(down, &type);
            if (result < 1)
                break;
            if (type == 'C') {
                result = gstDecodeU8(down, &subtype);
                if (result < 1)
                    break;
                if (subtype == 5) {
                    result = gstDecodePackedU24(down, &echoId);
                    if (result < 1)
                        break;
                    if (echoId != 0x84) {
                        result = 0x80100008;
                        break;
                    }
                    break;
                }
            }
            result = gstFreeDownPacket(down);
            if (result < 1)
                break;
            down = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            result = 0x8000000b;
            break;
        }
        if (gstClockHandler() - start >= 4000) {
            result = 0x8000000a;
            break;
        }
    }
    if (result < 1) {
        gstFreeDownPacket(down);
        gpiDriverSlot40(0, tags);
        return result;
    }
    result = gstFreeDownPacket(down);
    if (result < 1)
        return result;
    result = gstAllocUpPacket(&up, 0, 0x20);
    if (result < 1) {
        gpiDriverSlot40(0, tags);
        return result;
    }
    if ((result = gstEncodeLockstepJoinAck(lockstepId, up)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    start = gstClockHandler();
    for (;;) {
        result = gstGetDownPacket(&down, 0);
        if (result < 1)
            break;
        if (down) {
            result = gstDecodeU8(down, &type);
            if (result < 1)
                break;
            if (type == 'D') {
                result = gstDecodeU8(down, &subtype);
                if (result < 1)
                    break;
                if (subtype == 5)
                    break;
            }
            result = gstFreeDownPacket(down);
            if (result < 1)
                break;
            down = 0;
        }
        if (gstCancelCallback && gstCancelCallback()) {
            result = 0x8000000b;
            break;
        }
        if (gstClockHandler() - start >= 4000) {
            result = 0x8000000a;
            break;
        }
    }
    gpiDriverSlot40(0, tags);
    if (result < 1) {
        gstFreeDownPacket(down);
        return result;
    }
    result = gstFreeDownPacket(down);
    if (result < 1)
        return result;
    return 1;
}
#endif

/* Packet 'C' sub 5: joins the lockstep game. */
int gstEncodeLockstepJoin(int lockstepId, GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 'C');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 5);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, 0x84);
    if (result < 1)
        return result;
    result = gstEncodeU16(codec, 1);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, lockstepId);
    if (result < 1)
        return result;
    return 1;
}

/* Packet 'D' sub 5. */
int gstEncodeLockstepJoinAck(int lockstepId, GstCodec *codec)
{
    int result;

    result = gstEncodeU8(codec, 'D');
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 5);
    if (result < 1)
        return result;
    result = gstEncodePackedU24(codec, 0x84);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, 0);
    if (result < 1)
        return result;
    result = gstEncodeU8(codec, lockstepId);
    if (result < 1)
        return result;
    result = gstEncodePackedU32(codec, 0);
    if (result < 1)
        return result;
    return 1;
}

/* Sends a lockstep event packet ('i'); a `type` of 0 goes through the GPI send call. */
int gstNewEventUpPacketNet(int port, char type, int event, unsigned size, void *data)
{
    int result;
    GstCodec *up;

    result = gstAllocUpPacket(&up, port, size + 0x10);
    if (result < 1)
        return result;
    if ((result = gstEncodeU8(up, 'i')) < 1
        || (result = gstEncodeU8(up, event)) < 1
        || (result = gstEncodeBytes(up, size, data)) < 1) {
        gstFreeUpPacket(up, port);
        return result;
    }
    if (type == 0)
        result = gstSendUpPacketGPI(up, port);
    else
        result = gstSendEncodedUpPacket(up, port);
    if (result < 1) {
        gstFreeUpPacket(up, port);
        return result;
    }
    return 1;
}

/* Sends lockstep data ('P' for the lockstep game). */
int gstNewLockstepUpPacketNet(int port, char type, unsigned size, void *data)
{
    int result;
    GstCodec *up;

    result = gstAllocUpPacket(&up, port, size + 0x20);
    if (result < 1)
        return result;
    if ((result = gstEncodeU8(up, 'P')) < 1
        || (result = gstEncodeU8(up, 5)) < 1
        || (result = gstEncodePackedU24(up, 0x84)) < 1
        || (result = gstEncodePackedU32(up, size)) < 1
        || (result = gstEncodeBytes(up, size, data)) < 1) {
        gstFreeUpPacket(up, port);
        return result;
    }
    if (type == 0)
        result = gstSendUpPacketGPI(up, port);
    else
        result = gstSendEncodedUpPacket(up, port);
    if (result < 1) {
        gstFreeUpPacket(up, port);
        return result;
    }
    return 1;
}

#ifdef EQUIV
int gstExitLockstepNet(void)
{
    char tags[2] = { 'E', 0 };
    int result;
    GstCodec *up;

    result = gstAllocUpPacket(&up, 0, 0x20);
    if (result < 1)
        return result;
    if ((result = gstEncodeSessionExit(5, 0x84, up)) < 1
        || (result = gpiDriverSlot40(1, tags)) < 1
        || (result = gstSendEncodedUpPacket(up, 0)) < 1) {
        gstFreeUpPacket(up, 0);
        gpiDriverSlot40(0, tags);
        return result;
    }
    gpiDriverSlot40(0, tags);
    return 1;
}
#endif

#ifdef NETPLAY
/* Sends a chat line ('C' sub 6) to the lockstep game. */
int gstSendChatNet(int port, int unused, unsigned long sender, const char *text)
{
    char tags[2] = { 'C', 0 };
    int sendResult;
    GstCodec *up;

    sendResult = gstAllocUpPacket(&up, port, 0x110);
    if (sendResult < 1)
        return sendResult;
    if ((sendResult = gstEncodeU8(up, 'C')) < 1
        || (sendResult = gstEncodeU8(up, 6)) < 1
        || (sendResult = gstEncodePackedU24(up, sender)) < 1
        || (sendResult = gstEncodeU16(up, 0x100)) < 1
        || (sendResult = gstEncodeU8(up, strlen(text))) < 1
        || (sendResult = gstEncodeBytes(up, (unsigned char)strlen(text), text)) < 1) {
        gstFreeUpPacket(up, port);
        return sendResult;
    }
    sendResult = gstSendUpPacketGPI(up, port);
    if (sendResult < 1) {
        gstFreeUpPacket(up, port);
        return sendResult;
    }
    return 1;
    return 1;
}
#endif

#ifdef NETPLAY
/* Sends a player command ('C' sub 6, kind 0x80) to the lockstep game. */
int gstSendCommandNet(int port, int unused, unsigned long target, unsigned long sender, int command, short argument)
{
    char tags[2] = { 'C', 0 };
    int sendResult;
    GstCodec *up;

    sendResult = gstAllocUpPacket(&up, port, 0x110);
    if (sendResult < 1)
        return sendResult;
    if ((sendResult = gstEncodeU8(up, 'C')) < 1
        || (sendResult = gstEncodeU8(up, 6)) < 1
        || (sendResult = gstEncodePackedU24(up, sender)) < 1
        || (sendResult = gstEncodeU16(up, 0x80)) < 1
        || (sendResult = gstEncodeU8(up, 3)) < 1
        || (sendResult = gstEncodePackedU24(up, target)) < 1
        || (sendResult = gstEncodeU8(up, 1)) < 1
        || (sendResult = gstEncodeU8(up, 1)) < 1
        || (sendResult = gstEncodeU8(up, command)) < 1
        || (sendResult = gstEncodeS16(up, argument)) < 1) {
        gstFreeUpPacket(up, port);
        return sendResult;
    }
    sendResult = gstSendUpPacketGPI(up, port);
    if (sendResult < 1) {
        gstFreeUpPacket(up, port);
        return sendResult;
    }
    return 1;
    return 1;
}
#endif

int gstIdRequestNet(unsigned char which, unsigned long *out)
{
    switch (which) {
    case 0:
        *out = gstGpiSessionId;
        break;
    case 1:
        *out = gstTitleId;
        break;
    default:
        *out = 0;
        return gstReportError(GST_ERR_INVALID_PARAM, 0x10000a);
    }
    return gstReportError(1, 0x10000a);
}
