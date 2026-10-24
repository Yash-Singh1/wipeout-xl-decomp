#ifndef SFX_STARTED_H
#define SFX_STARTED_H

/* Sound driver startup status, shared with window setup. */
#ifdef NETPLAY
extern unsigned char sfxStarted;
#else
extern char sfxStarted;
#endif

#endif
