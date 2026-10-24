#ifndef PC_TEXTURE_H
#define PC_TEXTURE_H

/* One texture set in the PC renderer, 0x30 bytes. Type 3 enables blending. */
typedef struct RenderTextureInfo {
    short type;
    unsigned char reserved[46];
} RenderTextureInfo;

#endif
