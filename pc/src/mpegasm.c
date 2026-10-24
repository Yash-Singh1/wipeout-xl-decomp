/* Sound-mixing helpers (sample lookup mixers, packers, echo, delay and reverb), plus the
   plane and coefficient clears. The original routines in this file were hand-written
   assembly (no compiler frame, register use no C compiler produces), so each is a naked
   function with an __asm body. */
/* xchg: MSVC 4.20 encodes "xchg cl, ch" with the register order reversed, so the
   two xchg instructions below are spelt "xchg ch, cl" to produce the original bytes. */

#include "pc_sfx.h"
#include "pc_sfxmix.h"
#include "mpeg.h"

/* Original routine was hand-written assembly.
   Zeroes mixBlockWords 32-bit words at mixScratch. */
__declspec(naked) void ClearMixScratch(void)
{
    __asm {
        push   edi
        push   eax
        push   ecx
        mov    edi, dword ptr [mixScratch]
        mov    ecx, dword ptr [mixBlockWords]
        sar    ecx, 2
        xor    eax, eax
    loop1:
        mov    dword ptr [edi], eax
        mov    dword ptr [edi + 4], eax
        mov    dword ptr [edi + 8], eax
        mov    dword ptr [edi + 0xc], eax
        add    edi, 0x10
        dec    ecx
        jne    loop1
        pop    ecx
        pop    eax
        pop    edi
        ret
    }
}

/* Original routine was hand-written assembly.
   Fills mixCount bytes at mixClearDst with zero, or with 0x80 bytes when mixClearBits is 8. */
__declspec(naked) void ClearMixOutput(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    eax, dword ptr [mixClearDst]
        mov    edx, dword ptr [mixCount]
        mov    ebx, dword ptr [mixClearBits]
        mov    edi, eax
        mov    ecx, edx
        shr    ecx, 2
        xor    eax, eax
        cmp    ebx, 8
        jne    loop1
        mov    eax, 0x80808080
    loop1:
        mov    dword ptr [edi], eax
        mov    dword ptr [edi + 4], eax
        mov    dword ptr [edi + 8], eax
        mov    dword ptr [edi + 0xc], eax
        add    edi, 0x10
        sub    ecx, 4
        jne    loop1
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Stores one quantTable value per source byte of decodeSrc into lutOut, 8 bytes per output
   entry. The volume row comes from lutRowSingle. */
__declspec(naked) void LutStoreRow(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        xor    eax, eax
        mov    ax, word ptr [lutRowSingle]
        mov    ecx, dword ptr [lutCount]
        mov    ebx, dword ptr [quantTable]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], edx
        add    esi, 1
        add    edi, 8
        sub    ecx, 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    al, byte ptr [esi + 1]
        mov    dword ptr [edi], edx
        mov    edx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi + 8], edx
        add    esi, 2
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreRow, but the source advances by lutStep per output entry; the fraction is kept in
   lutAcc. */
