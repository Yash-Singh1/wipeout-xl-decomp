#include <ctype.h>
#include <string.h>
#include "gst.h"
#include "gstserver.h"
#include "gpi.h"
#include "gpi_api.h"

extern int gpiStatus;
__declspec(dllimport) void __stdcall DeleteCriticalSection(void *criticalSection);
extern int func_4b5cd9(void);
extern int func_4b5a8a(void);
extern int gstReleaseQueue(GstReliableQueue *queue);
extern int gpiConnectionContext;
extern int gpiCallbackContext;
extern int D_4ecc30;
extern int D_4ecc34;
extern GpiDirectPlay *gpiBase;
extern void *gpiServerEvent;
extern void *gpiPlayerEvent;
extern char gpiLockCreated;
extern char gpiReceiveLockCreated;
extern int func_4b5d4a();
extern int func_4b5fa2(void);
int gpiSystemMessage(unsigned long from, GstSysMessage *message);
int __stdcall func_4b5a05(int player, int message, int c, int d, int e);
int __stdcall func_4b59cf(int a, int context, int c, int d, int e);
extern int func_4b504b(char isHost, struct GpiSequence *sequence, struct GpiPacketData *data, unsigned id, char *failed);
extern GstServerConfig gpiPlayerConfig;
extern GstGuid gpiServiceGuid;
extern GstGuid gpiInterfaceIid;
extern const GstGuid gpiIidDirectPlay2;
extern int __stdcall DirectPlayOrdinal1(const GstGuid *service, GpiDirectPlay **base, void *outer);
extern int __stdcall DirectPlayOrdinal4(const GstGuid *service, GpiDirectPlay **lobby, void *outer, void *data,
                                        unsigned size);
__declspec(dllimport) void __stdcall InitializeCriticalSection(void *criticalSection);



typedef struct GpiEnumResult {
    char playerType;
    unsigned count;
} GpiEnumResult;

typedef struct GpiPendingRequest {
    struct GpiPendingRequest *next;
    int readyIndex;
    void *address;
    GstGuid applicationGuid;
} GpiPendingRequest;

/* The session description DirectPlay hands to the enumeration callback. */
typedef struct GpiSessionDesc {
    unsigned size;
    unsigned flags;
    GstGuid instance;
    GstGuid application;
    unsigned maxPlayers;
    unsigned currentPlayers;
    char *name;
    char *password;
    unsigned reserved[2];
    void *user[4];
} GpiSessionDesc;

/* What DirectPlay stores for a game that was started from a lobby (DPLCONNECTION). */
typedef struct GpiConnection {
    unsigned size;
    unsigned flags;
    GpiSessionDesc *session;
    void *playerName;
    GstGuid serviceProvider;
    void *address;
    unsigned addressSize;
} GpiConnection;
extern GpiConnection *gpiLobbyConnection;

/* The values the game is asked for when it starts the GPI layer (GstQueryFn ids). */
#define GPI_QUERY_FLAGS 1
#define GPI_QUERY_APPLICATION 0x40
#define GPI_QUERY_MAX_PLAYERS 0x41
#define GPI_QUERY_QUEUE_BYTES 0x42
#define GPI_QUERY_QUEUE_SLOTS 0x43
#define GPI_QUERY_RESEND_MS 0x44

/* Called for every session found: its request number, upper-cased name, and the player counts. */
typedef int (*GpiSessionFound)(int requestIndex, char *name, int unused, int currentPlayers, int maxPlayers);

typedef struct GpiConnectionOptions {
    unsigned size;
    unsigned char reserved[20];
    GstGuid applicationGuid;
    unsigned char trailing[40];
} GpiConnectionOptions;

typedef struct GpiShutdownLocals {
    GpiPendingRequest *current;
    GpiPendingRequest *request;
    int initResult;
    int closeResult;
} GpiShutdownLocals;

extern int gpiEventContext;
extern int gpiGroupContext;
extern char gpiSessionFlags;
extern int gpiSessionId;
extern int gpiPlayerId;
extern GpiPendingRequest *gpiPendingRequests;
extern int gpiReady;
extern GstGuid gpiApplicationGuid;
extern char gpiLobbied;
extern int gpiRequestId;
extern int __stdcall DirectPlayOrdinal2(
    int (__stdcall *callback)(const GstGuid *, const char *, int, int, GstProviderFn), GstProviderFn context);
