/* Game-side network layer of the NET build (session, lockstep and packet handling).
   Only the NET-WOXL target uses these declarations. */
#ifndef GST_H
#define GST_H

#define GST_MAX_QUEUE_LEN 1000

typedef int (*GstProviderFn)(int owner, const char *name);
typedef int (*GstQueryFn)(int what, void *result);

/* What a GstQueryFn callback stores: a byte, a 32-bit value or a pointer to its answer. */
typedef union GstQueryResult {
    char byte;
    unsigned long dword;
    const void *pointer;
} GstQueryResult;

/* SDK error codes (the high word selects the table gstErrorString looks them up in). */
#define GST_ERR_SDK_UNINITIALIZED 0x80000000
#define GST_ERR_INVALID_PARAM 0x8fff0007
#define GST_ERR_UNSUPPORTED_SDK_FUNCTION 0x80000008
#define GST_ERR_UNSUPPORTED_MODE_FUNCTION 0x80000009
#define GST_ERR_DECODE_OVERFLOW 0x80010000                 /* "Decode beyond packet size" */
#define GST_ERR_ENCODE_NULL_DATA 0x80010001                /* "Encode NULL DATA pointer/array" */
#define GST_ERR_ENCODE_RANGE 0x80010002                    /* "Encode DATA beyond MAX BITS" */
#define GST_ERR_ENCODE_OVERFLOW 0x80010003                 /* "Encode beyond MAX packet size" */

/* A packet being encoded or decoded: `size` is the number of valid bytes (the decoders stop
   there), `maxSize` the room the encoders may fill. */
typedef struct GstCodec GstCodec;
typedef int (*GstPacketHandler)(GstCodec *packet);
struct GstCodec {
    unsigned char *data;
    unsigned size;
    unsigned pos;
    unsigned maxSize;
};

typedef struct GstGuid {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} GstGuid;

/* A request the SDK keeps on a list until gstReleaseBuffers frees it. */
typedef struct GstBuffer {
    struct GstBuffer *next;
    int owner;
    int id;
    GstGuid guid;
} GstBuffer;

/* A lockstep event: six bytes of input for each of up to 20 players. */
#ifdef NETPLAY
#pragma pack(push, 1)
typedef struct GstPlayerInput {
    unsigned short pad, steer, aux;
} GstPlayerInput;
#pragma pack(pop)
#endif

typedef struct GstPacket {
#ifdef NETPLAY
    union {
        unsigned char player[20][6];
        GstPlayerInput input[20];
    };
#else
    unsigned char player[20][6];
#endif
} GstPacket;

typedef struct GstEvent {
    GstPacket packet;
    unsigned char kind;
} GstEvent;

typedef struct GstReliableQueue {
    unsigned writeIndex;
    unsigned readIndex;
    unsigned count;
    unsigned *entryOffsets;
    unsigned bufferSize;
    unsigned char *buffer;
} GstReliableQueue;

/* The reliable layer's record of a remote dpid. */
typedef struct GstPeerSlot {
    unsigned long dpid;
    unsigned short sendSeq;
    unsigned short recvSeq;
} GstPeerSlot;

/* A message waiting for acknowledgements from `destCount` destinations. */
#pragma pack(push, 1)
typedef struct GstSendDest {
    unsigned short seq;
    unsigned long dpid;
} GstSendDest;

typedef struct GstSendRecord {
    char active;
    unsigned long sentTime;
    unsigned long destCount;
    unsigned long pending;
    GstSendDest *dests;
} GstSendRecord;
#pragma pack(pop)

struct GstSysMessage;
typedef int (*GstPrepareFn)(char reliable, GstPeerSlot *slots, GstSendRecord *record, unsigned long target, char *notSent);
typedef int (*GstSystemFn)(unsigned long from, struct GstSysMessage *message);
typedef int (*GstPacketFn)(unsigned long from, GstCodec *packet, char *handled);

/* A peer (the player's own end or the server's) owns a send queue and a receive queue, the records of the
   packets awaiting acknowledgement, and the worker thread that resends them. */