__declspec(naked) void LutStoreRowStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    edx, edx
        xor    ebp, ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ax, word ptr [lutRowSingle]
        mov    ebx, dword ptr [quantTable]
        mov    bp, word ptr [lutStep]
        mov    dx, word ptr [lutAcc]
        mov    ecx, dword ptr [lutCount]
        and    ecx, 1
        je     loop1
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], edx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        add    edx, ebp
        xor    ecx, ecx
        add    edi, 8
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        add    edx, ebp
        xor    ecx, ecx
        add    edi, 8
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], dx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreRow, but adds each value into lutOut instead of storing it. */
__declspec(naked) void LutAddRow(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        xor    eax, eax
        mov    ax, word ptr [lutRowSingle]
        mov    ecx, dword ptr [lutCount]
        mov    ebx, dword ptr [quantTable]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], edx
        add    esi, 1
        add    edi, 8
        sub    ecx, 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    al, byte ptr [esi + 1]
        add    dword ptr [edi], edx
        mov    edx, dword ptr [ebx + eax*4]
        add    dword ptr [edi + 8], edx
        add    esi, 2
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutAddRow, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutAddRowStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    edx, edx
        xor    ebp, ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ax, word ptr [lutRowSingle]
        mov    ebx, dword ptr [quantTable]
        mov    bp, word ptr [lutStep]
        mov    dx, word ptr [lutAcc]
        mov    ecx, dword ptr [lutCount]
        and    ecx, 1
        je     loop1
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], edx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        add    edx, ebp
        xor    ecx, ecx
        add    edi, 8
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        add    edx, ebp
        xor    ecx, ecx
        add    edi, 8
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], dx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Zeroes lutCount 8-byte output entries at lutOut. */
__declspec(naked) void LutClearRow(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    edi, dword ptr [lutOut]
        mov    ecx, dword ptr [lutCount]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    dword ptr [edi], eax
        add    edi, 8
        sub    ecx, 1
        je     skip1
    loop1:
        mov    dword ptr [edi], eax
        mov    dword ptr [edi + 8], eax
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Stores two quantTable values per source byte into each 8-byte entry: the value for volume
   row lutRowA in the first dword and the one for lutRowB in the second. */
__declspec(naked) void LutStoreDual(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    ecx, ecx
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ah, byte ptr [lutRowA]
        mov    ch, byte ptr [lutRowB]
        mov    ebx, dword ptr [quantTable]
        mov    al, byte ptr [esi]
        mov    cl, byte ptr [esi]
        mov    edx, dword ptr [lutCount]
        and    edx, 1
        je     loop1
        mov    ebp, dword ptr [ebx + eax*4]
        mov    edx, dword ptr [ebx + ecx*4]
        mov    dword ptr [edi], ebp
        mov    dword ptr [edi + 4], edx
        add    edi, 8
        add    esi, 1
        sub    dword ptr [lutCount], 1
        je     skip1
        mov    al, byte ptr [esi]
        mov    cl, byte ptr [esi]
    loop1:
        mov    ebp, dword ptr [ebx + eax*4]
        mov    edx, dword ptr [ebx + ecx*4]
        mov    al, byte ptr [esi + 1]
        mov    cl, byte ptr [esi + 1]
        mov    dword ptr [edi], ebp
        mov    dword ptr [edi + 4], edx
        mov    ebp, dword ptr [ebx + eax*4]
        mov    edx, dword ptr [ebx + ecx*4]
        mov    al, byte ptr [esi + 2]
        mov    cl, byte ptr [esi + 2]
        mov    dword ptr [edi + 8], ebp
        mov    dword ptr [edi + 0xc], edx
        add    edi, 0x10
        add    esi, 2
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreDual, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutStoreDualStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    dl, byte ptr [lutRowA]
        mov    dh, byte ptr [lutRowB]
        mov    cx, word ptr [lutAcc]
        mov    ebx, dword ptr [quantTable]
    loop1:
        mov    al, byte ptr [esi]
        mov    ah, dl
        mov    ebp, dword ptr [ebx + eax*4]
        mov    ah, dh
        mov    dword ptr [edi], ebp
        mov    ebp, dword ptr [ebx + eax*4]
        xor    eax, eax
        mov    dword ptr [edi + 4], ebp
        add    cx, word ptr [lutStep]
        add    edi, 8
        mov    al, ch
        mov    ch, 0
        add    esi, eax
        dec    dword ptr [lutCount]
        jne    loop1
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], cx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreDual, but adds both values into the entry. */
__declspec(naked) void LutAddDual(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    ecx, ecx
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ah, byte ptr [lutRowA]
        mov    ch, byte ptr [lutRowB]
        mov    ebx, dword ptr [quantTable]
        mov    al, byte ptr [esi]
        mov    cl, byte ptr [esi]
        mov    edx, dword ptr [lutCount]
        and    edx, 1
        je     loop1
        mov    ebp, dword ptr [ebx + eax*4]
        mov    edx, dword ptr [ebx + ecx*4]
        add    dword ptr [edi], ebp
        add    dword ptr [edi + 4], edx
        add    edi, 8
        add    esi, 1
        sub    dword ptr [lutCount], 1
        je     skip1
        mov    al, byte ptr [esi]
        mov    cl, byte ptr [esi]
    loop1:
        mov    ebp, dword ptr [ebx + eax*4]
        mov    edx, dword ptr [ebx + ecx*4]
        mov    al, byte ptr [esi + 1]
        mov    cl, byte ptr [esi + 1]
        add    dword ptr [edi], ebp
        add    dword ptr [edi + 4], edx
        mov    ebp, dword ptr [ebx + eax*4]
        mov    edx, dword ptr [ebx + ecx*4]
        mov    al, byte ptr [esi + 2]
        mov    cl, byte ptr [esi + 2]
        add    dword ptr [edi + 8], ebp
        add    dword ptr [edi + 0xc], edx
        add    edi, 0x10
        add    esi, 2
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutAddDual, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutAddDualStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    dl, byte ptr [lutRowA]
        mov    dh, byte ptr [lutRowB]
        mov    cx, word ptr [lutAcc]
        mov    ebx, dword ptr [quantTable]
    loop1:
        mov    al, byte ptr [esi]
        mov    ah, dl
        mov    ebp, dword ptr [ebx + eax*4]
        mov    ah, dh
        add    dword ptr [edi], ebp
        mov    ebp, dword ptr [ebx + eax*4]
        xor    eax, eax
        add    dword ptr [edi + 4], ebp
        add    cx, word ptr [lutStep]
        add    edi, 8
        mov    al, ch
        mov    ch, 0
        add    esi, eax
        dec    dword ptr [lutCount]
        jne    loop1
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], cx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Stores the quantTable value for volume row lutRowA in the first dword of each 8-byte entry
   and its negation in the second. */
__declspec(naked) void LutStoreNeg(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ecx, dword ptr [lutCount]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], edx
        neg    edx
        mov    dword ptr [edi + 4], edx
        add    edi, 8
        add    esi, 1
        sub    ecx, 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    al, byte ptr [esi + 1]
        add    esi, 2
        mov    ebp, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], edx
        neg    edx
        mov    dword ptr [edi + 4], edx
        mov    dword ptr [edi + 8], ebp
        neg    ebp
        mov    dword ptr [edi + 0xc], ebp
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreNeg, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutStoreNegStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    edx, edx
        xor    ebp, ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    bp, word ptr [lutStep]
        mov    dx, word ptr [lutAcc]
        mov    ecx, dword ptr [lutCount]
        and    ecx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        neg    ecx
        mov    dword ptr [edi + 4], ecx
        add    edi, 8
        add    esi, 1
        sub    dword ptr [lutCount], 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        neg    ecx
        mov    dword ptr [edi + 4], ecx
        add    edi, 8
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        neg    ecx
        mov    dword ptr [edi + 4], ecx
        add    edi, 8
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], dx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreNeg, but adds both values into the entry. */
__declspec(naked) void LutAddNeg(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ecx, dword ptr [lutCount]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], edx
        neg    edx
        add    dword ptr [edi + 4], edx
        add    edi, 8
        add    esi, 1
        sub    ecx, 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    al, byte ptr [esi + 1]
        add    esi, 2
        mov    ebp, dword ptr [ebx + eax*4]
        add    dword ptr [edi], edx
        neg    edx
        add    dword ptr [edi + 4], edx
        add    dword ptr [edi + 8], ebp
        neg    ebp
        add    dword ptr [edi + 0xc], ebp
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutAddNeg, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutAddNegStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    edx, edx
        xor    ebp, ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    bp, word ptr [lutStep]
        mov    dx, word ptr [lutAcc]
        mov    ecx, dword ptr [lutCount]
        and    ecx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        neg    ecx
        add    dword ptr [edi + 4], ecx
        add    edi, 8
        add    esi, 1
        sub    dword ptr [lutCount], 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        neg    ecx
        add    dword ptr [edi + 4], ecx
        add    edi, 8
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        neg    ecx
        add    dword ptr [edi + 4], ecx
        add    edi, 8
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], dx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Zeroes lutCount pairs of dwords at lutOut. */
__declspec(naked) void ClearLutPairs(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    edi, dword ptr [lutOut]
        mov    ecx, dword ptr [lutCount]
    loop1:
        mov    dword ptr [edi], eax
        mov    dword ptr [edi + 4], eax
        add    edi, 8
        dec    ecx
        jne    loop1
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Stores each quantTable value for volume row lutRowA into both dwords of its 8-byte entry. */
__declspec(naked) void LutStoreDup(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ecx, dword ptr [lutCount]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], edx
        mov    dword ptr [edi + 4], edx
        add    edi, 8
        add    esi, 1
        sub    ecx, 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    al, byte ptr [esi + 1]
        add    esi, 2
        mov    ebp, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], edx
        mov    dword ptr [edi + 4], edx
        mov    dword ptr [edi + 8], ebp
        mov    dword ptr [edi + 0xc], ebp
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreDup, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutStoreDupStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    edx, edx
        xor    ebp, ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    bp, word ptr [lutStep]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    dx, word ptr [lutAcc]
        mov    ecx, dword ptr [lutCount]
        and    ecx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        mov    dword ptr [edi + 4], ecx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        mov    dword ptr [edi + 4], ecx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        mov    dword ptr [edi], ecx
        mov    dword ptr [edi + 4], ecx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], dx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutStoreDup, but adds the value into both dwords of the entry. */