extern int __stdcall func_4b52cf(GpiSessionDesc *session, unsigned *timeout, unsigned flags, GpiSessionFound found);

typedef struct GpiConnectState {
    unsigned short flags;
    unsigned attempt;
} GpiConnectState;


typedef struct GpiCallbackLocals {
    GstPacketHandler first;
    GstPacketHandler second;
    int result;
} GpiCallbackLocals;

typedef struct GpiSequence {
    unsigned id;
    unsigned short sequence;
} GpiSequence;

#pragma pack(1)
typedef struct GpiWirePacket {
    unsigned short sequence;
    unsigned id;
} GpiWirePacket;

typedef struct GpiPacketData {
    unsigned char reserved[5];
    int status;
    int queued;
    GpiWirePacket *packet;
} GpiPacketData;
#pragma pack()

void gpiBroadcastNoHost(unsigned long from)
{
    GpiConnectState state;

    state.flags = 0x8000;
    for (state.attempt = 0; state.attempt < 8; state.attempt++) {
        gpiInterface->vtable->send(gpiInterface, from, 0, 0, &state.flags, 2);
    }
}

#ifdef NETPLAY
/* Byte-exact in the NET build. */
/* Takes over the connection settings of a game that DirectPlay's lobby started: the session and service the
   lobby chose are handed to the game's callback, then the GPI layer is started with that service. Returns 3 when
   joining a lobby session, 4 when hosting one, 1 when the game was not started from a lobby. */
int gpiConnectLobby(GstQueryFn query)
{
    int answer;
    int err;
    unsigned size;

    gpiLobbied = 0;
    if (DirectPlayOrdinal4(0, &gpiDirectPlay, 0, 0, 0) < 0)
        return gstReportError(0x8f000000, 0xffff);
    size = 0;
    err = gpiDirectPlay->vtable->getConnectionSettings(gpiDirectPlay, 0, 0, &size);
    if (err == 0x8877042e)
        return gstReportError(1, 0xffff);
    if (err < 0 && err != 0x8877001e)
        return gstReportError(0x8f000000, 0xffff);
    gpiLobbyConnection = gstAllocHandler(size);
    if (!gpiLobbyConnection)
        return gstReportError(0x80000002, 0xffff);
    if (gpiDirectPlay->vtable->getConnectionSettings(gpiDirectPlay, 0, gpiLobbyConnection, &size) < 0)
        return gstReportError(0x8f000000, 0xffff);
    answer = query(GPI_QUERY_APPLICATION, &gpiLobbyConnection->session->application);
    if (answer != 1)
        return gstReportError(answer, 0xffff);
    answer = query(GPI_QUERY_MAX_PLAYERS, &gpiLobbyConnection->session->maxPlayers);
    if (answer != 1)
        return gstReportError(answer, 0xf000006);
    gpiLobbyConnection->session->flags = 0;
    if (gpiDirectPlay->vtable->setConnectionSettings(gpiDirectPlay, 0, 0, gpiLobbyConnection) < 0)
        return gstReportError(0x8f000000, 0xffff);
    answer = gpiInit(gpiLobbyConnection->serviceProvider, query);
    if (answer < 1)
        return gstReportError(answer, 0xffff);
    gpiLobbied = 1;
    if (gpiLobbyConnection->flags == 2) {
        return gstReportError(3, 0xffff);
    } else {
        return gstReportError(4, 0xffff);
    }
}
#endif

int gpiEnumProviders(int requestId, GstProviderFn callback)
{
    int result;

    gpiRequestId = requestId;
    result = DirectPlayOrdinal2(gpiProviderFound, callback);
    if (result < 0)
        return gstReportError(0x8f000000, 0xf000009);
    return gstReportError(1, 0xf000009);
}

#ifdef NETPLAY
/* Byte-exact in the NET build. */
/* Starts the GPI layer over the DirectPlay service: asks the game for its settings, creates the locks and
   events, sets up the packet dispatcher and the player's queue configuration, and creates the DirectPlay
   object. */