typedef struct GstPeer {
    GstReliableQueue sendQueue;
    GstSendRecord *sendRecords;     /* one per send queue entry */
    unsigned resendMs;
    void *userEvent;                /* set by the owner of the peer */
    void *sendEvent;
    GstReliableQueue receiveQueue;
    unsigned *recvStates;           /* one per receive queue entry */
    unsigned slotCount;
    GstPeerSlot *slots;             /* the remote dpids this peer talks to */
    unsigned long id;
    void *threadHandle;
    unsigned threadId;
    void *stopEvent;
    void *spareEvent;
    int status;
    GstPrepareFn onPrepare;
    GstSystemFn onSystem;
    GstPacketFn onPacket;
} GstPeer;

int gstPeerClose(GstPeer *peer);
void gstPeerReset(GstPeer *peer);
int gstPeerAllocSend(GstPeer *peer, int size, GstCodec **packet);
int gstPeerSend(char reliable, GstPeer *peer, GstCodec *packet, unsigned long target);
int gstPeerDropPlayer(GstPeer *peer, unsigned long dpid);
int gstPeerReleaseRecv(GstPeer *peer, GstCodec *packet);
int gstPeerReleaseSend(GstPeer *peer, GstCodec *packet);

extern GstPeer gstPlayer;
extern GstPeer gstServer;
extern int gstLockstepGroup;                            /* dpid of the lockstep group */
extern void (*gstDebugPrintCallback)(const char *format, ...);

extern GstEvent gstEventQueue[GST_MAX_QUEUE_LEN];
extern int gstEventTail;
extern int gstEventCount;

__declspec(dllimport) unsigned long __stdcall GetTickCount(void);

extern int gstMode;                                     /* 0 until the SDK is initialised */
extern unsigned gstFlags;
extern char gstVerbose;
extern void (*gstErrorCallback)(int error, int detail);
extern const char *gstGpiErrorNames[];
extern const char *gstCodecErrorNames[4];
extern const char *gstCodecFunctionNames[24];
extern const char *gstDispatchErrorNames[1];
extern const char *gstDispatchFunctionNames[9];
extern const char *gstDpErrorNames[];

const char *gstErrorString(int error);
const char *gstSdkErrorString(int error);
int gstReportError(int error, int detail);
const char *gstFunctionName(int code);
const char *gstSdkFunctionName(int code);
void *gstDefaultAlloc(unsigned size);
void gstDefaultFree(void *block);
void gstDefaultSleep(unsigned milliseconds);
unsigned long gstDefaultGetTime(void);
int gstAddRequest(int owner, const char *arg, int id, const GstGuid *guid, GstProviderFn start);
int gstLobbyGPI(GstQueryFn query);
int gstDial(const char *number);
int gstHangup(void);
int gstAllocUpPacket(GstCodec **packet, int port, unsigned size);
int gstSendEncodedUpPacket(GstCodec *packet, int port);
int gstFreeUpPacket(GstCodec *packet, int port);
int gstGetDownPacket(GstCodec **packet, int port);
int gstFreeDownPacket(GstCodec *packet);
char gstIsInitialised(void);
char gstTestFlag(int mask);
int gstReturnSuccess();


extern GstBuffer *gstBufferList;
extern void (*gstFreeHandler)(void *block);
extern int (*gstDriverFlush)(void);
extern int (*gstDriverCall)(GstCodec *packet, int arg);
extern char gstDefaultPort;

/* Callbacks the game installs (their signatures are not known yet). */
typedef int (*GstHandler)();
extern void *(*gstAllocHandler)(unsigned size);
extern GstHandler gstStatusCallback;
extern void (*gstDelayHandler)(unsigned milliseconds);
extern GstHandler gstCancelCallback;
extern unsigned short gstUserErrorCount;
extern const char **gstUserErrors;
extern int (*gstInitHook)(char keepOpen);