__declspec(naked) void LutAddDup(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    ecx, dword ptr [lutCount]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    edx, ecx
        and    edx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], edx
        add    dword ptr [edi + 4], edx
        add    edi, 8
        add    esi, 1
        sub    ecx, 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    edx, dword ptr [ebx + eax*4]
        mov    al, byte ptr [esi + 1]
        add    esi, 2
        mov    ebp, dword ptr [ebx + eax*4]
        add    dword ptr [edi], edx
        add    dword ptr [edi + 4], edx
        add    dword ptr [edi + 8], ebp
        add    dword ptr [edi + 0xc], ebp
        add    edi, 0x10
        sub    ecx, 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   As LutAddDup, but the source advances by lutStep per output entry, as in LutStoreRowStep. */
__declspec(naked) void LutAddDupStep(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        xor    eax, eax
        xor    edx, edx
        xor    ebp, ebp
        mov    esi, dword ptr [decodeSrc]
        mov    edi, dword ptr [lutOut]
        mov    bp, word ptr [lutStep]
        mov    ebx, dword ptr [quantTable]
        mov    ah, byte ptr [lutRowA]
        mov    dx, word ptr [lutAcc]
        mov    ecx, dword ptr [lutCount]
        and    ecx, 1
        je     loop1
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        add    dword ptr [edi + 4], ecx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 1
        je     skip1
    loop1:
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        add    dword ptr [edi + 4], ecx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        mov    al, byte ptr [esi]
        mov    ecx, dword ptr [ebx + eax*4]
        add    dword ptr [edi], ecx
        add    dword ptr [edi + 4], ecx
        xor    ecx, ecx
        add    edx, ebp
        mov    cl, dh
        add    edi, 8
        mov    dh, 0
        add    esi, ecx
        sub    dword ptr [lutCount], 2
        jne    loop1
    skip1:
        mov    dword ptr [decodeSrc], esi
        mov    dword ptr [lutOut], edi
        mov    word ptr [lutAcc], dx
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Converts the 32-bit samples in mixSrc to 16-bit words in decodeDst, clamped to the signed
   16-bit range. Each step reads two samples 8 bytes apart and writes two words. */
__declspec(naked) void PackMixMono16(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    edi, dword ptr [decodeDst]
        mov    ecx, dword ptr [mixCount]
        mov    esi, dword ptr [mixSrc]
        shr    ecx, 1
    loop1:
        mov    eax, dword ptr [esi]
        mov    ebx, dword ptr [esi + 8]
        mov    word ptr [edi], ax
        mov    word ptr [edi + 2], bx
        add    eax, 0x8000
        add    ebx, 0x8000
        cmp    eax, 0x10000
        jae    skip1
    loop2:
        cmp    ebx, 0x10000
        jae    skip3
    loop3:
        add    esi, 0x10
        add    edi, 4
        sub    ecx, 2
        jne    loop1
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        or     eax, eax
        js     skip2
        mov    word ptr [edi], 0x7fff
        jmp    loop2
    skip2:
        mov    word ptr [edi], 0x8000
        jmp    loop2
    skip3:
        or     ebx, ebx
        js     skip4
        mov    word ptr [edi + 2], 0x7fff
        jmp    loop3
    skip4:
        mov    word ptr [edi + 2], 0x8000
        jmp    loop3
    }
}

