#ifndef AVPLAY_H
#define AVPLAY_H

#include "common.h"
#include "malloc.h"
#include "iso9660.h"
#include "psyq.h"

/* The movie file of this build: the Makefile passes -DMOVIE_NAME=xtro1 (or make, xtro2, ...)
   and -DMOVIE_FRAMES=630. MOVIE_PATH(xtro1) is the string "\\xtro1.AV". */

/* Video standard: the Makefile's REGION selects VERSION_US (NTSC, the default) or VERSION_EU
   (PAL, PALANIM.EXE). The PAL player draws 256-line frames, 64 lines down in VRAM. */
#ifdef VERSION_EU
#define MOVIE_VIDEO_MODE 1 /* SetVideoMode: 1 is PAL */
#define MOVIE_FRAME_HEIGHT 256
#define MOVIE_TOP_Y 64
#else
#define MOVIE_VIDEO_MODE 0 /* SetVideoMode: 0 is NTSC */
#define MOVIE_FRAME_HEIGHT 240
#define MOVIE_TOP_Y 24
#endif

#define MOVIE_HEAP_BYTES 0x65000
#define MOVIE_FRAME_TIMEOUT 0x800000
#define MOVIE_STRIP_PIXELS 8
#define MOVIE_DECODE_BYTES 0x28000
#define MOVIE_RING_SECTORS 32
#define MOVIE_STREAM_MODE 0x1C0
#define MOVIE_RGB24 3
#define MOVIE_PAD_NEGCON 0x2300
#define MOVIE_PAD_START 0x0008
#define MOVIE_PAD_B 0x2000
#define MOVIE_PAD_CROSS 0x4000
#define MOVIE_BUTTON_THRESHOLD 101

/* Negcon and digital-pad packets share the header and active-low button word. */
typedef struct MoviePad {
    u_short header;
    u_short buttons;
    u_char steering;
    u_char buttonI, buttonII, buttonL;
} MoviePad;

#define MOVIE_PATH2(name) "\\" #name ".AV"
#define MOVIE_PATH(name) MOVIE_PATH2(name)

/* Movie player (avplay.c). Globals below are defined in the asm bss/sbss of XTRO1.EXE. */

/* Header of a movie frame in the stream ring. The two words at 0x14 and 0x18 must match the
   stream header that StGetNext returns for the same sector. */
typedef struct MovieFrame {
    u_long unk00[2];
    u_long frame_no;
    u_long unk0C;
    u_short width;
    u_short height;
    u_long sync0;
    u_long sync1;
} MovieFrame;

/* Playback state, at 0x80138ABC. */
typedef struct MovieState {
    u_long *ring[2]; /* 0x00: stream ring buffers, toggled by ring_idx */
    int ring_idx;    /* 0x08 */
    u_long *tex_buf; /* 0x0C: pixel data copied to VRAM by LoadImage */
    RECT buf[2];     /* 0x10: strip origin (x, y) and width/height per buffer */
    int buf_idx;     /* 0x20 */
    RECT cur;        /* 0x24: rectangle of the strip being loaded */
    int frame_ready; /* 0x2C */
} MovieState;

extern MovieState movie;
extern int cd_intr_pending;
extern int curFrame;
extern int prev_frame;
extern int frame_w;
extern int frame_h;
extern int load_pending;
extern int movie_end;
extern short bytes_per_pixel;
extern short frame_count;
extern short lastTwist;
extern int movieSize;
extern CdlLOC movieLoc;
extern DynHeap *movieHeap;

int LoadImage(RECT *rect, u_long *p);
int ClearImage(RECT *rect, u_char r, u_char g, u_char b);
int FntPrint(const char *fmt, ...);
int StGetNext(u_long **header, u_long **data);
void StFreeRing(u_long *header);
void StCdInterrupt(void);
void func_801205D4(u_long *buf, int size);
int printf(const char *fmt, ...);

void av_load_strip(void);
u_long *av_get_frame(MovieState *state);
void av_wait_frame(MovieState *state, int wait);

/* More PsyQ library calls. */
int CdInit();
int ResetGraph(int mode);
int SetVideoMode(int mode);
void DMACallback();
void InitPAD(void *buf1, int len1, void *buf2, int len2);
void StartPAD(void);
int StopPAD(void);
void ChangeClearPAD(int mode);

/* Program state, defined in the asm .bss and .data. */
extern DynHeap *mainheapcopy;
extern u_char heapStorage[];
extern MoviePad padBuf1;
extern u_char padBuf2[8];
extern u_char timedOut;

/* Functions defined elsewhere in the project, by their current symbol names. */
int iso_init(DynHeap *heap);
void close_iso_events(void);
void func_801216BC(void);
void func_80120514(u_long *ring, u_long *buf);
void PlayMovie(char *name, int frames, int pixelBytes, int x, u_short y);

#endif /* AVPLAY_H */
