#include <windows.h>
#include <string.h>
#include "gstserver.h"

/* ---- start-up ---- */
void gstLogQuitServer(int error)
{
    gstPeerLog(&gstServer, "QUIT SERVER [%s]", gstErrorString(error));
}


#ifdef EQUIV
/* FN_DPS_INIT: allocates the player table, asks the game for its limits and opens the reliable layer. */
int gstServerInit(void *event, unsigned long hostDpid, unsigned long group, unsigned dataSize, const void *data, GstQueryFn query)
{
    int result;
    unsigned long interval;
    unsigned long bytes;
    unsigned i;
    unsigned long maxPlayers;
    unsigned long slots;

    gstHostDpid = hostDpid;
    gstLockstepGroup = group;
    if (query == 0)
        return gstReportError(GST_ERR_INVALID_PARAM, 0xf010000);
    result = query(7, &maxPlayers);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    result = query(5, &slots);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    result = query(4, &bytes);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    result = query(6, &interval);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    result = query(8, &gstScoreCount);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    result = query(9, &gstMoveSize);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    gstServerPlayers = gstAllocHandler(maxPlayers * sizeof(GstServerPlayer));
    if (gstServerPlayers == 0)
        return gstReportError(0x80000002, 0xf010000);
    memset(gstServerPlayers, 0, maxPlayers * sizeof(GstServerPlayer));
    gstServerNextNumber = 1;
    for (i = 0; i < maxPlayers; i++) {
        gstServerPlayers[i].move = gstAllocHandler(gstMoveSize);
        if (gstServerPlayers[i].move == 0)
            return gstReportError(0x80000002, 0xf010000);
        gstServerPlayers[i].scores = gstAllocHandler(gstScoreCount * 2 + 2);
        if (gstServerPlayers[i].scores == 0)
            return gstReportError(0x80000002, 0xf010000);
    }
    if (dataSize && data) {
        gstServerDataSize = dataSize;
        gstServerData = gstAllocHandler(gstServerDataSize);
        if (gstServerData == 0)
            return gstReportError(0x80000002, 0xf010000);
        memcpy(gstServerData, data, gstServerDataSize);
    } else {
        gstServerDataSize = 0;
        gstServerData = 0;
    }
    gstServerResetMoves(1);
    gstServerConfig.playerCount = maxPlayers;
    gstServerConfig.sendSlots = slots;
    gstServerConfig.sendBytes = bytes;
    gstServerConfig.recvSlots = slots;
    gstServerConfig.resendMs = interval;
    gstServerConfig.recvBytes = bytes;
    gstServerConfig.onPrepare = gstServerPrepare;
    gstServerConfig.onSystem = gstServerSystemMessage;
    gstServerConfig.onPacket = gstServerOnPacket;
    result = gstPeerOpen(&gstServer, &gstServerConfig, gstHostDpid, event);
    if (result < 1)
        return gstReportError(result, 0xf010000);
    return gstReportError(1, 0xf000003);
}
#endif

/* Forgets which players have sent their move for this turn. */
void gstServerResetMoves(char flag)
{
    unsigned i;

    if (flag)
        gstMoveCounter = 1;
    for (i = 0; gstServerPlayerCount > i; i++)
        gstServerPlayers[i].moved = 0;
}

/* ---- DirectPlay system messages ---- */

int gstServerSystemMessage(unsigned long from, GstSysMessage *message)
{
    switch (message->type) {
    case DPSYS_ADDPLAYERTOGROUP:
        gstCheckLockstepGroup(message->group, message->player);
        break;
    case DPSYS_DELETEPLAYERFROMGRP:
        gstServerGroupRemove(message->group, message->player);
        break;
    case DPSYS_ADDPLAYER:
        gstServerAddPlayer(message->player);
        break;
    case DPSYS_DELETEPLAYER:
        gstServerRemovePlayer(message->player);
        break;
    case DPSYS_SETPLAYERORGROUPDATA:
        break;
    case DPSYS_SETPLAYERORGROUPNAME:
        break;
    case DPSYS_HOST:
        gstPeerLog(&gstServer, "DPSYS_HOST: die die die");
        gstLogQuitServer(-1);
        break;
    case DPSYS_SESSIONLOST:
        gstPeerLog(&gstServer, "DPSYS_SESSIONLOST: die die die");
        gstLogQuitServer(-1);
        break;
    default:
        gstPeerLog(&gstServer, "Unknown DPSYS message %d", message->type);
        break;
    }
    return 1;
}