/* Original routine was hand-written assembly.
   As PackMixMono16, but writes each pair of clamped samples as one dword, the first sample in
   the low word. */
__declspec(naked) void PackMixStereo16(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    edi, dword ptr [decodeDst]
        mov    ecx, dword ptr [mixCount]
        mov    esi, dword ptr [mixSrc]
        shr    ecx, 2
        mov    eax, dword ptr [esi]
        mov    ebx, dword ptr [esi + 4]
    loop1:
        mov    edx, ebx
        add    esi, 8
        shl    edx, 0x10
        add    dx, ax
        mov    dword ptr [edi], edx
        add    eax, 0x8000
        add    ebx, 0x8000
        cmp    eax, 0x10000
        jae    skip1
    loop2:
        cmp    ebx, 0x10000
        jae    skip3
    loop3:
        add    edi, 4
        mov    eax, dword ptr [esi]
        mov    ebx, dword ptr [esi + 4]
        dec    ecx
        jne    loop1
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        or     eax, eax
        js     skip2
        mov    word ptr [edi], 0x7fff
        jmp    loop2
    skip2:
        mov    word ptr [edi], 0x8000
        jmp    loop2
    skip3:
        or     ebx, ebx
        js     skip4
        mov    word ptr [edi + 2], 0x7fff
        jmp    loop3
    skip4:
        mov    word ptr [edi + 2], 0x8000
        jmp    loop3
    }
}

/* Original routine was hand-written assembly.
   Converts mixScratch to unsigned bytes, biased by 0x80 and clamped to 0..255. Each step reads
   two samples 8 bytes apart and writes two bytes. */
__declspec(naked) void PackMixMono8(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    edi, dword ptr [decodeDst]
        mov    ecx, dword ptr [mixCount]
        mov    esi, dword ptr [mixSrc]
        mov    esi, dword ptr [mixScratch]
    loop1:
        mov    eax, dword ptr [esi]
        mov    ebx, dword ptr [esi + 8]
        add    eax, 0x80
        add    ebx, 0x80
        cmp    eax, 0x100
        jae    skip1
    loop2:
        cmp    ebx, 0x100
        jae    skip3
    loop3:
        mov    byte ptr [edi], al
        mov    byte ptr [edi + 1], bl
        add    esi, 0x10
        add    edi, 2
        sub    ecx, 2
        jne    loop1
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        or     eax, eax
        js     skip2
        mov    eax, 0xff
        jmp    loop2
    skip2:
        mov    eax, 0
        jmp    loop2
    skip3:
        or     ebx, ebx
        js     skip4
        mov    ebx, 0xff
        jmp    loop3
    skip4:
        mov    ebx, 0
        jmp    loop3
    }
}

