#ifndef D3D_H
#define D3D_H

unsigned short RGBToPixel(unsigned short r, unsigned short g, unsigned short b);

/* The parts of Direct3D 3 (immediate mode, execute buffers) the renderer uses. */

typedef unsigned long D3DMATRIXHANDLE;
typedef unsigned long D3DTEXTUREHANDLE;
typedef unsigned long D3DCOLOR;
typedef long D3DHRESULT;

/* Direct3D 4x4 float matrix. */
typedef struct D3DMATRIX {
    float m[4][4];
} D3DMATRIX;

/* Direct3D lit vertex: position, reserved word, colour, specular colour and texture coordinates. */
typedef struct D3DLVERTEX {
    float x, y, z;
    unsigned long reserved;
    D3DCOLOR color;
    D3DCOLOR specular;
    float tu, tv;
} D3DLVERTEX;

#include "render_vertex.h"

/* Execute-buffer triangle: three vertex indices and a flags word. */
typedef struct D3DTRIANGLE {
    unsigned short v1, v2, v3;
    unsigned short flags;
} D3DTRIANGLE;

/* Execute-buffer instruction header: opcode, operand size and count (see the OP_* writers). */
typedef struct D3DINSTRUCTION {
    unsigned char opcode;
    unsigned char size;
    unsigned short count;
} D3DINSTRUCTION;

/* Execute-buffer state record: a D3DRENDERSTATE_* or D3DTRANSFORMSTATE_* type and its argument. */
typedef struct D3DSTATE {
    unsigned long type;
    unsigned long arg;
} D3DSTATE;

/* Execute-buffer PROCESSVERTICES record: flags, start and destination vertex, and vertex count. */
typedef struct D3DPROCESSVERTICES {
    unsigned long flags;
    unsigned short start;
    unsigned short dest;
    unsigned long count;
    unsigned long reserved;
} D3DPROCESSVERTICES;

/* Execute data passed to SetExecuteData and GetExecuteData: vertex and instruction offsets and lengths, status and extents. */
typedef struct D3DEXECUTEDATA {
    unsigned long size;
    unsigned long vertexOffset;
    unsigned long vertexCount;
    unsigned long instructionOffset;
    unsigned long instructionLength;
    unsigned long hvertexOffset;
    unsigned long statusFlags;
    unsigned long status;
    long extentLeft, extentTop, extentRight, extentBottom;
} D3DEXECUTEDATA;

/* Execute buffer description, as passed to CreateExecuteBuffer and Lock. */
typedef struct D3DEXECUTEBUFFERDESC {
    unsigned long size;
    unsigned long flags;
    unsigned long caps;
    unsigned long bufferSize;
    void *data;
} D3DEXECUTEBUFFERDESC;

#include "d3d_opcodes.h"

/* D3DTRANSFORMSTATE_* */
#define D3DTRANSFORMSTATE_WORLD       1
#define D3DTRANSFORMSTATE_VIEW        2
#define D3DTRANSFORMSTATE_PROJECTION  3

/* D3DRENDERSTATE_* */
#define D3DRENDERSTATE_TEXTUREHANDLE      1
#define D3DRENDERSTATE_TEXTUREPERSPECTIVE 4
#define D3DRENDERSTATE_ZENABLE            7
#define D3DRENDERSTATE_ZWRITEENABLE       14
#define D3DRENDERSTATE_TEXTUREMAG         17
#define D3DRENDERSTATE_TEXTUREMIN         18
#define D3DRENDERSTATE_SRCBLEND           19
#define D3DRENDERSTATE_DESTBLEND          20
#define D3DRENDERSTATE_TEXTUREMAPBLEND    21
#define D3DRENDERSTATE_CULLMODE           22
#define D3DRENDERSTATE_ALPHABLENDENABLE   27
#define D3DRENDERSTATE_SPECULARENABLE     29

#define D3DPROCESSVERTICES_TRANSFORMLIGHT 1