/* Reports a player id that is not the lockstep group. */
void gstCheckLockstepGroup(int dpid, int player)
{
    if (gstLockstepGroup != dpid) {
        gstPeerLog(&gstServer, "AddPlayerToGroup: dpid %d not the lockstep group\n", dpid);
        return;
    }
}

#ifdef EQUIV
/* DeletePlayerFromGroup */
void gstServerGroupRemove(unsigned long group, unsigned long player)
{
    unsigned groupIndex;

    if (gstLockstepGroup != group) {
        gstPeerLog(&gstServer, "DeletePlayerFromGroup: dpid %d not the lockstep group", group);
        return;
    }
    for (groupIndex = 0; gstServerPlayerCount > groupIndex; groupIndex++) {
        if (gstServerSlots[groupIndex].dpid == player && gstServerPlayers[groupIndex].inSession && gstServerPlayers[groupIndex].inLockstep) {
            gstServerLeaveLockstep(&gstServerPlayers[groupIndex], player);
            return;
        }
    }
    gstPeerLog(&gstServer, "DeletePlayerFromGroup: Player dpid %d not in lockstep", player);
}
#endif

#ifdef EQUIV
/* Takes a player out of the lockstep game and tells the others ('E' 5). */
int gstServerLeaveLockstep(GstServerPlayer *player, unsigned long dpid)
{
    GstCodec *reply;
    int result;
    unsigned numPlayers;
    unsigned i;

    numPlayers = 0;
    player->inLockstep = 0;
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerPlayers[i].inLockstep == 1)
            numPlayers++;
    }
    if (numPlayers > 0)
        gstServerAnnounceRoster(player->number, 0);
    else
        gstServerResetMoves(1);
    result = gstPeerAllocSend(&gstServer, 0x10, &reply);
    if (result < 1)
        return result;
    gstEncodeU8(reply, 'E');
    gstEncodeU8(reply, 5);
    result = gstPeerSend(1, &gstServer, reply, dpid);
    if (result < 1)
        return result;
    return 1;
}
#endif

#ifdef EQUIV
/* Sends the list of players still in the lockstep game ('F') to the lockstep group. */
int gstServerAnnounceRoster(unsigned long number, char flag)
{
    GstCodec *reply;
    int result;
    unsigned i;
    unsigned char playerCount;

    result = gstPeerAllocSend(&gstServer, 0x200, &reply);
    if (result < 1)
        return result;
    gstMoveCounter++;
    gstEncodeU8(reply, 'F');
    gstEncodeU8(reply, 5);
    gstEncodePackedU24(reply, 0x84);
    gstEncodeU8(reply, flag);
    gstEncodePackedU24(reply, number);
    gstEncodePackedU32(reply, 0);
    gstEncodePackedU32(reply, 0);
    gstEncodeU8(reply, gstMoveSize);
    gstEncodeU8(reply, gstMoveCounter);
    playerCount = 0;
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerPlayers[i].inLockstep == 1)
            playerCount++;
    }
    gstEncodeU8(reply, playerCount);
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerPlayers[i].inLockstep == 1)
            gstEncodePackedU24(reply, gstServerPlayers[i].number);
    }
    result = gstPeerSend(1, &gstServer, reply, gstLockstepGroup);
    if (result < 1)
        return result;
    return 1;
}
#endif

#ifdef EQUIV
/* CreatePlayerOrGroup: a new dpid gets the first free slot. */
void gstServerAddPlayer(unsigned long dpid)
{
    unsigned i;

    if (gstHostDpid == dpid)
        return;
    if (gstLockstepGroup == dpid)
        return;
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerSlots[i].dpid == 0) {
            gstServerSlots[i].dpid = dpid;
            gstServerSlots[i].sendSeq = 1;
            gstServerSlots[i].recvSeq = 1;
            gstServerPlayers[i].inSession = 0;
            gstServerPlayers[i].inLockstep = 0;
            gstPeerDropPlayer(&gstServer, dpid);
            return;
        }
    }
    gstPeerLog(&gstServer, "CreatePlayerOrGroup: Too many players in session");
}
#endif

