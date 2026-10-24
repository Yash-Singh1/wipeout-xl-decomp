#include "avplay.h"

MovieState movie;
DynHeap *mainheapcopy;
int curFrame;
int prev_frame;
int frame_w;
int frame_h;
int load_pending;
int movie_end;
short bytes_per_pixel;
short frame_count;

void av_load_strip(void)
{
    RECT unused;
    int remainingWidth;
    int blocks;
    int bytes;

    if (bytes_per_pixel == MOVIE_RGB24 && cd_intr_pending != 0) {
        StCdInterrupt();
        cd_intr_pending = 0;
    }
    LoadImage(&movie.cur, movie.tex_buf);

    if (load_pending != 0 && (remainingWidth = movie.buf[movie.buf_idx].w %
                                               (bytes_per_pixel * MOVIE_STRIP_PIXELS)) != 0) {
        load_pending = 0;
        movie.cur.x += (remainingWidth * bytes_per_pixel) / 2;
    } else {
        movie.cur.x += bytes_per_pixel * MOVIE_STRIP_PIXELS;
    }

    if (movie.cur.x < movie.buf[movie.buf_idx].x + movie.buf[movie.buf_idx].w) {
        blocks = (movie.cur.w - 1) / (bytes_per_pixel * MOVIE_STRIP_PIXELS) + 1;
        bytes = (blocks << 3) * bytes_per_pixel;
        func_801205D4(movie.tex_buf, ((bytes << 4) * ((movie.cur.h - 1) / 16 + 1)) >> 1);
    } else {
        movie.frame_ready = 1;
        load_pending = 1;
        movie.buf_idx = (movie.buf_idx == 0);
        movie.cur.x = movie.buf[movie.buf_idx].x;
        movie.cur.y = movie.buf[movie.buf_idx].y;
    }
}

u_long *av_get_frame(MovieState *state)
{
    RECT rect;
    u_long *header;
    MovieFrame *frame;
    int attempts;
    int result;
    u_short height;
    u_short widthWords;

    attempts = MOVIE_FRAME_TIMEOUT;
    do {
        result = StGetNext(&header, (u_long **)&frame);
        attempts--;
    } while (result != 0 && attempts != 0);
    if (result != 0) {
        return 0;
    }

    if (header[0] != frame->sync0 || header[1] != frame->sync1) {
        printf("header is wrong!\n");
        FntPrint("header is wrong!\n");
        StFreeRing(header);
        return 0;
    }

    prev_frame = curFrame;
    curFrame = frame->frame_no;
    if (curFrame < prev_frame) {
        movie_end = 1;
    }
    if (frame->frame_no >= frame_count - 2) {
        movie_end = 1;
    }

    if (frame_w != frame->width || frame_h != frame->height) {
        rect.x = 0;
        rect.y = 0;
        rect.w = 480;
        rect.h = 512;
        ClearImage(&rect, 0, 0, 0);
        frame_w = frame->width;
        frame_h = frame->height;
    }

    height = frame_h;
    state->buf[1].h = height;
    state->buf[0].h = height;
    state->cur.h = height;
    widthWords = (frame_w * bytes_per_pixel) / 2;
    state->buf[1].w = widthWords;
    state->buf[0].w = widthWords;
    return header;
}

u_char timedOut;
int movieSize;
short lastTwist;

/* Takes the next sector from the stream and decodes it into the buffer that is not in use.
   Returns 0 when a sector was decoded, -1 when the stream timed out. */
int DecodeFrame(MovieState *state)
{
    u_long *ring;

    ring = av_get_frame(state);
    if (ring == 0) {
        printf("timeout:%d\n (on frame %d)", MOVIE_FRAME_TIMEOUT, curFrame);
        timedOut = 1;
        return -1;
    }
    state->ring_idx = !state->ring_idx;
    func_80120514(ring, state->ring[state->ring_idx]);
    StFreeRing(ring);
    return 0;
}

void av_wait_frame(MovieState *state, int wait)
{
    /* Release a stalled frame when the decode times out. */
    volatile int timeout = MOVIE_FRAME_TIMEOUT;

    if (state->frame_ready == 0) {
        do {
            if (--timeout == 0) {
                FntPrint("time out in decoding !\n");
                state->frame_ready = 1;
                state->buf_idx = (state->buf_idx == 0);
                state->cur.x = state->buf[state->buf_idx].x;
                state->cur.y = state->buf[state->buf_idx].y;
            }
        } while (state->frame_ready == 0);
    }
    state->frame_ready = 0;
}

