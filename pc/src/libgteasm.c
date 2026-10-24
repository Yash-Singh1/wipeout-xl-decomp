#include "functions.h"

/* GTE helpers of the PC libgte (matrix application, Z averaging, perspective projection).
   These originals were hand-written assembly: they use 16-bit sign extension by shifts,
   adc carry chains and a one-operand imul feeding idiv, none of which the compiler emits,
   so each is a naked function with an __asm body. */

extern long screenZ;
extern long xOffset;
extern long yOffset;
extern long SCR2;
extern long MaxZValue;
extern long HalfZValue;
extern long FNEAR;
extern long FFAR;

/* Original routine was hand-written assembly.
   ApplyMatrix(MATRIX *m, SVECTOR *v, VECTOR *out): out = m * v, each product shifted right by
   12, plus the translation m->t. */
__declspec(naked) void ApplyMatrix(MATRIX *matrix, SVECTOR *vector, VECTOR *result)
{
    __asm {
        sub    esp, 0xc
        push   ebx
        push   edi
        mov    eax, dword ptr [esp + 0x18]
        mov    ebx, dword ptr [esp + 0x1c]
        mov    ecx, dword ptr [esp + 0x20]
        mov    edi, eax
        mov    edx, ecx
        mov    cx, word ptr [ebx]
        mov    ax, word ptr [ebx + 2]
        mov    bx, word ptr [ebx + 4]
        shl    ecx, 0x10
        shl    eax, 0x10
        shl    ebx, 0x10
        sar    ecx, 0x10
        sar    eax, 0x10
        sar    ebx, 0x10
        mov    dword ptr [esp + 0x10], ecx
        mov    dword ptr [esp + 0xc], eax
        mov    dword ptr [esp + 8], ebx
        mov    cx, word ptr [edi]
        mov    ax, word ptr [edi + 2]
        mov    bx, word ptr [edi + 4]
        shl    ecx, 0x10
        shl    eax, 0x10
        shl    ebx, 0x10
        sar    ecx, 0x10
        sar    eax, 0x10
        sar    ebx, 0x10
        imul   ecx, dword ptr [esp + 0x10]
        imul   eax, dword ptr [esp + 0xc]
        imul   ebx, dword ptr [esp + 8]
        sar    ecx, 0xc
        sar    eax, 0xc
        sar    ebx, 0xc
        add    ecx, eax
        add    ecx, ebx
        add    ecx, dword ptr [edi + 0x14]
        mov    dword ptr [edx], ecx
        mov    cx, word ptr [edi + 6]
        mov    ax, word ptr [edi + 8]
        mov    bx, word ptr [edi + 0xa]
        shl    ecx, 0x10
        shl    eax, 0x10
        shl    ebx, 0x10
        sar    ecx, 0x10
        sar    eax, 0x10
        sar    ebx, 0x10
        imul   ecx, dword ptr [esp + 0x10]
        imul   eax, dword ptr [esp + 0xc]
        imul   ebx, dword ptr [esp + 8]
        sar    ecx, 0xc
        sar    eax, 0xc
        sar    ebx, 0xc
        add    ecx, eax
        add    ecx, ebx
        add    ecx, dword ptr [edi + 0x18]
        mov    dword ptr [edx + 4], ecx
        mov    cx, word ptr [edi + 0xc]
        mov    ax, word ptr [edi + 0xe]
        mov    bx, word ptr [edi + 0x10]
        shl    ecx, 0x10
        shl    eax, 0x10
        shl    ebx, 0x10
        sar    ecx, 0x10
        sar    eax, 0x10
        sar    ebx, 0x10
        imul   ecx, dword ptr [esp + 0x10]
        imul   eax, dword ptr [esp + 0xc]
        imul   ebx, dword ptr [esp + 8]
        sar    ecx, 0xc
        sar    eax, 0xc
        sar    ebx, 0xc
        add    ecx, eax
        add    ecx, ebx
        add    ecx, dword ptr [edi + 0x1c]
        mov    dword ptr [edx + 8], ecx
        xor    eax, eax
        pop    edi
        pop    ebx
        add    esp, 0xc
        ret
    }
}

/* Original routine was hand-written assembly.
   AverageZ3(sz0, sz1, sz2): the average of three Z values, computed with shifts and carries
   instead of a divide. */
__declspec(naked) void AverageZ3(void)
{
    __asm {
        sub    esp, 4
        push   ebx
        mov    eax, dword ptr [esp + 0xc]
        mov    ebx, dword ptr [esp + 0x10]
        mov    ecx, dword ptr [esp + 0x14]
        clc
        adc    eax, ebx
        adc    eax, ecx
        sar    eax, 2
        mov    ebx, eax
        sar    ebx, 2
        adc    eax, ebx
        sar    ebx, 2
        adc    eax, ebx
        sar    ebx, 2
        adc    eax, ebx
        sar    ebx, 2
        adc    eax, ebx
        sar    eax, 2
        mov    dword ptr [esp + 4], eax
        mov    eax, dword ptr [esp + 4]
        pop    ebx
        add    esp, 4
        ret
    }
}

/* Original routine was hand-written assembly.
   AverageZ4(sz0, sz1, sz2, sz3): the sum of the Z values divided by 16. The fourth argument
   is loaded but never added, so only sz0 to sz2 contribute. */