#ifdef EQUIV
/* DestroyPlayerOrGroup */
void gstServerRemovePlayer(unsigned long dpid)
{
    unsigned i;

    if (gstHostDpid == dpid)
        return;
    if (gstLockstepGroup == dpid)
        return;
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerSlots[i].dpid == dpid) {
            if (gstServerPlayers[i].inSession) {
                if (gstServerPlayers[i].inLockstep)
                    gstServerLeaveLockstep(&gstServerPlayers[i], dpid);
                gstServerClearPlayer(&gstServerPlayers[i], dpid);
            }
            gstPeerDropPlayer(&gstServer, dpid);
            gstServerSlots[i].dpid = 0;
            gstServerSlots[i].sendSeq = 0;
            gstServerSlots[i].recvSeq = 0;
            return;
        }
    }
    gstPeerLog(&gstServer, "DestroyPlayerOrGroup: Player dpid %d not in session", dpid);
}
#endif

#ifdef NETPLAY
/* Takes a player out of the session and tells him ('E' 3). */
int gstServerClearPlayer(GstServerPlayer *player, unsigned long dpid)
{
    GstCodec *response;
    int status;

    player->inSession = 0;
    player->name[0] = 0;
    player->number = 0;
    status = gstPeerAllocSend(&gstServer, 0x10, &response);
    if (status < 1)
        return status;
    gstEncodeU8(response, 'E');
    gstEncodeU8(response, 3);
    status = gstPeerSend(1, &gstServer, response, dpid);
    if (status < 1)
        return status;
    return 1;
}
#endif

/* ---- client packets ---- */

/* Called by the reliable layer for each received packet; the first byte says what it is. */
int gstServerOnPacket(unsigned long from, GstCodec *packet, char *handled)
{
    *handled = 0;
    if (packet == 0)
        return 1;
    switch (packet->data[0]) {
    case 'P':
        gstServerHandlePost(packet, &from);
        break;
    case 'i':
        gstServerHandleMove(packet, &from);
        break;
    case 'B':
        gstServerHandleGet(packet, &from);
        break;
    case 'C':
        gstServerHandleChange(packet, &from);
        break;
    case 'D':
        gstServerHandleJoin(packet, &from);
        break;
    case 'A':
        gstServerHandleMake(packet, &from);
        break;
    case 'E':
        gstServerHandleQuit(packet, &from);
        break;
    default:
        gstPeerLog(&gstServer, "Unknown GST Packet %d Size %d", packet->data[0], packet->size);
        gstLogQuitServer(-1);
        break;
    }
    *handled = 1;
    return 1;
}

#ifdef EQUIV
/* 'P': relays a message to everybody in the lockstep group. */
int gstServerHandlePost(GstCodec *packet, unsigned long *from)
{
    GstCodec *reply;
    unsigned char version;
    int result;
    unsigned long number;
    unsigned long size;
    unsigned long session;
    unsigned char kind;
    unsigned i;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    gstDecodePackedU24(packet, &session);
    gstDecodePackedU32(packet, &size);
    if (session != gstServerSessionId)
        gstPeerLog(&gstServer, "PostAsynch: Session %d not current %d", session, gstServerSessionId);
    result = gstPeerAllocSend(&gstServer, size + 0x10, &reply);
    if (result < 1)
        return result;
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerSlots[i].dpid == *from && gstServerPlayers[i].inSession) {
            number = gstServerPlayers[i].number;
            break;
        }
    }
    if (gstServerPlayerCount <= i) {
        number = 0;
        gstPeerLog(&gstServer, "PostAsynch: Player %d not in session", *from);
    }
    gstEncodeU8(reply, 'S');
    gstEncodePackedU24(reply, number);
    gstEncodePackedU32(reply, size);
    gstEncodeBytes(reply, size, packet->pos + packet->data);
    return gstPeerSend(1, &gstServer, reply, gstLockstepGroup);
}
#endif