/* Direct3D error codes: MAKE_DDHRESULT(n) in the SDK headers, i.e. 0x88760000 | n. */
#define D3DERR_BADMAJORVERSION 0x887602bcL
#define D3DERR_BADMINORVERSION 0x887602bdL
#define D3DERR_EXECUTE_CREATE_FAILED 0x887602c6L
#define D3DERR_EXECUTE_DESTROY_FAILED 0x887602c7L
#define D3DERR_EXECUTE_LOCK_FAILED 0x887602c8L
#define D3DERR_EXECUTE_UNLOCK_FAILED 0x887602c9L
#define D3DERR_EXECUTE_LOCKED 0x887602caL
#define D3DERR_EXECUTE_NOT_LOCKED 0x887602cbL
#define D3DERR_EXECUTE_FAILED 0x887602ccL
#define D3DERR_EXECUTE_CLIPPED_FAILED 0x887602cdL
#define D3DERR_TEXTURE_NO_SUPPORT 0x887602d0L
#define D3DERR_TEXTURE_CREATE_FAILED 0x887602d1L
#define D3DERR_TEXTURE_DESTROY_FAILED 0x887602d2L
#define D3DERR_TEXTURE_LOCK_FAILED 0x887602d3L
#define D3DERR_TEXTURE_UNLOCK_FAILED 0x887602d4L
#define D3DERR_TEXTURE_LOAD_FAILED 0x887602d5L
#define D3DERR_TEXTURELOCKED 0x887602d7L
#define D3DERR_TEXTURE_NOT_LOCKED 0x887602d8L
#define D3DERR_MATRIX_CREATE_FAILED 0x887602daL
#define D3DERR_MATRIX_DESTROY_FAILED 0x887602dbL
#define D3DERR_MATRIX_SETDATA_FAILED 0x887602dcL
#define D3DERR_SETVIEWPORTDATA_FAILED 0x887602deL
#define D3DERR_MATERIAL_CREATE_FAILED 0x887602e4L
#define D3DERR_MATERIAL_DESTROY_FAILED 0x887602e5L
#define D3DERR_MATERIAL_SETDATA_FAILED 0x887602e6L
#define D3DERR_LIGHT_SET_FAILED 0x887602eeL

struct IDirect3DExecuteBuffer;
struct IDirect3DDevice;
struct IDirect3DViewport;

typedef struct IDirect3DExecuteBufferVtbl {
    D3DHRESULT (__stdcall *QueryInterface)(struct IDirect3DExecuteBuffer *, const void *, void **);
    unsigned long (__stdcall *AddRef)(struct IDirect3DExecuteBuffer *);
    unsigned long (__stdcall *Release)(struct IDirect3DExecuteBuffer *);
    D3DHRESULT (__stdcall *Initialize)(struct IDirect3DExecuteBuffer *, struct IDirect3DDevice *, D3DEXECUTEBUFFERDESC *);
    D3DHRESULT (__stdcall *Lock)(struct IDirect3DExecuteBuffer *, D3DEXECUTEBUFFERDESC *);
    D3DHRESULT (__stdcall *Unlock)(struct IDirect3DExecuteBuffer *);
    D3DHRESULT (__stdcall *SetExecuteData)(struct IDirect3DExecuteBuffer *, D3DEXECUTEDATA *);
    D3DHRESULT (__stdcall *GetExecuteData)(struct IDirect3DExecuteBuffer *, D3DEXECUTEDATA *);
    D3DHRESULT (__stdcall *Validate)(struct IDirect3DExecuteBuffer *, unsigned long *, void *, void *, unsigned long);
    D3DHRESULT (__stdcall *Optimize)(struct IDirect3DExecuteBuffer *, unsigned long);
} IDirect3DExecuteBufferVtbl;

typedef struct IDirect3DExecuteBuffer {
    IDirect3DExecuteBufferVtbl *lpVtbl;
} IDirect3DExecuteBuffer;