/* The driver routines the SDK dispatches to; the slot number is the offset into the table at 4e3640. */
extern int (*gstDriverSlot08)(void);
extern int (*gstDriverSlot0c)(void);
extern int (*gstDriverSlot10)(GstCodec **packet, int port, unsigned size);
extern int (*gstDriverSlot18)(GstCodec *packet, int arg);
extern int (*gstDriverSlot1c)(GstCodec **packet, int arg);
extern int (*gstDriverSlot20)(GstCodec *packet);
extern int (*gstDriverSlot24)(char mode);
extern int (*gstDriverSlot30)(GstPacketHandler handler);
extern int (*gstDriverSlot44)(int timeout);
extern int (*gpiDriverSlot28)(char mode, unsigned char *list);
extern int (*gpiDriverSlot34)(GstPacketHandler handler, unsigned char *list);
extern int (*gpiDriverSlot38)(char mode, unsigned char *list);
extern unsigned long (*gstClockHandler)(void);
extern unsigned short gstUserFunctionCount;
extern const char **gstUserFunctions;

int gstRegisterUserFunctions(void *buffer, unsigned short size);
int gstRegisterUserClock(unsigned long (*handler)(void));
int gstRegisterUserFree(void (*handler)(void *block));
int gstInitSDK(GstQueryFn query);
int gstExitSDK(void);
int gstRequestSpecificPackets(int mode, unsigned char firstTag, ...);
int gstRegisterSpecificPacketHandler(GstPacketHandler handler, unsigned char firstTag, ...);
int gstRegisterPolledPackets(int mode, unsigned char firstTag, ...);
int gstRegisterUserErrors(void *buffer, unsigned short size);
int gstRegisterDefaultPacketHandler(GstPacketHandler handler);
int gstRegisterErrorCallback(void (*callback)(int error, int detail), char verbose);
int gstRegisterUserAlloc(void *(*handler)(unsigned size));
int gstRegisterStatusCallback(GstHandler handler);
int gstRegisterUserDelay(void);
int gstRegisterCancelCallback(GstHandler handler);
int gstRegisterDebugPrint(void (*callback)(const char *format, ...));
int gstInitialise(int mode);
int gstCheckInitialised(void);
void gstDebugPrintf(const char *format, ...);
void gstPeerLog(GstPeer *peer, const char *format, ...);


/* Packet decoders (call ids 0x20000-0x2000b) and encoders (0x2000c-0x20017). The "packed"
   forms use the top bits of the first byte for the length: 0xxxxxxx is one byte, 10xxxxxx
   two, 11xxxxxx four (24-bit variants: three). */
int gstDecodeBytes(GstCodec *codec, unsigned count, void *dest);
int gstDecodeU16Array(GstCodec *codec, unsigned count, unsigned short *dest);
int gstDecodeS16Array(GstCodec *codec, unsigned count, short *dest);
int gstDecodePackedU32Array(GstCodec *codec, unsigned count, unsigned long *dest);
int gstDecodePackedS32Array(GstCodec *codec, unsigned count, long *dest);
int gstDecodePackedU24Array(GstCodec *codec, unsigned count, unsigned long *dest);
int gstDecodeU8(GstCodec *codec, unsigned char *dest);
int gstDecodeU16(GstCodec *codec, unsigned short *dest);
int gstDecodeS16(GstCodec *codec, short *dest);
int gstDecodePackedU32(GstCodec *codec, unsigned long *dest);
int gstDecodePackedS32(GstCodec *codec, long *dest);
int gstDecodePackedU24(GstCodec *codec, unsigned long *dest);
int gstEncodeBytes(GstCodec *codec, unsigned count, const void *src);
int gstEncodeU16Array(GstCodec *codec, unsigned count, const unsigned short *src);
int gstEncodeS16Array(GstCodec *codec, unsigned count, const short *src);
int gstEncodePackedU32Array(GstCodec *codec, unsigned count, const unsigned long *src);
int gstEncodePackedS32Array(GstCodec *codec, unsigned count, const long *src);
int gstEncodePackedU24Array(GstCodec *codec, unsigned count, const unsigned long *src);
int gstEncodeU8(GstCodec *codec, unsigned char value);
int gstEncodeU16(GstCodec *codec, unsigned short value);
int gstEncodeS16(GstCodec *codec, short value);
int gstEncodePackedU32(GstCodec *codec, unsigned long value);
int gstEncodePackedS32(GstCodec *codec, long value);
int gstEncodePackedU24(GstCodec *codec, unsigned long value);