#ifdef EQUIV
/* 'i': a player's move; when everybody has moved the moves go out together ('j'). */
int gstServerHandleMove(GstCodec *packet, unsigned long *from)
{
    unsigned char version;
    unsigned char kind;
    unsigned i;
    unsigned char needed;
    unsigned char moved;
    unsigned index;
    GstCodec *reply;
    int result;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerSlots[i].dpid == *from && gstServerPlayers[i].inLockstep)
            break;
    }
    if (gstServerPlayerCount <= i) {
        gstPeerLog(&gstServer, "MovePkt: Player %d not in lockstep", *from);
        gstLogQuitServer(-1);
        return 1;
    }
    index = i;
    gstServerPlayers[index].moved = 1;
    gstDecodeBytes(packet, gstMoveSize, gstServerPlayers[index].move);
    needed = 0;
    moved = 0;
    for (i = 0; gstServerPlayerCount > i; i++) {
        if (gstServerPlayers[i].inLockstep == 1) {
            needed++;
            if (gstServerPlayers[i].moved == 1)
                moved++;
        }
    }
    if (needed == moved) {
        result = gstPeerAllocSend(&gstServer, gstMoveSize * gstServerPlayerCount + 2, &reply);
        if (result < 1)
            return result;
        gstEncodeU8(reply, 'j');
        gstEncodeU8(reply, gstMoveCounter);
        for (i = 0; gstServerPlayerCount > i; i++) {
            if (gstServerPlayers[i].moved)
                gstEncodeBytes(reply, gstMoveSize, gstServerPlayers[i].move);
        }
        result = gstPeerSend(1, &gstServer, reply, gstLockstepGroup);
        if (result < 1)
            return result;
        gstServerResetMoves(0);
    } else if (needed < moved) {
        gstPeerLog(&gstServer, "NumMoves %d not NumMovesNeeded %d", moved, needed);
        gstLogQuitServer(-1);
    }
    return 1;
}
#endif

#ifdef EQUIV
/* 'B': describes the session or the lockstep game to the asking player. */
int gstServerHandleGet(GstCodec *packet, unsigned long *from)
{
    GstCodec *reply;
    unsigned char version;
    int result;
    unsigned char arg1;
    unsigned char arg2;
    unsigned short value;
    unsigned char kind;
    unsigned char k;
    unsigned long id;
    unsigned j;
    unsigned char count;
    unsigned long lockstepId;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    switch (kind) {
    case 5:
        gstDecodePackedU24(packet, &id);
        gstDecodeU8(packet, 0);
        gstDecodeU16(packet, &value);
        gstDecodeU8(packet, 0);
        gstDecodePackedU24(packet, 0);
        gstDecodeU8(packet, &arg1);
        gstDecodeU8(packet, &arg2);
        gstDecodeBytes(packet, arg2, 0);
        result = gstPeerAllocSend(&gstServer, 0x800, &reply);
        if (result < 1)
            return result;
        gstEncodeU8(reply, 'B');
        gstEncodeU8(reply, 5);
        gstEncodePackedU24(reply, 0x84);
        gstEncodeU8(reply, 6);
        gstEncodeU16(reply, value);
        gstEncodeU8(reply, 3);
        gstEncodePackedU24(reply, gstServerSessionId);
        gstEncodeU8(reply, arg1);
        gstEncodeU8(reply, gstScoreCount);
        for (k = 1; k <= gstScoreCount; k++)
            gstEncodeU8(reply, k);
        count = 0;
        for (j = 0; gstServerPlayerCount > j; j++) {
            if (gstServerPlayers[j].inLockstep == 1)
                count++;
        }
        gstEncodeU8(reply, count);
        for (j = 0; gstServerPlayerCount > j; j++) {
            if (gstServerPlayers[j].inLockstep == 1) {
                gstEncodePackedU24(reply, gstServerPlayers[j].number);
                gstEncodeU8(reply, 1);
                gstEncodeU8(reply, strlen(gstServerPlayers[j].name));
                gstEncodeBytes(reply, strlen(gstServerPlayers[j].name), gstServerPlayers[j].name);
                gstEncodeU8(reply, strlen("999"));
                gstEncodeBytes(reply, strlen("999"), "999");
                gstEncodeU8(reply, 1);
                for (k = 1; k <= gstScoreCount; k++)
                    gstEncodeS16(reply, gstServerPlayers[j].scores[k]);
            }
        }
        result = gstPeerSend(1, &gstServer, reply, *from);
        if (result < 1)
            return result;
        break;
    case 3:
        gstDecodePackedU24(packet, &lockstepId);
        gstDecodeU8(packet, 0);
        gstDecodeU16(packet, &value);
        result = gstPeerAllocSend(&gstServer, 0x800, &reply);
        if (result < 1)
            return result;
        gstEncodeU8(reply, 'B');
        gstEncodeU8(reply, 3);
        gstEncodePackedU24(reply, lockstepId);
        gstEncodeU8(reply, 3);
        gstEncodeU16(reply, value);
        gstEncodeU8(reply, 2);
        gstEncodePackedU24(reply, 0x3e7);
        gstEncodeU8(reply, 0);
        gstEncodeU8(reply, 4);
        gstEncodeBytes(reply, 4, "name");
        gstEncodeU8(reply, 4);
        gstEncodeBytes(reply, 4, "desc");
        if (gstServerDataSize > 0 && gstServerData) {
            gstEncodePackedU32(reply, gstServerDataSize);
            gstEncodeBytes(reply, gstServerDataSize, gstServerData);
        } else
            gstEncodePackedU32(reply, 0);
        gstEncodePackedU32(reply, gstServerPlayerCount);
        gstEncodePackedU32(reply, gstServerPlayerCount);
        result = gstPeerSend(1, &gstServer, reply, *from);
        if (result < 1)
            return result;
        break;
    default:
        gstPeerLog(&gstServer, "GetPkt: Bad Object Type %d", kind);
        gstLogQuitServer(-1);
        break;
    }
    return 1;
}
#endif

