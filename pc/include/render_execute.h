#ifndef RENDER_EXECUTE_H
#define RENDER_EXECUTE_H

typedef struct RenderExecuteData {
    unsigned long size, vertexOffset, vertexCount;
    unsigned long instructionOffset, instructionLength, homogeneousVertexOffset;
    unsigned long statusFlags, status;
    long extentLeft, extentTop, extentRight, extentBottom;
} RenderExecuteData;

struct RenderExecuteBuffer;
typedef struct RenderExecuteBufferMethods {
    long (__stdcall *QueryInterface)(struct RenderExecuteBuffer *, const void *, void **);
    unsigned long (__stdcall *AddRef)(struct RenderExecuteBuffer *);
    unsigned long (__stdcall *Release)(struct RenderExecuteBuffer *);
    long (__stdcall *Initialize)(struct RenderExecuteBuffer *, void *device, void *description);
    long (__stdcall *Lock)(struct RenderExecuteBuffer *, void *description);
    long (__stdcall *Unlock)(struct RenderExecuteBuffer *);
    long (__stdcall *SetExecuteData)(struct RenderExecuteBuffer *, RenderExecuteData *);
} RenderExecuteBufferMethods;
typedef struct RenderExecuteBuffer {
    RenderExecuteBufferMethods *methods;
} RenderExecuteBuffer;


extern int renderBufferIndex, renderBufferState[];
extern RenderExecuteBuffer *executeBuffers[];
extern RenderExecuteData executeData;
extern unsigned long sourceVertexCount;

#endif
