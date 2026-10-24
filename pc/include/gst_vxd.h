#ifndef GST_VXD_H
#define GST_VXD_H

#ifdef NETPLAY
#include "gst.h"

int gstVxdRequestSpecific(char mode, unsigned char *tags);
int gstVxdSetHandler(GstPacketHandler handler, unsigned char *tags);
int gstVxdSetPolled(char mode, unsigned char *tags);
int gstVxdSetPollHandler(GstPacketHandler handler, unsigned char *tags);
int gstVxdSetEnabled(char enable, unsigned char *tags);
#endif

#endif
