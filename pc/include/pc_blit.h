#ifndef PC_BLIT_H
#define PC_BLIT_H

/* Fill record consumed by the PC renderer; coordinates are unsigned here. */
typedef struct Tile {
    unsigned long tag;
    unsigned char r0, g0, b0, code;
    unsigned short x0, y0;
    unsigned short w, h;
} Tile;

struct IDirectDrawSurface;

/* Texture surfaces retain per-texture metadata alongside the DirectDraw handle. */
typedef struct TextureSurface {
    struct IDirectDrawSurface *surface;
    long info[11];
} TextureSurface;

extern int blitCount[2][2];

void AddFill(int layer, Tile *tile);

#endif