int gpiInit(GstGuid service, GstQueryFn query)
{
    int bytes;
    int supported;
    int count;
    int resendMs;
    int answer;
    int err;

    gstMode = 0;
    gstFlags = 1;
    answer = query(GPI_QUERY_FLAGS, &supported);
    if (answer != 1)
        return gstReportError(answer, 0xf000006);
    answer = query(GPI_QUERY_QUEUE_BYTES, &bytes);
    if (answer != 1)
        return gstReportError(answer, 0xf000006);
    answer = query(GPI_QUERY_QUEUE_SLOTS, &count);
    if (answer != 1)
        return gstReportError(answer, 0xf000006);
    answer = query(GPI_QUERY_RESEND_MS, &resendMs);
    if (answer < 1)
        return gstReportError(answer, 0xf010000);
    answer = query(GPI_QUERY_APPLICATION, &gpiApplicationGuid);
    if (answer != 1)
        return gstReportError(answer, 0xf000006);
    InitializeCriticalSection(&gpiReceiveLock);
    gpiReceiveLockCreated = 1;
    InitializeCriticalSection(&g_gpiCriticalSection);
    gpiLockCreated = 1;
    gpiPlayerEvent = CreateEventA(0, 1, 0, 0);
    if (gpiPlayerEvent == 0)
        return gstReportError(0x80000005, 0xf000003);
    gpiServerEvent = CreateEventA(0, 1, 0, 0);
    if (gpiServerEvent == 0)
        return gstReportError(0x80000005, 0xf000003);
    answer = gstDispatchInit(count);
    if (answer < 1)
        return gstReportError(answer, 0xf000006);
    gpiPlayerConfig.playerCount = 1;
    gpiPlayerConfig.sendSlots = count;
    gpiPlayerConfig.sendBytes = bytes;
    gpiPlayerConfig.resendMs = resendMs;
    gpiPlayerConfig.recvSlots = count;
    gpiPlayerConfig.recvBytes = bytes;
    gpiPlayerConfig.onPrepare = func_4b504b;
    gpiPlayerConfig.onSystem = gpiSystemMessage;
    gpiPlayerConfig.onPacket = gpiDispatchPacket;
    gpiServiceGuid = service;
    err = DirectPlayOrdinal1(&gpiServiceGuid, &gpiBase, 0);
    if (err < 0)
        return gstReportError(0x8f000003, 0xf000006);
    gpiInterfaceIid = gpiIidDirectPlay2;
    err = gpiBase->vtable->queryInterface(gpiBase, &gpiInterfaceIid, (void **)&gpiInterface);
    if (err < 0)
        return gstReportError(0x8f000004, 0xf000006);
    switch (supported) {
    case 1:
    case 2:
    case 4:
    case 8:
        return gstReportError(0x80000007, 0xf000006);
    case 16:
        break;
    default:
        func_4b5d4a();
        return gstReportError(0x80000006, 0xf000006);
    }
    gpiStatus = 0x8f000001;
    gstMode = 0x40;
    gstFlags = supported;
    func_4b5fa2();
    return gstReportError(1, 0xf000006);
}
#endif

int __stdcall gpiProviderFound(const GstGuid *guid, const char *service,
                          int flags, int reserved, GstProviderFn callback)
{
    int id;

    if (service && strncmp(service, "IPX", 3) == 0) {
        id = gpiRequestId;
        gpiRequestId++;
        if (gstAddRequest(id, service, 0, guid, callback) < 1)
            return 0;
    }
    return 1;
}

/* DirectPlay system messages for the player: logs them, and notes when the player's own id is destroyed. */
int gpiSystemMessage(unsigned long from, GstSysMessage *message)
{
    switch (message->type) {
    case DPSYS_ADDPLAYERTOGROUP:
        gstPeerLog(&gstPlayer, "DP: Add Player %d To Group", message->player);
        break;
    case DPSYS_DELETEPLAYERFROMGRP:
        gstPeerLog(&gstPlayer, "DP: Delete Player %d From Group", message->player);
        break;
    case DPSYS_ADDPLAYER:
        gstPeerLog(&gstPlayer, "DP: Create Player Or Group %d ", message->player);
        break;
    case DPSYS_DELETEPLAYER:
        gstPeerLog(&gstPlayer, "DP: Destroy Player or Group %d", message->player);
        if (gstPlayer.id && message->player == gstPlayer.id) {
            gpiStatus = 0x8f000002;
            ResetEvent(gstPlayer.stopEvent);
        }
        break;
    case DPSYS_HOST:
        gstPeerLog(&gstPlayer, "DP: DPSYSY_HOST");
        break;
    case DPSYS_SESSIONLOST:
        gstPeerLog(&gstPlayer, "DP: Session Lost");
        break;
    case DPSYS_SETPLAYERORGROUPDATA:
        gstPeerLog(&gstPlayer, "DP: Set Player Or Group Data %d", from);
        break;
    case DPSYS_SETPLAYERORGROUPNAME:
        gstPeerLog(&gstPlayer, "DP: Set Player Or Group Name %d", from);
        break;
    default:
        gstPeerLog(&gstPlayer, "DP: Unknown %Xh", message->type);
        break;
    }
    return 1;
}

