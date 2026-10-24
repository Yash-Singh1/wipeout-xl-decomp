#ifndef GPI_API_H
#define GPI_API_H

#ifdef NETPLAY
#include "gst.h"

int gpiConnectLobby(GstQueryFn query);
int gpiEnumProviders(int requestId, GstProviderFn callback);
int __stdcall gpiProviderFound(const GstGuid *guid, const char *service,
                              int flags, int reserved, GstProviderFn callback);
int gpiInit(GstGuid service, GstQueryFn query);
int gpiCreateSession(GstQueryFn query);
int gpiJoinSession(GstQueryFn query);
int gpiConnect(GstQueryFn unusedQuery);
int gpiEnumSessions(GstSessionEnumFn callback);
int gpiJoinLockstep(GstQueryFn unusedQuery);
int gpiSendUnreliable(GstCodec *packet, int unusedPort);
int gpiDispatchPacket(unsigned long unusedFrom, GstCodec *packet, char *handled);

int gpiUnsupportedPutChar(char unusedByte);
int gpiUnsupportedGetChar(char *unusedByte);
int gpiUnsupportedRelease(char unusedKeepOpen);
int gstDialGPI(void);
int gstHangupGPI(void);
int gpiAllocUpPacket(GstCodec **packet, int unusedPort, unsigned size);
int gpiSendUpPacket(GstCodec *packet, int unusedPort);
int gpiFreeUpPacket(GstCodec *packet, int unusedPort);
int gpiGetDownPacket(GstCodec **packet, int timeout);
int gpiFreeDownPacket(GstCodec *packet);
int gpiUnsupportedRequestAll(char unusedMode);
int gpiUnsupportedRequestSpecific(char unusedMode, unsigned char *unusedList);
int gstRegisterLockstepCallback(void);
int gpiSetDefaultHandler(GstPacketHandler handler);
int gpiSetTagHandler(GstPacketHandler handler, unsigned char *tags);
int gpiSetPolledTags(char mode, unsigned char *tags);
int gpiSetTagPollHandler(GstPacketHandler handler, unsigned char *tags);
int gpiSetEnabledTags(char enable, unsigned char *tags);
int gpiUnsupportedWait(int unusedTimeout);

#endif
#endif