/* ---- GPI descriptors (what the game hands to the connect/session calls) ---- */

/* The game answers these queries from the GPI layer (see the *Net functions in gstgpi.c). */
typedef enum GstQuery {
    GST_QUERY_TITLE,
    GST_QUERY_CAMPAIGN,
    GST_QUERY_SESSION,
    GST_QUERY_SESSION_RESULT
} GstQuery;


/* Called once per session with (id, name, description, player count, player count); the library calls it through
   a pointer without a prototype, so the counts are passed as full words. */
typedef int (*GstSessionEnumFn)();
typedef int (*GstLockstepEnumFn)(unsigned long id, char *name, char *description, unsigned char count, unsigned char *kinds, short *scores);

typedef struct GstTitle {
    const char *name;
    const char *version;
    const char *description;
} GstTitle;

#pragma pack(push, 1)
/* One kind of score the title keeps; `labels` names the values min..max when `labelled` is set. */
typedef struct GstScoreType {
    const char *name;
    short min;
    short max;
    char flags;
    char sort;
    char labelled;
    char display;
    char scope;
    char **labels;
} GstScoreType;

typedef struct GstCampaign {
    char count;
    GstScoreType *scores;
} GstCampaign;

typedef struct GstJoinInfo {
    char count;
    unsigned short flags;
    unsigned short *values;
} GstJoinInfo;
#pragma pack(pop)

typedef struct GstSession {
    const char *name;
    const char *description;
    unsigned long dataSize;
    const void *data;
} GstSession;

extern unsigned long gstCampaignId;
extern unsigned long gstGpiSessionId;
extern unsigned long gstTitleId;
extern int (*gpiDriverSlot40)(char enable, unsigned char *tags);


/* GPI entry points (gstapi.c) and their network implementations (gstgpi.c). */
int gstEnumProvidersGPI(GstProviderFn callback);
int gstInitGPI(GstQueryFn query);
int gstConnectGPI(GstQueryFn query);
int gstDisconnectGPI(void);
int gstEnumSessionsGPI(GstSessionEnumFn callback);
int gstCreateSessionGPI(GstQueryFn query);
int gstJoinSessionGPI(GstQueryFn query);
int gstNewUpPacket(int port, char type, int destination, int size, void *data);
int gstExitSessionGPI(void);
int gstEnumLockstepGPI(unsigned long sessionId, unsigned char *kinds, GstLockstepEnumFn callback, unsigned timeout);
int gstJoinLockstepGPI(GstQueryFn query);
int gstNewEventUpPacketGPI(int port, char type, int event, unsigned size, void *data);
int gstNewLockstepUpPacket(int port, char type, int size, void *data);
int gstExitLockstepGPI(void);
int gstSendChatGPI(int port, int unused, unsigned long sender, const char *text);
int gstSendCommandGPI(int port, int unused, unsigned long target, unsigned long sender, int command, short argument);
int gstIDRequestGPI(unsigned char which, unsigned long *out);
int gstSendUpPacketGPI(GstCodec *packet, int port);