/* Original routine was hand-written assembly.
   Converts adjacent sample pairs of mixScratch to unsigned bytes, each biased by 0x80 and
   clamped to 0..255. */
__declspec(naked) void PackMixStereo8(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    edi, dword ptr [decodeDst]
        mov    ecx, dword ptr [mixCount]
        mov    esi, dword ptr [mixSrc]
        shr    ecx, 1
        mov    esi, dword ptr [mixScratch]
    loop1:
        mov    eax, dword ptr [esi]
        mov    ebx, 0x80
        add    eax, 0x80
        add    ebx, dword ptr [esi + 4]
        cmp    eax, 0x100
        jae    skip1
    loop2:
        cmp    ebx, 0x100
        jae    skip3
    loop3:
        mov    byte ptr [edi], al
        add    esi, 8
        mov    byte ptr [edi + 1], bl
        add    edi, 2
        dec    ecx
        jne    loop1
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        or     eax, eax
        js     skip2
        mov    eax, 0xff
        jmp    loop2
    skip2:
        mov    eax, 0
        jmp    loop2
    skip3:
        or     ebx, ebx
        js     skip4
        mov    ebx, 0xff
        jmp    loop3
    skip4:
        mov    ebx, 0
        jmp    loop3
    }
}

/* Original routine was hand-written assembly.
   Mono echo: adds the delayed sample, scaled by reverbDepth, into each mixed sample and writes
   the mixed sample into the delay ring (delayLen dwords at delayBuf, wrapping). */
__declspec(naked) void MonoEcho(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    edi, dword ptr [delayPos]
        mov    esi, dword ptr [mixScratch]
        mov    ebp, dword ptr [delayBuf]
        xor    ecx, ecx
        mov    cl, byte ptr [reverbDepth]
        mov    edx, dword ptr [mixBufFrames]
        mov    dword ptr [sndWordsLeft], edx
    loop1:
        mov    edx, dword ptr [sndWordsLeft]
        or     edx, edx
        je     skip5
        mov    eax, edi
        sub    eax, ebp
        sar    eax, 2
        mov    ebx, dword ptr [delayLen]
        sub    ebx, eax
        mov    dword ptr [sndRunLeft], ebx
        cmp    edx, ebx
        jg     skip1
        mov    eax, edx
        jmp    skip2
    skip1:
        mov    eax, ebx
    skip2:
        sub    ebx, eax
        sub    edx, eax
        mov    dword ptr [sndRunLeft], ebx
        mov    dword ptr [sndWordsLeft], edx
        mov    ebx, eax
        mov    eax, dword ptr [esi]
        mov    dword ptr [edi], eax
        add    edi, 4
        dec    ebx
        or     ebx, ebx
        je     skip3
        mov    eax, ebx
        and    eax, 1
        je     loop2
        mov    eax, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], eax
        mov    eax, dword ptr [esi + 8]
        mov    dword ptr [edi], eax
        add    esi, 8
        add    edi, 4
        dec    ebx
        je     skip3
    loop2:
        mov    eax, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], eax
        mov    eax, dword ptr [esi + 8]
        mov    dword ptr [edi], eax
        mov    eax, dword ptr [edi + 4]
        sar    eax, cl
        add    dword ptr [esi + 8], eax
        mov    eax, dword ptr [esi + 0x10]
        mov    dword ptr [edi + 4], eax
        add    esi, 0x10
        add    edi, 8
        sub    ebx, 2
        jne    loop2
    skip3:
        mov    edx, dword ptr [sndRunLeft]
        or     edx, edx
        jne    skip4
        mov    edi, ebp
    skip4:
        mov    eax, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], eax
        add    esi, 8
        jmp    loop1
    skip5:
        mov    dword ptr [delayPos], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Stereo echo: as MonoEcho, but the delay ring holds the average of each left and right pair.
   The delayed sample is added to the left channel and subtracted from the right. */