#ifdef EQUIV
/* 'C': a change to the session data, a player's name or a player's score. */
int gstServerHandleChange(GstCodec *packet, unsigned long *from)
{
    GstCodec *reply;
    unsigned char version;
    unsigned long id;
    unsigned char kind;
    unsigned short value;
    int result;
    unsigned char arg;
    unsigned char len;
    unsigned i;
    unsigned char op;
    unsigned long number;
    unsigned char flag;
    unsigned char scoreIndex;
    short score;
    unsigned char extra;
    unsigned j;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    gstDecodePackedU24(packet, &id);
    switch (kind) {
    case 5:
        gstDecodeU16(packet, &value);
        gstDecodeU8(packet, &arg);
        result = gstPeerAllocSend(&gstServer, 0x20, &reply);
        if (result < 1)
            return result;
        gstEncodeU8(reply, 'C');
        gstEncodeU8(reply, 5);
        gstEncodePackedU24(reply, 0x84);
        gstEncodeU16(reply, value);
        gstEncodeU8(reply, 1);
        result = gstPeerSend(1, &gstServer, reply, *from);
        if (result < 1)
            return result;
        break;
    case 6:
        gstDecodeU16(packet, &value);
        if (value == 0x100) {
            for (i = 0; gstServerPlayerCount > i; i++) {
                if (gstServerSlots[i].dpid == *from && gstServerPlayers[i].inSession)
                    break;
            }
            if (gstServerPlayerCount == i) {
                gstPeerLog(&gstServer, "ChangePkt: Player: DPID %d not in session", *from);
                break;
            }
            gstDecodeU8(packet, &len);
            if (len >= 0x64) {
                gstDecodeBytes(packet, 0x63, gstServerPlayers[i].name);
                gstDecodeBytes(packet, len - 0x63, 0);
                len = 0x63;
            } else
                gstDecodeBytes(packet, len, gstServerPlayers[i].name);
            gstServerPlayers[i].name[len] = 0;
            result = gstPeerAllocSend(&gstServer, 0x10, &reply);
            if (result < 1)
                return result;
            gstEncodeU8(reply, 'C');
            gstEncodeU8(reply, 6);
            result = gstPeerSend(1, &gstServer, reply, *from);
            if (result < 1)
                return result;
            gstServerAnnounceRoster(gstServerPlayers[i].number, 1);
            break;
        } else {
            gstDecodeU8(packet, &op);
            gstDecodePackedU24(packet, &number);
            gstDecodeU8(packet, &extra);
            gstDecodeU8(packet, &flag);
            gstDecodeU8(packet, &scoreIndex);
            gstDecodeS16(packet, &score);
            for (j = 0; gstServerPlayerCount > j; j++) {
                if (gstServerSlots[j].dpid == *from && gstServerPlayers[j].inSession)
                    break;
            }
            if (gstServerPlayerCount == j) {
                gstPeerLog(&gstServer, "ChangePkt: Player: DPID %d not in session", *from);
                gstLogQuitServer(-1);
                break;
            }
            if (scoreIndex <= gstScoreCount) {
                gstServerPlayers[j].scores[scoreIndex] = score;
            } else {
                gstPeerLog(&gstServer, "ChangePkt: Player: Bad Score Index %d", scoreIndex);
                gstLogQuitServer(-1);
                break;
            }
            result = gstPeerAllocSend(&gstServer, 0x20, &reply);
            if (result < 1)
                return result;
            gstEncodeU8(reply, 'C');
            gstEncodeU8(reply, 6);
            gstEncodePackedU24(reply, id);
            gstEncodeU16(reply, value);
            gstEncodeU8(reply, op);
            gstEncodePackedU24(reply, number);
            gstEncodeU8(reply, flag);
            gstEncodeU8(reply, scoreIndex);
            result = gstPeerSend(1, &gstServer, reply, *from);
            if (result < 1)
                return result;
        }
        break;
    case 3:
        gstDecodeU16(packet, &value);
        if (gstServerData)
            gstFreeHandler(gstServerData);
        gstServerDataSize = 0;
        gstServerData = 0;
        gstDecodePackedU32(packet, &gstServerDataSize);
        if (gstServerDataSize > 0) {
            gstServerData = gstAllocHandler(gstServerDataSize);
            if (gstServerData == 0)
                return 0x80000002;
            gstDecodeBytes(packet, gstServerDataSize, gstServerData);
        }
        break;
    default:
        gstPeerLog(&gstServer, "ChangePkt: Unknown object type %d", kind);
        gstLogQuitServer(-1);
        break;
    }
    return 1;
}
#endif

