#ifndef PC_PALETTE_H
#define PC_PALETTE_H

/* Win32 palette entries have four byte fields. Keep this declaration available
   to game modules without bringing in the Win32 RECT definition. */
#ifndef _WINGDI_
typedef struct tagPALETTEENTRY {
    unsigned char peRed;
    unsigned char peGreen;
    unsigned char peBlue;
    unsigned char peFlags;
} PALETTEENTRY;
#endif

extern PALETTEENTRY savedPalette[256];

/* CVECTOR (r, g, b, cd) and PALETTEENTRY (peRed, peGreen, peBlue, peFlags) are both four
   unsigned bytes in red, green, blue order, so a colour buffer can be passed as palette entries. */
unsigned char FindNearestColourIndex(const PALETTEENTRY *colours, short count);

/* mpegPaletteEntries is the saved MPEG palette. mpegDestBase follows it in memory, 0x400
   bytes past the table. mpegDisplayPalette is the live palette the MPEG code sets. */
extern PALETTEENTRY mpegPaletteEntries[256];
extern unsigned long mpegDestBase;
extern PALETTEENTRY mpegDisplayPalette[256];

#endif