int gstConnectNet(GstQueryFn query);
int gstDisconnectNet(void);
int gstEnumSessionsNet(GstSessionEnumFn callback);
int gstCreateSessionNet(GstQueryFn query);
int gstJoinSessionNet(GstQueryFn query);
int gstNewUpPacketNet(int port, char type, unsigned long destination, unsigned size, void *data);
int gstExitSessionNet(void);
int gstEnumLockstepNet(unsigned long sessionId, unsigned char *kinds, GstLockstepEnumFn callback, unsigned timeout);
int gstJoinLockstepNet(GstQueryFn query);
int gstNewEventUpPacketNet(int port, char type, int event, unsigned size, void *data);
int gstNewLockstepUpPacketNet(int port, char type, unsigned size, void *data);
int gstExitLockstepNet(void);
int gstSendChatNet(int port, int unused, unsigned long sender, const char *text);
int gstSendCommandNet(int port, int unused, unsigned long target, unsigned long sender, int command, short argument);
int gstIdRequestNet(unsigned char which, unsigned long *out);
int gstEncodeTitle(GstTitle *title, GstCampaign *campaign, GstCodec *codec);
int gstEncodeCampaignRequest(GstCodec *codec);
int gstEncodeDisconnect(GstCodec *codec);
int gstEncodeSessionQuery(unsigned char kind1, unsigned long value1, unsigned char kind2, unsigned short value2, GstCodec *codec);
int gstEncodeSessionCreate(GstSession *session, GstCodec *codec);
int gstEncodeSessionJoin(unsigned long id, GstJoinInfo *info, GstCodec *codec);
int gstEncodeSessionExit(unsigned char kind, unsigned long id, GstCodec *codec);
int gstEncodeQueryTerm(unsigned char kind, unsigned long id, unsigned char flag, GstCodec *codec);
int gstEncodeByteList(unsigned char count, const unsigned char *bytes, GstCodec *codec);
int gstEncodeLockstepJoin(int lockstepId, GstCodec *codec);
int gstEncodeLockstepJoinAck(int lockstepId, GstCodec *codec);


/* ---- settings file (key=value lines) ---- */
#define GST_INI_MAX_ENTRIES 200
#define GST_INI_KEY_SIZE 0x14
#define GST_INI_ENTRY_SIZE 0x10e

#pragma pack(push, 1)
typedef struct GstIni {
    char path[0x50];
    char loaded;
    short count;
    char *entries[GST_INI_MAX_ENTRIES];     /* key at +0, value at +GST_INI_KEY_SIZE */
} GstIni;
#pragma pack(pop)

GstIni *gstIniNew(void);
GstIni *gstIniLoad(const char *path, char flag);
GstIni *gstIniLoadFile(const char *path);
void gstIniStripNewline(char *line);
int gstIniFree(GstIni *ini);
int gstIniSet(GstIni *ini, const char *key, const char *value);
char *gstIniGet(GstIni *ini, const char *key);
int gstIniSave(GstIni *ini);
int gstIniSaveAs(GstIni *ini, const char *path);
int gstFileExists(const char *path);


/* ---- lockstep race glue (lockstep.c) ---- */
typedef struct GstShipSync {
    char valid;
    int x;
    int y;
    int z;
    int spare;
} GstShipSync;

extern int gstEventHead;
extern GstShipSync gstSyncLocal[16];
extern GstShipSync gstSyncRemote[16];
extern char gstSyncMessage[16][100];
extern unsigned long gstidLockstepPlayers[15];
extern unsigned long gstPlayerId;
extern unsigned long gstSessionId;
extern unsigned char gstRestartReason;
extern unsigned char gstRestartShip;
extern unsigned char raceControlTimerEnabled;
extern unsigned char raceControlTimerStarted;
extern unsigned char raceControlTimerBusy;
extern unsigned char raceControlError;
extern unsigned char gstRestartPending;
extern int localRandomSeed;
extern int incomingLockstepEventCount;
extern int lockstepEventCount;
extern unsigned char gstLockstepChannel;
extern unsigned char gstOldLockstepChannel;
extern unsigned char racePlayerCount;
extern unsigned raceControlTimerId;
extern int gstRestartSeen;
extern int tournamentTrack;
extern unsigned char gstDebugOutputEnabled;

void gstDumpSync(char shipNo);
void gstHandleRestart(GstCodec *packet);
int gstShutdownNetworkRace(int forced);
extern void gstDebugWriteText(int x, int y, const char *text);