int gpiDispatchPacket(unsigned long unused, GstCodec *packet, char *handled)
{
    GpiCallbackLocals callbacks;

    *handled = 0;
    if (!packet)
        return gstReportError(1, 0x10002);

    callbacks.result = gstDispatchLookup(packet, &callbacks.first, &callbacks.second, handled);
    if (callbacks.result >= 1) {
        if (callbacks.first) {
            callbacks.first(packet);
            *handled = 1;
        }
        if (callbacks.second) {
            callbacks.second(packet);
            *handled = 1;
        }
    }
    return gstReportError(callbacks.result, 0x10002);
}

#ifdef EQUIV
/* Branch layout differs, but the NET checker proves equivalent packet updates. */
int func_4b504b(char isHost, GpiSequence *sequence, GpiPacketData *data,
                unsigned id, char *failed)
{
    GpiWirePacket *packet;

    if (sequence->id == id && sequence->sequence != 0) {
        packet = data->packet;
        packet->id = sequence->id;
        if (isHost) {
            packet->sequence = sequence->sequence;
            sequence->sequence++;
            if (sequence->sequence > 0x3fff)
                sequence->sequence = 1;
        } else {
            packet->sequence = 0;
        }
        data->status = 1;
        data->queued = 1;
        *failed = 0;
    } else {
        *failed = 1;
    }
    return 1;
}
#endif

int gpiConnect(GstQueryFn unusedQuery)
{
    if (gpiLobbied) {
        if (gpiDirectPlay->vtable->connect(
                gpiDirectPlay, 0, &gpiInterface, 0) < 0)
            return gstReportError(0x8f000000, 0x0f000007);
    }
    return gstReportError(1, 0x0f000007);
}

int func_4b5171(void)
{
    int result;

    if (gpiLobbied) {
        result = gpiInterface->vtable->close(gpiInterface);
        if (result < 0 && result != 0x887700dc)
            return gstReportError(0x8f000010, 0x0f000008);
    }
    return gstReportError(1, 0x0f000008);
}

#ifdef EQUIV
/* The initialization path is equivalent; only the pending-list loop layout differs. */
int gpiEnumSessions(GstSessionEnumFn context)
{
    GpiPendingRequest *request;
    GpiPendingRequest *current;
    GpiConnectionOptions options;
    int result;

    current = gpiPendingRequests;
    if (current)
        gpiPendingRequests = 0;
    while (current) {
        request = current;
        current = current->next;
        gstFreeHandler(request);
    }

    gpiReady = 1;
    memset(&options, 0, 0x50);
    options.size = 0x50;
    options.applicationGuid = gpiApplicationGuid;
    result = gpiInterface->vtable->enumSessions(gpiInterface, &options, 0,
                                               (GpiEnumSessionsCallback)func_4b52cf,
                                               (void *)context, 1);
    if (result < 0)
        return gstReportError(0x80100006, 0x0f00000a);
    return gstReportError(1, 0x0f00000a);
}
#endif

/* Session enumeration callback: remembers the session (its instance guid and user data) as a pending
   request and reports it to the game with its name in capitals. */