__declspec(naked) void StereoEcho(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    edi, dword ptr [delayPos]
        mov    esi, dword ptr [mixScratch]
        mov    ebp, dword ptr [delayBuf]
        xor    ecx, ecx
        mov    cl, byte ptr [reverbDepth]
        mov    edx, dword ptr [mixBufFrames]
        mov    dword ptr [sndWordsLeft], edx
    loop1:
        mov    edx, dword ptr [sndWordsLeft]
        or     edx, edx
        je     skip5
        mov    eax, edi
        sub    eax, ebp
        sar    eax, 2
        mov    ebx, dword ptr [delayLen]
        sub    ebx, eax
        mov    dword ptr [sndRunLeft], ebx
        cmp    edx, ebx
        jg     skip1
        mov    eax, edx
        jmp    skip2
    skip1:
        mov    eax, ebx
    skip2:
        sub    ebx, eax
        sub    edx, eax
        mov    dword ptr [sndRunLeft], ebx
        mov    dword ptr [sndWordsLeft], edx
        mov    ebx, eax
        mov    eax, dword ptr [esi]
        add    eax, dword ptr [esi + 4]
        sar    eax, 1
        mov    dword ptr [edi], eax
        add    edi, 4
        dec    ebx
        or     ebx, ebx
        je     skip3
        mov    eax, ebx
        and    eax, 1
        je     loop2
        mov    eax, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], eax
        sub    dword ptr [esi + 4], eax
        add    esi, 8
        mov    eax, dword ptr [esi]
        add    eax, dword ptr [esi + 4]
        sar    eax, 1
        mov    dword ptr [edi], eax
        add    edi, 4
        dec    ebx
        je     skip3
    loop2:
        mov    eax, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], eax
        sub    dword ptr [esi + 4], eax
        mov    eax, dword ptr [esi + 8]
        add    eax, dword ptr [esi + 0xc]
        sar    eax, 1
        mov    dword ptr [edi], eax
        mov    eax, dword ptr [edi + 4]
        sar    eax, cl
        add    dword ptr [esi + 8], eax
        sub    dword ptr [esi + 0xc], eax
        mov    eax, dword ptr [esi + 0x10]
        add    eax, dword ptr [esi + 0x14]
        sar    eax, 1
        mov    dword ptr [edi + 4], eax
        add    esi, 0x10
        add    edi, 8
        sub    ebx, 2
        jne    loop2
    skip3:
        mov    edx, dword ptr [sndRunLeft]
        or     edx, edx
        jne    skip4
        mov    edi, ebp
    skip4:
        mov    eax, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], eax
        sub    dword ptr [esi + 4], eax
        add    esi, 8
        jmp    loop1
    skip5:
        mov    dword ptr [delayPos], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    }
}

/* Original routine was hand-written assembly.
   Mono delay: writes the input (scaled by reverbDepth) plus the delayed sample, less the
   delayed sample scaled by delayCoeff, into the delay ring, and adds the delayed sample into
   the mix. */
__declspec(naked) void MonoDelay(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    esi, dword ptr [mixScratch]
        mov    edi, dword ptr [delayPos]
        mov    eax, dword ptr [mixBufFrames]
        mov    dword ptr [sndSamples], eax
        xor    ecx, ecx
        mov    cl, byte ptr [reverbDepth]
        mov    ch, byte ptr [delayCoeff]
        mov    ebp, dword ptr [delayLen]
        shl    ebp, 2
        add    ebp, dword ptr [delayBuf]
        sub    ebp, 4
    loop1:
        mov    eax, dword ptr [esi]
        mov    edx, edi
        add    edi, 4
        cmp    edi, ebp
        jg     skip1
    loop2:
        mov    ebx, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], ebx
        add    esi, 8
        add    eax, ebx
        xchg   ch, cl
        sar    ebx, cl
        xchg   ch, cl
        sub    eax, ebx
        mov    dword ptr [edx], eax
        dec    dword ptr [sndSamples]
        jne    loop1
        mov    dword ptr [delayPos], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        mov    edi, dword ptr [delayBuf]
        jmp    loop2
    }
}

/* Original routine was hand-written assembly.
   Stereo delay: as MonoDelay, using the average of each left and right pair as the input. The
   delayed sample is added to the left channel and subtracted from the right. */
__declspec(naked) void StereoDelay(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    esi, dword ptr [mixScratch]
        mov    edi, dword ptr [delayPos]
        mov    eax, dword ptr [mixBufFrames]
        mov    dword ptr [sndSamples], eax
        xor    ecx, ecx
        mov    cl, byte ptr [reverbDepth]
        mov    ch, byte ptr [delayCoeff]
        mov    ebp, dword ptr [delayLen]
        shl    ebp, 2
        add    ebp, dword ptr [delayBuf]
        sub    ebp, 4
    loop1:
        mov    eax, dword ptr [esi]
        add    eax, dword ptr [esi + 4]
        sar    eax, 1
        mov    edx, edi
        add    edi, 4
        cmp    edi, ebp
        jg     skip1
    loop2:
        mov    ebx, dword ptr [edi]
        sar    eax, cl
        add    dword ptr [esi], ebx
        add    esi, 4
        sub    dword ptr [esi], ebx
        add    esi, 4
        add    eax, ebx
        xchg   ch, cl
        sar    ebx, cl
        xchg   ch, cl
        sub    eax, ebx
        mov    dword ptr [edx], eax
        dec    dword ptr [sndSamples]
        jne    loop1
        mov    dword ptr [delayPos], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        mov    edi, dword ptr [delayBuf]
        jmp    loop2
    }
}