typedef struct GstShipOrderMsg {
    unsigned char type;
    unsigned char pad;
    short order[15];
} GstShipOrderMsg;
extern short gstGlobalShipOrder[15];
extern unsigned char gstRaceMode;
extern int flipView;
typedef struct GstRestartMark {
    char set;
    int when;
} GstRestartMark;
extern GstRestartMark gstRestartHack[16];
extern GstRestartMark gstCheckFail[16];
void gstHandleAsynch(GstCodec *packet);


/* ---- packet dispatch: tag handler tables and the queue of polled packets (gstdispatch.c) ---- */
#define GST_TAG_POLLED 0x01                     /* queue packets with this tag for polling */
#define GST_TAG_HANDLER 0x02                    /* gstTagHandler has a handler for this tag */
#define GST_TAG_ENABLED 0x10                    /* the tag is wanted (queued like polled ones) */
#define GST_TAG_POLL_HANDLER 0x20               /* gstTagPollHandler has a handler for this tag */

/* A ring buffer of packet pointers: head is where the next one goes, tail where the next one
   comes out. One slot stays empty. */
typedef struct GstPacketQueue {
    unsigned head;
    unsigned tail;
    unsigned size;
    GstCodec **items;
} GstPacketQueue;

extern GstPacketQueue gstDownQueue;
extern GstPacketHandler gstDefaultPacketHandler;
extern unsigned char gstTagFlags[256];
extern GstPacketHandler gstTagHandler[256];
extern GstPacketHandler gstTagPollHandler[256];

int gstQueueCreate(GstPacketQueue *queue, unsigned size);
int gstQueueDestroy(GstPacketQueue *queue);
void gstQueueReset(GstPacketQueue *queue);
void gstQueueClear(GstPacketQueue *queue);
int gstQueuePop(GstPacketQueue *queue, GstCodec **packet);
int gstQueuePush(GstPacketQueue *queue, GstCodec *packet);
int gstDispatchInit(unsigned queueSize);
int gstDispatchFlush(void);
int gstDispatchShutdown(void);
int gstDispatchSetDefault(GstPacketHandler handler);
int gstDispatchSetHandler(GstPacketHandler handler, const unsigned char *tags);
int gstDispatchSetPolled(char enable, const unsigned char *tags);
int gstDispatchSetPollHandler(GstPacketHandler handler, const unsigned char *tags);
int gstDispatchSetEnabled(char enable, const unsigned char *tags);
int gstDispatchLookup(GstCodec *packet, GstPacketHandler *first, GstPacketHandler *second, char *unhandled);
int gstDispatchGetPolled(GstCodec **packet);


/* ---- VxD serial driver (gstvxd.c) ---- */

/* The settings the game hands the VxD, queried with GstQueryFn ids 2-6; vxdEvents is filled in
   when the driver is opened. */
typedef struct GstVxdConfig {
    int flags;
    unsigned queueSize;
    unsigned eventCount;
    int param3;
    int param4;
    void **vxdEvents;
} GstVxdConfig;

/* A diagnostic request (DeviceIoControl 0x80). */
typedef struct GstVxdRequest {
    int command;
    int arg1;
    int arg2;
    int arg3;
} GstVxdRequest;

extern int (*gpiDriverSlot00)(char c);
extern int (*gpiDriverSlot04)(char *c);
extern int (*gpiDriverSlot2c)(void);
extern int (*gpiDriverSlot3c)(GstPacketHandler handler, unsigned char *tags);

int gstModemCommand(const char *text, char sendReturn, const char **replies);
int gstModemWaitFor(const char *prompt, int length);
char *gstGetUserId(void);
char *gstGetGameName(void);
int gstVxdOpen(unsigned long mode, GstQueryFn query);
int gstDispatchDownPacket(GstCodec *packet);
int gstVxdClose(void);
int gstVxdFreeDownPacket(GstCodec *packet);
int gstVxdGetChar(char *c);
int gstVxdReadByte(unsigned char offset, unsigned char *out);
int gstVxdGetLastError(int *error);
int gstVxdGetStatusBits(unsigned char *bits);

#endif
