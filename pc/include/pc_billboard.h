#ifndef PC_BILLBOARD_H
#define PC_BILLBOARD_H

/* One sprite submitted to the PC renderer (0x20 bytes): position at 0, texture set at 8, RGB
   at 0x0C, the width and height scales at 0x10 and the texture's spans at 0x14, then the four
   corner UVs at 0x18. pad06 is the position padding word; padColour is the colour padding byte.
   DrawExplode never writes them and BuildQuads never reads them. */
typedef struct PCBillboard {
    short x, y, z, pad06;
    long textureSet;
    unsigned char r, g, b, padColour;
    short widthScale;
    short heightScale;
    short textureWidth, textureHeight;
    unsigned char u0, v0, u1, v1, u2, v2, u3, v3;
} PCBillboard;

#endif
