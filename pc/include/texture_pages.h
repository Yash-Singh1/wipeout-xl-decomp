#ifndef TEXTURE_PAGES_H
#define TEXTURE_PAGES_H

int CreateTexturePages(void);
void LoadBitmapTextures(int unused, char *name, short firstPage, short pageCount);
void Load16BitBitmapTextures(int unused, char *name, short firstPage, short pageCount);

#endif
