#ifndef FONT_H
#define FONT_H

/* The checkpoint timer uses the first word of the font lookup storage.
   Character lookup only reads entries 32 through 95. */
typedef union GlyphIndex {
    int bonusFrames;
    unsigned char character[96];
} GlyphIndex;
extern GlyphIndex glyphIndex;

#endif