/* Original routine was hand-written assembly.
   Mono reverb: three comb filters (comb1 to comb3 rings) are summed into workBuf, then an all-
   pass filter adds the result into the mixed samples, scaled by reverbDepth. */
__declspec(naked) void MonoReverb(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    esi, dword ptr [mixScratch]
        mov    edi, dword ptr [workBuf]
        mov    eax, dword ptr [mixBufFrames]
        mov    dword ptr [sndSamples], eax
        mov    ebp, dword ptr [comb1Len]
        shl    ebp, 2
        add    ebp, dword ptr [comb1Buf]
        sub    ebp, 4
        mov    dword ptr [sndEndB], ebp
        mov    ebp, dword ptr [comb2Len]
        shl    ebp, 2
        add    ebp, dword ptr [comb2Buf]
        sub    ebp, 4
        mov    dword ptr [sndEndC], ebp
        mov    ebp, dword ptr [comb3Len]
        shl    ebp, 2
        add    ebp, dword ptr [comb3Buf]
        sub    ebp, 4
        mov    dword ptr [sndEndD], ebp
    loop1:
        mov    eax, dword ptr [esi]
        add    esi, 8
        mov    dword ptr [sndCarry], eax
        mov    ecx, dword ptr [comb1Pos]
        mov    edx, ecx
        add    ecx, 4
        cmp    ecx, dword ptr [sndEndB]
        jg     skip1
    loop2:
        mov    dword ptr [comb1Pos], ecx
        mov    ebx, dword ptr [ecx]
        mov    ebp, ebx
        add    eax, ebx
        sar    ebx, 2
        sub    eax, ebx
        mov    dword ptr [edx], eax
        mov    ecx, dword ptr [comb2Pos]
        mov    edx, ecx
        add    ecx, 4
        cmp    ecx, dword ptr [sndEndC]
        jg     skip2
    loop3:
        mov    dword ptr [comb2Pos], ecx
        mov    ebx, dword ptr [ecx]
        add    ebp, ebx
        mov    eax, dword ptr [sndCarry]
        add    eax, ebx
        sar    ebx, 3
        sub    eax, ebx
        mov    dword ptr [edx], eax
        mov    ecx, dword ptr [comb3Pos]
        mov    edx, ecx
        add    ecx, 4
        cmp    ecx, dword ptr [sndEndD]
        jg     skip3
    loop4:
        mov    dword ptr [comb3Pos], ecx
        mov    ebx, dword ptr [ecx]
        add    ebp, ebx
        mov    eax, dword ptr [sndCarry]
        add    eax, ebx
        sar    ebx, 3
        sub    eax, ebx
        mov    dword ptr [edx], eax
        mov    dword ptr [edi], ebp
        add    edi, 4
        dec    dword ptr [sndSamples]
        jne    loop1
        mov    esi, dword ptr [workBuf]
        mov    edi, dword ptr [allPassPos]
        mov    ebx, dword ptr [mixScratch]
        xor    ecx, ecx
        mov    cl, byte ptr [reverbDepth]
        mov    eax, dword ptr [mixBufFrames]
        mov    dword ptr [sndSamples], eax
        mov    ebp, dword ptr [allPassLen]
        shl    ebp, 2
        add    ebp, dword ptr [allPassBuf]
        sub    ebp, 4
        mov    dword ptr [sndEndE], ebp
    loop5:
        mov    eax, dword ptr [esi]
        add    esi, 4
        mov    edx, edi
        add    edi, 4
        cmp    edi, dword ptr [sndEndE]
        jg     skip4
    loop6:
        mov    ebp, dword ptr [edi]
        add    eax, ebp
        shl    eax, 2
        sub    eax, ebp
        sar    eax, 2
        mov    dword ptr [edx], eax
        sub    ebp, eax
        sar    eax, 2
        add    ebp, eax
        sar    ebp, cl
        add    dword ptr [ebx], ebp
        add    ebx, 8
        dec    dword ptr [sndSamples]
        jne    loop5
        mov    dword ptr [allPassPos], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        mov    ecx, dword ptr [comb1Buf]
        jmp    loop2
    skip2:
        mov    ecx, dword ptr [comb2Buf]
        jmp    loop3
    skip3:
        mov    ecx, dword ptr [comb3Buf]
        jmp    loop4
    skip4:
        mov    edi, dword ptr [allPassBuf]
        jmp    loop6
    }
}