int __stdcall func_4b52cf(GpiSessionDesc *session, unsigned *timeout, unsigned flags, GpiSessionFound found)
{
    char text[100];
    int length;
    GpiPendingRequest *pending;

    if (session) {
        pending = gstDefaultAlloc(sizeof(GpiPendingRequest));
        if (!pending)
            return 0;
        pending->next = gpiPendingRequests;
        pending->readyIndex = gpiReady;
        gpiReady++;
        pending->address = session->user[0];
        pending->applicationGuid = session->instance;
        gpiPendingRequests = pending;

        strncpy(text, session->name, 99);
        length = strlen(session->name);
        if (length > 99)
            length = 99;
        text[length] = 0;
        for (length--; length >= 0; length--)
            text[length] = toupper(text[length]);

        if (found(pending->readyIndex, text, 0, session->currentPlayers, session->maxPlayers) < 1)
            return 0;
        return 1;
    } else {
        return 0;
    }
}

#ifdef NETPLAY
/* Byte-exact in the NET build. */
/* Hosts a session: creates the DirectPlay session, the server player and the lockstep group, starts the in-process
   server, then joins as a player. */
int gpiCreateSession(GstQueryFn query)
{
    GpiName playerName;
    char playerKind;
    unsigned long maxPlayers;
    GpiSessionDesc description;
    GpiName group;
    int err;
    GstSession *session;

    err = query(GST_QUERY_SESSION, &session);
    if (err < 1) {
        return gstReportError(err, 0xf00000b);
    } else if (session == 0) {
        return gstReportError(0x80100001, 0xf00000b);
    }
    err = query(7, &maxPlayers);
    if (err < 1)
        return gstReportError(err, 0xf00000b);
    if (!gpiLobbied) {
        memset(&description, 0, sizeof(GpiSessionDesc));
        description.size = sizeof(GpiSessionDesc);
        description.flags = 0;
        description.application = gpiApplicationGuid;
        description.maxPlayers = maxPlayers;
        description.name = (char *)session->name;
        description.user[0] = (void *)0x17;
        if (gpiInterface->vtable->open(gpiInterface, &description, 2) < 0)
            return gstReportError(0x8f000008, 0xf00000b);
    }
    memset(&playerName, 0, sizeof(GpiName));
    playerName.size = sizeof(GpiName);
    playerName.shortName = "Server";
    playerName.longName = "GPI Server";
    playerKind = 1;
    if (gpiInterface->vtable->createPlayer(gpiInterface, &gpiPlayerId, &playerName, gpiServerEvent, &playerKind, 1, 0) < 0)
        return gstReportError(0x8f000009, 0xf00000b);
    gpiSessionFlags = 1;
    err = gstServerInit(gpiServerEvent, gpiPlayerId, gpiSessionId, session->dataSize, session->data, query);
    if (err < 1)
        return err;
    memset(&group, 0, sizeof(GpiName));
    group.size = sizeof(GpiName);
    group.shortName = "Group";
    group.longName = "LS Group";
    if (gpiInterface->vtable->createGroup(gpiInterface, &gpiSessionId, &group, 0, 0, 0) < 0)
        return gstReportError(0x8f00000a, 0xf00000b);
    if (gstDebugPrintCallback)
        gstDebugPrintCallback("Server DPID %d Group DPID %d", gpiPlayerId, gpiSessionId);
    return gpiJoinSession(query);
}
#endif

#ifdef NETPLAY
/* Byte-exact in the NET build. */
/* Joins the session the game picked (or the one the server just created): opens it, finds the server player and
   the lockstep group, creates the player and opens the player's peer. */