#ifdef EQUIV
/* 'D': a player joins the session (kind 3) or the lockstep game (kind 5). */
int gstServerHandleJoin(GstCodec *packet, unsigned long *from)
{
    GstCodec *reply;
    unsigned char version;
    unsigned long id;
    int result;
    unsigned char count;
    unsigned char k;
    unsigned slot;
    unsigned char kind;
    unsigned char arg;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    switch (kind) {
    case 3:
        gstDecodePackedU24(packet, &id);
        gstDecodeU8(packet, &count);
        gstDecodeS16(packet, 0);
        for (k = 0; k < count; k++)
            gstDecodeS16(packet, 0);
        for (slot = 0; gstServerPlayerCount > slot; slot++) {
            if (gstServerSlots[slot].dpid == *from) {
                if (gstServerPlayers[slot].inSession) {
                    gstPeerLog(&gstServer, "JoinPkt: Session: Player already in session");
                } else
                    break;
            }
        }
        if (gstServerPlayerCount <= slot) {
            gstPeerLog(&gstServer, "JoinPkt: Session: Player Unavailable");
            id = 0;
        } else {
            gstServerPlayers[slot].inSession = 1;
            gstServerPlayers[slot].inLockstep = 0;
            gstServerPlayers[slot].name[0] = 0;
            gstServerPlayers[slot].number = gstServerNextNumber;
            gstServerNextNumber++;
            result = gstPeerAllocSend(&gstServer, 0x10, &reply);
            if (result < 1)
                return result;
            gstEncodeU8(reply, 'A');
            gstEncodeU8(reply, 6);
            gstEncodePackedU24(reply, gstServerPlayers[slot].number);
            result = gstPeerSend(1, &gstServer, reply, *from);
            if (result < 1)
                return result;
        }
        result = gstPeerAllocSend(&gstServer, 0x10, &reply);
        if (result < 1)
            return result;
        gstEncodeU8(reply, 'D');
        gstEncodeU8(reply, 3);
        gstEncodePackedU24(reply, id);
        result = gstPeerSend(1, &gstServer, reply, *from);
        if (result < 1)
            return result;
        break;
    case 5:
        gstDecodePackedU24(packet, 0);
        gstDecodeU8(packet, 0);
        gstDecodeU8(packet, &arg);
        gstDecodePackedU24(packet, 0);
        for (slot = 0; gstServerPlayerCount > slot; slot++) {
            if (gstServerSlots[slot].dpid == *from) {
                if (gstServerPlayers[slot].inSession) {
                    if (gstServerPlayers[slot].inLockstep) {
                        gstPeerLog(&gstServer, "JoinPkt: Lockstep: Player already in lockstep");
                    } else
                        break;
                } else
                    gstPeerLog(&gstServer, "JoinPkt: Lockstep: Player Not in session");
            }
        }
        if (gstServerPlayerCount <= slot) {
            gstPeerLog(&gstServer, "JoinPkt: Lockstep: Player Unavailable");
            return 1;
        }
        gstServerPlayers[slot].inLockstep = 1;
        gstServerPlayers[slot].moved = 0;
        result = gstPeerAllocSend(&gstServer, 0x10, &reply);
        if (result < 1)
            return result;
        gstEncodeU8(reply, 'D');
        gstEncodeU8(reply, 5);
        gstEncodePackedU24(reply, 0x84);
        result = gstPeerSend(1, &gstServer, reply, *from);
        if (result < 1)
            return result;
        break;
    default:
        gstPeerLog(&gstServer, "JoinPkt: Bad object type %d", kind);
        gstLogQuitServer(-1);
        break;
    }
    return result;
}
#endif

