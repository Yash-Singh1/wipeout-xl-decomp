/* Types and imports shared by the GPI layer (gpi.c) and the reliable transport (reliable.c). */
#ifndef GPI_H
#define GPI_H

#include "gst.h"

__declspec(dllimport) void __stdcall EnterCriticalSection(void *criticalSection);
__declspec(dllimport) void __stdcall LeaveCriticalSection(void *criticalSection);
__declspec(dllimport) int __stdcall CloseHandle(void *handle);
__declspec(dllimport) int __stdcall SetEvent(void *event);
__declspec(dllimport) unsigned __stdcall WaitForSingleObject(void *object, unsigned milliseconds);
__declspec(dllimport) int __stdcall TerminateThread(void *thread, unsigned exitCode);
__declspec(dllimport) int __stdcall ResetEvent(void *event);
__declspec(dllimport) void * __stdcall CreateEventA(void *attributes, int manualReset, int initialState, const char *name);
__declspec(dllimport) void * __stdcall CreateThread(void *attributes, unsigned stackSize,
                                                    unsigned (__stdcall *start)(void *), void *parameter,
                                                    unsigned flags, unsigned *threadId);
__declspec(dllimport) unsigned __stdcall WaitForMultipleObjects(unsigned count, void **handles, int waitAll,
                                                                unsigned milliseconds);
#define WAIT_TIMEOUT 0x102
extern char g_gpiCriticalSection;
extern char gpiReceiveLock;

/* The game's DirectPlay objects: the lobby object that makes the connection, and the IDirectPlay2 interface
   (vtable offsets 0x00 to 0x7c, only the entries the game calls are typed). */
typedef int (__stdcall *GpiEnumSessionsCallback)(void *session, unsigned long *timeout, unsigned long flags,
                                                 void *context);
typedef int (__stdcall *GpiEnumCallback)(int id, int type, int name, int data, int flags);

/* A player or group name (DPNAME). */
typedef struct GpiName {
    unsigned size;
    unsigned flags;
    const char *shortName;
    const char *longName;
} GpiName;

typedef struct GpiInterfaceVtable {
    void *queryInterface;
    void *addRef;
    int (__stdcall *release)(void *object);
    int (__stdcall *addPlayerToGroup)(void *object, int group, int player);
    int (__stdcall *close)(void *object);
    int (__stdcall *createGroup)(void *object, unsigned long *id, GpiName *name, void *data, unsigned long size,
                                 unsigned long flags);
    int (__stdcall *createPlayer)(void *object, unsigned long *id, GpiName *name, void *event, void *data,
                                  unsigned long size, unsigned long flags);
    int (__stdcall *deletePlayerFromGroup)(void *object, int group, int player);
    int (__stdcall *destroyGroup)(void *object, int group);
    int (__stdcall *destroyPlayer)(void *object, int player);
    void *enumGroupPlayers;
    int (__stdcall *enumGroups)(void *object, const GstGuid *instance, GpiEnumCallback callback, void *context,
                                unsigned long flags);
    int (__stdcall *enumPlayers)(void *object, const GstGuid *instance, GpiEnumCallback callback, void *context,
                                 unsigned long flags);
    int (__stdcall *enumSessions)(void *object, void *session, unsigned long timeout,
                                  GpiEnumSessionsCallback callback, void *context, unsigned long flags);
    void *getCaps;
    void *getGroupData;
    void *getGroupName;
    void *getMessageCount;
    void *getPlayerAddress;
    void *getPlayerCaps;
    int (__stdcall *getPlayerData)(void *object, int player, void *data, unsigned long *size, unsigned long flags);
    void *getPlayerName;
    void *getSessionDesc;
    void *initialize;
    int (__stdcall *open)(void *object, void *session, unsigned long flags);
    int (__stdcall *receive)(void *object, unsigned long *from, unsigned long *to, unsigned long flags,
                             void *data, unsigned long *size);
    int (__stdcall *send)(void *object, unsigned long from, unsigned long to, unsigned long flags,
                          void *data, unsigned long size);
    void *setGroupData;
    void *setGroupName;
    void *setPlayerData;
    void *setPlayerName;
    void *setSessionDesc;
} GpiInterfaceVtable;

typedef struct GpiInterface {
    GpiInterfaceVtable *vtable;
} GpiInterface;

typedef int (__stdcall *GpiLobbyConnect)(void *object, unsigned long flags, GpiInterface **interface, void *outer);

typedef struct GpiDirectPlayVtable {
    int (__stdcall *queryInterface)(void *object, const GstGuid *iid, void **out);
    void *addRef;
    int (__stdcall *release)(void *object);
    GpiLobbyConnect connect;
    void *createAddress;
    void *enumAddress;
    void *enumAddressTypes;
    void *enumLocalApplications;
    int (__stdcall *getConnectionSettings)(void *object, unsigned long application, void *settings,
                                           unsigned long *size);
    void *receiveLobbyMessage;
    void *runApplication;
    void *sendLobbyMessage;
    int (__stdcall *setConnectionSettings)(void *object, unsigned long flags, unsigned long application,
                                           void *settings);
    void *setLobbyMessageEvent;
} GpiDirectPlayVtable;

typedef struct GpiDirectPlay {
    GpiDirectPlayVtable *vtable;
} GpiDirectPlay;

extern GpiInterface *gpiInterface;
extern GpiDirectPlay *gpiDirectPlay;

#endif
