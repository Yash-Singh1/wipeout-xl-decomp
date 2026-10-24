#ifndef RACE_PACKETS_H
#define RACE_PACKETS_H

#ifdef NETPLAY
#include "gst.h"

void HandleMakePlayer(GstCodec *packet);
void HandleJoinPkt(GstCodec *packet);
void HandleEventPkt(GstCodec *packet);
#endif

#endif