int gpiJoinSession(GstQueryFn query)
{
    GpiPendingRequest *cur;
    GpiName playerName;
    GpiSessionDesc sd;
    unsigned long *pick;
    int res;

    if (!gpiLobbied && !gpiSessionFlags) {
        res = query(GST_QUERY_SESSION_RESULT, &pick);
        if (res < 1) {
            return gstReportError(res, 0xf00000c);
        } else if (pick == 0) {
            return gstReportError(0x8fff0007, 0xf00000c);
        }
        cur = gpiPendingRequests;
        if (cur) {
            while (cur && cur->readyIndex != *pick)
                cur = cur->next;
        }
        if (cur == 0)
            return gstReportError(0x8f00000d, 0xf00000c);
        memset(&sd, 0, sizeof(GpiSessionDesc));
        sd.size = sizeof(GpiSessionDesc);
        sd.instance = cur->applicationGuid;
        if (gpiInterface->vtable->open(gpiInterface, &sd, 1) < 0)
            return gstReportError(0x8f000008, 0xf00000c);
        *pick = (unsigned long)cur->address;
    } else {
    }
    gpiConnectionContext = 0;
    if (gpiInterface->vtable->enumPlayers(gpiInterface, 0, func_4b5a05, 0, 0) < 0)
        return gstReportError(0x8f00000c, 0xf00000c);
    if (gpiConnectionContext == 0)
        return gstReportError(0x8f00000c, 0xf00000c);
    gpiCallbackContext = 0;
    if (gpiInterface->vtable->enumGroups(gpiInterface, 0, func_4b59cf, 0, 0) < 0)
        return gstReportError(0x8f00000a, 0xf00000c);
    if (gpiCallbackContext == 0) {
        if (gpiLobbied && !gpiSessionFlags) {
            return gstReportError(0x8f00000b, 0xf00000c);
        } else {
            return gstReportError(0x8f00000a, 0xf00000c);
        }
    }
    memset(&playerName, 0, sizeof(GpiName));
    playerName.size = sizeof(GpiName);
    playerName.shortName = "PLAYER";
    playerName.longName = "GPI PLAYER";
    if (gpiInterface->vtable->createPlayer(gpiInterface, &gpiEventContext, &playerName, gpiPlayerEvent, 0, 0, 0) < 0)
        return gstReportError(0x8f000009, 0xf00000c);
    res = gstPeerOpen(&gstPlayer, &gpiPlayerConfig, gpiEventContext, gpiPlayerEvent);
    if (res < 1)
        return gstReportError(res, 0xf00000c);
    gstPlayer.slots[0].sendSeq = 1;
    gstPlayer.slots[0].recvSeq = 1;
    gstPlayer.slots[0].dpid = gpiConnectionContext;
    return gstReportError(1, 0xf00000c);
}
#endif

int __stdcall func_4b59cf(int a, int context, int c, int d, int e)
{
    if (context == 0 && gpiCallbackContext == 0)
        gpiCallbackContext = a;
    return 1;
}

int __stdcall func_4b5a05(int player, int message, int c, int d, int e)
{
    GpiEnumResult result;

    if (message == 1) {
        result.count = 1;
        if (gpiInterface->vtable->getPlayerData(gpiInterface, player, &result.playerType,
                                              &result.count, 0) >= 0 &&
            result.count > 0 && result.playerType == 1)
            gpiConnectionContext = player;
    }
    return 1;
}

int func_4b5a8a(void)
{
    GpiShutdownLocals locals;

    locals.initResult = gstPeerClose(&gstPlayer);
    if (locals.initResult < 1)
        return gstReportError(locals.initResult, 0xf00000d);

    if (gpiEventContext != 0) {
        if (gpiInterface->vtable->destroyPlayer(gpiInterface, gpiEventContext) < 0)
            return gstReportError(0x8f000010, 0xf00000d);
        gpiEventContext = 0;
    }

    if (gpiSessionFlags) {
        gstServerExit();
        if (gpiInterface->vtable->destroyPlayer(gpiInterface, gpiPlayerId) < 0)
            return gstReportError(0x8f000009, 0xf00000d);
        if (gpiInterface->vtable->destroyGroup(gpiInterface, gpiSessionId) < 0)
            return gstReportError(0x8f00000a, 0xf00000d);
        gpiSessionId = 0;
        gpiPlayerId = 0;
        gpiSessionFlags = 0;
    }

    if (!gpiLobbied) {
        locals.closeResult = gpiInterface->vtable->close(gpiInterface);
        if (locals.closeResult < 0 && locals.closeResult != 0x887700dc)
            return gstReportError(0x8f000010, 0xf00000d);
    }

    gpiCallbackContext = 0;
    gpiConnectionContext = 0;
    locals.request = gpiPendingRequests;
    if (locals.request) {
        gpiPendingRequests = 0;
        while (locals.request) {
            locals.current = locals.request;
            locals.request = locals.request->next;
            gstFreeHandler(locals.current);
        }
    }
    gpiReady = 1;
    return gstReportError(1, 0xf00000d);
}

int gpiJoinLockstep(GstQueryFn unusedQuery)
{
    if (gpiInterface->vtable->addPlayerToGroup(gpiInterface, gpiCallbackContext,
                                         gpiEventContext) < 0)
        return gstReportError(0x8f00000e, 0xf00000e);
    gpiGroupContext = gpiCallbackContext;
    return gstReportError(1, 0xf00000e);
}