typedef struct IDirect3DDeviceVtbl {
    D3DHRESULT (__stdcall *QueryInterface)(struct IDirect3DDevice *, const void *, void **);
    unsigned long (__stdcall *AddRef)(struct IDirect3DDevice *);
    unsigned long (__stdcall *Release)(struct IDirect3DDevice *);
    D3DHRESULT (__stdcall *Initialize)(struct IDirect3DDevice *, void *, void *, void *);
    D3DHRESULT (__stdcall *GetCaps)(struct IDirect3DDevice *, void *, void *);
    D3DHRESULT (__stdcall *SwapTextureHandles)(struct IDirect3DDevice *, void *, void *);
    D3DHRESULT (__stdcall *CreateExecuteBuffer)(struct IDirect3DDevice *, D3DEXECUTEBUFFERDESC *, IDirect3DExecuteBuffer **, void *);
    D3DHRESULT (__stdcall *GetStats)(struct IDirect3DDevice *, void *);
    D3DHRESULT (__stdcall *Execute)(struct IDirect3DDevice *, IDirect3DExecuteBuffer *, struct IDirect3DViewport *, unsigned long);
    D3DHRESULT (__stdcall *AddViewport)(struct IDirect3DDevice *, struct IDirect3DViewport *);
    D3DHRESULT (__stdcall *DeleteViewport)(struct IDirect3DDevice *, struct IDirect3DViewport *);
    D3DHRESULT (__stdcall *NextViewport)(struct IDirect3DDevice *, struct IDirect3DViewport *, struct IDirect3DViewport **, unsigned long);
    D3DHRESULT (__stdcall *Pick)(struct IDirect3DDevice *, void *, void *, unsigned long, void *);
    D3DHRESULT (__stdcall *GetPickRecords)(struct IDirect3DDevice *, unsigned long *, void *);
    D3DHRESULT (__stdcall *EnumTextureFormats)(struct IDirect3DDevice *, void *, void *);
    D3DHRESULT (__stdcall *CreateMatrix)(struct IDirect3DDevice *, D3DMATRIXHANDLE *);
    D3DHRESULT (__stdcall *SetMatrix)(struct IDirect3DDevice *, D3DMATRIXHANDLE, const D3DMATRIX *);
    D3DHRESULT (__stdcall *GetMatrix)(struct IDirect3DDevice *, D3DMATRIXHANDLE, D3DMATRIX *);
    D3DHRESULT (__stdcall *DeleteMatrix)(struct IDirect3DDevice *, D3DMATRIXHANDLE);
    D3DHRESULT (__stdcall *BeginScene)(struct IDirect3DDevice *);
    D3DHRESULT (__stdcall *EndScene)(struct IDirect3DDevice *);
    D3DHRESULT (__stdcall *GetDirect3D)(struct IDirect3DDevice *, void **);
} IDirect3DDeviceVtbl;

typedef struct IDirect3DDevice {
    IDirect3DDeviceVtbl *lpVtbl;
} IDirect3DDevice;


/* Execute buffer instruction writers; ptr is the buffer's write pointer. */
#define OP_STATE_TRANSFORM(n, ptr) \
    ((D3DINSTRUCTION *)(ptr))->opcode = D3DOP_STATETRANSFORM; \
    ((D3DINSTRUCTION *)(ptr))->size = sizeof(D3DSTATE); \
    ((D3DINSTRUCTION *)(ptr))->count = (unsigned short)(n); \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DINSTRUCTION))

#define OP_STATE_RENDER(n, ptr) \
    ((D3DINSTRUCTION *)(ptr))->opcode = D3DOP_STATERENDER; \
    ((D3DINSTRUCTION *)(ptr))->size = sizeof(D3DSTATE); \
    ((D3DINSTRUCTION *)(ptr))->count = (unsigned short)(n); \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DINSTRUCTION))

#define OP_PROCESS_VERTICES(n, ptr) \
    ((D3DINSTRUCTION *)(ptr))->opcode = D3DOP_PROCESSVERTICES; \
    ((D3DINSTRUCTION *)(ptr))->size = sizeof(D3DPROCESSVERTICES); \
    ((D3DINSTRUCTION *)(ptr))->count = (unsigned short)(n); \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DINSTRUCTION))

#define OP_TRIANGLE_LIST(n, ptr) \
    ((D3DINSTRUCTION *)(ptr))->opcode = D3DOP_TRIANGLE; \
    ((D3DINSTRUCTION *)(ptr))->size = sizeof(D3DTRIANGLE); \
    ((D3DINSTRUCTION *)(ptr))->count = (unsigned short)(n); \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DINSTRUCTION))

#define OP_EXIT(ptr) \
    ((D3DINSTRUCTION *)(ptr))->opcode = D3DOP_EXIT; \
    ((D3DINSTRUCTION *)(ptr))->size = 0; \
    ((D3DINSTRUCTION *)(ptr))->count = 0; \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DINSTRUCTION))

#define STATE_DATA(ty, ar, ptr) \
    ((D3DSTATE *)(ptr))->type = (ty); \
    ((D3DSTATE *)(ptr))->arg = (ar); \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DSTATE))

#define PROCESSVERTICES_DATA(fl, st, cnt, ptr) \
    ((D3DPROCESSVERTICES *)(ptr))->flags = (fl); \
    ((D3DPROCESSVERTICES *)(ptr))->start = (st); \
    ((D3DPROCESSVERTICES *)(ptr))->dest = (st); \
    ((D3DPROCESSVERTICES *)(ptr))->count = (cnt); \
    ((D3DPROCESSVERTICES *)(ptr))->reserved = 0; \
    (ptr) = (void *)((char *)(ptr) + sizeof(D3DPROCESSVERTICES))

#endif