/* Original routine was hand-written assembly. It averages the two channels
   into the comb filters, then adds opposite reverb contributions to them. */
__declspec(naked) void StereoReverb(void)
{
    __asm {
        push   eax
        push   ebx
        push   ecx
        push   edx
        push   esi
        push   edi
        push   ebp
        mov    esi, dword ptr [mixScratch]
        mov    edi, dword ptr [workBuf]
        mov    eax, dword ptr [mixBufFrames]
        mov    dword ptr [sndSamples], eax
        mov    ebp, dword ptr [comb1Len]
        shl    ebp, 2
        add    ebp, dword ptr [comb1Buf]
        sub    ebp, 4
        mov    dword ptr [sndEndB], ebp
        mov    ebp, dword ptr [comb2Len]
        shl    ebp, 2
        add    ebp, dword ptr [comb2Buf]
        sub    ebp, 4
        mov    dword ptr [sndEndC], ebp
        mov    ebp, dword ptr [comb3Len]
        shl    ebp, 2
        add    ebp, dword ptr [comb3Buf]
        sub    ebp, 4
        mov    dword ptr [sndEndD], ebp
    loop1:
        mov    eax, dword ptr [esi]
        add    esi, 4
        add    eax, dword ptr [esi]
        add    esi, 4
        sar    eax, 1
        mov    dword ptr [sndCarry], eax
        mov    ecx, dword ptr [comb1Pos]
        mov    edx, ecx
        add    ecx, 4
        cmp    ecx, dword ptr [sndEndB]
        jg     skip1
    loop2:
        mov    dword ptr [comb1Pos], ecx
        mov    ebx, dword ptr [ecx]
        mov    ebp, ebx
        add    eax, ebx
        sar    ebx, 2
        sub    eax, ebx
        mov    dword ptr [edx], eax
        mov    ecx, dword ptr [comb2Pos]
        mov    edx, ecx
        add    ecx, 4
        cmp    ecx, dword ptr [sndEndC]
        jg     skip2
    loop3:
        mov    dword ptr [comb2Pos], ecx
        mov    ebx, dword ptr [ecx]
        add    ebp, ebx
        mov    eax, dword ptr [sndCarry]
        add    eax, ebx
        sar    ebx, 3
        sub    eax, ebx
        mov    dword ptr [edx], eax
        mov    ecx, dword ptr [comb3Pos]
        mov    edx, ecx
        add    ecx, 4
        cmp    ecx, dword ptr [sndEndD]
        jg     skip3
    loop4:
        mov    dword ptr [comb3Pos], ecx
        mov    ebx, dword ptr [ecx]
        add    ebp, ebx
        mov    eax, dword ptr [sndCarry]
        add    eax, ebx
        sar    ebx, 3
        sub    eax, ebx
        mov    dword ptr [edx], eax
        mov    dword ptr [edi], ebp
        add    edi, 4
        dec    dword ptr [sndSamples]
        jne    loop1
        mov    esi, dword ptr [workBuf]
        mov    edi, dword ptr [allPassPos]
        mov    ebx, dword ptr [mixScratch]
        xor    ecx, ecx
        mov    cl, byte ptr [reverbDepth]
        mov    eax, dword ptr [mixBufFrames]
        mov    dword ptr [sndSamples], eax
        mov    ebp, dword ptr [allPassLen]
        shl    ebp, 2
        add    ebp, dword ptr [allPassBuf]
        sub    ebp, 4
        mov    dword ptr [sndEndE], ebp
    loop5:
        mov    eax, dword ptr [esi]
        add    esi, 4
        mov    edx, edi
        add    edi, 4
        cmp    edi, dword ptr [sndEndE]
        jg     skip4
    loop6:
        mov    ebp, dword ptr [edi]
        add    eax, ebp
        shl    eax, 2
        sub    eax, ebp
        sar    eax, 2
        mov    dword ptr [edx], eax
        sub    ebp, eax
        sar    eax, 2
        add    ebp, eax
        sar    ebp, cl
        add    dword ptr [ebx], ebp
        add    ebx, 4
        sub    dword ptr [ebx], ebp
        add    ebx, 4
        dec    dword ptr [sndSamples]
        jne    loop5
        mov    dword ptr [allPassPos], edi
        pop    ebp
        pop    edi
        pop    esi
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        ret
    skip1:
        mov    ecx, dword ptr [comb1Buf]
        jmp    loop2
    skip2:
        mov    ecx, dword ptr [comb2Buf]
        jmp    loop3
    skip3:
        mov    ecx, dword ptr [comb3Buf]
        jmp    loop4
    skip4:
        mov    edi, dword ptr [allPassBuf]
        jmp    loop6
    }
}

