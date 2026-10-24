#ifndef SCREEN_OFFSET_H
#define SCREEN_OFFSET_H

/* Two adjacent screen-coordinate words at 0xA72448. The base symbol retains
   its existing name; renderer and blitter use the same fields. */
typedef struct ScreenOffset {
    short x, y;
} ScreenOffset;
extern ScreenOffset screenOffsetX;
extern float screenScaleX;
extern float screenScaleY;
extern int rgbTextures;
extern unsigned char vertexAlpha;

#endif