int func_4b5cd9(void)
{
    if (gpiGroupContext != 0) {
        if (gpiInterface->vtable->deletePlayerFromGroup(gpiInterface, gpiGroupContext,
                                              gpiEventContext) < 0)
            return gstReportError(0x8f000010, 0xf00000f);
        gpiGroupContext = 0;
    }
    return gstReportError(1, 0xf00000f);
}

int func_4b5d4a(void)
{
    struct {
        int mode;
        int result;
        int closeResult;
    } locals;

    locals.mode = gstMode;
    gstMode = 0;
    gstFlags = 1;
    func_4b5cd9();
    func_4b5a8a();
    locals.result = gstDispatchShutdown();
    if (locals.result < 1)
        return gstReportError(locals.result, 0x0f000012);

    locals.closeResult = gpiInterface->vtable->close(gpiInterface);
    if (locals.closeResult < 0 && locals.closeResult != 0x887700dc)
        return gstReportError(0x8f000010, 0x0f000012);

    locals.closeResult = gpiInterface->vtable->release(gpiInterface);
    if (locals.closeResult < 0)
        return gstReportError(0x8f000004, 0x0f000012);

    locals.closeResult = gpiBase->vtable->release(gpiBase);
    if (locals.closeResult < 0)
        return gstReportError(0x8f000004, 0x0f000012);

    if (gpiServerEvent) {
        CloseHandle(gpiServerEvent);
        gpiServerEvent = 0;
    }
    if (gpiPlayerEvent) {
        CloseHandle(gpiPlayerEvent);
        gpiPlayerEvent = 0;
    }
    if (gpiLockCreated) {
        DeleteCriticalSection(&g_gpiCriticalSection);
        gpiLockCreated = 0;
    }
    if (gpiReceiveLockCreated) {
        DeleteCriticalSection(&gpiReceiveLock);
        gpiReceiveLockCreated = 0;
    }
    if (gpiLobbied) {
        if (gpiLobbyConnection)
            gstFreeHandler(gpiLobbyConnection);
        gpiLobbied = 0;
    }
    return gstReportError(1, 0x0f000012);
}

int gstCheckGPIReady(void)
{
    if (gpiStatus < 1) {
        func_4b5cd9();
        func_4b5a8a();
        return gpiStatus;
    }
    return 1;
}

#ifdef EQUIV
/* Its branch layout differs, but the NET checker proves the same paths and effects. */
int gpiSendUnreliable(GstCodec *packet, int unused)
{
    if (gstCheckGPIReady() < 1) {
        return gstReportError(gpiStatus, 0xf000016);
    } else {
        return gstPeerSend(0, &gstPlayer, packet, gpiConnectionContext);
    }
}
#endif

int func_4b5fa2(void)
{
    gpiDriverSlot00 = gpiUnsupportedPutChar;
    gpiDriverSlot04 = gpiUnsupportedGetChar;
    gstDriverFlush = func_4b5d4a;
    gstInitHook = gpiUnsupportedRelease;
    gstDriverSlot08 = gstDialGPI;
    gstDriverSlot0c = gstHangupGPI;
    gstDriverSlot10 = gpiAllocUpPacket;
    gstDriverCall = gpiSendUpPacket;
    gstDriverSlot18 = gpiFreeUpPacket;
    gstDriverSlot1c = gpiGetDownPacket;
    gstDriverSlot20 = gpiFreeDownPacket;
    gstDriverSlot24 = gpiUnsupportedRequestAll;
    gpiDriverSlot28 = gpiUnsupportedRequestSpecific;
    gpiDriverSlot2c = gstRegisterLockstepCallback;
    gstDriverSlot30 = gpiSetDefaultHandler;
    gpiDriverSlot34 = gpiSetTagHandler;
    gpiDriverSlot38 = gpiSetPolledTags;
    gpiDriverSlot3c = gpiSetTagPollHandler;
    gpiDriverSlot40 = gpiSetEnabledTags;
    gstDriverSlot44 = gpiUnsupportedWait;
}

int gpiUnsupportedPutChar(char unusedByte)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xffff);
}