#ifdef EQUIV
/* 'A': a player asks to create the session; there is only the one. */
int gstServerHandleMake(GstCodec *packet, unsigned long *from)
{
    GstCodec *reply;
    unsigned char version;
    unsigned long dataLen;
    int result;
    unsigned long descLen;
    unsigned char nameLen;
    unsigned long extraLen;
    unsigned char kind;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    if (kind != 3) {
        gstPeerLog(&gstServer, "MakePkt: Not session object %d", kind);
        return 1;
    }
    gstDecodeU8(packet, 0);
    gstDecodeU8(packet, &nameLen);
    gstDecodePackedU32(packet, &descLen);
    gstDecodePackedU32(packet, &extraLen);
    if (nameLen > 0)
        gstDecodeBytes(packet, nameLen, 0);
    if (descLen > 0)
        gstDecodeBytes(packet, descLen, 0);
    if (extraLen > 0)
        gstDecodeBytes(packet, extraLen, 0);
    gstDecodePackedU32(packet, &dataLen);
    if (dataLen > 0)
        gstDecodeBytes(packet, dataLen, 0);
    result = gstPeerAllocSend(&gstServer, 0x10, &reply);
    if (result < 1)
        return result;
    gstEncodeU8(reply, 'A');
    gstEncodeU8(reply, 3);
    gstEncodePackedU24(reply, gstServerSessionId);
    result = gstPeerSend(1, &gstServer, reply, *from);
    if (result < 1)
        return result;
    return 1;
}
#endif

