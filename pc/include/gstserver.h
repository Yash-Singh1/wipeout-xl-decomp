/* The in-process GST server (FN_DPS_*): keeps the session and lockstep tables and answers the
   client packets ('A' make, 'B' get, 'C' change, 'D' join, 'E' quit, 'P' post, 'i' move). */
#ifndef GSTSERVER_H
#define GSTSERVER_H

#include "gst.h"

#define DPSYS_ADDPLAYER 0x0003
#define DPSYS_DELETEPLAYER 0x0005
#define DPSYS_ADDPLAYERTOGROUP 0x0007
#define DPSYS_DELETEPLAYERFROMGRP 0x0021
#define DPSYS_SESSIONLOST 0x0031
#define DPSYS_HOST 0x0101
#define DPSYS_SETPLAYERORGROUPDATA 0x0102
#define DPSYS_SETPLAYERORGROUPNAME 0x0103

typedef struct GstSysMessage {
    unsigned long type;
    unsigned long group;        /* DPMSG_GROUPADD: dpIdGroup, dpIdPlayer; player messages: dpId at +8 */
    unsigned long player;
} GstSysMessage;

/* One player of the session: 0x73 bytes. */
#pragma pack(push, 1)
typedef struct GstServerPlayer {
    char inSession;
    char inLockstep;
    char name[0x64];
    short *scores;
    unsigned long number;
    char moved;
    unsigned char *move;
} GstServerPlayer;
#pragma pack(pop)

typedef struct GstServerConfig {
    unsigned playerCount;
    unsigned sendSlots;
    unsigned sendBytes;
    unsigned recvSlots;
    unsigned recvBytes;
    unsigned resendMs;
    GstPrepareFn onPrepare;
    GstSystemFn onSystem;
    GstPacketFn onPacket;
} GstServerConfig;

extern unsigned gstServerPlayerCount;
extern GstPeerSlot *gstServerSlots;
extern GstServerPlayer *gstServerPlayers;
extern GstServerConfig gstServerConfig;
extern unsigned long gstServerSessionId;
extern unsigned long gstServerDataSize;
extern void *gstServerData;
extern unsigned long gstServerNextNumber;
extern unsigned char gstScoreCount;
extern unsigned char gstMoveSize;
extern unsigned char gstMoveCounter;
extern unsigned long gstHostDpid;

/* Reliable-layer calls (not decompiled here yet). */
extern int gstPeerOpen(GstPeer *peer, GstServerConfig *config, unsigned long local, void *event);
void gpiBroadcastNoHost(unsigned long from);

int gstServerInit(void *event, unsigned long hostDpid, unsigned long group, unsigned dataSize, const void *data, GstQueryFn query);
void gstServerResetMoves(char flag);
int gstServerSystemMessage(unsigned long from, GstSysMessage *message);
void gstLogQuitServer(int error);
void gstCheckLockstepGroup(int dpid, int player);
void gstServerGroupRemove(unsigned long group, unsigned long player);
int gstServerLeaveLockstep(GstServerPlayer *player, unsigned long dpid);
int gstServerAnnounceRoster(unsigned long number, char flag);
void gstServerAddPlayer(unsigned long dpid);
void gstServerRemovePlayer(unsigned long dpid);
int gstServerClearPlayer(GstServerPlayer *player, unsigned long dpid);
int gstServerOnPacket(unsigned long from, GstCodec *packet, char *handled);
int gstServerHandlePost(GstCodec *packet, unsigned long *from);
int gstServerHandleMove(GstCodec *packet, unsigned long *from);
int gstServerHandleGet(GstCodec *packet, unsigned long *from);
int gstServerHandleChange(GstCodec *packet, unsigned long *from);
int gstServerHandleJoin(GstCodec *packet, unsigned long *from);
int gstServerHandleMake(GstCodec *packet, unsigned long *from);
int gstServerHandleQuit(GstCodec *packet, unsigned long *from);
int gstServerPrepare(char reliable, GstPeerSlot *slots, GstSendRecord *record, unsigned long target, char *notSent);
int gstServerExit(void);

#endif