int gpiUnsupportedGetChar(char *unusedByte)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xffff);
}

int gpiUnsupportedRelease(char unusedKeepOpen)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xffff);
}

int gstDialGPI(void)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xf000013);
}

int gstHangupGPI(void)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xf000014);
}

int gpiAllocUpPacket(GstCodec **packet, int unusedPort, unsigned size)
{
    int result;

    if (gstCheckGPIReady() < 1)
        return gstReportError(gpiStatus, 0xf000015);
    result = gstPeerAllocSend(&gstPlayer, size, packet);
    return result;
}

int gpiSendUpPacket(GstCodec *a, int b)
{
    if (gstCheckGPIReady() < 1)
        return gstReportError(gpiStatus, 0xf000016);
    return gstPeerSend(1, &gstPlayer, a, gpiConnectionContext);
}

int gpiFreeUpPacket(GstCodec *value, int unusedPort)
{
    if (gstCheckGPIReady() < 1)
        return gstReportError(gpiStatus, 0xffff);
    return gstPeerReleaseSend(&gstPlayer, value);
}

/* Fetches the next received packet from the polled queue; with a timeout (zero polls once, -1 waits forever)
   it keeps trying. */
int gpiGetDownPacket(GstCodec **packet, int timeout)
{
    int oldFirst;
    int oldSecond;
    int pollResult;
    unsigned startTick;

    *packet = 0;
    if (gstCheckGPIReady() < 1)
        return gstReportError(gpiStatus, 0x0f000017);

    oldFirst = D_4ecc30;
    oldSecond = D_4ecc34;
    startTick = gstClockHandler();
    pollResult = 1;
    for (;;) {
        pollResult = gstDispatchGetPolled(packet);
        if (pollResult < 1)
            break;
        else if (*packet)
            break;
        else if (!gstTestFlag(0x1c) || timeout == 0)
            break;
        if (gstCancelCallback && gstCancelCallback()) {
            pollResult = 0x8000000b;
            break;
        }
        if (timeout && timeout != -1 && gstClockHandler() - startTick >= timeout) {
            pollResult = 0x8000000a;
            break;
        }
    }
    return gstReportError(pollResult, 0x0f000017);
}

int gpiFreeDownPacket(GstCodec *value)
{
    if (gstCheckGPIReady() < 1)
        return gstReportError(gpiStatus, 0xf000018);
    return gstPeerReleaseRecv(&gstPlayer, value);
}

int gpiUnsupportedRequestAll(char unusedMode)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xf000019);
}

int gpiUnsupportedRequestSpecific(char unusedMode, unsigned char *unusedList)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xf00001a);
}

int gstRegisterLockstepCallback(void)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0xf00001b);
}

int gpiSetDefaultHandler(GstPacketHandler handler)
{
    int result;

    EnterCriticalSection(&g_gpiCriticalSection);
    result = gstDispatchSetDefault(handler);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return gstReportError(result, 0x0f00001c);
}

int gpiSetTagHandler(GstPacketHandler handler, unsigned char *tags)
{
    int result;

    EnterCriticalSection(&g_gpiCriticalSection);
    result = gstDispatchSetHandler(handler, tags);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return gstReportError(result, 0x0f00001d);
}

int gpiSetPolledTags(char mode, unsigned char *tags)
{
    int result;

    EnterCriticalSection(&g_gpiCriticalSection);
    result = gstDispatchSetPolled(mode, tags);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return gstReportError(result, 0x0f00001e);
}

int gpiSetTagPollHandler(GstPacketHandler handler, unsigned char *tags)
{
    int result;

    EnterCriticalSection(&g_gpiCriticalSection);
    result = gstDispatchSetPollHandler(handler, tags);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return gstReportError(result, 0x0f00001f);
}

int gpiSetEnabledTags(char enable, unsigned char *tags)
{
    int result;

    EnterCriticalSection(&g_gpiCriticalSection);
    result = gstDispatchSetEnabled(enable, tags);
    LeaveCriticalSection(&g_gpiCriticalSection);
    return gstReportError(result, 0x0f000020);
}

int gpiUnsupportedWait(int unusedTimeout)
{
    return gstReportError(GST_ERR_UNSUPPORTED_SDK_FUNCTION, 0x0f000021);
}