#ifdef EQUIV
/* 'E': a player leaves the session (kind 3) or the lockstep game (kind 5). */
int gstServerHandleQuit(GstCodec *packet, unsigned long *from)
{
    unsigned char version;
    unsigned long id;
    unsigned slot;
    unsigned char kind;

    gstDecodeU8(packet, &version);
    gstDecodeU8(packet, &kind);
    switch (kind) {
    case 3:
        gstDecodePackedU24(packet, &id);
        for (slot = 0; gstServerPlayerCount > slot; slot++) {
            if (gstServerSlots[slot].dpid == *from && gstServerPlayers[slot].inSession) {
                if (gstServerPlayers[slot].inLockstep)
                    gstServerLeaveLockstep(&gstServerPlayers[slot], *from);
                gstServerClearPlayer(&gstServerPlayers[slot], *from);
                break;
            }
        }
        if (gstServerPlayerCount <= slot)
            gstPeerLog(&gstServer, "QuitPkt: Session: Player %d not in session", *from);
        break;
    case 5:
        for (slot = 0; gstServerPlayerCount > slot; slot++) {
            if (gstServerSlots[slot].dpid == *from && gstServerPlayers[slot].inSession && gstServerPlayers[slot].inLockstep) {
                gstServerLeaveLockstep(&gstServerPlayers[slot], *from);
                break;
            }
        }
        if (gstServerPlayerCount <= slot)
            gstPeerLog(&gstServer, "QuitPkt: Lockstep: Player %d not in lockstep", *from);
        break;
    default:
        gstPeerLog(&gstServer, "QuitPkt: Invalid object type %d", kind);
        break;
    }
    return 1;
}
#endif

#ifdef EQUIV
/* Reliable layer callback: fills `record` with the destinations of a message to `target`
   (the lockstep group, or one player); `notSent` is set when nobody can receive it. */
int gstServerPrepare(char reliable, GstPeerSlot *slots, GstSendRecord *record, unsigned long target, char *notSent)
{
    unsigned i;
    GstServerPlayer *player;
    GstPeerSlot *slot;
    int count = 0;
    GstSendDest *dest;

    dest = &record->dests[count];
    if (gstLockstepGroup == target) {
        slot = slots;
        player = gstServerPlayers;
        for (i = 0; gstServerPlayerCount > i; i++, slot++, player++) {
            if (player->inSession && slot->sendSeq != 0) {
                dest->dpid = slot->dpid;
                if (reliable) {
                    dest->seq = slot->sendSeq;
                    slot->sendSeq++;
                    if (slot->sendSeq > 0x3fff)
                        slot->sendSeq = 1;
                } else
                    dest->seq = 0;
                count++;
                dest++;
            }
        }
    } else {
        slot = slots;
        player = gstServerPlayers;
        for (i = 0; gstServerPlayerCount > i; i++, slot++, player++) {
            if (slot->dpid == target && slot->sendSeq != 0) {
                dest->dpid = slot->dpid;
                if (reliable) {
                    dest->seq = slot->sendSeq;
                    slot->sendSeq++;
                    if (slot->sendSeq > 0x3fff)
                        slot->sendSeq = 1;
                } else
                    dest->seq = 0;
                count++;
                dest++;
                break;
            }
        }
    }
    if (count) {
        record->destCount = count;
        record->pending = count;
        *notSent = 0;
    } else {
        gstPeerLog(&gstServer, "Message To %d not sent", target);
        *notSent = 1;
    }
    return 1;
}
#endif

#ifdef EQUIV
/* FN_DPS_EXIT: tells the host to stop, then frees everything. */
int gstServerExit(void)
{
    int result;
    unsigned serverCount;
    unsigned i;

    if (gstHostDpid) {
        for (i = 0; gstServerPlayerCount > i; i++) {
            if (gstServerSlots[i].dpid != 0) {
                if (gstServerPlayers[i].inLockstep || gstServerPlayers[i].inSession) {
                    gpiBroadcastNoHost(gstHostDpid);
                    Sleep(2000);
                    break;
                }
            }
        }
        serverCount = gstServerPlayerCount;
        result = gstPeerClose(&gstServer);
        if (result < 1)
            return gstReportError(result, 0xf010001);
        if (gstServerData)
            gstFreeHandler(gstServerData);
        gstServerDataSize = 0;
        gstServerData = 0;
        if (gstServerPlayers) {
            for (i = 0; i < serverCount; i++) {
                gstFreeHandler(gstServerPlayers[i].move);
                gstFreeHandler(gstServerPlayers[i].scores);
            }
            gstFreeHandler(gstServerPlayers);
        }
        gstServerPlayers = 0;
        gstHostDpid = 0;
    }
    return gstReportError(1, 0xf000004);
}
#endif
