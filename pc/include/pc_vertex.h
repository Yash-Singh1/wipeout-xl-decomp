#ifndef PC_VERTEX_H
#define PC_VERTEX_H

/* One transformed vertex in the PC renderer's object buffer, 0x20 bytes. */
typedef struct PCVertex {
    float x, y, z, rhw;
    unsigned char b, g, r, a;
    unsigned long specular;
    float u, v;
} PCVertex;

#endif