void PlayMovie(char *name, int frames, int pixelBytes, int x, u_short y)
{
    /* The initial environments and previous button word are intentionally uninitialised. */
    DISPENV disp;
    DRAWENV draw;
    /* Unused movie workspace. */
    u_char scratch[0x824];
    u_long *ring;
    long fd;
    u_long buttons;
    u_long previousButtons;
    short displayX;
    long stripBlocks;
    long vramX;
    int displayBuffer;
    int pressedButtons;

    bytes_per_pixel = pixelBytes;
    frame_count = frames;
    load_pending = 1;
    movie_end = 0;
    fd = iso_open(name, 0);
    if (fd != -1) {
        LogicalSectorToLoc(iso_fileloc(fd), &movieLoc);
        movieSize = iso_filesize(fd);
        iso_close(fd);
    } else {
        printf("File not found\n");
    }

    movie.ring[0] = DAlloc(movieHeap, MOVIE_DECODE_BYTES);
    movie.ring[1] = DAlloc(movieHeap, MOVIE_DECODE_BYTES);
    movie.ring_idx = 0;
    movie.tex_buf = DAlloc(movieHeap, bytes_per_pixel << 13);
    {
        /* Keep the setup values in the retail registers without extending their lifetimes. */
        register int initialPixelBytes asm("$5") = bytes_per_pixel;
        register short initialHeight asm("$6") = MOVIE_FRAME_HEIGHT;

        vramX = ((short)x * initialPixelBytes) / 2;
        movie.buf_idx = 0;
        movie.frame_ready = 0;
        movie.buf[0].y = y;
        movie.buf[0].h = initialHeight;
        movie.buf[1].y = y + initialHeight;
        movie.buf[1].h = initialHeight;
        movie.cur.y = y;
        movie.cur.h = initialHeight;
        movie.buf[0].w = initialPixelBytes * 160;
        movie.buf[1].w = initialPixelBytes * 160;
        movie.cur.w = MOVIE_STRIP_PIXELS;
        movie.cur.w *= bytes_per_pixel;
        movie.buf[0].x = vramX;
        movie.buf[1].x = vramX;
        movie.cur.x = vramX;
    }

    func_801204D0(0);
    CdDataCallback(av_load_strip);
    ring = DAlloc(movieHeap, 0x4000);
    StSetRing(ring, MOVIE_RING_SECTORS);
    StSetStream(bytes_per_pixel == MOVIE_RGB24, 1, 0x0FFFFFFF, 0, 0);
    ResetGraph(0);
    SetGraphDebug(0);
    SetDispMask(0);
    SetVideoMode(MOVIE_VIDEO_MODE);

    if (bytes_per_pixel == MOVIE_RGB24) {
        disp.isrgb24 = 1;
        disp.disp.w = disp.disp.w * 2 / 3;
    }
    PutDispEnv(&disp);
    PutDrawEnv(&draw);

    do {
    } while (CdControl(CD_SEEK_LOGICAL, &movieLoc, 0) == 0);
    do {
    } while (CdRead2(MOVIE_STREAM_MODE) == 0);

    DecodeFrame(&movie);
    displayX = x;
    while (curFrame < (short)frames) {
        previousButtons = buttons;
        buttons = padBuf1.buttons;
        if (padBuf1.header != 0xFF) {
            buttons = ~buttons;
        }
        pressedButtons = buttons & (buttons ^ previousButtons);
        func_80120558(movie.ring[movie.ring_idx], bytes_per_pixel, previousButtons);

        stripBlocks = (movie.cur.w - 1) / (bytes_per_pixel * MOVIE_STRIP_PIXELS) + 1;
        stripBlocks *= 8;
        func_801205D4(movie.tex_buf,
                      ((stripBlocks * bytes_per_pixel << 4) * ((movie.cur.h - 1) / 16 + 1)) >> 1);

        DecodeFrame(&movie);
        if (timedOut == 1) {
            break;
        }
        av_wait_frame(&movie, 0);
        VSync(0);

        displayBuffer = movie.buf_idx == 0;
        SetDefDispEnv(&disp, movie.buf[displayBuffer].x - (displayX * bytes_per_pixel) / 2,
                      movie.buf[displayBuffer].y - (short)y, movie.buf[displayBuffer].w,
                      movie.buf[displayBuffer].h);
        SetDefDrawEnv(&draw, movie.buf[displayBuffer].x, movie.buf[displayBuffer].y,
                      movie.buf[displayBuffer].w, movie.buf[displayBuffer].h);
        if (bytes_per_pixel == MOVIE_RGB24) {
            disp.isrgb24 = 1;
            disp.disp.w = disp.disp.w * 2 / 3;
        }
        PutDispEnv(&disp);
        PutDrawEnv(&draw);
        SetDispMask(1);

        if (movie_end == 1) {
            CdControl(8, 0, 0);
            break;
        }
        if (padBuf1.header == MOVIE_PAD_NEGCON) {
            if (pressedButtons & (MOVIE_PAD_B | MOVIE_PAD_START)) {
                break;
            }
            if (padBuf1.buttonI >= MOVIE_BUTTON_THRESHOLD && lastTwist < MOVIE_BUTTON_THRESHOLD) {
                break;
            }
            lastTwist = padBuf1.buttonI;
        } else if (pressedButtons & (MOVIE_PAD_CROSS | MOVIE_PAD_START)) {
            break;
        }
    }

    StClearRing();
    StUnSetRing();
    CdDataCallback(0);
    do {
    } while (CdControl(8, 0, 0) == 0);
    DrawSync(0);
    func_80122100(ring);
    func_80122100(movie.ring[0]);
    func_80122100(movie.ring[1]);
    func_80122100(movie.tex_buf);
}

/* Debug banner printed by the heap checks in malloc.c. The original prints only the banner;
   the message string the callers pass in is never shown. */
void PrintWarning(char *msg)
{
    printf("Warning!!!  ");
}

void main(void)
{
    /* DMACallback takes an argument, but main calls it with whatever __main left in $a0. */
    DMACallback();
    mainheapcopy = (DynHeap *)heapStorage;
    InitDynamicMem(mainheapcopy, MOVIE_HEAP_BYTES);
    InitPAD(&padBuf1, sizeof(padBuf1), padBuf2, 0);
    StartPAD();
    ChangeClearPAD(0);
    CdInit();
    iso_init(mainheapcopy);
    PlayMovie(MOVIE_PATH(MOVIE_NAME), MOVIE_FRAMES, MOVIE_RGB24, 0, MOVIE_TOP_Y);
    close_iso_events();
    func_801216BC();
    StopPAD();
    ResetGraph(3);
    CdInit();
    printf("Animation finished - returning\n");
}
