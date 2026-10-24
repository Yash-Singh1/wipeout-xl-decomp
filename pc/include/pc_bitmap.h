#ifndef PC_BITMAP_H
#define PC_BITMAP_H

/* Bitmap dimensions followed by the stored pixel bytes. */
typedef struct BitmapData {
    int width;
    int height;
    unsigned char pixels[1];
} BitmapData;

int LoadBitmapData(char *filename, BitmapData **bitmap);

#endif