__declspec(naked) void AverageZ4(void)
{
    __asm {
        sub    esp, 4
        push   ebx
        mov    eax, dword ptr [esp + 0xc]
        mov    ebx, dword ptr [esp + 0x10]
        mov    ecx, dword ptr [esp + 0x14]
        mov    edx, dword ptr [esp + 0x18]
        clc
        adc    eax, ebx
        adc    eax, ecx
        sar    eax, 4
        mov    dword ptr [esp + 4], eax
        mov    eax, dword ptr [esp + 4]
        pop    ebx
        add    esp, 4
        ret
    }
}

/* Original routine was hand-written assembly.
   RotTransPersV(long *sxy, VECTOR *v, long *flag): projects one camera-space vector to the
   screen. It clamps Z to the near/far and max range (setting flag bits when it does), divides
   screenZ-scaled x and y by z, adds the screen offsets and clamps them to the screen. The
   packed x/y word goes to *sxy and the flag word to *flag. */
__declspec(naked) void RotTransPersV(long *screen, VECTOR *vector, long *flags)
{
    __asm {
        sub    esp, 0x1c
        push   ebx
        mov    eax, dword ptr [esp + 0x24]
        mov    ebx, dword ptr [esp + 0x28]
        mov    ecx, dword ptr [esp + 0x2c]
        mov    dword ptr [esp + 0x18], eax
        mov    dword ptr [esp + 0x1c], ebx
        mov    dword ptr [esp + 0x14], ecx
        mov    dword ptr [esp + 4], 0
        mov    eax, dword ptr [ebx]
        mov    dword ptr [esp + 0x10], eax
        mov    eax, dword ptr [ebx + 4]
        mov    dword ptr [esp + 0xc], eax
        mov    eax, dword ptr [ebx + 8]
        mov    dword ptr [esp + 8], eax
        cmp    dword ptr [esp + 8], 0
        jge    depthNonnegative
        or     dword ptr [esp + 4], 0x40000
    depthNonnegative:
        mov    ebx, dword ptr [HalfZValue]
        cmp    dword ptr [esp + 8], ebx
        jle    depthWithinHalfRange
        or     dword ptr [esp + 4], 0x400000
    depthWithinHalfRange:
        mov    ebx, dword ptr [SCR2]
        cmp    dword ptr [esp + 8], ebx
        jge    depthAboveScreenMinimum
        mov    dword ptr [esp + 8], ebx
        or     dword ptr [esp + 4], 0x20000
    depthAboveScreenMinimum:
        mov    ebx, dword ptr [MaxZValue]
        cmp    dword ptr [esp + 8], ebx
        jle    depthWithinMaximum
        mov    dword ptr [esp + 8], ebx
    depthWithinMaximum:
        mov    ebx, dword ptr [FNEAR]
        cmp    dword ptr [esp + 8], ebx
        jge    testFarDepth
        or     dword ptr [esp + 4], 0x1000
        jmp    clampCameraX
    testFarDepth:
        mov    ebx, dword ptr [FFAR]
        cmp    dword ptr [esp + 8], ebx
        jle    clampCameraX
        or     dword ptr [esp + 4], 0x1000
    clampCameraX:
        cmp    dword ptr [esp + 0x10], 0x7fff
        jle    clampCameraY
        mov    dword ptr [esp + 0x10], 0x7fff
        or     dword ptr [esp + 4], 0x81000000
    clampCameraY:
        cmp    dword ptr [esp + 0xc], 0x7fff
        jle    clampNegativeCameraX
        mov    dword ptr [esp + 0xc], 0x7fff
        or     dword ptr [esp + 4], 0x80800000
    clampNegativeCameraX:
        cmp    dword ptr [esp + 0x10], 0xffff8001
        jge    clampNegativeCameraY
        mov    dword ptr [esp + 0x10], 0xffff8001
        or     dword ptr [esp + 4], 0x81000000
    clampNegativeCameraY:
        cmp    dword ptr [esp + 0xc], 0xffff8001
        jge    projectScreenPosition
        mov    dword ptr [esp + 0xc], 0xffff8001
        or     dword ptr [esp + 4], 0x80800000
    projectScreenPosition:
        mov    eax, dword ptr [screenZ]
        imul   dword ptr [esp + 0x10]
        idiv   dword ptr [esp + 8]
        add    eax, dword ptr [xOffset]
        mov    ebx, eax
        mov    eax, dword ptr [screenZ]
        imul   dword ptr [esp + 0xc]
        idiv   dword ptr [esp + 8]
        add    eax, dword ptr [yOffset]
        mov    ecx, eax
        cmp    ebx, 0xfffffc00
        jge    clampScreenRight
        mov    ebx, 0xfffffc00
        or     dword ptr [esp + 4], 0x80004000
    clampScreenRight:
        cmp    ebx, 0x3ff
        jle    clampScreenTop
        mov    ebx, 0x3ff
        or     dword ptr [esp + 4], 0x80004000
    clampScreenTop:
        cmp    ecx, 0xfffffc00
        jge    clampScreenBottom
        mov    ecx, 0xfffffc00
        or     dword ptr [esp + 4], 0x80002000
    clampScreenBottom:
        cmp    ecx, 0x3ff
        jle    packScreenPosition
        mov    ecx, 0x3ff
        or     dword ptr [esp + 4], 0x80002000
    packScreenPosition:
        and    ebx, 0xffff
        shl    ecx, 0x10
        or     ebx, ecx
        mov    edx, dword ptr [esp + 0x18]
        mov    dword ptr [edx], ebx
        mov    ebx, dword ptr [esp + 4]
        mov    edx, dword ptr [esp + 0x14]
        mov    dword ptr [edx], ebx
        xor    eax, eax
        pop    ebx
        add    esp, 0x1c
        ret
    }
}

