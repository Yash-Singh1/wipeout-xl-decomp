"""Semantic equivalence of a compiled function and the original (x86, MSVC 4.20).

Byte-exact matching is the goal, but large functions often differ only in
register allocation, stack-slot placement or instruction scheduling. This
module proves that two machine-code functions compute the same thing:

  * both are split into basic blocks and walked together from the entry;
  * each block pair is evaluated symbolically: register and stack-slot values
    become expressions over the block's inputs, loads depend on the earlier
    stores/calls they may alias, stores and calls are recorded as effects;
  * the pair must produce the same observable effects (so independent work
    may be reordered, dependent work may not), the same branch condition
    (possibly inverted, or with compare operands swapped) and, for every
    location the successor blocks read, a value that can be bound between
    the two programs; that binding (register / stack slot renaming) is carried
    along the edges and must be consistent at merges and loops.

Fallbacks prove private byte lifetimes, exact memory forwarding, dominating
path conditions, literal/projection joins, and uniquely grounded modular
recipes. Frame-pointer callees need binary non-retention and byte footprints;
relocation preserves complete accessible ranges. A binary-proved fixed-rounding
x87 conversion consumes only the top cell and preserves lower cells. Aliasing
stores, calls and REP operations remain in load histories when relevant;
unmatched live bits and unproved pointer extents refuse. A store with a resolved
return-address overlap also refuses.

Anything the evaluator doesn't model raises Unsupported, which counts as a
difference: the checker can reject an equivalent function but should not
accept a different one.

API: equivalent(cand_bytes, exe_bytes, addr, returns_value=True) -> (ok, reason)
Optional quick=True tries only the original tier; failures need the full search.
"""
import re
import hashlib
import os
import capstone
import collections
import bisect
import math
import threading
from capstone import x86_const as X

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True

M32 = 0xffffffff
REG32 = ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')
SUBREG = {
    'al': ('eax', 0, 8), 'ah': ('eax', 8, 8), 'ax': ('eax', 0, 16),
    'cl': ('ecx', 0, 8), 'ch': ('ecx', 8, 8), 'cx': ('ecx', 0, 16),
    'dl': ('edx', 0, 8), 'dh': ('edx', 8, 8), 'dx': ('edx', 0, 16),
    'bl': ('ebx', 0, 8), 'bh': ('ebx', 8, 8), 'bx': ('ebx', 0, 16),
    'si': ('esi', 0, 16), 'di': ('edi', 0, 16), 'bp': ('ebp', 0, 16), 'sp': ('esp', 0, 16),
}
for r in REG32:
    SUBREG[r] = (r, 0, 32)
CALLEE_SAVED = ('ebx', 'esi', 'edi', 'ebp')
SP = ('SP',)
CALLEE_POP = None        # set by the caller: direct call target -> bytes popped by its ret
IMPORT_NAME = None       # set by the caller: IAT address -> imported function name
VARARGS_IMPORTS = {'wsprintfA', 'wsprintfW', 'wvsprintfA'}
EXE_READ = None          # set by the caller: function address -> original bytes
CONST_READ = None        # set by the caller: (address, size) -> bytes if read-only data, else None
STACK_PROBE = None       # set by the caller: address -> True for the _chkstk stack probe
THUNK_IAT = None         # set by the caller: address of a `jmp [IAT]` import thunk -> IAT address, else None
PURE = None              # set by the caller: address -> True if the function touches no memory
X87_UNARY_POP = None     # binary-proved fixed-rounding TOP-only integer pop
FPU_OUT = None           # set by the caller: (function address, x87 depth on entry) -> depth at ret, or None
ADJ_FDIV = {}             # verified CRT /QIfdiv memory helpers: address -> (arg bytes, name)
FRAME_INPUT = None       # target, input register/argument -> (pointer spans, incoming-stack spans), or None
ARG_WIDTH = None         # set by the caller: function address -> {arg index: bytes the callee reads} or None
RETURN_MASK = None       # original binary callers' observed integer return bits; unknown is FULL
CC_SWAP = {'e': 'e', 'ne': 'ne', 'l': 'g', 'g': 'l', 'le': 'ge', 'ge': 'le',
           'b': 'a', 'a': 'b', 'be': 'ae', 'ae': 'be'}
CC_NEG = {'e': 'ne', 'ne': 'e', 'l': 'ge', 'ge': 'l', 'g': 'le', 'le': 'g',
          'b': 'ae', 'ae': 'b', 'a': 'be', 'be': 'a', 's': 'ns', 'ns': 's',
          'o': 'no', 'no': 'o', 'p': 'np', 'np': 'p'}
CC_ALIAS = {'z': 'e', 'nz': 'ne', 'c': 'b', 'nc': 'ae', 'nae': 'b', 'nb': 'ae',
            'nbe': 'a', 'na': 'be', 'nge': 'l', 'nl': 'ge', 'nle': 'g', 'ng': 'le',
            'pe': 'p', 'po': 'np'}


class Unsupported(Exception):
    pass


_digests = {}


def D(x):
    """short content digest of an expression/effect, memoised by object identity"""
    k = id(x)
    hit = _digests.get(k)
    if hit is not None and hit[0] is x:
        return hit[1]
    if isinstance(x, tuple):
        h = hashlib.blake2b(repr(tuple(D(c) if isinstance(c, tuple) else c for c in x)).encode(), digest_size=10).hexdigest()
    else:
        h = repr(x)
    _digests[k] = (x, h)
    return h


# ---------------------------------------------------------------- expressions

def C(v):
    return ('c', v & M32)


def is_c(x):
    return x[0] == 'c'


def lin_parts(x):
    if x[0] == 'c':
        return x[1], {}
    if x[0] == 'lin':
        return x[1], dict(x[2])
    return 0, {x: 1}


def mk_lin(const, terms):
    const &= M32
    terms = {k: c & M32 for k, c in terms.items() if c & M32}
    if not terms:
        return ('c', const)
    if const == 0 and len(terms) == 1:
        (k, c), = terms.items()
        if c == 1:
            return k
    return ('lin', const, tuple(sorted(terms.items(), key=lambda kv: repr(kv[0]))))


def add(*xs):
    const, terms = 0, {}
    for x in xs:
        c, t = lin_parts(x)
        const += c
        for k, v in t.items():
            terms[k] = terms.get(k, 0) + v
    return mk_lin(const, terms)


def scale(x, k):
    c, t = lin_parts(x)
    return mk_lin(c * k, {kk: v * k for kk, v in t.items()})


def neg(x):
    return scale(x, -1)


def sub(a, b):
    return add(a, neg(b))


def op(name, *args, comm=False):
    if name in ('and', 'or', 'xor') and args and all(is_c(x) for x in args):
        value = args[0][1]
        for operand in args[1:]:
            value = value & operand[1] if name == 'and' else \
                    value | operand[1] if name == 'or' else value ^ operand[1]
        return C(value)
    if name == 'not' and len(args) == 1 and is_c(args[0]):
        return C(~args[0][1])
    if name in ('shl', 'sal', 'shr', 'sar') and len(args) in (2, 3) and \
            all(is_c(x) for x in args[:2]) and (len(args) == 2 or args[2] in (1, 2, 4)):
        # The optional operand size is metadata, not another value. x86 masks
        # counts to five bits even for byte and word shifts.
        width = 8 * args[2] if len(args) == 3 else 32
        mask = (1 << width) - 1
        value, count = args[0][1] & mask, args[1][1] & 31
        if name == 'sar' and value & (1 << (width - 1)):
            value -= 1 << width
        return C(((value << count) if name in ('shl', 'sal') else value >> count) & mask)
    if name == 'or' and len(args) == 2:
        for x, c in (args, args[::-1]):
            if is_c(c) and c[1] == 0:
                return x
    if _EXTENDED and name in ('and', 'or') and len(args) == 2:
        for x, c in (args, args[::-1]):
            if is_c(c) and x[0] == 'ext' and x[1] == 0 and 0 < x[2] <= 32:
                # Extraction zeroes every bit above its width. The literal
                # may therefore select none or all of the remaining live bits.
                mask = (1 << x[2]) - 1
                selected = c[1] & mask
                if name == 'and' and selected == 0:
                    return C(0)
                if name == 'and' and selected == mask:
                    return x
                if name == 'or' and selected == mask:
                    return c
    if _EXTENDED and name == 'and':
        a, b = args
        for x, c in ((a, b), (b, a)):
            if is_c(c):
                if c[1] == 0:
                    return C(0)
                if (c[1] & (c[1] + 1)) == 0:
                    return ext(x, 0, c[1].bit_length())
    if name == 'fl' and len(args) == 3 and args[0] == 'and':
        # TEST/AND flags depend only on bits selected by the constant mask.
        a, b = args[1:]
        if is_c(b) and not is_c(a):
            a, b = b, a
        if is_c(a):
            if b[0] != 'ld' and not (b[0] == 'ext' and b[3][0] == 'ld'):
                b = ext(b, 0, a[1].bit_length()) if a[1] else C(0)
            k = a[1].bit_length() - 1
            if a[1] and a[1] == 1 << k and 0 < k < 8:
                # a single-bit test below bit 8: ZF, SF (0), PF (one bit set or none),
                # CF and OF (0) are those of testing that bit alone with mask 1
                a, b = C(1), ext(b, k, 1)
            args = ('and', a, b)
    if comm and name in ('or', 'xor'):
        # associative as well as commutative: (a | b) | c and a | (b | c) are one value
        flat = []
        for x in args:
            flat.extend(x[1:] if isinstance(x, tuple) and x and x[0] == name else (x,))
        args = flat
    if _EXTENDED and name == 'or':
        # x | (x & y) = x, including a masked slice of the same read.
        args = tuple(dict.fromkeys(args))
        args = tuple(x for x in args if not (is_c(x) and x[1] == 0) and
                     not (x[0] == 'and' and any(term in args for term in x[1:])))
        if not args:
            return C(0)
        if len(args) == 1:
            return args[0]
    if comm:
        args = tuple(sorted(args, key=repr))
    return (name,) + tuple(args)


def mul(a, b):
    if is_c(a):
        return scale(b, a[1])
    if is_c(b):
        return scale(a, b[1])
    # Integer low-32-bit products form a commutative, associative operation.
    # x87 multiplication uses separate expression tags and never comes here.
    factors = []
    for x in (a, b):
        factors.extend(x[1:] if x[0] == 'mul' else (x,))
    return op('mul', *factors, comm=True)


def literal_flags(flags, size, result, carry=None):
    """Keep width when data or ADC/SBB's integer operands become literal."""
    return ('literal-flags', size, ext(result, 0, 8 * size), flags, carry) \
            if is_c(result) or (flags[0:2] in (('fl', 'adc'), ('fl', 'sbb')) and
                               is_c(flags[2]) and is_c(flags[3])) else flags


def ext(x, lo, n):
    """Extract n bits starting at lo, with all higher result bits zero."""
    if n == 32 and lo == 0:
        return x
    if lo == 0 and ((x[0] == 'in' and _is_slot(x[1]) and n >= 8 * x[1][2]) or
                    (x[0] == 'ld' and n >= 8 * x[1])):
        return x                                # a narrow slot or load already fits
    if x[0] == 'c':
        return C((x[1] >> lo) & ((1 << n) - 1))
    if _EXTENDED and x[0] == 'in' and _is_slot(x[1]) and 0 < x[1][2] <= 4 and \
            lo >= 0 and lo % 8 == 0 and n in (8, 16) and lo + n <= 8 * x[1][2]:
        # A bounded byte or word view names exactly those original stack bytes.
        # Frame relocation has already selected the stored location.
        return ('in', ('s', x[1][1] + lo // 8, n // 8))
    if x[0] == 'ld' and lo == 0 and n in (8, 16) and 8 * x[1] > n:
        # the low bytes of a wider load are a narrower load of the same address
        # (little-endian); its dependencies are a superset, so this only merges
        # loads that see the same stores
        return ('ld', n // 8, x[2], x[3])
    if x[0] == 'ld' and x[1] > 1 and 0 <= lo and 0 < n and lo + n <= 8:
        # Any field wholly in the low byte sees the same bits as a byte load.
        # Retain the wider read's complete dependency history.
        return ext(('ld', 1, x[2], x[3]), lo, n)
    if x[0] == 'ins':
        _, base, ilo, in_, v = x
        if ilo == lo and in_ == n:
            return v
        if lo + n <= ilo or ilo + in_ <= lo:
            return ext(base, lo, n)
        if ilo <= lo and lo + n <= ilo + in_:
            return ext(v, lo - ilo, n)          # entirely inside the inserted field
        # the window spans several inserted fields (e.g. al and ah written
        # separately): rebuild it from the fields, so the underlying register only
        # appears where some of its bits really survive
        fields, b = [], x
        while b[0] == 'ins':
            fields.append((b[2], b[3], b[4]))
            b = b[1]
        covered = [False] * n
        for flo, fn, _ in fields:
            for k in range(max(lo, flo), min(lo + n, flo + fn)):
                covered[k - lo] = True
        out = C(0) if all(covered) else ext(b, lo, n)
        for flo, fn, fv in reversed(fields):         # innermost first
            a, z = max(lo, flo), min(lo + n, flo + fn)
            if a < z:
                out = ins(out, a - lo, z - a, ext(fv, a - flo, z - a))
        return out
    if x[0] == 'cat':
        # Stack bytes assembled from several stores, least significant first.
        # Extract exactly the intersecting fields, with zero beyond the cat.
        parts, offset = [], 0
        for width, value in x[1]:
            a, z = max(lo, offset), min(lo + n, offset + width)
            if a < z:
                parts.append((z - a, ext(value, a - offset, z - a)))
            offset += width
        if not parts:
            return C(0)
        if len(parts) == 1:
            return parts[0][1]
        return cat(parts)
    if x[0] in ('zx', 'sx') and lo == 0 and n == x[1]:
        return x[2]
    if _EXTENDED and x[0] == 'lin' and 0 < lo and 0 < n and lo + n <= 32:
        # A common factor 2**lo leaves those low bits zero, so extracting
        # above them divides every coefficient exactly before projecting.
        # Unsigned modulo-2**32 coefficients also preserve negative products.
        mask = (1 << lo) - 1
        if not x[1] & mask and all(not coefficient & mask for _, coefficient in x[2]):
            return ext(mk_lin(x[1] >> lo, {term: coefficient >> lo
                                         for term, coefficient in x[2]}), 0, n)
    if x[0] == 'lin' and lo == 0:
        if _EXTENDED:
            # A shared power of two is a left shift. Only the remaining low
            # source bits can reach this window; keep their original load history.
            values = [x[1]] + [coefficient for _, coefficient in x[2]]
            zero_bits = min(((value & -value).bit_length() - 1 for value in values if value), default=0)
            if zero_bits:
                if zero_bits >= n:
                    return C(0)
                inner = mk_lin(x[1] >> zero_bits,
                               {term: coefficient >> zero_bits for term, coefficient in x[2]})
                return scale(ext(inner, 0, n - zero_bits), 1 << zero_bits)
        # the low n bits of a sum (with constant factors) only depend on the low n
        # bits of its terms: drop wrappers that only touch higher bits
        def low(t):
            while True:
                if t[0] == 'ins' and t[2] == 0 and t[3] >= n:
                    t = t[4]
                elif t[0] == 'ext' and t[1] == 0 and t[2] >= n:
                    t = t[3]
                elif t[0] in ('zx', 'sx') and t[1] >= n:
                    t = t[2]
                else:
                    return t
        terms = dict(x[2])
        const, new = x[1], {}
        for t, k in terms.items():
            # a stripped wrapper may expose a sum: fold it into this one
            c2, t2 = lin_parts(low(t))
            const += c2 * k
            for kk, vv in t2.items():
                new[kk] = (new.get(kk, 0) + vv * k) & M32
        # likewise the constant and the factors only matter modulo 2**n
        # (x + 0xffff and x - 1 agree in their low 16 bits)
        mask = (1 << n) - 1
        const &= mask
        new = {t: k & mask for t, k in new.items() if k & mask}
        if new != terms or const != x[1]:
            return ext(mk_lin(const, new), 0, n)
    if x[0] == 'zx' and lo == 0 and n > x[1]:
        return ('zx', x[1], x[2]) if n == 32 else op('zxn', n, x[1], x[2])
    if _EXTENDED and x[0] in ('and', 'or'):
        # Both operations act independently on each bit, so a projection
        # observes only the same window in each operand.
        return op(x[0], *(ext(v, lo, n) for v in x[1:]), comm=True)
    if _EXTENDED and x[0] == 'shl' and is_c(x[2]):
        # Project only a window inside the shifted operand's x86 width.
        # SHL masks its count to five bits, including byte and word operands.
        # Extraction retains the source expression and its memory dependencies.
        width = x[3] * 8 if len(x) == 4 else 32
        if 0 <= lo and 0 < n and lo + n <= width:
            shift = x[2][1] & 31
            if lo >= shift:
                return ext(x[1], lo - shift, n)
            if lo + n <= shift:
                return C(0)
            # The window starts in the zero-filled low bits and ends in the
            # source. This product fits in n bits and needs no signed extension.
            return scale(ext(x[1], 0, lo + n - shift), 1 << (shift - lo))
    if x[0] == 'ext':
        # ('ext', lo0, n0, X) is bits lo0..lo0+n0 of X, zero above n0
        lo0, n0, inner = x[1], x[2], x[3]
        if lo + n <= n0:
            return ext(inner, lo0 + lo, n)
        if lo >= n0:
            return C(0)
        if lo == 0 and n >= n0:
            return x                            # already fits in n bits
    return ('ext', lo, n, x)


def cat(parts):
    """Concatenate little-endian fields, folding only literal integer bytes."""
    parts = tuple(parts)
    if parts and sum(width for width, _ in parts) <= 32 and all(
            width > 0 and is_c(value) for width, value in parts):
        result, offset = 0, 0
        for width, value in parts:
            result |= (value[1] & ((1 << width) - 1)) << offset
            offset += width
        return C(result)
    return ('cat', parts)


def ins(base, lo, n, v):
    if n == 32:
        return v
    if 0 <= lo and 0 < n and lo + n <= 32 and is_c(base) and is_c(v):
        mask = ((1 << n) - 1) << lo
        return C((base[1] & ~mask) | ((v[1] << lo) & mask))
    if _EXTENDED and 0 <= lo and 0 < n and lo + n <= 32 and ext(base, lo, n) == ext(v, 0, n):
        # Reinserting the same field leaves all 32 bits unchanged.
        return base
    if lo == 0 and base == ('c', 0):
        return zx(n, v)
    if base[0] == 'ins' and base[2] == lo and base[3] == n:
        return ins(base[1], lo, n, v)
    if _EXTENDED and 0 <= lo and 0 < n and lo + n <= 32:
        base = sliced_insert_base(base, lo, n)
        if base == C(0) and lo == 0:
            return zx(n, v)
    return ('ins', base, lo, n, v)


def sliced_insert_base(base, lo, n):
    """Reuse a leaf slice only when every bit outside an insert agrees exactly.

    Literals, extracts, inserts and AND/OR are evaluated bit by bit. All other
    expressions are opaque leaves, including a load's full dependency history.
    The inserted window is unobserved here; no other output bit may be waived.
    """
    root = base
    while root[0] == 'ext':
        root = root[3]
    if root[0] not in ('and', 'or'):
        return base
    memo = {}

    def combine(tag, terms):
        annihilator, identity = (0, 1) if tag == 'and' else (1, 0)
        if annihilator in terms:
            return annihilator
        terms = set(t for t in terms if t != identity)
        flat = set()
        for t in terms:
            flat.update(t[1] if isinstance(t, tuple) and t[0] == tag else (t,))
        opposite = 'or' if tag == 'and' else 'and'
        terms = {t for t in flat if not (isinstance(t, tuple) and t[0] == opposite and
                                        any(part in flat for part in t[1]))}
        if not terms:
            return identity
        if len(terms) == 1:
            return next(iter(terms))
        return (tag, tuple(sorted(terms, key=repr)))

    def bit(value, index):
        key = (id(value), index)
        if key in memo:
            return memo[key]
        if value[0] == 'c':
            result = (value[1] >> index) & 1
        elif value[0] == 'ext':
            result = bit(value[3], value[1] + index) if index < value[2] else 0
        elif value[0] == 'ins':
            result = bit(value[4], index - value[2]) if value[2] <= index < value[2] + value[3] \
                     else bit(value[1], index)
        elif value[0] in ('and', 'or'):
            result = combine(value[0], [bit(v, index) for v in value[1:]])
        else:
            result = ('bit', value, index)
        memo[key] = result
        return result

    observed = [(i, bit(base, i)) for i in range(32) if not lo <= i < lo + n]
    live = [(i, b) for i, b in observed if b != 0]
    if not live:
        return C(0)
    # A slice can supply these bits only if every nonzero bit names the same
    # leaf at the same relative offset. Literal ones and Boolean formulas
    # remain in the original expression when they cannot be represented so.
    if any(not isinstance(b, tuple) or b[0] != 'bit' for _, b in live):
        return base
    first_index, first_bit = live[0]
    leaf, offset = first_bit[1], first_bit[2] - first_index
    width = live[-1][0] + 1
    if offset < 0 or offset + width > 32:
        return base
    if any(b != (('bit', leaf, offset + i) if i < width else 0) for i, b in observed):
        return base
    return ext(leaf, offset, width)


def zx(n, v):
    """zero-extend the low n bits of v: the same value as ext(v, 0, n)"""
    return ext(v, 0, n)


def sx(n, v):
    if v[0] == 'c':
        x = v[1] & ((1 << n) - 1)
        if x >> (n - 1):
            x -= 1 << n
        return C(x)
    return ('sx', n, ext(v, 0, n))


def stack_off(addr):
    """frame offset if addr is entry-esp + constant"""
    c, t = lin_parts(addr)
    if t == {SP: 1}:
        c &= M32
        return c - (1 << 32) if c & 0x80000000 else c
    return None


def has_sp(x):
    if x == SP:
        return True
    if isinstance(x, tuple):
        return any(isinstance(a, tuple) and has_sp(a) for a in x)
    return False


# ---------------------------------------------------------------- aliasing

NOALIAS = False     # the file was built with /Oa or /Ow (set by for_file)
_ASSUME = False     # inside the retry that relies on it
_EXTENDED = False   # bounded edge recipes / path facts, tried after the original proof
_DEAD_STORES = False  # CFG-proven overwrites, tried after the existing proof tiers
_PRECISE_FRAME = False  # bounded local bytes, including proved non-retaining callees
_LOCAL_LIFETIMES = False  # retire overwritten local bytes after indexed uses end
_EDGE_HISTORY = False  # bounded correlated predecessor histories
_JOIN_RECIPES = False  # join and endpoint refinement, after the original proof tiers
_FRAME_FORWARD = True  # retry actual frame reads when holder choices differ
_PROJECTED_FRAME = False  # exact private byte views after all existing proof tiers
_PROOF_LOCK = threading.RLock()   # entropy judges can invoke the checker concurrently


NOALIAS_FUNCS = set()  # functions inside a #pragma optimize("a", on) region of their file


def for_file(src, target='d3d'):
    """the original was compiled assuming that stores through pointers never touch
    named globals (/Oa, or /Ow between calls): proofs may rely on the same contract.
    A function the draft wraps in #pragma optimize("a", on) declares the same for
    itself (the exe's own scheduling relies on it). The contract is the one of the
    exe being compared: another target builds the same file with its own flags."""
    global NOALIAS
    try:
        import symbols
        NOALIAS = any(f in ('/Oa', '/Ow') for f in symbols.flags_for(src, target))
    except Exception:
        NOALIAS = False
    NOALIAS_FUNCS.clear()
    try:
        import re
        on = False
        table = symbols.load(symbols.pv_paths(target)[0]) if target in symbols.TARGETS else symbols.load()
        for line in open(src, errors='replace'):
            m = re.match(r'\s*#\s*pragma\s+optimize\s*\(\s*"a"\s*,\s*(on|off)\s*\)', line)
            if m:
                on = m.group(1) == 'on'
                continue
            f = re.match(r'[A-Za-z_][\w\s\*]*?\b(\w+)\s*\([^;]*$', line)
            if on and f and f.group(1) in table.by_name:
                NOALIAS_FUNCS.add(table.by_name[f.group(1)][0])
    except Exception:
        pass


def unsigned_bounds(x):
    """Conservative unsigned bounds for explicitly narrow integer values."""
    if is_c(x):
        return x[1], x[1]
    if x[0] == 'ext':
        return 0, (1 << x[2]) - 1
    if x[0] == 'ld' and x[1] < 4:
        return 0, (1 << (8 * x[1])) - 1
    if x[0] == 'in' and _is_slot(x[1]) and x[1][2] < 4:
        return 0, (1 << (8 * x[1][2])) - 1
    if x[0] == 'and':
        for v in x[1:]:
            if is_c(v):
                return 0, v[1]
    return 0, M32


def address_intervals(addr, size):
    """Byte ranges of a bounded linear address, including modulo-32-bit wrap."""
    if size <= 0:
        return [(0, 1 << 32)]
    lo, terms = lin_parts(addr)
    hi = lo + size - 1
    for term, coefficient in terms.items():
        a, b = unsigned_bounds(term)
        k = coefficient if coefficient < 0x80000000 else coefficient - (1 << 32)
        lo += min(k * a, k * b)
        hi += max(k * a, k * b)
    if hi - lo >= M32:
        return [(0, 1 << 32)]
    first, last = lo & M32, hi & M32
    return [(first, last + 1)] if first <= last else [(first, 1 << 32), (0, last + 1)]


def may_alias(a1, s1, a2, s2):
    c1, t1 = lin_parts(a1)
    c2, t2 = lin_parts(a2)
    if _EXTENDED and all(b <= c or d <= a for a, b in address_intervals(a1, s1)
           for c, d in address_intervals(a2, s2)):
        return False
    if _ASSUME and ((not t1 and t2 and SP not in t2) or (not t2 and t1 and SP not in t1)):
        return False        # a named global and a store/load through a pointer
    if _ASSUME and ((t1 == {SP: 1} and t2 and SP not in t2) or (t2 == {SP: 1} and t1 and SP not in t1)):
        return False        # likewise a named local (fixed frame slot) and a pointer access
    if _ASSUME and len(t1) == 1 and len(t2) == 1 and c1 >= 0x10000 and c2 >= 0x10000:
        (x1, k1), = t1.items()
        (x2, k2), = t2.items()
        if x1 == x2 and k1 != k2 and SP not in t1:
            # two global arrays indexed by the same variable with different element
            # sizes: different named objects (one object is never reached through two
            # names under the contract)
            return False
    if _ASSUME and t1 and t2 and SP not in t1 and SP not in t2 and not (set(t1) & set(t2)) and \
            c1 < 0x10000 and c2 < 0x10000:
        # two different pointers (fields off distinct bases): the same contract says
        # one object is not reached through two names
        return False
    if t1 == t2:
        d = (c2 - c1) & M32
        d = d - (1 << 32) if d & 0x80000000 else d
        return not (d >= s1 or -d >= s2)
    # A common divisor of every symbolic coefficient fixes both addresses'
    # residues. Include 2**32 so wrapping arithmetic preserves those residues.
    # An unknown pointer base has coefficient 1 and disables this test.
    modulus = 1 << 32
    for coefficient in (*t1.values(), *t2.values()):
        modulus = math.gcd(modulus, coefficient)
    if modulus > 1 and s1 <= modulus and s2 <= modulus:
        distance = (c2 - c1) % modulus
        if s1 <= distance <= modulus - s2:
            return False
    sp1, sp2 = SP in t1, SP in t2
    if sp1 != sp2 and not (len(t1) > (1 if sp1 else 0) and sp1) and not (len(t2) > (1 if sp2 else 0) and sp2):
        # stack vs global constant address
        if (not t1 or t1 == {SP: 1}) and (not t2 or t2 == {SP: 1}):
            return False
    return True


def memory_dependencies(effects, address, size):
    """Keep the complete history of effects that can change these bytes.

    A store is excluded only when the existing byte-range alias proof separates
    it from the load or store. Calls and unbounded string operations are always
    barriers; their destinations and complete byte ranges are unknown.
    """
    return tuple(sorted(D(effect) for effect in effects
                        if effect[0] in ('call', 'str') or
                        (effect[0] == 'st' and may_alias(effect[2], effect[1], address, size))))


# ---------------------------------------------------------------- decoding

def k_next_is_frame(block, ins):
    """the instruction after ins in block is mov ebp, esp"""
    for a, b in zip(block, block[1:]):
        if a is ins:
            return b.mnemonic == 'mov' and b.op_str == 'ebp, esp'
    return False


class Func:
    def __init__(self, code, addr):
        self.code = code
        self.addr = addr
        self.end = addr + len(code)
        self.ins = {}
        self.blocks = {}      # start -> list of insns
        self.succ = {}
        self.dead_global_addresses = set()
        self.fdepth = {}      # block start -> x87 stack depth on entry
        self.byte_tables = {}  # switch byte index table address -> number of entries
        self._decode()
        self.reg_iat = self._reg_imports()
        self.call_iat = self._call_imports()
        self.frame_floor = self._frame_floor()

    def _frame_floor(self):
        """esp offset of the steady frame (after the prologue): from an epilogue's pops
        and `add esp, N` before ret, or for ebp frames from the prologue's `sub esp, N`
        plus the pops before `leave`"""
        sub_n = None
        entry = self.blocks.get(self.addr, [])
        seen_ebp = False
        for i in entry:
            if i.mnemonic == 'mov' and i.op_str == 'ebp, esp':
                seen_ebp = True
            elif seen_ebp and i.mnemonic in ('sub', 'add') and i.operands[0].type == X.X86_OP_REG and \
                    md.reg_name(i.operands[0].reg) == 'esp' and i.operands[1].type == X.X86_OP_IMM:
                n = i.operands[1].imm
                sub_n = n if i.mnemonic == 'sub' else (-n) & M32 if n & 0x80000000 else -n
                break
        floors = []
        for blk in self.blocks.values():
            if blk[-1].mnemonic != 'ret':
                continue
            n = 0
            leave = False
            body = blk[:-1]
            for k in range(len(body) - 1, -1, -1):
                i = body[k]
                if i.mnemonic == 'pop':
                    n += 4
                elif i.mnemonic == 'add' and i.operands[0].type == X.X86_OP_REG and md.reg_name(i.operands[0].reg) == 'esp' \
                        and i.operands[1].type == X.X86_OP_IMM:
                    # caller cleanup of the call just before it is not part of the frame
                    prev = next((p for p in reversed(body[:k]) if p.mnemonic in ('call', 'push', 'pop') or
                                 any(md.reg_name(r) == 'esp' for r in p.regs_access()[1])), None)
                    if prev is not None and prev.mnemonic == 'call':
                        break
                    n += i.operands[1].imm
                elif i.mnemonic == 'leave' and n == 0:
                    leave = True
                elif i.mnemonic in ('call', 'push', 'leave') or \
                        any(md.reg_name(r) in ('esp', 'ebp') for r in i.regs_access()[1]):
                    break
                # anything else (stores, loads, arithmetic scheduled into the
                # epilogue) leaves esp alone
            if leave:
                if sub_n is None and not seen_ebp:
                    continue
                floors.append(-4 - (sub_n or 0) - n)
            else:
                floors.append(-n)
        # the prologue's own view: callee-saved pushes and `sub esp, N` before the
        # first argument push or call. An epilogue can hide the frame size when one
        # `add esp, N` frees both a call's arguments and the locals.
        restored = set()
        self.save_offs = set()      # where the prologue saves callee-saved registers
        for blk in self.blocks.values():
            if blk[-1].mnemonic == 'ret':
                for i in blk:
                    if i.mnemonic == 'pop' and i.operands[0].type == X.X86_OP_REG:
                        restored.add(md.reg_name(i.operands[0].reg))
        pro, d = None, 0
        saved = set()
        written = set()
        for i in entry:
            m = i.mnemonic
            # a callee-saved register parked in a local before it is first written
            # (mov [esp+n], ebx instead of push ebx) is a save as well
            if m == 'mov' and len(i.operands) == 2 and i.operands[0].type == X.X86_OP_MEM and \
                    i.operands[1].type == X.X86_OP_REG and i.operands[0].mem.base and \
                    md.reg_name(i.operands[0].mem.base) == 'esp' and not i.operands[0].mem.index:
                r = md.reg_name(i.operands[1].reg)
                if r in ('ebx', 'esi', 'edi', 'ebp') and r not in written and r not in saved:
                    self.save_offs.add(d + i.operands[0].mem.disp)
                    saved.add(r)
            for w in i.regs_access()[1]:
                n = md.reg_name(w)
                written.add(SUBREG.get(n, (n,))[0])
            r = md.reg_name(i.operands[0].reg) if m == 'push' and i.operands[0].type == X.X86_OP_REG else None
            if r == 'ebp' and k_next_is_frame(entry, i):
                # push ebp; mov ebp, esp: the frame pointer's own save (leave restores it)
                saved.add(r)
                d -= 4
                pro = d
                self.save_offs.add(d)
            elif r in ('ebx', 'esi', 'edi', 'ebp') and r in restored and r not in saved:
                saved.add(r)            # each register is saved once; a second push is an argument
                d -= 4
                pro = d
                self.save_offs.add(d)
            elif m in ('sub', 'add') and i.operands[0].type == X.X86_OP_REG and \
                    md.reg_name(i.operands[0].reg) == 'esp' and i.operands[1].type == X.X86_OP_IMM:
                n = i.operands[1].imm
                n = n - (1 << 32) if n & 0x80000000 else n
                d += -n if m == 'sub' else n
                pro = d
            elif m in ('push', 'call', 'pop', 'leave', 'ret') or m.startswith('j'):
                break
            elif m == 'mov' and i.op_str == 'ebp, esp':
                continue
        if pro is not None and pro < 0:
            floors.append(pro)
        # early-exit paths may return before the prologue: the steady frame is the
        # deepest one. Too deep is safe (a wrong guess shows up as esp != entry at
        # ret); too shallow would let locals pass for outgoing arguments.
        return min(floors) if floors else None

    def _reg_imports(self):
        """registers whose only loads (ignoring push/pop save-restores) are from one
        constant address, typically an import table entry: `call reg` calls that import"""
        loads, other = {}, set()
        for blk in self.blocks.values():
            for i in blk:
                if i.mnemonic == 'mov' and len(i.operands) == 2 and i.operands[0].type == X.X86_OP_REG and \
                        i.operands[1].type == X.X86_OP_MEM and i.operands[1].mem.base == 0 and i.operands[1].mem.index == 0:
                    loads.setdefault(md.reg_name(i.operands[0].reg), set()).add(i.operands[1].mem.disp & M32)
                    continue
                if i.mnemonic in ('pop', 'push', 'call'):
                    continue
                _, w = i.regs_access()
                for r in w:
                    n = md.reg_name(r)
                    other.add(SUBREG.get(n, (n,))[0])
        return {r: next(iter(a)) for r, a in loads.items() if len(a) == 1 and r not in other}

    def _call_imports(self):
        """`call reg` address -> the constant address (an import table entry) the
        register was loaded from on every path reaching the call"""
        entry = {self.addr: {}}
        work = [self.addr]

        def step(state, i, calls=None):
            ops = i.operands
            if i.mnemonic == 'call':
                if calls is not None and ops[0].type == X.X86_OP_REG:
                    r = md.reg_name(ops[0].reg)
                    if r in state:
                        calls[i.address] = state[r]
                for r in ('eax', 'ecx', 'edx'):
                    state.pop(r, None)
                return
            if i.mnemonic == 'mov' and len(ops) == 2 and ops[0].type == X.X86_OP_REG and \
                    ops[1].type == X.X86_OP_MEM and ops[1].mem.base == 0 and ops[1].mem.index == 0 and \
                    ops[1].mem.segment == 0:
                state[md.reg_name(ops[0].reg)] = ops[1].mem.disp & M32
                return
            if i.mnemonic == 'push':
                return
            _, w = i.regs_access()
            for r in w:
                n = md.reg_name(r)
                state.pop(SUBREG.get(n, (n,))[0], None)

        while work:
            s = work.pop()
            state = dict(entry[s])
            for i in self.blocks[s]:
                step(state, i)
            for t in self.succ.get(s, []):
                if t not in self.blocks:
                    continue
                if t not in entry:
                    entry[t] = dict(state)
                    work.append(t)
                    continue
                new = {r: v for r, v in entry[t].items() if state.get(r) == v}
                if new != entry[t]:
                    entry[t] = new
                    work.append(t)
        calls = {}
        for s in entry:
            state = dict(entry[s])
            for i in self.blocks[s]:
                step(state, i, calls)
        return calls

    def caller_cleans(self, call_addr):
        """the first instruction after the call that changes esp, before any branch or
        other call, is `add esp, imm`"""
        a = call_addr + self.insn(call_addr).size
        for _ in range(16):
            if not self.addr <= a < self.end:
                return False
            i = self.insn(a)
            m = i.mnemonic
            if m == 'call' or m == 'ret' or m.startswith('j') or m in ('loop', 'jecxz', 'push', 'pop', 'leave'):
                return False
            _, w = i.regs_access()
            if any(md.reg_name(r) == 'esp' for r in w):
                return m == 'add' and i.operands[1].type == X.X86_OP_IMM
            a += i.size
        return False

    def insn(self, a):
        if a not in self.ins:
            if not self.addr <= a < self.end:
                raise Unsupported('jump outside function to %x' % a)
            i = next(md.disasm(self.code[a - self.addr:a - self.addr + 16], a, 1), None)
            if i is None:
                raise Unsupported('undecodable at %x' % a)
            self.ins[a] = i
        return self.ins[a]

    def u32(self, a):
        o = a - self.addr
        if not 0 <= o <= len(self.code) - 4:
            raise Unsupported('table outside function')
        return int.from_bytes(self.code[o:o + 4], 'little')

    def u8(self, a):
        o = a - self.addr
        if not 0 <= o < len(self.code):
            raise Unsupported('table outside function')
        return self.code[o]

    def _switch_targets(self, i, block):
        m = i.operands[0].mem
        tb = m.disp & M32
        # bound from the cmp X, K / ja just before the dispatch (possibly in the
        # previous basic block)
        bound = None
        for p in reversed(self._linear_block(i.address)[:-1]):
            if p.mnemonic == 'cmp' and p.operands[1].type == X.X86_OP_IMM:
                bound = p.operands[1].imm + 1
                break
        bi = self._byte_index(block)
        stop = (bi.operands[1].mem.disp & M32) if bi is not None else None
        if bi is not None:
            # the byte index table holds one slot number per case value 0..bound-1
            if bound is None:
                raise Unsupported('byte-indexed switch without a bound')
            self.byte_tables[stop] = bound
        targets = []
        k = 0
        while self.addr <= tb + 4 * k < self.end and tb + 4 * k != stop:
            t = self.u32(tb + 4 * k)
            if not self.addr <= t < self.end:
                break
            targets.append(t)
            k += 1
            if bi is None and bound is not None and k >= bound:
                break
        if not targets:
            raise Unsupported('empty switch table')
        return targets

    def _byte_index(self, block):
        for p in block:
            if p.mnemonic in ('mov', 'movzx') and len(p.operands) == 2 and \
                    p.operands[1].type == X.X86_OP_MEM and p.operands[1].size == 1 and \
                    self.addr <= (p.operands[1].mem.disp & M32) < self.end:
                return p
        return None

    def _decode(self):
        work = [self.addr]
        leaders = {self.addr}
        seen = set()
        sw = {}
        # first pass: find leaders by recursive descent
        while work:
            a = work.pop()
            while True:
                if a in seen:
                    break
                seen.add(a)
                i = self.insn(a)
                m = i.mnemonic
                nxt = a + i.size
                if m == 'ret':
                    break
                if m == 'jmp':
                    if i.operands[0].type == X.X86_OP_IMM:
                        t = i.operands[0].imm
                        if self.addr <= t < self.end:
                            leaders.add(t)
                            work.append(t)
                        break
                    if i.operands[0].type == X.X86_OP_MEM and i.operands[0].mem.index != 0 and i.operands[0].mem.scale == 4:
                        blk = self._linear_block(a)
                        sw[a] = list(self._switch_targets(i, blk))
                        for t in sw[a]:
                            leaders.add(t)
                            work.append(t)
                        break
                    break   # tail jump / indirect: terminates
                if m.startswith('j') or m in ('loop', 'jecxz'):
                    t = i.operands[0].imm
                    leaders.add(t)
                    leaders.add(nxt)
                    work.append(t)
                    work.append(nxt)
                    break
                a = nxt
        # second pass: build blocks
        for s in sorted(leaders):
            blk = []
            a = s
            while True:
                i = self.insn(a)
                blk.append(i)
                m = i.mnemonic
                a += i.size
                if m == 'ret' or m == 'jmp' or m.startswith('j') or m in ('loop', 'jecxz'):
                    break
                if a in leaders:
                    break
            self.blocks[s] = blk
            last = blk[-1]
            m = last.mnemonic
            if m == 'ret':
                self.succ[s] = []
            elif m == 'jmp':
                o = last.operands[0]
                self.succ[s] = [o.imm] if o.type == X.X86_OP_IMM and self.addr <= o.imm < self.end else sw.get(last.address, [])
            elif m.startswith('j') or m in ('loop', 'jecxz'):
                self.succ[s] = [last.operands[0].imm, a]
            else:
                self.succ[s] = [a]

    def _linear_block(self, end_addr):
        """up to 12 decoded instructions running contiguously up to end_addr (the
        cmp/ja bound sits a few instructions before the jmp)"""
        out = [self.insn(end_addr)]
        before = {i.address + i.size: i for i in self.ins.values()}
        a = end_addr
        while len(out) < 12 and a in before:
            i = before[a]
            out.insert(0, i)
            a = i.address
        return out


# ---------------------------------------------------------------- evaluation

class State:
    def __init__(self, entry):
        self.entry = entry          # callable loc -> entry symbol
        self.regs = {}
        self.slots = {}             # frame offset -> (size, value)
        self.fpu = []               # x87 stack, top is last
        self.flags = FLAGS_IN       # not yet written in this block: the entry flags
        self.comparison = None      # exact CMP/TEST flags object and operand width
        self.fsw = None
        self.read = set()           # entry locations read
        self.written = set()
        self.pushed = {}            # frame offset -> value written by push (call arguments)
        self.sp_rec = None          # when collecting: frame offsets of stack addresses made

    def reg(self, r):
        if r not in self.regs:
            self.read.add(r)
            return self.entry(r)
        return self.regs[r]

    def set_reg(self, r, v):
        self.regs[r] = v
        self.written.add(r)
        if self.sp_rec is not None and r != 'esp':
            o = stack_off(v)
            if o is not None:
                self.sp_rec.add(o)
            elif SP in lin_parts(v)[1]:
                self.sp_rec.add(None)       # a computed stack address: anything is reachable
        if r == 'esp' and self.pushed:
            off = stack_off(v)
            if off is not None:
                for o in [o for o in self.pushed if o < off]:
                    del self.pushed[o]

    def get(self, name):
        full, lo, n = SUBREG[name]
        return ext(self.reg(full), lo, n)

    def set(self, name, v):
        full, lo, n = SUBREG[name]
        if n == 32:
            self.set_reg(full, v)
        else:
            self.set_reg(full, ins(self.reg(full), lo, n, ext(v, 0, n) if v[0] != 'c' else C(v[1] & ((1 << n) - 1))))


class Block:
    """symbolic summary of one basic block"""

    def __init__(self):
        self.effects = []
        self.term = None


class Evaluator:
    def __init__(self, func, escaped, returns_value, remap=None):
        self.f = func
        self.escaped = escaped
        self.returns_value = returns_value
        self.remap = remap          # frame relocation (FrameMap) applied to stack addresses
        self.access_log = None      # list of (insn address, frame offset, size, indexed) when gathering
        self.cur = None

    # -- memory
    def addr_of(self, st, m, size=0):
        base = st.reg(Evaluator.rn(m.base)) if m.base else C(0)
        idx = scale(st.reg(Evaluator.rn(m.index)), m.scale) if m.index else C(0)
        a = add(base, idx, C(m.disp))
        if st.sp_rec is not None and m.index and stack_off(add(base, C(m.disp))) is not None:
            st.sp_rec.add(stack_off(add(base, C(m.disp))))     # an indexed local array
        return self.frame_address(a, size)

    def frame_address(self, a, size):
        """Rename every frame memory access, including implicit stack accesses.

        ESP retains the machine's offsets. Explicit operands, pushes, pops and
        the callee argument reads must all access the same renamed storage.
        Otherwise a relocation can change which value a call receives.
        """
        if self.escaped and (self.remap is not None or self.access_log is not None):
            c, t = lin_parts(a)
            if t.get(SP) == 1:
                off = c if c < 0x80000000 else c - (1 << 32)
                floor = self.f.frame_floor
                if off < 0 and (floor is None or off >= floor):
                    if self.access_log is not None:
                        self.access_log.append((self.cur, off, size, len(t) > 1))
                    if self.remap is not None:
                        t[SP] = 1
                        a = mk_lin(self.remap(off) & M32, t)
        return a

    @staticmethod
    def rn(r):
        return md.reg_name(r)

    @staticmethod
    def flags_of(st):
        """the flags for a condition; flags set in a predecessor block are an entry value"""
        if st.flags is FLAGS_IN:
            st.read.add('flags')
            return st.entry('flags')
        return st.flags

    def in_frame_memory(self, off, size=1):
        """with the frame's address taken, its locals are memory; the outgoing
        argument area below the steady frame never is"""
        if not self.escaped or off >= 0:
            return False
        if off in getattr(self.f, 'save_offs', ()):
            return False        # a saved callee-saved register: not an addressable local
        retired = getattr(self.f, 'local_retired_bytes', {}).get(self.cur, ())
        if size > 0 and all(byte in retired for byte in range(off, off + size)):
            return False
        if hasattr(self.f, 'frame_memory_ranges'):
            return any(off < high and low < off + size for low, high in self.f.frame_memory_ranges)
        floor = self.f.frame_floor
        made = getattr(self.f, 'sp_rec', None)
        if made is not None and not getattr(self.f, 'collect_sp', False) \
                and None not in made:
            # only what lies at or above the lowest stack address the code ever forms can
            # be reached through a pointer (objects extend upward from their address);
            # locals below it are plain slots
            lows = [o for o in made if o < 0 and (floor is None or o >= floor)]
            if not lows or off < min(lows):
                return False
        return floor is None or off >= floor

    def load(self, st, eff, addr, size):
        off = stack_off(addr)
        if off is not None and not self.in_frame_memory(off, size):
            return self.slot_load(st, off, size)
        if self.f.addr <= (addr[1] if addr[0] == 'c' else -1) < self.f.end:
            raise Unsupported('load from code')
        c, t = lin_parts(addr)
        if t and self.f.addr <= c < self.f.end:
            # indexed read of a table inside the function (a switch's byte index
            # table): the value is identified by the table's contents, not its address
            if size != 1 or c not in self.f.byte_tables:
                raise Unsupported('indexed load from code')
            n = self.f.byte_tables[c]
            table = bytes(self.f.code[c - self.f.addr:c - self.f.addr + n])
            return ('tbl', hashlib.blake2b(table, digest_size=10).hexdigest(), mk_lin(0, t))
        if addr[0] == 'c' and size == 4 and IMPORT_NAME and IMPORT_NAME(addr[1]) is not None:
            # an import table entry: written only by the loader, so it holds the same
            # function address however often it is read (the compiler may keep it in a
            # register across calls or reload it each time)
            return ('imp', IMPORT_NAME(addr[1]))
        if addr[0] == 'c' and CONST_READ:
            # read-only data (compiler constants): identified by what it holds, since a
            # draft may reference another copy of the same constant
            k = CONST_READ(addr[1], size)
            if k is not None:
                return C(int.from_bytes(k, 'little')) if size <= 4 else ('k', size, k.hex())
        if _EXTENDED and _FRAME_FORWARD and size == 4 and stack_off(addr) is not None and stack_off(addr) < 0:
            holders = [r for a, n, r in getattr(st, 'global_holders', ()) if a == addr and n == size]
            if holders and not any(e[0] in ('call', 'str') or
                    (e[0] == 'st' and may_alias(e[2], e[1], addr, size)) for e in eff):
                # Every incoming path proved this frame word equals the same
                # entry register. Keep all stores observable; only forward its read.
                return st.entry(min(holders))
        # a store to the same place earlier in the block, with nothing after it that
        # could change that memory: the load reads back what was stored
        for k in range(len(eff) - 1, -1, -1):
            e = eff[k]
            if e[0] == 'st' and e[1] == size and D(e[2]) == D(addr):
                if size < 4:
                    return ext(e[3], 0, 8 * size)
                return e[3]
            if e[0] in ('call', 'str') or (e[0] == 'st' and may_alias(e[2], e[1], addr, size)):
                break
        self.mem_read = True
        deps = memory_dependencies(eff, addr, size)
        return ('ld', size, addr, deps)

    def store(self, st, eff, addr, size, val):
        off = stack_off(addr)
        if off is not None and off < 4 and off + size > 0:
            # ret reads the return address from [entry esp]; overwriting it changes control flow
            raise Unsupported('store over the return address')
        if addr[0] == 'c' and IMPORT_NAME and any(IMPORT_NAME(addr[1] + k) is not None for k in range(-3, size)):
            raise Unsupported('store to the import table')
        if off is not None and not self.in_frame_memory(off, size):
            return self.slot_store(st, off, size, val)
        if _DEAD_STORES and self.overwritten_before_observation(addr, size):
            # Record deleted targets while deriving the original memory facts.
            # Their later overwrites must not be removed using those facts.
            self.f.dead_global_addresses.add(addr)
            if hasattr(self.f, 'global_holders'):
                return
        if val[0] == 'ld' and val[1] == size and val[2] == addr and addr[0] == 'c':
            # writing a global back the value just read from it (the compiler spilling
            # a cached global to its own home) changes nothing, provided no call or
            # possibly aliasing store came after the load
            deps = set(val[3])
            if not [e for e in eff if D(e) not in deps and
                    (e[0] in ('call', 'str') or (e[0] == 'st' and may_alias(e[2], e[1], addr, size)))]:
                return
        if _EXTENDED and addr[0] == 'c' and addr not in getattr(self.f, 'dead_global_addresses', ()) and \
                any(a == addr and n == size and D(val) == D(st.entry(r))
                                 for a, n, r in getattr(st, 'global_holders', ())):
            if not any(e[0] in ('call', 'str') or
                       (e[0] == 'st' and may_alias(e[2], e[1], addr, size)) for e in eff):
                return          # memory already equals this block-entry value
        deps = memory_dependencies(eff, addr, size)
        eff.append(('st', size, addr, val, deps))

    def overwritten_before_observation(self, addr, size):
        """A fixed global store is dead only if every path overwrites its bytes
        before a possible read, unknown memory access, call, or function exit.
        This scans machine instructions, so reads used only by later branches
        and return values remain barriers too. Loops are rejected conservatively.
        """
        if addr[0] != 'c':
            return False
        block = self.block_of(self.cur)
        start = next(k + 1 for k, i in enumerate(self.f.blocks[block]) if i.address == self.cur)
        safe_register_ops = {
            'mov', 'movsx', 'movzx', 'add', 'sub', 'and', 'or', 'xor', 'adc', 'sbb',
            'cmp', 'test', 'inc', 'dec', 'neg', 'not', 'shl', 'shr', 'sar', 'sal',
            'imul', 'mul', 'idiv', 'div', 'cdq', 'cwde', 'nop',
            'lea', 'jmp', 'clc', 'stc',
        }
        memo, active = {}, set()

        def walk(block, start):
            key = (block, start)
            if key in memo:
                return memo[key]
            if key in active:
                return False
            active.add(key)
            for i in self.f.blocks[block][start:]:
                if i.mnemonic not in safe_register_ops and not i.mnemonic.startswith(('j', 'set')):
                    result = False
                    break
                if i.mnemonic == 'lea':
                    continue
                killed = False
                for operand in i.operands:
                    if operand.type != X.X86_OP_MEM:
                        continue
                    m = operand.mem
                    if m.segment:
                        result = False
                        break
                    if not m.base and not m.index:
                        other = C(m.disp)
                    elif m.base and self.rn(m.base) == 'esp' and not m.index:
                        other = add(SP, C(m.disp))
                    else:
                        result = False
                        break
                    if not operand.access:
                        result = False
                        break
                    if operand.access & capstone.CS_AC_READ and may_alias(addr, size, other, operand.size):
                        result = False
                        break
                    if operand.access & capstone.CS_AC_WRITE and may_alias(addr, size, other, operand.size):
                        if other == addr and operand.size >= size:
                            killed = True
                        else:
                            result = False
                            break
                else:
                    if killed:
                        result = True
                        break
                    continue
                break
            else:
                successors = self.f.succ.get(block, ())
                result = bool(successors) and all(walk(t, 0) for t in successors)
            active.remove(key)
            memo[key] = result
            return result

        return walk(block, start)

    def slot_load(self, st, off, size):
        def entry(loc):
            st.read.add(loc)
            return st.entry(loc)
        v = slot_value(st.slots, off, size, entry)
        return entry(('s', off, size)) if v is None else v

    def slot_store(self, st, off, size, val):
        # keep the parts of older, overlapping stores that this one does not cover
        for o in list(st.slots):
            s, v = st.slots[o]
            if o < off + size and off < o + s:
                del st.slots[o]
                if o < off:
                    st.slots[o] = (off - o, ext(v, 0, (off - o) * 8))
                if o + s > off + size:
                    st.slots[off + size] = (o + s - off - size, ext(v, (off + size - o) * 8, (o + s - off - size) * 8))
        if size < 4:
            val = ext(val, 0, size * 8) if val[0] != 'c' else C(val[1] & ((1 << (size * 8)) - 1))
        st.slots[off] = (size, val)
        st.written.add(('s', off, size))

    # -- operands
    def rd(self, st, eff, o):
        if o.type == X.X86_OP_REG:
            return st.get(self.rn(o.reg))
        if o.type == X.X86_OP_IMM:
            # an 8- or 16-bit operation only sees that many bits of its immediate
            return C(o.imm & ((1 << (8 * o.size)) - 1)) if o.size in (1, 2) else C(o.imm)
        if o.type == X.X86_OP_MEM:
            return self.load(st, eff, self.addr_of(st, o.mem, o.size), o.size)
        raise Unsupported('operand')

    def wr(self, st, eff, o, v):
        if o.type == X.X86_OP_REG:
            st.set(self.rn(o.reg), v)
        elif o.type == X.X86_OP_MEM:
            self.store(st, eff, self.addr_of(st, o.mem, o.size), o.size, v)
        else:
            raise Unsupported('write operand')

    def push(self, st, eff, v):
        if st.sp_rec is not None and stack_off(v) is not None:
            st.sp_rec.add(stack_off(v))
        esp = sub(st.reg('esp'), C(4))
        st.set_reg('esp', esp)
        self.store(st, eff, self.frame_address(esp, 4), 4, v)
        st.pushed[stack_off(esp)] = v

    def pop(self, st, eff):
        esp = st.reg('esp')
        st.pushed.pop(stack_off(esp), None)
        v = self.load(st, eff, self.frame_address(esp, 4), 4)
        st.set_reg('esp', add(esp, C(4)))
        return v

    # -- fpu
    def fpush(self, st, v):
        st.fpu.append(v)

    def fpop(self, st):
        if not st.fpu:
            raise Unsupported('fpu stack underflow / live across blocks')
        return st.fpu.pop()

    def fst(self, st, i):
        if i >= len(st.fpu):
            raise Unsupported('fpu value from outside block')
        return st.fpu[-1 - i]

    def fset(self, st, i, v):
        if i >= len(st.fpu):
            raise Unsupported('fpu value from outside block')
        st.fpu[-1 - i] = v

    # -- block
    def run(self, start, entry, chain=()):
        st = State(entry)
        if getattr(self.f, 'collect_sp', False):
            st.sp_rec = self.f.sp_rec
        # x87 values live into the block: ('f', k) is st(k) on entry
        d = self.f.fdepth.get(start, 0)
        for k in reversed(range(d)):
            st.read.add(('f', k))
            st.fpu.append(entry(('f', k)))
        st.global_holders = getattr(self.f, 'global_holders', {}).get(start, set())
        eff = []
        blk = list(self.f.blocks[start])
        for nxt_blk in chain:
            # a block that runs straight on into the next (fall-through or jmp): one
            # longer block, as if the compiler had duplicated the tail here
            if blk[-1].mnemonic == 'jmp':
                blk = blk[:-1]
            blk += self.f.blocks[nxt_blk]
        b = Block()
        for k, i in enumerate(blk):
            nxt = blk[k + 1] if k + 1 < len(blk) else None
            t = self.step(st, eff, i, nxt)
            if t is not None and k == len(blk) - 1:
                # A supplied continuation replaces an earlier terminator. This
                # includes a jcc proved to be a jump by its dominating path.
                b.term = t
        if b.term is None:
            last = blk[-1]
            b.term = ('fall', last.address + last.size)
        b.effects = eff
        b.state = st
        st.written |= {('f', k) for k in range(len(st.fpu))}
        if st.flags is not FLAGS_IN:
            st.written.add('flags')
        return b

    def step(self, st, eff, i, nxt):
        m = i.mnemonic
        ops = i.operands
        self.cur = i.address
        if m in ('nop',) or (m == 'lea' and ops[0].type == X.X86_OP_REG and ops[1].type == X.X86_OP_MEM and
                             self.rn(ops[0].reg) in REG32 and ops[1].mem.disp == 0 and ops[1].mem.index == 0 and
                             ops[1].mem.base == ops[0].reg):
            return None
        if m == 'mov':
            self.wr(st, eff, ops[0], self.rd(st, eff, ops[1]))
        elif m in ('movsx', 'movzx'):
            v = self.rd(st, eff, ops[1])
            n = ops[1].size * 8
            r = (sx if m == 'movsx' else zx)(n, v)
            if ops[0].size < 4:
                r = ext(r, 0, ops[0].size * 8)
            self.wr(st, eff, ops[0], r)
        elif m == 'lea':
            a = self.addr_of(st, ops[1].mem)
            o = stack_off(a)
            if o is not None and o >= 4:
                self.f.arg_address_taken = True
            self.wr(st, eff, ops[0], a)
        elif m in ('add', 'sub', 'and', 'or', 'xor', 'adc', 'sbb'):
            a = self.rd(st, eff, ops[0])
            b = self.rd(st, eff, ops[1])
            n = ops[0].size * 8
            if m in ('xor', 'sub') and ops[0].type == X.X86_OP_REG and ops[1].type == X.X86_OP_REG and ops[0].reg == ops[1].reg:
                r = C(0)
                st.flags = ('fl', 'zero')
            elif m == 'add':
                r = add(a, b)
                st.flags = op('fl', 'add', a, b, comm=True)
            elif m == 'sub':
                r = sub(a, b)
                st.flags = ('fl', 'sub', a, b)
            elif m in ('adc', 'sbb'):
                cf = self.flags_of(st)
                if cf is None:
                    raise Unsupported('carry from unknown flags')
                # x - x - carry is independent of x, modulo the operand width.
                r = op(m, C(0), C(0), cf) if m == 'sbb' and a == b else op(m, a, b, cf)
                st.flags = ('fl', m, a, b, cf)
            else:
                r = op(m, a, b, comm=True)
                if m == 'and' and is_c(b) and b[1] == 0:
                    r = C(0)
                elif m == 'and':
                    # and with a low-bit mask 2^k-1 keeps the low k bits: one canonical
                    # form with movzx and partial-register zero extension
                    for x, c in ((a, b), (b, a)):
                        if is_c(c) and c[1] and (c[1] & (c[1] + 1)) == 0:
                            k = c[1].bit_length()
                            r = x if k >= 32 else ext(x, 0, k)
                            break
                st.flags = op('fl', m, a, b, comm=True)
            if n < 32 and r[0] != 'c':
                r = ext(r, 0, n)
            st.flags = literal_flags(st.flags, ops[0].size, r)
            if m == 'add':
                st.add_result = (st.flags, ops[0].size, r)
            self.wr(st, eff, ops[0], r)
        elif m in ('cmp', 'test'):
            a = self.rd(st, eff, ops[0])
            b = self.rd(st, eff, ops[1])
            if m == 'test' and a == b:
                st.flags = ('fl', 'sub', a, C(0))
            elif m == 'cmp':
                st.flags = ('fl', 'sub', a, b)
            else:
                st.flags = op('fl', 'and', a, b, comm=True)
            result = sub(a, b) if m == 'cmp' else op('and', a, b, comm=True)
            st.flags = literal_flags(st.flags, ops[0].size, result)
            st.comparison = (st.flags, ops[0].size)
        elif m in ('inc', 'dec'):
            a = self.rd(st, eff, ops[0])
            r = add(a, C(1 if m == 'inc' else -1))
            if ops[0].size < 4:
                r = ext(r, 0, ops[0].size * 8)
            previous = (st.entry('flags') if st.flags is FLAGS_IN else st.flags) if is_c(r) else None
            st.flags = literal_flags(('fl', m, a), ops[0].size, r, previous)
            self.wr(st, eff, ops[0], r)
        elif m == 'neg':
            a = self.rd(st, eff, ops[0])
            r = neg(a) if ops[0].size == 4 else ext(neg(a), 0, ops[0].size * 8)
            st.flags = literal_flags(('fl', 'neg', a), ops[0].size, r)
            self.wr(st, eff, ops[0], r)
        elif m == 'not':
            a = self.rd(st, eff, ops[0])
            self.wr(st, eff, ops[0], sub(C(-1), a) if ops[0].size == 4 else op('not', a))
        elif m in ('shl', 'sal', 'sar', 'shr', 'rol', 'ror'):
            a = self.rd(st, eff, ops[0])
            c = self.rd(st, eff, ops[1]) if len(ops) > 1 else C(1)
            if m in ('shl', 'sal') and is_c(c) and ops[0].size == 4:
                r = scale(a, 1 << (c[1] & 31))
            else:
                r = op(m, a, c, ops[0].size)
            st.flags = ('fl', m, ops[0].size, a, c)
            self.wr(st, eff, ops[0], r)
        elif m in ('shld', 'shrd'):
            a = self.rd(st, eff, ops[0])
            b = self.rd(st, eff, ops[1])
            c = self.rd(st, eff, ops[2])
            st.flags = ('fl', m, a, b, c)
            self.wr(st, eff, ops[0], op(m, a, b, c))
        elif m == 'imul':
            if len(ops) == 1:
                a = st.get('eax')
                b = self.rd(st, eff, ops[0])
                st.set('eax', mul(a, b))
                st.set('edx', op('mulhi', a, b, comm=True))
            else:
                a = self.rd(st, eff, ops[1] if len(ops) == 3 else ops[0])
                b = self.rd(st, eff, ops[2] if len(ops) == 3 else ops[1])
                self.wr(st, eff, ops[0], mul(a, b) if ops[0].size == 4 else ext(mul(a, b), 0, ops[0].size * 8))
            # CF/OF describe this binary signed multiplication's overflow,
            # which is not associative even though its low-bit result is.
            st.flags = ('fl', 'imul', ops[0].size, op('imul_args', a, b, comm=True))
        elif m == 'mul':
            a = st.get('eax')
            b = self.rd(st, eff, ops[0])
            st.set('eax', op('umul', a, b, comm=True))
            st.set('edx', op('umulhi', a, b, comm=True))
            st.flags = ('fl', 'mul')
        elif m in ('idiv', 'div') and ops[0].size == 1:
            # ax / r8: al = quotient, ah = remainder
            num = st.get('ax')
            d = self.rd(st, eff, ops[0])
            q, r = op(m + 'q8', num, d), op(m + 'r8', num, d)
            st.set('al', q)
            st.set('ah', r)
            st.flags = None
        elif m in ('idiv', 'div') and ops[0].size == 2:
            # dx:ax / r16: ax = quotient, dx = remainder
            lo, hi = st.get('ax'), st.get('dx')
            d = self.rd(st, eff, ops[0])
            q, r = op(m + 'q16', hi, lo, d), op(m + 'r16', hi, lo, d)
            st.set('ax', q)
            st.set('dx', r)
            st.flags = None
        elif m in ('idiv', 'div'):
            lo, hi = st.get('eax'), st.get('edx')
            d = self.rd(st, eff, ops[0])
            st.set('eax', op(m + 'q', hi, lo, d))
            st.set('edx', op(m + 'r', hi, lo, d))
            st.flags = None
        elif m == 'cdq':
            st.set('edx', op('sign', st.get('eax')))
        elif m == 'cwd':
            st.set('dx', ext(op('sign', sx(16, st.get('ax'))), 0, 16))
        elif m == 'cwde':
            st.set('eax', sx(16, st.get('ax')))
        elif m == 'cbw':
            st.set('ax', ext(sx(8, st.get('al')), 0, 16))
        elif m == 'xchg':
            a = self.rd(st, eff, ops[0])
            b = self.rd(st, eff, ops[1])
            self.wr(st, eff, ops[0], b)
            self.wr(st, eff, ops[1], a)
        elif m == 'push':
            self.push(st, eff, self.rd(st, eff, ops[0]))
        elif m == 'pop':
            self.wr(st, eff, ops[0], self.pop(st, eff))
        elif m.startswith('set'):
            cc = CC_ALIAS.get(m[3:], m[3:])
            self.wr(st, eff, ops[0], ('setcc', canon_cond(cc, self.flags_of(st))))
        elif m == 'repe cmpsb':
            return self.compare_string(st, eff, i)
        elif m.replace('rep ', '').replace('repne ', '').replace('repe ', '') in ('stosd', 'movsd', 'stosb', 'movsb', 'stosw', 'movsw', 'scasb') \
                or m in ('stosd', 'movsd', 'stosb', 'movsb', 'stosw', 'movsw'):
            return self.string_op(st, eff, m, i)
        elif m == 'call':
            return self.call(st, eff, i, nxt)
        elif m == 'ret':
            esp = st.reg('esp')
            if stack_off(esp) not in (None, 0):
                # the stack model went wrong somewhere (a miscounted call): refuse
                raise Unsupported('esp at ret is %s, not the entry value' % stack_off(esp))
            ret = st.get('eax') if self.returns_value else None
            if ret is not None:
                ret = live_projection(ret, getattr(self.f, 'return_mask', FULL))
            fret = st.fpu[-1] if st.fpu else None
            return ('ret', ret, fret, stack_off(esp))
        elif m == 'jmp':
            o = ops[0]
            if o.type == X.X86_OP_IMM:
                if self.f.addr <= o.imm < self.f.end:
                    return ('jmp', o.imm)
                off = stack_off(st.reg('esp'))
                # the x87 stack goes along too (a tail jump to _ftol converts st(0))
                return ('tail', C(o.imm), tuple(self.call_args(st, eff, off, 4 * self.pushed_count(st, off))),
                        tuple(st.fpu))
            if o.type == X.X86_OP_MEM and o.mem.index != 0 and o.mem.scale == 4:
                index = st.get(self.rn(o.mem.index))
                return ('switch', index, self.f._switch_targets(i, self.f.blocks.get(self.block_of(i.address), [i])))
            raise Unsupported('indirect jump')
        elif m.startswith('j') or m == 'loop':
            cc = m[1:]
            cc = CC_ALIAS.get(cc, cc)
            if cc not in CC_NEG:
                raise Unsupported('jcc ' + m)
            cond = canon_cond(cc, self.flags_of(st))
            added = getattr(st, 'add_result', None)
            if _EXTENDED and cc in ('s', 'ns') and added is not None and \
                    st.flags is added[0] and added[1] == 4:
                # SF observes only bit31 of the actual modular ADD result.
                # Preserve operand pairing for overflow/carry conditions and
                # preserve the distinct sign bits of byte/word instructions.
                cond = canon_cond(cc, ('fl', 'sub', added[2], C(0)))
            if _EXTENDED and st.comparison is not None and st.flags is st.comparison[0]:
                cond = bounded_zero_comparison(cond, st.comparison[1])
            decided = getattr(self.f, 'decided_branches', {}).get(i.address) if _EXTENDED else None
            if decided is not None:
                return ('jmp', decided)
            literal = literal_comparison(cc, self.flags_of(st)) if _EXTENDED else None
            if literal is not None:
                return ('jmp', ops[0].imm if literal else i.address + i.size)
            if _EXTENDED and cond[1] in ('e', 'ne') and cond[2][0:2] == ('fl', 'sub'):
                a, b = cond[2][2:]
                if a == b or (is_c(a) and is_c(b)):
                    equal = a == b
                    return ('jmp', ops[0].imm if equal == (cond[1] == 'e') else i.address + i.size)
            return ('jcc', cond, ops[0].imm, i.address + i.size)
        elif m == 'sahf':
            st.flags = ('fl', 'sahf', st.get('ah'))
        elif m.startswith('f'):
            self.fpu_op(st, eff, i)
        elif m in ('cld', 'std', 'fwait', 'wait'):
            pass
        elif m == 'cpuid':
            leaf = st.get('eax')
            for r in ('eax', 'ebx', 'ecx', 'edx'):
                st.set(r, ('cpuid', r, leaf))
        elif m in ('clc', 'stc'):
            st.flags = ('fl', m)
        elif m == 'leave':
            st.set_reg('esp', st.reg('ebp'))
            st.set_reg('ebp', self.pop(st, eff))
        else:
            raise Unsupported('instruction ' + m)
        return None

    def block_of(self, a):
        for s, blk in self.f.blocks.items():
            if blk and blk[0].address <= a <= blk[-1].address:
                return s
        return None

    def compare_string(self, st, eff, instruction):
        # Keep the first-mismatch result opaque, including every byte that can
        # decide it. Equal summaries prove equal flags and iteration counts;
        # no rule assumes that REPE always consumes the complete range.
        if instruction.addr_size != 4 or instruction.prefix[1]:
            raise Unsupported('non-flat repeated byte comparison')
        count = st.get('ecx')
        if not is_c(count) or count[1] > 64:
            raise Unsupported('unbounded repeated byte comparison')
        if any(i.mnemonic == 'std' for i in self.f.ins.values()):
            raise Unsupported('backward repeated byte comparison')
        if count[1] == 0:
            return None
        source, destination = st.get('esi'), st.get('edi')
        pairs = tuple((self.load(st, eff, add(source, C(j)), 1),
                       self.load(st, eff, add(destination, C(j)), 1))
                      for j in range(count[1]))
        result = ('cmpsb', pairs)
        steps = ('cmps_steps', result)
        st.set('ecx', sub(count, steps))
        st.set('esi', add(source, steps))
        st.set('edi', add(destination, steps))
        st.flags = ('fl', 'cmpsb', result)
        return None

    def string_op(self, st, eff, m, i):
        rep = m.startswith('rep ')
        base = m.split()[-1]
        size = {'d': 4, 'w': 2, 'b': 1}[base[-1]]
        if base.startswith('scas'):
            if not m.startswith('repne'):
                raise Unsupported('scas form')
            edi, al, ecx = st.get('edi'), st.get('al'), st.get('ecx')
            r = ('scas', edi, al, ecx, tuple(sorted(D(x) for x in eff)))
            st.set('ecx', ('scas_ecx', r))
            st.set('edi', ('scas_edi', r))
            st.flags = ('fl', 'scas', r)
            return None
        n = st.get('ecx') if rep else C(1)
        if not is_c(n) or n[1] > 64:
            edi = st.get('edi')
            src = st.get('esi') if not base.startswith('stos') else st.get('eax')
            if has_sp(edi) or (not base.startswith('stos') and has_sp(src)):
                if not self.escaped:
                    raise Unsupported('variable-length string op on the stack')
            e = ('str', base, edi, src, n, tuple(sorted(D(x) for x in eff)))
            eff.append(e)
            st.set('edi', add(edi, scale(n, size)))
            if not base.startswith('stos'):
                st.set('esi', add(src, scale(n, size)))
            st.set('ecx', C(0))
            return None
        k = n[1]
        edi = st.get('edi')
        if base.startswith('stos'):
            val = ext(st.get('eax'), 0, size * 8) if size < 4 else st.get('eax')
            for j in range(k):
                self.store(st, eff, add(edi, C(j * size)), size, val)
        else:
            esi = st.get('esi')
            for j in range(k):
                v = self.load(st, eff, add(esi, C(j * size)), size)
                self.store(st, eff, add(edi, C(j * size)), size, v)
            self.string_end(st, 'esi', add(esi, C(k * size)))
        self.string_end(st, 'edi', add(edi, C(k * size)))
        if rep:
            st.set('ecx', C(0))
        return None

    def register_read_after(self, reg):
        """Any surviving bit read before overwrite, including function exits."""
        block = self.block_of(self.cur)
        instructions = self.f.blocks[block]
        start = next(k + 1 for k, i in enumerate(instructions) if i.address == self.cur)
        pending, seen = [(block, start, FULL)], set()

        def bits(register, writing=False):
            name = self.rn(register)
            full, low, width = SUBREG.get(name, (name, 0, 32))
            if not _JOIN_RECIPES:
                # Preserve the original conservative scan in the first tiers.
                return FULL if (name == reg if writing else full == reg) else 0
            return ((1 << width) - 1) << low if full == reg else 0

        while pending:
            block, start, remaining = pending.pop()
            if (block, start, remaining) in seen:
                continue
            seen.add((block, start, remaining))
            instructions = self.f.blocks[block]
            for k, i in enumerate(instructions[start:], start):
                reads, writes = i.regs_access()
                read_mask = 0
                for r in reads:
                    read_mask |= bits(r)
                operands = i.operands
                if _JOIN_RECIPES and i.mnemonic in ('xor', 'sub') and len(operands) == 2 and \
                        all(o.type == X.X86_OP_REG for o in operands) and operands[0].reg == operands[1].reg:
                    # A self-zero has no dependence on its old value or flags.
                    read_mask = 0
                elif _JOIN_RECIPES and i.mnemonic == 'push' and operands[0].type == X.X86_OP_REG and k + 1 < len(instructions):
                    call = instructions[k + 1]
                    if call.mnemonic == 'call' and call.operands[0].type == X.X86_OP_IMM and ARG_WIDTH is not None:
                        widths = ARG_WIDTH(call.operands[0].imm)
                        if widths is not None:
                            # This push is argument zero of the immediately
                            # following direct call. Use its binary-derived
                            # read width, just as call_args does.
                            width = widths.get(0, 0)
                            read_mask &= (1 << (8 * width)) - 1 if width < 4 else FULL
                if read_mask & remaining:
                    return True
                for r in writes:
                    remaining &= ~bits(r, writing=True) & FULL
                if not remaining:
                    break
            else:
                successors = self.f.succ.get(block, ())
                if not successors:
                    return True             # the caller can observe a preserved register
                pending.extend((s, 0, remaining) for s in successors)
        return False

    def string_end(self, st, reg, value):
        # A constant-length copy's one-past pointer exposes no further object
        # when every path overwrites it before any use. Include partial reads,
        # subsequent string operations, and function exits in that check.
        recording = st.sp_rec
        if recording is not None and not self.register_read_after(reg):
            st.sp_rec = None
        st.set(reg, value)
        st.sp_rec = recording

    # -- calls
    def args_snapshot(self, st):
        return ()

    def pushed_count(self, st, off):
        n, o = 0, off
        floor = self.f.frame_floor
        while o in st.pushed and (floor is None or o < floor):
            v = st.pushed[o]
            if v[0] == 'preserved' or v == ('in', 'ebp'):
                break
            n += 1
            o += 4
        return n

    def call_args(self, st, eff, off, nbytes):
        """the callee's argument area: nbytes from esp upwards, read as dwords"""
        args = []
        for o in range(off, off + nbytes, 4):
            args.append(self.load(st, eff, self.frame_address(add(SP, C(o)), 4), 4))
        return args

    def call(self, st, eff, i, nxt):
        o = i.operands[0]
        if o.type == X.X86_OP_IMM and STACK_PROBE is not None and STACK_PROBE(o.imm):
            # _chkstk: allocates eax bytes of frame below the return address
            n = st.get('eax')
            if not is_c(n):
                raise Unsupported('stack probe with unknown size')
            st.set_reg('esp', sub(st.reg('esp'), n))
            st.set_reg('eax', ('junk', 'eax', i.address))
            return None
        if o.type == X.X86_OP_IMM:
            target = C(o.imm)
        else:
            target = self.rd(st, eff, o)
        esp = st.reg('esp')
        off = stack_off(esp)
        if off is None:
            raise Unsupported('esp not frame-relative at call')
        npush = self.pushed_count(st, off)
        floor = self.f.frame_floor
        below = (floor - off) // 4 if floor is not None and floor > off else 0
        if getattr(self.f, 'use_floor', False):
            npush = max(npush, below)
        pops = self.callee_pops(o, npush, nxt, target, i.address)
        caller = nxt is not None and nxt.mnemonic == 'add' and nxt.operands[0].type == X.X86_OP_REG and \
            self.rn(nxt.operands[0].reg) == 'esp' and nxt.operands[1].type == X.X86_OP_IMM
        # compare the whole argument area: everything between esp and the steady frame
        nbytes = max(pops, nxt.operands[1].imm if caller else 0, 4 * npush, 4 * below)
        w = ARG_WIDTH(target[1]) if target[0] == 'c' and ARG_WIDTH is not None else None
        if w:
            # A delayed cleanup or an over-deep frame estimate can hide the
            # outgoing area. Every binary-derived callee read is still an input.
            nbytes = max(nbytes, 4 * (max(w) + 1))
        if floor is None:
            for so in st.slots:
                if off <= so < off + 64 and so >= off + nbytes and so not in st.pushed:
                    raise Unsupported('cannot size the argument area of a call')
        args = self.call_args(st, eff, off, nbytes)
        # a callee that only reads the low byte(s) of an argument doesn't see the rest
        # (char/short parameters pushed from a register with stale upper bits)
        if w is not None:
            args = [ext(a, 0, 8 * w[k]) if 0 < w.get(k, 0) < 4 else (('unread',) if w.get(k, 0) == 0 else a)
                    for k, a in enumerate(args)]
            # trailing slots the callee never reads are not arguments at all (one
            # side may have pushed a later call's values early)
            while args and args[-1] == ('unread',):
                args.pop()
        # A binary-proved unary pop observes TOP and preserves lower x87
        # values. Unknown bodies retain the full incoming context.
        adj_fdiv = ADJ_FDIV.get(target[1]) if target[0] == 'c' else None
        if adj_fdiv is not None and not st.fpu:
            raise Unsupported('x87 adjustment helper called without st(0)')
        if adj_fdiv is not None and pops != adj_fdiv[0]:
            raise Unsupported('x87 adjustment helper has unexpected callee pop')
        pure_call = target[0] == 'c' and ((PURE is not None and PURE(target[1])) or adj_fdiv is not None)
        # Caller control-word writes are effects without a carried CW state.
        # A floating pure call cannot use that unordered representation as
        # proof of its rounding mode, even when its own body restores CW.
        if pure_call and st.fpu and any(instruction.mnemonic == 'fldcw'
                                       for instruction in self.f.ins.values()):
            raise Unsupported('caller x87 control state at call is unmodeled')
        environment_sites = getattr(self.f, 'x87_environment_predecessors', None)
        if environment_sites is None:
            predecessors = collections.defaultdict(set)
            environment_sites = set()
            for block, instructions in self.f.blocks.items():
                for index, instruction in enumerate(instructions):
                    if instruction.mnemonic in ('fnstsw', 'fstsw', 'fnstenv', 'fstenv',
                            'fnsave', 'fsave', 'fninit', 'finit', 'fnclex', 'fclex', 'fwait', 'wait'):
                        environment_sites.add(instruction.address)
                    successors = [instructions[index + 1].address] if index + 1 < len(instructions) else self.f.succ[block]
                    for successor in successors:
                        predecessors[successor].add(instruction.address)
            work = list(environment_sites)
            while work:
                for predecessor in predecessors[work.pop()]:
                    if predecessor not in environment_sites:
                        environment_sites.add(predecessor)
                        work.append(predecessor)
            self.f.x87_environment_predecessors = environment_sites
        environment = i.address in environment_sites
        unary = X87_UNARY_POP(target[1]) if pure_call and st.fpu and not environment and \
                X87_UNARY_POP is not None else None
        fargs = (st.fpu[-1],) if unary is not None or adj_fdiv is not None else tuple(st.fpu)
        # x87 helpers (_ftol, _CIcos, ...) take their input in st(0)
        if pure_call:
            # no memory reads or writes: a value, not an effect (independent of stores)
            e = ('call', target, tuple(args), fargs, ())
        else:
            e = ('call', target, tuple(args), fargs, tuple(sorted(D(x) for x in eff)))
            eff.append(e)
        st.set_reg('eax', ('ret', e))
        for register in ('ecx', 'edx'):
            if unary is None or register not in unary:
                st.set_reg(register, ('junk', register, e))
        st.flags = None
        uses_st0 = nxt is not None and nxt.mnemonic.startswith('f') and \
            nxt.mnemonic not in ('fld', 'fild', 'fldz', 'fld1', 'fldpi', 'fnclex', 'fninit', 'fldcw', 'fnstcw')
        out = 1 if adj_fdiv is not None else (0 if unary is not None else (FPU_OUT(target[1], 1 if st.fpu else 0) if target[0] == 'c' and FPU_OUT is not None else None))
        if out is None and getattr(self.f, 'require_call_depth', False):
            # The caller's next instruction is not a binary proof of the callee's
            # stack effect. In particular a two-input CRT thunk may consume both
            # cells, so guessing a unary result would invent a live lower cell.
            raise Unsupported('callee x87 depth is unknown')
        if st.fpu:
            # the helper consumed st(0); it either leaves a result there or pops it
            # (_ftol pops; _CIcos and friends replace it)
            if out == 0 or (out is None and not uses_st0):
                st.fpu.pop()
            elif out == 1 or out is None:
                st.fpu[-1] = ('fret', e)
            else:
                raise Unsupported('call changes the x87 stack by %d' % (out - 1))
        elif out == 1 or (out is None and uses_st0):
            st.fpu = [('fret', e)]
        elif out not in (0, None):
            raise Unsupported('call leaves %d x87 values' % out)
        # the callee owns its argument slots and may have changed them
        for k in range(nbytes // 4):
            slot = stack_off(self.frame_address(add(SP, C(off + 4 * k)), 4))
            if slot is None:
                raise Unsupported('relocated call argument is not frame-relative')
            if slot in st.slots or any(slot <= o < slot + 4 for o in st.slots):
                for o in [o for o in st.slots if slot <= o < slot + 4]:
                    del st.slots[o]
                st.slots[slot] = (4, ('argafter', D(e), k))
        if pops:
            st.set_reg('esp', add(esp, C(pops)))
        return None

    def callee_pops(self, o, nargs, nxt, target=None, call_addr=None):
        """bytes the callee removes from the stack"""
        iat = None
        if o.type == X.X86_OP_IMM:
            iat = THUNK_IAT(o.imm) if THUNK_IAT else None
            if iat is None:
                return CALLEE_POP(o.imm) if CALLEE_POP else 0
        elif o.type == X.X86_OP_MEM and o.mem.base == 0 and o.mem.index == 0:
            iat = o.mem.disp & M32
        elif target is not None and target[0] == 'imp':
            return 0 if target[1] in VARARGS_IMPORTS else 4 * nargs
        elif target is not None and target[0] == 'ld' and target[2][0] == 'c':
            iat = target[2][1]          # register loaded from an import table entry
        elif call_addr is not None and call_addr in self.f.call_iat:
            iat = self.f.call_iat[call_addr]
        elif o.type == X.X86_OP_REG and md.reg_name(o.reg) in self.f.reg_iat:
            iat = self.f.reg_iat[md.reg_name(o.reg)]
        if iat is not None and IMPORT_NAME:
            name = IMPORT_NAME(iat)
            if name is not None:
                return 0 if name in VARARGS_IMPORTS else 4 * nargs
        # function pointers: cdecl if the caller cleans up after the call (the
        # `add esp, N` may be scheduled a few instructions later), else stdcall (COM)
        if call_addr is not None:
            return 0 if self.f.caller_cleans(call_addr) else 4 * nargs
        caller = nxt is not None and nxt.mnemonic == 'add' and nxt.operands[0].type == X.X86_OP_REG and \
            md.reg_name(nxt.operands[0].reg) == 'esp'
        return 0 if caller else 4 * nargs

    def _returns_float(self, nxt):
        return nxt is not None and nxt.mnemonic.startswith('fst')

    # -- x87
    def fpu_op(self, st, eff, i):
        m = i.mnemonic
        ops = i.operands

        def stv(o):
            if o.type == X.X86_OP_REG:
                n = self.rn(o.reg)
                if n.startswith('st'):
                    return int(n[3]) if '(' in n else 0
            return None

        if m in ('fld', 'fild'):
            o = ops[0]
            if o.type == X.X86_OP_MEM:
                v = self.load(st, eff, self.addr_of(st, o.mem, o.size), o.size)
                v = ('i2f', o.size, v) if m == 'fild' else fval(o.size, v)
            else:
                v = self.fst(st, stv(o))
            self.fpush(st, v)
        elif m in ('fldz', 'fld1', 'fldpi', 'fldln2', 'fldlg2', 'fldl2e', 'fldl2t'):
            self.fpush(st, ('fconst', m))
        elif m in ('fst', 'fstp', 'fist', 'fistp'):
            o = ops[0]
            v = self.fst(st, 0)
            if o.type == X.X86_OP_MEM:
                val = ('f2i', o.size, v) if m.startswith('fi') else ('tof', o.size, v)
                self.store(st, eff, self.addr_of(st, o.mem, o.size), o.size, val)
            else:
                self.fset(st, stv(o), v)
            if m.endswith('p'):
                self.fpop(st)
        elif m == 'fxch':
            j = stv(ops[-1]) if ops else 1     # capstone lists fxch st(i) as (st(0), st(i))
            a, b = self.fst(st, 0), self.fst(st, j)
            self.fset(st, 0, b)
            self.fset(st, j, a)
        elif m in ('fchs', 'fabs', 'fsqrt', 'fsin', 'fcos', 'frndint', 'f2xm1'):
            self.fset(st, 0, (m, self.fst(st, 0)))
        elif m in ('fpatan', 'fyl2x', 'fscale', 'fprem'):
            a, b = self.fst(st, 0), self.fst(st, 1)
            if m == 'fscale':
                self.fset(st, 0, (m, a, b))
            else:
                self.fset(st, 1, (m, b, a))
                self.fpop(st)
        elif re.match(r'^fi?(add|sub|subr|mul|div|divr)p?$', m):
            base = re.match(r'^fi?(add|sub|subr|mul|div|divr)p?$', m).group(1)
            pop = m.endswith('p') and base + 'p' != m[-len(base) - 1:] or m.endswith('p')
            if m in ('fsubp', 'fsubrp', 'fdivp', 'fdivrp', 'faddp', 'fmulp') or (m.endswith('p') and ops):
                # op st(i), st ; pop
                j = stv(ops[0]) if ops else 1
                a, b = self.fst(st, j), self.fst(st, 0)
                self.fset(st, j, farith(base, a, b))
                self.fpop(st)
            elif ops and ops[0].type == X.X86_OP_MEM:
                v = self.load(st, eff, self.addr_of(st, ops[0].mem, ops[0].size), ops[0].size)
                v = ('i2f', ops[0].size, v) if m.startswith('fi') else fval(ops[0].size, v)
                self.fset(st, 0, farith(base, self.fst(st, 0), v))
            elif len(ops) == 2:
                d, s = stv(ops[0]), stv(ops[1])
                self.fset(st, d, farith(base, self.fst(st, d), self.fst(st, s)))
            elif len(ops) == 1:
                s = stv(ops[0])
                self.fset(st, 0, farith(base, self.fst(st, 0), self.fst(st, s)))
            else:
                raise Unsupported('fpu arith form ' + i.op_str)
        elif m in ('fcom', 'fcomp', 'fcompp', 'fucom', 'fucomp', 'fucompp', 'ficom', 'ficomp'):
            a = self.fst(st, 0)
            if ops and ops[0].type == X.X86_OP_MEM:
                b = self.load(st, eff, self.addr_of(st, ops[0].mem, ops[0].size), ops[0].size)
                b = ('i2f', ops[0].size, b) if m.startswith('fi') else ('f', ops[0].size, b)
            else:
                b = self.fst(st, stv(ops[0]) if ops else 1)
            st.fsw = ('fcmp', a, b)
            n = 2 if m.endswith('pp') else 1 if m.endswith('p') else 0
            for _ in range(n):
                self.fpop(st)
        elif m == 'ftst':
            st.fsw = ('fcmp', self.fst(st, 0), ('fconst', 'fldz'))
        elif m == 'fnstsw':
            if st.fsw is None:
                raise Unsupported('fnstsw without compare')
            self.wr(st, eff, ops[0], ('fsw', st.fsw))
        elif m in ('fnclex', 'fclex', 'fninit', 'fwait'):
            pass
        elif m in ('fnstcw', 'fstcw'):
            self.wr(st, eff, ops[0], ('fcw',))
        elif m == 'fldcw':
            v = self.rd(st, eff, ops[0])
            eff.append(('fldcw', v))
        else:
            raise Unsupported('fpu ' + m)


def fval(size, v):
    """value read by an x87 load of size bytes from memory holding v. A spill of an
    exactly representable int conversion (a 16-bit or 32-bit int in a double, a
    16-bit int in a float) reloads as that same conversion."""
    w = v
    if w[0] == 'ext' and w[1] == 0 and w[2] == size * 8:
        w = w[3]
    if w[0] == 'tof' and w[1] == size:
        x = w[2]
        if x[0] == 'i2f' and ((size == 8 and x[1] in (2, 4)) or
                              (size == 4 and x[2][0] == 'sx' and x[2][1] <= 16)):
            return x
    return ('f', size, v)


def farith(base, a, b):
    if base in ('add', 'mul'):
        return op('f' + base, a, b, comm=True)
    if base == 'subr':
        return ('fsub', b, a)
    if base == 'divr':
        return ('fdiv', b, a)
    return ('f' + base, a, b)


FLAGS_IN = ('flags-in',)


def literal_comparison(cc, flags):
    """Decide CMP/SUB only from literal operands and its machine width."""
    if flags is None or flags[0] != 'literal-flags' or flags[1] not in (1, 2, 4) or \
            flags[3][:2] != ('fl', 'sub') or not all(is_c(v) for v in flags[3][2:]):
        return None
    width = flags[1] * 8
    mask, sign = (1 << width) - 1, 1 << (width - 1)
    a, b = (v[1] & mask for v in flags[3][2:])
    result = (a - b) & mask
    carry, zero = a < b, result == 0
    negative, overflow = bool(result & sign), bool((a ^ b) & (a ^ result) & sign)
    parity = (result & 255).bit_count() % 2 == 0
    return {'e': zero, 'ne': not zero, 'b': carry, 'ae': not carry,
            'be': carry or zero, 'a': not carry and not zero,
            's': negative, 'ns': not negative, 'o': overflow, 'no': not overflow,
            'l': negative != overflow, 'ge': negative == overflow,
            'le': zero or negative != overflow, 'g': not zero and negative == overflow,
            'p': parity, 'np': not parity}.get(CC_ALIAS.get(cc, cc))


def canon_cond(cc, flags):
    if flags is None:
        raise Unsupported('branch on unknown flags')
    cc = CC_ALIAS.get(cc, cc)
    if flags[0] == 'literal-flags':
        if cc in ('e', 'ne') and is_c(flags[2]):
            # These ALU instructions always update ZF from the width-truncated
            # result. Other conditions retain width, operands and saved carry.
            return ('cond', cc, ('fl', 'sub', C(int(flags[2][1] != 0)), C(0)))
        if cc in ('b', 'ae', 'be', 'a') and flags[3][0:2] in (('fl', 'inc'), ('fl', 'dec')) \
                and flags[4] is None:
            raise Unsupported('carry preserved from unknown flags')
    if cc in ('e', 'ne') and flags[0:2] == ('fl', 'sub'):
        a, b = flags[2:]
        if a == C(0):
            a, b = b, a
        if b == C(0) and a[0] == 'and' and len(a) == 3:
            mask, value = a[1:]
            if is_c(value):
                mask, value = value, mask
            if is_c(mask) and 0 < mask[1] < 0x80:
                # Only the selected low bits decide whether the result is zero.
                flags = op('fl', 'and', mask, value, comm=True)
    if cc in ('e', 'ne') and flags[0] == 'fl' and flags[1] == 'and' and len(flags) == 4:
        # a zero test of x & mask only depends on the masked bits: testing a byte of
        # x against m is testing x against m shifted to that byte
        a, b = flags[2], flags[3]
        if is_c(b) and not is_c(a):
            a, b = b, a
        if is_c(a) and b[0] == 'ext':
            lo, n, x = b[1], b[2], b[3]
            flags = op('fl', 'and', C(((a[1] & ((1 << n) - 1)) << lo) & M32), x, comm=True)
            a, b = (flags[2], flags[3]) if is_c(flags[2]) else (flags[3], flags[2])
        if is_c(a) and b[0] == 'ld' and b[1] > 1 and a[1]:
            # a mask within one byte of a wider load tests that byte (little-endian)
            m = a[1] & ((1 << (8 * b[1])) - 1)
            k = (m.bit_length() - 1) // 8
            if m and m >> (8 * k) <= 0xff and m & ((1 << (8 * k)) - 1) == 0:
                flags = op('fl', 'and', C(m >> (8 * k)), ('ld', 1, add(b[2], C(k)), b[3]), comm=True)
    if flags[0] == 'fl' and flags[1] == 'sub' and cc in CC_SWAP:
        a, b = flags[2], flags[3]
        if repr(a) > repr(b):
            return ('cond', CC_SWAP[cc], ('fl', 'sub', b, a))
    fpred = x87_compare_predicate(cc, flags)
    if fpred is not None:
        return fpred
    return ('cond', cc, flags)


# x87 compare outcomes and the status-word condition bits FCOM leaves for each
# (C0 = 0x100, C2 = 0x400, C3 = 0x4000 after FNSTSW): st(0) < src, equal,
# greater, unordered.
X87_OUTCOMES = (('lt', 0x100), ('eq', 0x4000), ('gt', 0), ('un', 0x4500))
X87_MIRROR = {'lt': 'gt', 'gt': 'lt', 'eq': 'eq', 'un': 'un'}


def x87_compare_predicate(cc, flags):
    """A zero test of FNSTSW's C0/C2/C3 bits after FCOM a, b is a predicate over the
    four compare outcomes. Written as the set of outcomes that take the branch, with
    the operands in a canonical order (swapping them mirrors < and >), `fcom a, b;
    test ah, 41h; je` and `fcom b, a; test ah, 1; jne` become the same condition.
    Masks touching any other status bit are left alone."""
    if cc not in ('e', 'ne') or flags[0:2] != ('fl', 'and') or len(flags) != 4:
        return None
    m, x = flags[2], flags[3]
    if is_c(x) and not is_c(m):
        m, x = x, m
    if not is_c(m) or not m[1] or m[1] & ~0x4500:
        return None
    if x[0] == 'ext' and x[1] == 0 and x[2] >= m[1].bit_length():
        x = x[3]
    if x[0] != 'fsw' or x[1][0] != 'fcmp':
        return None
    a, b = x[1][1], x[1][2]
    taken = {name for name, bits in X87_OUTCOMES if ((bits & m[1]) == 0) == (cc == 'e')}
    if repr(a) > repr(b):
        a, b = b, a
        taken = {X87_MIRROR[name] for name in taken}
    return ('cond', 'fpred', ('fpred', tuple(sorted(taken)), a, b))


def neg_cond(c):
    if c[1] == 'fpred':
        rest = tuple(sorted({name for name, _ in X87_OUTCOMES} - set(c[2][1])))
        return ('cond', 'fpred', ('fpred', rest) + c[2][2:])
    return ('cond', CC_NEG[c[1]], c[2])


def bounded_zero_comparison(condition, size):
    """Decide a 32-bit signed zero comparison from its complete value range.

    The range comes from machine-sized loads, extracts, masks and modular linear
    arithmetic. A narrow CMP has a different sign bit and is never widened here.
    Only the condition changes; the actual flags and read histories remain live.
    """
    _, cc, flags = condition
    if size != 4 or cc not in ('g', 'ge', 'l', 'le') or \
            flags[0:2] != ('fl', 'sub') or flags[2] != C(0):
        return condition
    intervals = address_intervals(flags[3], 1)
    if len(intervals) != 1:
        return condition
    low, end = intervals[0]
    if not 0 <= low < end <= 0x80000000:
        return condition
    if cc in ('ge', 'l') and low == 0:
        return condition                 # equality with zero remains possible
    taken = cc in ('l', 'le')
    return ('cond', 'e' if taken else 'ne', ('fl', 'sub', C(0), C(0)))


# ---------------------------------------------------------------- liveness

def esp_deltas(func, escaped, returns_value):
    """frame offset of esp at each block entry (entry block: 0). Argument counts of
    calls through pointers are ambiguous; use whichever counting rule keeps esp
    consistent across the whole function."""
    err = None
    for floor in (False, True):
        func.use_floor = floor
        try:
            return _esp_deltas(func, escaped, returns_value)
        except Unsupported as e:
            err = e
    func.use_floor = False
    raise err


def _esp_deltas(func, escaped, returns_value):
    """per block entry: {register: frame offset} for registers holding a stack address
    on every incoming path (esp always). Registers whose stack-ness differs between
    paths are recorded in func.ambiguous and must not be live there."""
    ev = Evaluator(func, escaped, returns_value)
    spr = {func.addr: {'esp': 0}}
    func.spr = spr          # kept for debugging
    func.sp_rec, func.collect_sp = set(), True
    func.ambiguous = {}
    func.ambiguous_offs = {}    # block -> {register: stack offsets it holds on some path}
    func.fdepth = {func.addr: getattr(func, 'entry_fdepth', 0)}
    work = [func.addr]
    guard = 0
    while work:
        guard += 1
        if guard > 20000:
            raise Unsupported('stack register analysis does not converge')
        s = work.pop()
        m = spr[s]
        entry = lambda loc, m=m: add(SP, C(m[loc])) if loc in m else ('in', loc)
        b = ev.run(s, entry)
        out = {}
        for r in REG32:
            v = b.state.regs.get(r, entry(r))
            o = stack_off(v)
            if o is not None:
                out[r] = o
            elif has_sp(v) and not escaped:
                raise Unsupported('register %s holds a computed stack address' % r)
        if 'esp' not in out:
            raise Unsupported('esp not frame-relative at end of %x' % s)
        depth = len(b.state.fpu)
        for t in term_succs(b.term):
            if t not in func.blocks:
                raise Unsupported('successor %x not a block' % t)
            if func.fdepth.setdefault(t, depth) != depth:
                raise Unsupported('inconsistent x87 stack depth at %x' % t)
            if t not in spr:
                spr[t] = dict(out)
                work.append(t)
                continue
            cur = spr[t]
            if cur.get('esp') != out['esp']:
                raise Unsupported('inconsistent esp at %x (%s from %x, %s before)' % (t, out['esp'], s, cur.get('esp')))
            new = {r: o for r, o in cur.items() if out.get(r) == o}
            amb = {r for r in set(cur) | set(out) if cur.get(r) != out.get(r)}
            if amb:
                func.ambiguous.setdefault(t, set()).update(amb)
                offs = func.ambiguous_offs.setdefault(t, {})
                for r in amb:
                    offs.setdefault(r, set()).update(x[r] for x in (cur, out) if r in x)
            if new != cur:
                spr[t] = new
                work.append(t)
    func.collect_sp = False
    if escaped and any(func.ambiguous_offs.values()):
        # A stale pointer overwritten before use at a merge cannot expose locals.
        # Compute liveness conservatively with the entire frame in memory first.
        recorded = func.sp_rec
        func.sp_rec = None
        live = liveness(summarize(func, escaped, returns_value, spr))
        func.sp_rec = recorded
        if any(regs & live.get(block, set()) for block, regs in func.ambiguous.items()):
            func.sp_rec.add(None)   # a live pointer varies by path: keep all memory
    return spr


def stack_entry(m):
    return lambda loc: add(SP, C(m[loc])) if isinstance(loc, str) and loc in m else ('in', loc)


def summarize(func, escaped, returns_value, delta, remap=None):
    """Per block: (read set, written set, successors, evaluated block), for liveness."""
    ev = Evaluator(func, escaped, returns_value, remap)
    out = {}
    for s in func.blocks:
        if s not in delta:
            continue
        m = delta[s]
        b = ev.run(s, stack_entry(m))
        succ = term_succs(b.term)
        out[s] = (b.state.read - set(m), b.state.written, succ, b)
    return out


def global_holder_facts(func, summ, delta):
    """Must facts: fixed global/frame memory equals a register on every incoming path.

    Start the entry at no facts and other reachable blocks at the finite top.
    Calls and potentially aliasing writes kill facts; exact stores establish
    them. Intersect predecessor facts, including back edges, to convergence.
    """
    objects = {(v[2], v[1]) for _, _, _, b in summ.values() for v in b.effects
               if v[0] == 'st' and v[1] == 4 and
               (v[2][0] == 'c' or (stack_off(v[2]) is not None and stack_off(v[2]) < 0))}
    if not objects:
        return {}
    top = {(a, n, r) for a, n in objects for r in REG32 if r != 'esp'}
    incoming = {s: set() if s == func.addr else set(top) for s in summ}
    preds = {s: [] for s in summ}
    for s, (_, _, succs, _) in summ.items():
        for t in succs:
            if t in preds:
                preds[t].append(s)

    def transfer(s):
        b = summ[s][3]
        entry = stack_entry(delta[s])
        entry_values = {r: entry(r) for r in REG32 if r != 'esp'}
        values = {(a, n): {entry_values[r] for aa, nn, r in incoming[s] if (aa, nn) == (a, n)}
                  for a, n in objects}
        for effect in b.effects:
            if effect[0] in ('call', 'str'):
                for key in values:
                    values[key] = set()
            elif effect[0] == 'st':
                for (a, n) in values:
                    if effect[2] == a and effect[1] == n:
                        values[a, n] = {effect[3]}
                    elif may_alias(effect[2], effect[1], a, n):
                        values[a, n] = set()
        # Each block has one exit value per register. Reusing these digests also
        # avoids retaining fresh entry tuples for every memory object and round.
        if not any(values.values()):
            return set()
        exit_digests = {r: D(exit_value(b.state, r, entry)) for r in REG32
                        if r != 'esp' and r not in delta[s]}
        return {(a, n, r) for (a, n), vs in values.items() for digest in {D(v) for v in vs}
                for r, value in exit_digests.items() if digest == value}

    for _ in range(len(summ) * (len(top) + 1) + 1):
        outgoing = {s: transfer(s) for s in summ}
        new = {s: set() if s == func.addr or not preds[s] else
               set.intersection(*(outgoing[p] for p in preds[s])) for s in summ}
        if new == incoming:
            return incoming
        incoming = new
    raise Unsupported('global holder facts do not converge')


def term_succs(t):
    k = t[0]
    if k == 'jmp':
        return [t[1]]
    if k == 'fall':
        return [t[1]]
    if k == 'jcc':
        return [t[2], t[3]]
    if k == 'switch':
        return list(t[2])
    return []


FULL = M32


class Live(dict):
    """block -> set of live-in locations, plus .masks: block -> {register: live bits}"""
    masks = None


def _uses(x, out, mask=FULL):
    """record in out[loc] the bits of each entry location ('in', loc) that value x
    depends on, following ext/ins/zx/sx so that unused high bits stay dead"""
    if not isinstance(x, tuple) or not x or not mask:
        return
    k = x[0]
    if not isinstance(k, str):
        for y in x:
            _uses(y, out)
        return
    if k == 'in':
        out[x[1]] = out.get(x[1], 0) | mask
        return
    if k in ('c', 'junk', 'preserved', 'unbound'):
        return
    if k in ('f', 'i2f') and len(x) == 3 and isinstance(x[1], int):
        # A floating load or integer conversion consumes every source byte.
        # FULL is a register's 32-bit mask, not a double or qword's width.
        _uses(x[2], out, (1 << (8 * x[1])) - 1)
        return
    if k == 'ext':
        _uses(x[3], out, (mask & ((1 << x[2]) - 1)) << x[1])
        return
    if k == 'ins':
        region = ((1 << x[3]) - 1) << x[2]
        _uses(x[1], out, mask & ~region & FULL)
        _uses(x[4], out, (mask & region) >> x[2])
        return
    if k in ('zx', 'sx'):
        _uses(x[2], out, (1 << x[1]) - 1)
        return
    if k == 'fl' and len(x) == 4 and x[1] == 'and':
        # the flags of a test of x against a constant only depend on its bits
        a, b = x[2], x[3]
        if is_c(b) and not is_c(a):
            a, b = b, a
        if is_c(a):
            _uses(b, out, a[1] & FULL)
            return
    if _EXTENDED and k == 'cat' and sum(width for width, _ in x[1]) <= 32:
        offset = 0
        for width, value in x[1]:
            _uses(value, out, (mask >> offset) & ((1 << width) - 1))
            offset += width
        return
    if _EXTENDED and k == 'and':
        a, b = x[1:]
        if is_c(a):
            _uses(b, out, mask & a[1])
        elif is_c(b):
            _uses(a, out, mask & b[1])
        else:
            _uses(a, out, mask)
            _uses(b, out, mask)
        return
    if _EXTENDED and k == 'shl' and is_c(x[2]):
        _uses(x[1], out, mask >> (x[2][1] & 31))
        return
    if k == 'lin':
        # carries only run upward: bits up to the highest one used need the same low
        # bits of every term (times a constant) and nothing above
        low = (1 << mask.bit_length()) - 1
        for t, _ in x[2]:
            _uses(t, out, low)
        return
    for y in x[1:]:
        _uses(y, out)


def liveness(summ):
    """live-in locations per block. Registers are tracked per bit: a register whose
    successors only use its low bits is only compared on those bits."""
    seeds = {}
    for s, (r, w, sc, b) in summ.items():
        u = {}
        _uses(b.term, u)
        for e in b.effects:
            _uses(e, u)
        for loc in r:                       # flags and x87 values: whole
            if not (isinstance(loc, str) and loc in REG32) and not _is_slot(loc):
                u[loc] = FULL
        # stack slots, like registers, are live only where their entry value flows
        # into an effect, a branch or a live exit value (an uninitialised pad word
        # pushed as an argument the callee never reads is dead)
        seeds[s] = u
    live = {s: dict(seeds[s]) for s in summ}
    entry = lambda l: ('in', l)
    changed = True
    rounds = 0
    while changed:
        changed = False
        rounds += 1
        if rounds > 1000:
            raise Unsupported('liveness does not converge')
        for s, (r, w, sc, b) in summ.items():
            new = dict(seeds[s])
            outs = {}
            for t in sc:
                for loc, m in live.get(t, {}).items():
                    outs[loc] = outs.get(loc, 0) | m
            for loc, m in outs.items():
                if loc == 'esp':
                    continue
                _uses(exit_value(b.state, loc, entry), new, m)
            if new != live[s]:
                live[s] = new
                changed = True
    out = Live({s: set(v) for s, v in live.items()})
    out.masks = live
    return out


# ---------------------------------------------------------------- comparison

def frame_pointer_value(value):
    """Whether the value itself contains a frame address. A load's address
    and a call result's dependencies do not make its data a frame pointer.
    Storing a frame pointer into memory already disables the precise tier.
    """
    if value == SP:
        return True
    if isinstance(value, tuple) and value and value[0] not in ('ld', 'ret', 'junk'):
        return any(frame_pointer_value(v) for v in value if isinstance(v, tuple))
    return False


def bounded_frame_memory(func, delta, returns_value):
    """Exact byte regions for bounded indexed accesses whose pointers stay local.
    Unknown callees, published pointers and unbounded addresses use the existing
    conservative frame model. Fixed local addresses need only ordinary slots.
    """
    made = func.sp_rec
    if made is None or None in made or func.frame_floor is None:
        return None
    class Collector(Evaluator):
        def __init__(self):
            super().__init__(func, True, returns_value)
            self.indexed = []
            self.published = False
        def remember(self, addr, size):
            if has_sp(addr) and stack_off(addr) is None:
                self.indexed.append((addr, size))
        def load(self, st, eff, addr, size):
            self.remember(addr, size)
            return super().load(st, eff, addr, size)
        def store(self, st, eff, addr, size, val):
            self.remember(addr, size)
            if frame_pointer_value(val):
                self.published = True
            return super().store(st, eff, addr, size, val)
        def call(self, st, eff, i, nxt):
            # A frame address in a register can be an implicit register argument.
            # Keep the conservative model even when it was not pushed.
            if any(frame_pointer_value(st.reg(r)) for r in REG32 if r != 'esp'):
                self.published = True
            count = len(eff)
            result = super().call(st, eff, i, nxt)
            for effect in eff[count:]:
                if effect[0] == 'call' and frame_pointer_value(effect[1:4]):
                    self.published = True
            return result
    ev = Collector()
    func.sp_rec = None
    try:
        for block, registers in delta.items():
            summary = ev.run(block, stack_entry(registers))
            if summary.term[0] == 'tail' or (summary.term[0] == 'ret' and frame_pointer_value(summary.term[1:3])):
                ev.published = True
    finally:
        func.sp_rec = made
    if ev.published:
        return None
    regions = []
    for addr, size in ev.indexed:
        const, terms = lin_parts(addr)
        if terms.pop(SP, None) != 1:
            return None
        for low, high in address_intervals(mk_lin(const, terms), size):
            if low < 0x80000000 or high > 1 << 32:
                return None
            low -= 1 << 32
            high -= 1 << 32
            if low < func.frame_floor or high > 0:
                return None
            if any(low < off + 4 and off < high for off in getattr(func, 'save_offs', ())):
                return None
            regions.append((low, high))
    return regions


def frame_origins(value, mask=FULL):
    """Pointer provenance of surviving bits, including partial overwrites."""
    if not isinstance(value, tuple) or not value or not mask:
        return set()
    if value[0] == 'capture':
        return set(value[2])
    if value == SP:
        return {'own-frame'}
    if value[0] == 'data':
        return set()
    if value[0] == 'ext':
        return frame_origins(value[3], (mask & ((1 << value[2]) - 1)) << value[1])
    if value[0] == 'ins':
        region = ((1 << value[3]) - 1) << value[2]
        return frame_origins(value[1], mask & ~region & FULL) | \
               frame_origins(value[4], (mask & region) >> value[2])
    if value[0] in ('shl', 'shr', 'sar') and is_c(value[2]):
        shift = value[2][1] & 31
        source = mask >> shift if value[0] == 'shl' else (mask << shift) & FULL
        if value[0] == 'sar' and shift and mask & (FULL << (32 - shift)):
            source |= 1 << 31
        return frame_origins(value[1], source)
    if value[0] in ('sx', 'zx'):
        source = mask & ((1 << value[1]) - 1)
        if value[0] == 'sx' and mask >> value[1]:
            source |= 1 << (value[1] - 1)
        return frame_origins(value[2], source)
    if value[0] == 'lin':
        origins = set()
        for term, coefficient in value[2]:
            zeros = (coefficient & -coefficient).bit_length() - 1
            source = ((1 << mask.bit_length()) - 1) >> zeros
            origins.update(frame_origins(term, source))
        return origins
    if value[0] == 'and' and len(value) == 3:
        if is_c(value[1]):
            return frame_origins(value[2], mask & value[1][1])
        if is_c(value[2]):
            return frame_origins(value[1], mask & value[2][1])
    return set().union(*(frame_origins(v) for v in value if isinstance(v, tuple)))


def input_frame_footprint(code, addr, source, oracle):
    """Bytes reachable from one input, proved without retaining its pointer bits.

    Other inputs are independent data. Loops meet pointer provenance and widen
    unrelated arithmetic to instruction-local data. Every pointer access needs
    a fixed displacement, and every nested call needs a binary-derived stack
    footprint and a separate proof for each supplied pointer. Unknown targets,
    published pointer bits, pointer-controlled branches, and varying pointer
    arithmetic refuse. The second result covers reads AND writes through the
    callee's incoming stack, including accesses beyond its declared arguments.
    """
    if len(code) > 8192:
        return None
    f = Func(code, addr)
    if len(f.ins) > 1024 or len(f.blocks) > 128:
        return None
    for depth in (0, 1):
        try:
            f.entry_fdepth = depth
            delta = esp_deltas(f, stack_escapes(f), True)
            break
        except Unsupported:
            if depth == 1:
                raise
    root = ('capture', 'input', ('input',))
    spans, stack_spans = set(), set()

    def initial(loc):
        if loc == 'esp':
            return SP
        if loc == source:
            return root
        if isinstance(source, int) and _is_slot(loc):
            low = 4 + source * 4
            if low <= loc[1] and loc[1] + loc[2] <= low + 4:
                return ext(root, (loc[1] - low) * 8, loc[2] * 8)
            if loc[1] < low + 4 and low < loc[1] + loc[2]:
                return ('capture', ('partial', loc), ('input',))
        return ('data', ('entry', loc))

    def abstract(value, key):
        if is_c(value) or stack_off(value) is not None or value[0] in ('capture', 'data'):
            return value
        offset, terms = lin_parts(value)
        if terms == {root: 1}:
            return value
        origins = frame_origins(value)
        return ('capture', key, tuple(sorted(origins))) if origins else ('data', key)

    def merge(values, key):
        if all(v == values[0] for v in values):
            return values[0]
        origins = set().union(*(frame_origins(v) for v in values))
        return ('capture', key, tuple(sorted(origins))) if origins else ('data', key)

    class Footprint(Evaluator):
        def pointer_access(self, address, size):
            origins = frame_origins(address)
            if not origins:
                return
            offset, terms = lin_parts(address)
            if terms != {root: 1} or origins != {'input'}:
                raise Unsupported('unbounded input pointer access')
            offset = offset if offset < 0x80000000 else offset - (1 << 32)
            if not -4096 <= offset <= 4096 or not 0 < size <= 16:
                raise Unsupported('input pointer span exceeds bound')
            spans.add((offset, size))

        def stack_access(self, off, size):
            if off + size > 0:
                if off < 4 or off + size > 4096:
                    raise Unsupported('callee access overlaps return address or unbounded arguments')
                stack_spans.add((off, size))

        def load(self, st, eff, address, size):
            off = stack_off(address)
            if off is not None:
                self.stack_access(off, size)
                return self.slot_load(st, off, size)
            self.pointer_access(address, size)
            if frame_origins(address):
                # An input object's field can itself contain a pointer. Keep
                # those bits tainted until a mask/overwrite removes them; an
                # unbounded dereference or publication cannot omit its target.
                return ('capture', (self.cur, size), ('loaded-input',))
            return ('data', (self.cur, size))

        def store(self, st, eff, address, size, value):
            off = stack_off(address)
            if off is not None:
                self.stack_access(off, size)
                if off >= 0 and frame_origins(value):
                    raise Unsupported('pointer published into incoming argument area')
                self.slot_store(st, off, size, value)
                return
            if frame_origins(value):
                raise Unsupported('callee publishes pointer bits')
            self.pointer_access(address, size)

        def string_op(self, st, eff, mnemonic, instruction):
            # Expanded constant copies use load/store callbacks. Variable copies
            # would require a proved byte count and cannot supply a footprint.
            base = mnemonic.split()[-1]
            values = [st.get('edi')]
            if not base.startswith('stos'):
                values.append(st.get('esi'))
            if base.startswith('scas') or (mnemonic.startswith('rep ') and
                    (not is_c(st.get('ecx')) or st.get('ecx')[1] > 256)):
                if any(frame_origins(v) for v in values):
                    raise Unsupported('unbounded pointer string operation')
                # Do not hide pointer bits in an unexpanded STOS value.
                if base.startswith('stos') and frame_origins(st.get('eax')):
                    raise Unsupported('string store publishes pointer bits')
            return super().string_op(st, eff, mnemonic, instruction)

        def nested(self, st, eff, instruction, nxt, tail=False):
            if instruction.operands[0].type != X.X86_OP_IMM:
                raise Unsupported('indirect footprint call')
            target = instruction.operands[0].imm
            contract = oracle(target, None)
            if contract is None:
                raise Unsupported('unproved nested incoming-stack accesses')
            off = stack_off(st.reg('esp'))
            if off is None or (tail and off != 0):
                raise Unsupported('unresolved nested stack')
            if any(frame_origins(v) for v in st.fpu):
                raise Unsupported('pointer bits supplied on x87 stack')
            inputs = {r: st.reg(r) for r in REG32 if r != 'esp'}
            # Read every byte the nested machine code can touch in the incoming
            # area. Caller cleanup/declared argument counts do not bound it.
            for relative, size in contract[1]:
                address = off + relative - (0 if tail else 4)
                self.stack_access(address, size)
                for k in range((relative - 4) // 4, (relative + size - 5) // 4 + 1):
                    loc = off + k * 4 + (4 if tail else 0)
                    inputs[k] = self.slot_load(st, loc, 4)
            for key, value in inputs.items():
                origins = frame_origins(value)
                if not origins:
                    continue
                child = oracle(target, key)
                if child is None:
                    raise Unsupported('nested callee can retain input pointer')
                own = stack_off(value)
                for offset, size in child[0]:
                    if own is not None:
                        self.stack_access(own + offset, size)
                        content = self.slot_load(st, own + offset, size)
                        if frame_origins(content):
                            raise Unsupported('nested callee reads pointer-valued frame bytes')
                        # Reads and writes share a conservative span. A write
                        # can destroy local saved pointers or register saves.
                        self.slot_store(st, own + offset, size, ('data', (self.cur, key, offset)))
                    else:
                        self.pointer_access(add(value, C(offset)), size)
            # A nested positive stack write must also be reflected in local
            # spills, independently of the ordinary call argument heuristic.
            for relative, size in contract[1]:
                address = off + relative - (0 if tail else 4)
                old_value = self.slot_load(st, address, size)
                origins = frame_origins(old_value)
                value = ('capture', (self.cur, 'stack', relative), tuple(sorted(origins))) if origins else \
                        ('data', (self.cur, 'stack', relative))
                # A read-only or partial-write call can leave old pointer bits
                # in its argument cells. May provenance must survive the call.
                self.slot_store(st, address, size, value)
            if tail:
                st.set_reg('eax', ('data', (self.cur, 'eax')))
                return ('ret', st.reg('eax'), None, 0)
            count, before = len(eff), dict(st.slots)
            result = super().call(st, eff, instruction, nxt)
            for register in ('eax', 'ecx', 'edx'):
                st.set_reg(register, ('data', (self.cur, register)))
            for offset, (size, value) in list(st.slots.items()):
                if value[0] == 'argafter':
                    previous = slot_value(before, offset, size, initial) or initial(('s', offset, size))
                    origins = frame_origins(previous)
                    value = ('capture', (self.cur, offset), tuple(sorted(origins))) if origins else \
                            ('data', (self.cur, offset))
                    st.slots[offset] = (size, value)
            del eff[count:]
            return result

        def call(self, st, eff, instruction, nxt):
            return self.nested(st, eff, instruction, nxt)

        def step(self, st, eff, instruction, nxt):
            term = super().step(st, eff, instruction, nxt)
            if term is not None and term[0] == 'tail':
                return self.nested(st, eff, instruction, nxt, tail=True)
            return term

    ev = Footprint(f, False, True)
    outputs, predecessors = {}, collections.defaultdict(set)
    for block in delta:
        for successor in f.succ[block]:
            if successor in delta:
                predecessors[successor].add(block)
    work, rounds = [addr], 0
    while work:
        rounds += 1
        if rounds > 10000:
            raise Unsupported('input footprint does not converge')
        block = work.pop()
        parents = [outputs[p] for p in predecessors[block] if p in outputs]
        if block == addr:
            if parents:
                raise Unsupported('back edge to callee entry')
            regs, slots, fpu = {}, {}, []
        elif not parents:
            continue
        else:
            regs = {r: merge([p[0][r] for p in parents], (block, r)) for r in REG32}
            slots = {}
            boundaries = sorted({off + end for _, ss, _ in parents
                                 for off, (size, _) in ss.items() for end in (0, size)})
            for low, high in zip(boundaries, boundaries[1:]):
                loc = ('s', low, high - low)
                values = [slot_value(ss, low, high - low, initial) or initial(loc)
                          for _, ss, _ in parents]
                slots[low] = (high - low, merge(values, (block, loc)))
            if len({len(p[2]) for p in parents}) != 1:
                raise Unsupported('unequal footprint x87 depths')
            fpu = [merge([p[2][k] for p in parents], (block, 'f', k))
                   for k in range(len(parents[0][2]))]

        def entry(loc):
            if isinstance(loc, str):
                return regs.get(loc, initial(loc))
            if _is_slot(loc):
                return slot_value(slots, loc[1], loc[2], initial) or initial(loc)
            if loc[0] == 'f' and loc[1] < len(fpu):
                return fpu[-1 - loc[1]]
            return initial(loc)
        summary = ev.run(block, entry)
        if summary.term[0] == 'ret':
            if frame_origins(summary.term[1:3]):
                raise Unsupported('callee returns pointer bits')
            if any(summary.state.regs.get(r, entry(r)) != initial(r) for r in CALLEE_SAVED):
                raise Unsupported('callee fails to preserve saved register')
        elif frame_origins(summary.term):
            raise Unsupported('pointer controls callee branch')
        carrier = State(initial)
        carrier.slots = dict(slots)
        for off, (size, value) in summary.state.slots.items():
            ev.slot_store(carrier, off, size, abstract(value, (block, off, size)))
        output = ({r: abstract(summary.state.regs.get(r, entry(r)), (block, r)) for r in REG32},
                  carrier.slots, [abstract(v, (block, 'f', k)) for k, v in enumerate(summary.state.fpu)])
        if outputs.get(block) != output:
            outputs[block] = output
            work.extend(s for s in f.succ[block] if s in delta and s not in work)
    if len(outputs) != len(delta):
        raise Unsupported('unreached callee footprint blocks')
    return tuple(sorted(spans)), tuple(sorted(stack_spans))


def bounded_callee_frame_memory(func, delta, returns_value, known_only=False):
    """Partition local bytes using all indexed accesses and supplied addresses.

    A supplied frame pointer requires a per-input, non-retaining binary proof.
    Calls keep the ordinary ABI argument-area model; available binary stack
    summaries additionally include direct incoming-stack reads and writes.
    Pointer-valued local spills or unbounded/opaque pointer inputs refuse.
    known_only collects constraints without claiming privacy: opaque inputs are
    skipped, so they cannot hide known extents before or after another call.
    """
    if func.frame_floor is None or (not known_only and
            (FRAME_INPUT is None or None in (func.sp_rec or ()))):
        return None
    floor, regions, views, constraints = func.frame_floor, [], [], []
    indexed, supplied, incoming, fixed = [], [], [], []

    def remember(low, high):
        if low >= 0 or high <= floor:
            return
        if known_only:
            regions.append((max(low, floor), min(high, 0)))
            return
        if low < floor or high > 0 or any(low < off + 4 and off < high for off in func.save_offs):
            raise Unsupported('callee frame span leaves local objects')
        regions.append((low, high))

    class Regions(Evaluator):
        def address(self, address, size):
            if not frame_pointer_value(address) or stack_off(address) is not None:
                return
            offset, terms = lin_parts(address)
            if terms.pop(SP, None) != 1:
                raise Unsupported('unresolved indexed frame address')
            for low, high in address_intervals(mk_lin(offset, terms), size):
                # Relative addresses are modular. Keep every possible local
                # byte even if a numeric index also reaches incoming arguments.
                # No nominal array size or type supplies an extent.
                for first, end, bias in ((max(low, 0x80000000), high, 1 << 32),
                                         (low, min(high, 0x80000000), 0)):
                    if first < end:
                        base = offset if offset < 0x80000000 else offset - (1 << 32)
                        constraints.append((base, first - bias, end - bias))
                        indexed.append((first - bias, end - bias))
                        first, end = max(first - bias, floor), min(end - bias, 0)
                        if first < end:
                            remember(first, end)

        def load(self, st, eff, address, size):
            off = stack_off(address)
            if off is not None and floor <= off < 0 and off + size <= 0:
                views.append((off, off + size))
            self.address(address, size)
            return super().load(st, eff, address, size)

        def store(self, st, eff, address, size, value):
            off = stack_off(address)
            if not known_only and frame_pointer_value(value) and not (off is not None and off < floor):
                raise Unsupported('frame pointer stored outside outgoing arguments')
            if off is not None and floor <= off < 0 and off + size <= 0:
                views.append((off, off + size))
            self.address(address, size)
            return super().store(st, eff, address, size, value)

        def string_op(self, st, eff, mnemonic, instruction):
            base = mnemonic.split()[-1]
            addresses = [st.get('edi')]
            if not base.startswith('stos'):
                addresses.append(st.get('esi'))
            if not known_only and any(frame_pointer_value(v) for v in addresses):
                if base.startswith('scas') or (mnemonic.startswith('rep ') and
                        (not is_c(st.get('ecx')) or st.get('ecx')[1] > 256)):
                    raise Unsupported('unbounded local string access')
            if not known_only and base.startswith('stos') and frame_pointer_value(st.get('eax')):
                raise Unsupported('string store publishes frame pointer')
            return super().string_op(st, eff, mnemonic, instruction)

        def call(self, st, eff, instruction, nxt):
            inputs = {r: st.reg(r) for r in REG32 if r != 'esp'}
            offset = stack_off(st.reg('esp'))
            if offset is None:
                raise Unsupported('unresolved frame call stack')
            target = instruction.operands[0].imm if instruction.operands[0].type == X.X86_OP_IMM else None
            contract = FRAME_INPUT(target, None) if target is not None and FRAME_INPUT is not None else None
            count = max(self.pushed_count(st, offset), max(0, (floor - offset) // 4))
            if nxt is not None and nxt.mnemonic == 'add' and nxt.operands[0].type == X.X86_OP_REG and \
                    self.rn(nxt.operands[0].reg) == 'esp' and nxt.operands[1].type == X.X86_OP_IMM:
                count = max(count, nxt.operands[1].imm // 4)
            if contract is not None:
                for relative, size in contract[1]:
                    count = max(count, (relative + size - 1) // 4)
            raw_arguments = {k: self.load(st, eff, self.frame_address(add(SP, C(offset + 4 * k)), 4), 4)
                             for k in range(count)}
            result = super().call(st, eff, instruction, nxt)
            ret = st.reg('eax')
            if ret[0] != 'ret':
                if known_only:
                    return result
                raise Unsupported('unmodeled frame call result')
            call = ret[1]
            target = call[1][1] if call[1][0] == 'c' else None
            contract = FRAME_INPUT(target, None) if target is not None and FRAME_INPUT is not None else None
            if contract is not None:
                for relative, size in contract[1]:
                    remember(offset + relative - 4, offset + relative - 4 + size)
                    incoming.append((offset + relative - 4, offset + relative - 4 + size))
            else:
                for k in range(len(call[2])):
                    remember(offset + k * 4, offset + k * 4 + 4)
                    incoming.append((offset + k * 4, offset + k * 4 + 4))
            inputs.update(enumerate(call[2]))
            # Liveness-based ARG_WIDTH may discard an input whose pointer bits
            # later survive in an argument cell. The machine access summary and
            # raw outgoing area, not narrowed call effects, decide privacy.
            inputs.update(raw_arguments)
            for source, value in inputs.items():
                if not frame_pointer_value(value):
                    continue
                base = stack_off(value)
                footprint = FRAME_INPUT(target, source) if target is not None and FRAME_INPUT is not None else None
                if base is None or footprint is None:
                    if not known_only:
                        raise Unsupported('unproved escaped frame pointer extent')
                    if footprint is not None:
                        for relative, size in footprint[0]:
                            self.address(add(value, C(relative)), size)
                    continue
                supplied.append((target, source, base, footprint[0]))
                for relative, size in footprint[0]:
                    if not known_only and (base + relative < floor or base + relative + size > 0):
                        raise Unsupported('escaped object leaves local frame')
                    if known_only and not floor <= base < 0:
                        # Outgoing/argument addresses are not relocated. Any
                        # known span reaching locals must remain at identity.
                        fixed.append((base + relative, base + relative + size))
                    else:
                        constraints.append((base, base + relative, base + relative + size))
                    remember(base + relative, base + relative + size)
            return result

    ev = Regions(func, True, returns_value)
    recorded, func.sp_rec = func.sp_rec, None
    try:
        for block, registers in delta.items():
            summary = ev.run(block, stack_entry(registers))
            if not known_only and (summary.term[0] == 'tail' or frame_pointer_value(summary.term[1:3])):
                return None
    except Unsupported:
        return None
    finally:
        func.sp_rec = recorded
    func.callee_frame_fixed = fixed
    func.callee_frame_views = views
    func.callee_frame_indexed = indexed
    func.callee_frame_supplied = supplied
    func.callee_frame_incoming = incoming
    func.callee_frame_constraints = constraints
    return regions


def relocate_callee_regions(func, regions, mapping, check_saved=True, image_floor=None):
    """Validate relocation over every reachable byte, including callee extents.

    An indexed address or supplied pointer must retain one displacement over
    its complete machine-derived extent. Distinct accessible physical bytes
    must remain distinct after mapping, even when the old relocation's local
    instruction log did not include the nested callee's accesses.
    """
    floor = func.frame_floor
    for base, low, high in func.callee_frame_constraints:
        if not floor <= base < 0:
            return None
        bias = mapping(base) - base
        if (low < floor or high > 0) and bias:
            return None
        low, high = max(low, floor), min(high, 0)
        pieces = [(max(low, a), min(high, b), d) for a, b, d in mapping.ranges if low < b and a < high]
        if sum(b - a for a, b, _ in pieces) != high - low or any(d != bias for _, _, d in pieces):
            return None
    accessible = []
    for low, high in sorted(regions + func.callee_frame_views):
        if accessible and low <= accessible[-1][1]:
            accessible[-1][1] = max(accessible[-1][1], high)
        else:
            accessible.append([low, high])
    images = []
    for low, high in accessible:
        pieces = [(max(low, a), min(high, b), d) for a, b, d in mapping.ranges if low < b and a < high]
        if sum(b - a for a, b, _ in pieces) != high - low:
            return None
        images.extend((a + d, b + d) for a, b, d in pieces)
    images.sort()
    if any(a < last for (_, last), (a, _) in zip(images, images[1:])):
        return None
    renamed = []
    for low, high in regions:
        renamed.extend((max(low, a) + d, min(high, b) + d) for a, b, d in mapping.ranges
                       if low < b and a < high)
    destination_floor = floor if image_floor is None else image_floor
    if not all(destination_floor <= a < b <= 0 and (not check_saved or
               not any(a < save + 4 and save < b for save in func.save_offs)) for a, b in renamed):
        return None
    return renamed


def local_retired_bytes(func, delta, returns_value):
    """Overwrite old array bytes after their last possible indexed observation.

    No frame address may be published or supplied to any callee. Earlier arrays
    remain memory regardless of their extent; a later scalar becomes a slot
    only after its bytes are overwritten on every path. An uninitialized read,
    future indexed access, or mixed memory/slot view keeps the original model.
    """
    floor = func.frame_floor
    if floor is None or floor < -4096 or len(func.ins) > 4096 or None in (func.sp_rec or ()):
        return None
    events, observers = {}, set()
    accesses, reads = collections.defaultdict(list), collections.defaultdict(set)

    class Lifetime(Evaluator):
        def load(self, st, eff, address, size):
            off = stack_off(address)
            if off is not None and floor <= off < 0:
                accesses[self.cur].append(set(range(off, off + size)))
                reads[self.cur].update(range(off, off + size))
            if frame_pointer_value(address) and off is None:
                observers.add(self.cur)
            return super().load(st, eff, address, size)

        def store(self, st, eff, address, size, value):
            off = stack_off(address)
            if frame_pointer_value(value):
                raise Unsupported('stored frame address in local lifetime analysis')
            if frame_pointer_value(address) and off is None:
                observers.add(self.cur)
            if off is not None and floor <= off < 0:
                bytes_ = set(range(off, off + size))
                accesses[self.cur].append(bytes_)
                if size > 0 and off + size <= 0 and not any(
                        off < save + 4 and save < off + size for save in func.save_offs):
                    events.setdefault(self.cur, set()).update(bytes_)
            return super().store(st, eff, address, size, value)

        def string_op(self, st, eff, mnemonic, instruction):
            base = mnemonic.split()[-1]
            if frame_pointer_value(st.get('edi')) or (not base.startswith('stos') and
                                                     frame_pointer_value(st.get('esi'))):
                # Variable REP and SCAS do not expand into load/store callbacks.
                observers.add(self.cur)
            if base.startswith('stos') and frame_pointer_value(st.get('eax')):
                raise Unsupported('string store publishes a frame address')
            return super().string_op(st, eff, mnemonic, instruction)

        def call(self, st, eff, instruction, nxt):
            if any(frame_pointer_value(st.reg(r)) for r in REG32 if r != 'esp'):
                raise Unsupported('callee can observe a frame address')
            result = super().call(st, eff, instruction, nxt)
            value = st.reg('eax')
            if value[0] != 'ret' or frame_pointer_value(value[1][1:4]):
                raise Unsupported('unproved frame lifetime call inputs')
            return result

    ev = Lifetime(func, True, returns_value)
    made, func.sp_rec = func.sp_rec, None
    try:
        for block, registers in delta.items():
            summary = ev.run(block, stack_entry(registers))
            if summary.term[0] == 'tail' or frame_pointer_value(summary.term[1:3]):
                return None
    except Unsupported:
        return None
    finally:
        func.sp_rec = made
    if not observers:
        return None
    # Instructions that can reach another pointer observation, including loops.
    following = {}
    predecessors = collections.defaultdict(list)
    for block in delta:
        instructions = func.blocks[block]
        for k, instruction in enumerate(instructions):
            successors = [instructions[k + 1].address] if k + 1 < len(instructions) else \
                         [s for s in func.succ[block] if s in delta]
            following[instruction.address] = successors
            for successor in successors:
                predecessors[successor].append(instruction.address)
    future, work = set(observers), list(observers)
    while work:
        for predecessor in predecessors[work.pop()]:
            if predecessor not in future:
                future.add(predecessor)
                work.append(predecessor)
    writes = {a: bytes_ for a, bytes_ in events.items()
              if a not in future and not bytes_ & reads[a]}
    universe = set().union(*writes.values()) if writes else set()
    # Keep overlapping views together. An unrelated word temporary may be
    # read as a dword with old high bytes; leave its complete view in memory.
    # This avoids creating a mixed memory/slot load for a partial overwrite.
    while True:
        mixed = set().union(*(bytes_ & universe for ranges in accesses.values()
                              for bytes_ in ranges if bytes_ & universe and not bytes_ <= universe))
        if not mixed:
            break
        universe -= mixed
    writes = {a: bytes_ & universe for a, bytes_ in writes.items() if bytes_ & universe}
    if not universe:
        return None
    incoming = {a: set() if a == func.addr else set(universe) for a in following}
    work = list(following)
    rounds = 0
    while work:
        rounds += 1
        if rounds > 100000:
            return None
        address = work.pop()
        parents = predecessors[address]
        new = set.intersection(*(incoming[p] | writes.get(p, set()) for p in parents)) \
              if parents and address != func.addr else set()
        if new != incoming[address]:
            incoming[address] = new
            work.extend(following[address])
    # A slot write on only one incoming path cannot disappear into memory at
    # a merge where its bytes are still read. Dead bytes may drop at an exit.
    live = {a: set() for a in following}
    work = list(following)
    rounds = 0
    while work:
        rounds += 1
        if rounds > 100000:
            return None
        address = work.pop()
        downstream = set().union(*(live[s] for s in following[address]))
        new = (reads[address] | (downstream - events.get(address, set()))) & universe
        if address in observers:
            new |= universe
        if new != live[address]:
            live[address] = new
            work.extend(predecessors[address])
    if any(((incoming[a] | writes.get(a, set())) - incoming[s]) & live[s]
           for a, successors in following.items() for s in successors):
        return None
    # These stores do not read any byte they retire. RMW instructions can
    # update already-private bytes, but cannot establish a new private value.
    for address, bytes_ in writes.items():
        incoming[address] |= bytes_
    # A wide access cannot straddle the memory/slot boundary. Keep the old
    # evaluator for partial overwrites that would require mixing both models.
    if any(bytes_ & incoming[a] and not bytes_ <= incoming[a]
           for a, ranges in accesses.items() for bytes_ in ranges):
        return None
    return incoming


def decided_path_branches(func, delta, escaped, returns_value):
    """Repeat a comparison only on the unique path selected by its earlier branch.

    Every intervening block, including the repeated test, must have exactly one
    predecessor. Evaluate the complete path with one state and memory history;
    only literally identical conditions or their negations decide the test.
    Shared joins, loops, different flags, and changed reads establish no fact.
    """
    if len(delta) > 512 or len(func.ins) > 4096:
        return {}
    parents = collections.defaultdict(set)
    for start in delta:
        for target in func.succ[start]:
            if target in delta:
                parents[target].add(start)

    class Comparisons(Evaluator):
        def __init__(self):
            super().__init__(func, escaped, returns_value)
            self.conditions = {}
        def step(self, state, effects, instruction, nxt):
            term = super().step(state, effects, instruction, nxt)
            if term is not None and term[0] == 'jcc':
                self.conditions[instruction.address] = term[1]
            return term

    decisions = {}
    for start in delta:
        branch = func.blocks[start][-1]
        if not branch.mnemonic.startswith('j') or branch.mnemonic == 'jmp':
            continue
        successors = func.succ[start]
        if len(successors) != 2 or successors[0] == successors[1]:
            continue
        for target in successors:
            previous, chain = start, []
            while target in delta and target != func.addr and target != start and \
                    target not in chain and len(chain) < 8 and parents[target] == {previous}:
                chain.append(target)
                last = func.blocks[target][-1]
                if last.mnemonic.startswith('j') and last.mnemonic != 'jmp':
                    ev = Comparisons()
                    try:
                        ev.run(start, stack_entry(delta[start]), tuple(chain))
                    except Unsupported:
                        break
                    first, repeated = ev.conditions.get(branch.address), ev.conditions.get(last.address)
                    taken = successors.index(chain[0]) == 0
                    # Func.succ lists the direct branch destination first.
                    if first is not None and repeated is not None:
                        if repeated == first:
                            decision = taken
                        elif repeated == neg_cond(first):
                            decision = not taken
                        else:
                            break
                        decisions[last.address] = last.operands[0].imm if decision else last.address + last.size
                    break
                next_blocks = func.succ[target]
                if len(next_blocks) != 1:
                    break
                previous, target = target, next_blocks[0]
    return decisions


def stack_escapes(func):
    # ebp addresses the frame only in functions that set it up as the frame pointer;
    # elsewhere it is an ordinary register
    fp = ('esp', 'ebp') if any(i.mnemonic == 'mov' and i.op_str == 'ebp, esp'
                               for i in func.blocks.get(func.addr, [])) else ('esp',)
    for blk in func.blocks.values():
        for i in blk:
            # a local array indexed by a register: its elements are only reachable as
            # memory, so the frame is treated as address-taken
            for o in i.operands:
                if o.type == X.X86_OP_MEM and o.mem.index and o.mem.base and \
                        md.reg_name(o.mem.base) in fp:
                    return True
            if i.mnemonic == 'lea':
                m = i.operands[1].mem
                b = md.reg_name(m.base) if m.base else None
                if b in fp and md.reg_name(i.operands[0].reg) not in ('esp', 'ebp'):
                    return True
            if i.mnemonic in ('mov', 'push') and i.operands[-1].type == X.X86_OP_REG and \
                    md.reg_name(i.operands[-1].reg) in ('esp',) and not (
                        i.mnemonic == 'mov' and md.reg_name(i.operands[0].reg) == 'ebp'):
                return True
    return False


def norm_entry(loc, first=True):
    """entry symbol. At the function's entry block the callee-saved registers hold
    the caller's values (only ever saved and restored); in later blocks they may hold
    anything, so they get an ordinary symbol bound by the edge pairing."""
    if first and loc in CALLEE_SAVED and loc != 'ebp':
        return ('preserved', loc)       # the caller's value of that register
    if loc == 'esp':
        return SP
    return ('in', loc)


def integer_sum_recipe(parts, constant):
    # Holder names may occur in different orders after register renaming.
    # Integer addition is commutative modulo 2**32; coefficients stay attached.
    return ('sum', tuple(sorted(parts, key=repr)), constant)


def equivalent(cand, exe, addr, returns_value=True, quick=False):
    # Mode, alias assumptions, and LAST_DIFF belong to a complete proof attempt.
    with _PROOF_LOCK:
        # The digest cache retains its expression objects. Compiler-entropy
        # retries must not retain every earlier proof's expression graph.
        _digests.clear()
        try:
            return _equivalent_modes(cand, exe, addr, returns_value, quick)
        finally:
            _digests.clear()


def _equivalent_modes(cand, exe, addr, returns_value=True, quick=False):
    global _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES, _EDGE_HISTORY, _LOCAL_LIFETIMES, _FRAME_FORWARD
    global _PROJECTED_FRAME
    original_mode, original_assume, original_dead, original_frame, original_join = _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES
    original_history = _EDGE_HISTORY
    original_lifetimes = _LOCAL_LIFETIMES
    original_forwarding = _FRAME_FORWARD
    original_projection = _PROJECTED_FRAME
    failures = []
    def missing_partition(reason):
        prerequisite = 'unsupported: x87 integer joins require proved callee frame partitions'
        return reason.startswith(prerequisite) and all(
            part in (prerequisite, 'frame relocated: ' + prerequisite) or
            part.startswith('frame relocation unavailable: ')
            for part in reason.split(' | '))
    try:
        modes = [(False, False, False, False), (True, False, False, False),
                 (True, True, False, False), (True, False, False, True),
                 (True, True, False, True), (True, False, True, True), (True, True, True, True)]
        modes = [mode + (False,) for mode in modes] + [
                 (True, False, False, True, True), (True, True, False, True, True),
                 (True, False, True, True, True), (True, True, True, True, True)]
        modes = [mode + (False,) for mode in modes]
        modes[3:3] = [(True, False, False, False, False, True),
                      (True, False, True, False, False, False)]
        # Preserve every existing proof first. A uniform register holder can
        # exist on only one side of a join; retry the complete proof using both
        # actual frame loads instead of asserting a holder on the other side.
        modes = [mode + (True,) for mode in modes] + [mode + (False,) for mode in modes if mode[0]]
        # Preserve all existing proofs before refining private byte views.
        # Extra precision can require a different recipe at a later loop join.
        original = (EXE_READ(addr) or exe) if EXE_READ else exe
        decoded = [list(md.disasm(code, addr)) for code in (cand, original)]
        narrow_frame = any(
            operand.type == X.X86_OP_MEM and operand.size in (1, 2) and
            (md.reg_name(operand.mem.base) == 'esp' or
             (frame_pointer and md.reg_name(operand.mem.base) == 'ebp'))
            for instructions in decoded
            for frame_pointer in [any(i.mnemonic == 'mov' and i.op_str == 'ebp, esp'
                                      for i in instructions[:16])]
            for i in instructions for operand in i.operands)
        modes = [mode + (False,) for mode in modes] + [
            mode + (True,) for mode in modes if mode[0] and narrow_frame]
        if quick:
            # A search can try the original proof tier across compiler layouts
            # before spending time on every fallback for an unsuccessful one.
            # A failed quick attempt is inconclusive; retry the complete proof.
            modes = modes[:1]
        integer_only = None
        for extended, dead, precise, joins, history, lifetimes, forwarding, projected in modes:
            if joins:
                # Floating functions can refine integer holders only in the
                # precise frame tier. That attempt must independently prove
                # both callee partitions before using the join facts.
                if integer_only is None:
                    integer_only = not any(i.mnemonic.startswith('f')
                                           for instructions in decoded for i in instructions)
                if not integer_only and not precise:
                    continue
            _EXTENDED, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES = extended, dead, precise, joins
            _EDGE_HISTORY = history
            _LOCAL_LIFETIMES = lifetimes
            _FRAME_FORWARD = forwarding
            _PROJECTED_FRAME = projected
            retained = ', frame reads retained' if not forwarding else ''
            if projected:
                retained += ', private byte equalities'
            _ASSUME = False
            result = _equivalent_any(cand, exe, addr, returns_value)
            if result[0]:
                return result[0], result[1] + (', edge recipes' if extended else '') + (', overwritten stores' if dead else '') + (', bounded frame objects' if precise else '') + (', loop joins' if joins else '') + (', predecessor histories' if history else '') + (', local array lifetimes' if lifetimes else '') + retained
            unavailable = joins and integer_only is False and missing_partition(result[1])
            if NOALIAS or addr in NOALIAS_FUNCS:
                _ASSUME = True
                assumed = _equivalent_any(cand, exe, addr, returns_value)
                if assumed[0]:
                    return True, assumed[1] + ', assuming no aliasing (/Oa)' + (', edge recipes' if extended else '') + (', overwritten stores' if dead else '') + (', bounded frame objects' if precise else '') + (', loop joins' if joins else '') + (', predecessor histories' if history else '') + (', local array lifetimes' if lifetimes else '') + retained
                unavailable = unavailable and missing_partition(assumed[1])
                result = False, result[1] + ' | assuming no aliasing (/Oa): ' + assumed[1]
            if unavailable:
                # This added tier has no independent privacy prerequisite.
                # Keep the last applicable tier's actual comparison failure.
                continue
            failures.append((False, result[1] + retained))
        return failures[-1]
    finally:
        _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES = original_mode, original_assume, original_dead, original_frame, original_join
        _EDGE_HISTORY = original_history
        _LOCAL_LIFETIMES = original_lifetimes
        _FRAME_FORWARD = original_forwarding
        _PROJECTED_FRAME = original_projection


def _equivalent_any(cand, exe, addr, returns_value=True):
    global LAST_DIFF
    LAST_DIFF = None
    r = _guarded(_equivalent, cand, exe, addr, returns_value)
    if r[0]:
        return r
    partial_diff = LAST_DIFF
    raw_difference = partial_diff
    full = _guarded(_equivalent, cand, exe, addr, returns_value, None, True)
    if full[0]:
        return full
    first = _store_pair(LAST_DIFF, None) or _store_pair(partial_diff, None)
    # address-taken locals may simply sit at other frame offsets: retry with the
    # draft's frame relocated onto the exe's layout, if a safe relocation exists
    # The map starts from aligned instructions; when a comparison then fails on two
    # stores or call arguments differing only in frame offsets, that pair is pinned and
    # the map rebuilt. A wrong pin can only make the proof fail: injectivity and the
    # full comparison still decide.
    pins = {}
    unpinned_reason = None
    map_failure = None
    if first:
        # Adjacent initializers can already vote for one coherent object range.
        # A pin for just its first narrow store would reset that range after two
        # bytes. Try the complete voted map before imposing corrective pins.
        fm = _guarded(frame_map, cand, exe, addr, returns_value)
        if isinstance(fm, FrameMap):
            LAST_DIFF = None
            free = _guarded(_equivalent, cand, exe, addr, returns_value, fm, True)
            if free[0]:
                return True, free[1] + ', frame relocated'
            unpinned_reason = free[1]

    def pin(oc, oe_size):
        pins[oc] = oe_size
        oe, size = oe_size
        # two neighbouring locals swapped: the other one moves the opposite way
        if size and oe != oc and abs(oe - oc) == size and oe not in pins:
            pins[oe] = (oc, size)
    if first:
        pin(*first)
    relocated = None
    for _ in range(16):
        fm = _guarded(frame_map, cand, exe, addr, returns_value, pins)
        if not isinstance(fm, FrameMap):
            map_failure = frame_map.why if fm is None else fm[1]
            break
        LAST_DIFF = None
        r2 = _guarded(_equivalent, cand, exe, addr, returns_value, fm)
        if r2[0]:
            return True, r2[1] + ', frame relocated'
        relocated = r2[1]
        partial_diff = LAST_DIFF
        # Treating additional locals as observable memory is conservative. Some
        # frame layouts need this common representation to compare both sides.
        full = _guarded(_equivalent, cand, exe, addr, returns_value, fm, True)
        if full[0]:
            return True, full[1] + ', frame relocated'
        learned = _store_pair(LAST_DIFF, fm) or _store_pair(partial_diff, fm)
        if learned is None or learned[0] in pins:
            break
        pin(*learned)
    if _EXTENDED and _PRECISE_FRAME:
        fm = _guarded(word_frame_map, cand, exe, addr, returns_value, raw_difference)
        if isinstance(fm, FrameMap):
            LAST_DIFF = None
            words = _guarded(_equivalent, cand, exe, addr, returns_value, fm)
            if words[0]:
                return True, words[1] + ', frame relocated, private word permutation'
            relocated = words[1] + ', private word permutation'
    relocated = relocated or unpinned_reason
    if relocated:
        entry_failure = 'block %x/%x:' % (addr, addr)
        if unpinned_reason and relocated.startswith(entry_failure) and not unpinned_reason.startswith(entry_failure):
            relocated = unpinned_reason
        # with the frame relocated the comparison usually gets further: say where
        return False, r[1] + ' | frame relocated: ' + relocated
    if map_failure:
        return False, r[1] + ' | frame relocation unavailable: ' + map_failure
    return r


LAST_DIFF = None        # (draft effect, exe effect) of the last block whose effects differed


def _store_pair(diff, fm):
    """(draft frame offset, exe frame offset) when the first differing effects are
    matching stores or call arguments at constant frame offsets. A pin is only
    a search hint; the rebuilt injective map and full proof must still pass."""
    if not diff or not diff[0] or not diff[1]:
        return None
    a, b = diff
    if a[0] == b[0] == 'call':
        if D(a[1]) != D(b[1]) or D(a[3:]) != D(b[3:]) or len(a[2]) != len(b[2]):
            return None
        pairs = [(x, y) for x, y in zip(a[2], b[2]) if D(x) != D(y)]
        if not pairs:
            return None
        for x, y in pairs:
            for value in (x, y):
                offset, terms = lin_parts(value)
                # Positive entry-SP offsets are arguments, not frame objects.
                if terms != {SP: 1} or not 0x80000000 <= offset <= 0xffffffff:
                    return None
        # Learn one object at a time. Do not infer its extent from the callee.
        x, y = pairs[0]
        a, b = ('st', 0, x, C(0)), ('st', 0, y, C(0))
    if a[0] == 'str' and b[0] == 'str' and a[1] == b[1] and D(a[3:]) == D(b[3:]):
        size = 0            # a string copy of the same data: its length may be symbolic
    elif a[0] != 'st' or b[0] != 'st' or a[1] != b[1] or D(a[3]) != D(b[3]):
        return None
    else:
        size = a[1]
    offs = []
    for e in (a, b):
        c, t = lin_parts(e[2])
        if t != {SP: 1}:
            return None
        offs.append(c if c < 0x80000000 else c - (1 << 32))
    yc, oe = offs
    if fm is None:
        return yc, (oe, size)
    # the draft's address is already relocated: map it back to the draft's layout
    for lo, hi, d in fm.ranges:
        if lo + d <= yc < hi + d:
            return yc - d, (oe, size)
    return None


def _guarded(fn, *a):
    _digests.clear()
    try:
        return fn(*a)
    except Unsupported as e:
        return False, 'unsupported: %s' % e
    except RecursionError:
        return False, 'too deep'
    except Exception as e:      # an evaluator bug must never count as a proof
        return False, 'internal error: %s: %s' % (type(e).__name__, e)
    finally:
        # Frame-map searches can try many layouts within one public proof.
        # Keep each attempt's cache, rather than all previous layouts' graphs.
        _digests.clear()


class FrameMap:
    """piecewise relocation of frame offsets: [(start, end, delta)], injective"""
    def __init__(self, ranges):
        self.ranges = ranges

    def __call__(self, off):
        for a, b, d in self.ranges:
            if a <= off < b:
                return off + d
        raise Unsupported('frame offset %d not covered by the relocation' % off)


def _gather_frame_accesses(f, returns_value, stores=None):
    d = esp_deltas(f, True, returns_value)
    f.sp_rec = None             # collect every frame store for relocation votes
    ev = Evaluator(f, True, returns_value)
    ev.access_log = []
    for s0, m in d.items():
        b = ev.run(s0, stack_entry(m))
        if stores is not None:
            for e in b.effects:
                if e[0] == 'st' and not has_sp(e[3]) and not _mentions_entry(e[3]):
                    c, t = lin_parts(e[2])
                    if t == {SP: 1}:
                        off = c if c < 0x80000000 else c - (1 << 32)
                        stores.setdefault((e[1], D(e[3])), []).append(off)
    log = {}
    for a, off, size, indexed in ev.access_log:
        log.setdefault(a, []).append((off, size, indexed))
    return log


def _mentions_entry(x):
    """value built from block-entry symbols (which differ between the two sides)"""
    if isinstance(x, tuple):
        if x and x[0] in ('in', 'preserved', 'unbound'):
            return True
        return any(_mentions_entry(y) for y in x)
    return False


def _insn_key(i):
    kinds = ''.join('r' if o.type == X.X86_OP_REG else 'm' if o.type == X.X86_OP_MEM else 'i' for o in i.operands)
    return i.mnemonic + ' ' + kinds


def frame_map(cand, exe, addr, returns_value, pins=None):
    """relocation of the draft's address-taken frame onto the exe's, learned from the
    stack offsets of aligned instructions; None unless it is injective and covers
    every frame access of the draft. The diagnostic in why describes this
    search's final result, never a discarded intermediate collision."""
    import difflib
    frame_map.why = None
    if EXE_READ is not None:
        exe = EXE_READ(addr) or exe
    fc, fe = Func(cand, addr), Func(exe, addr)
    if not (stack_escapes(fc) or stack_escapes(fe)):
        frame_map.why = 'no address-taken or indexed frame'
        return None
    stc, ste = {}, {}
    lc = _gather_frame_accesses(fc, returns_value, stc)
    le = _gather_frame_accesses(fe, returns_value, ste)
    ic = [fc.ins[a] for a in sorted(fc.ins)]
    ie = [fe.ins[a] for a in sorted(fe.ins)]
    sm = difflib.SequenceMatcher(None, [_insn_key(i) for i in ic], [_insn_key(i) for i in ie], autojunk=False)
    votes = {}
    for a, b, n in sm.get_matching_blocks():
        if n < 2:
            continue                            # lone matches are mostly alignment noise
        for k in range(n):
            xc, xe = lc.get(ic[a + k].address), le.get(ie[b + k].address)
            if xc and xe and len(xc) == len(xe):
                for (oc, sc, _), (oe, se, _) in zip(xc, xe):
                    if sc == se:
                        votes.setdefault(oc, collections.Counter())[oe] += 1
    # a value stored to the frame exactly once on each side pins that slot
    for k, offs in stc.items():
        if len(offs) == 1 and len(ste.get(k, ())) == 1:
            votes.setdefault(offs[0], collections.Counter())[ste[k][0]] += 3
    # correspondences learned from earlier comparisons outvote everything else
    for oc, (oe, _) in (pins or {}).items():
        votes.setdefault(oc, collections.Counter())[oe] += 1000
    if not votes:
        frame_map.why = 'no frame correspondences inferred'
        return None
    # accept the strongest correspondences first; drop any whose image would collide
    # with one already accepted (alignment noise)
    size_at = {}
    for v in lc.values():
        for off, size, _ in v:
            size_at[off] = max(size_at.get(off, 0), size or 4)
    cand = []
    for oc, cnt in votes.items():
        top = cnt.most_common(2)
        if len(top) > 1 and top[0][1] == top[1][1]:
            continue                            # tie: let the surrounding range decide
        cand.append((-top[0][1], oc, top[0][0] - oc))
    accepted, taken = [], []
    for _, oc, dl in sorted(cand):
        lo, hi = oc + dl, oc + dl + size_at.get(oc, 4)
        if any(lo < b and a < hi for a, b in taken):
            continue
        taken.append((lo, hi))
        accepted.append((oc, dl))
    strength = {oc: -w for w, oc, _ in cand}
    points = sorted(accepted)
    accesses = [x for v in lc.values() for x in v]
    # a wrong guess only makes the proof fail; injectivity is what keeps it sound.
    # When the ranges collide, drop the weakest point involved and rebuild.
    # a pinned store covers exactly its own bytes; what follows it starts afresh
    ends = {oc: oc + size for oc, (_, size) in (pins or {}).items() if size}
    while points:
        fm, culprits = _frame_ranges(points, accesses, ends)
        if fm is not None or not culprits:
            if fm is not None:
                frame_map.why = None
            return fm
        removable = [oc for oc in culprits if any(p[0] == oc for p in points)]
        if not removable:
            return None
        weakest = min(removable, key=lambda oc: strength.get(oc, 0))
        points = [p for p in points if p[0] != weakest]
    frame_map.why = 'no injective frame map inferred'
    return None


def known_frame_map_constraints_valid(cand, addr, returns_value, mapping, image_floor=None):
    """Keep discovered constraints intact in every frame relocation tier.

    Opaque calls cannot discard constraints from other known accesses. This
    check does not certify their unknown behavior or any private bytes. It
    adds known pointer and indexed extents to the existing call ABI model.
    """
    global _JOIN_RECIPES
    saved = _JOIN_RECIPES
    try:
        _JOIN_RECIPES = False
        func = Func(cand, addr)
        delta = esp_deltas(func, True, returns_value)
        regions = bounded_callee_frame_memory(func, delta, returns_value, known_only=True)
        if regions is None:
            return False
        if any(low < b and a < high and d for low, high in func.callee_frame_fixed
               for a, b, d in mapping.ranges):
            return False
        # These are known observations, not a partition certifying private
        # bytes. They may include saved bytes; do not publish a privacy proof.
        return relocate_callee_regions(func, regions, mapping, check_saved=False,
                                       image_floor=image_floor) is not None
    finally:
        _JOIN_RECIPES = saved


def word_frame_map(cand, exe, addr, returns_value, difference):
    """Infer a unique permutation of isolated word cells from a paired call.

    Only the smallest cell set named by the differing pointer arguments moves.
    Every supplied pointer needs a complete non-retaining binary footprint;
    indexed regions, other supplied objects and crossing direct views cannot
    overlap a moved cell. The complete equivalence proof still checks the map.
    """
    if FRAME_INPUT is None or not difference or not all(difference):
        return None
    left, right = difference
    if left[0] != 'call' or right[0] != 'call' or left[1][0] != 'c' or \
            D(left[1]) != D(right[1]) or D(left[3:]) != D(right[3:]) or len(left[2]) != len(right[2]):
        return None
    target, forced, allowed = left[1][1], {}, [set(), set()]
    for source, (a, b) in enumerate(zip(left[2], right[2])):
        if D(a) == D(b):
            continue
        x, y = stack_off(a), stack_off(b)
        footprint = FRAME_INPUT(target, source)
        if x is None or y is None or x >= 0 or y >= 0 or x % 2 or y % 2 or \
                footprint is None or footprint[0] != ((0, 2),):
            return None
        if x in forced and forced[x] != y:
            return None
        forced[x] = y
        allowed[0].add((target, source, x))
        allowed[1].add((target, source, y))
    if not forced or len(forced) > 8 or len(set(forced.values())) != len(forced):
        return None
    cells = set(forced) | set(forced.values())
    # Forced edges form cycles and open paths. One open path has exactly one
    # completion on this cell set. Two paths admit distinct completions.
    missing_sources = cells - forced.keys()
    missing_targets = cells - set(forced.values())
    if len(missing_sources) != len(missing_targets) or len(missing_sources) > 1:
        return None
    permutation = dict(forced)
    if missing_sources:
        permutation[next(iter(missing_sources))] = next(iter(missing_targets))
    if any(a != b and abs(a - b) < 2 for a in cells for b in cells):
        return None
    if EXE_READ is not None:
        exe = EXE_READ(addr) or exe
    funcs = [Func(cand, addr), Func(exe, addr)]
    floor = funcs[0].frame_floor
    if floor is None or floor != funcs[1].frame_floor or not all(floor <= a and a + 2 <= 0 for a in cells):
        return None
    mapping = []
    cursor = floor
    for cell in sorted(cells):
        if cursor < cell:
            mapping.append((cursor, cell, 0))
        mapping.append((cell, cell + 2, permutation[cell] - cell))
        cursor = cell + 2
    if cursor < 0:
        mapping.append((cursor, 0, 0))
    fm = FrameMap(mapping)
    observed = []
    global _JOIN_RECIPES
    saved = _JOIN_RECIPES
    try:
        # Privacy is proved independently of the integer join refinement.
        _JOIN_RECIPES = False
        for side, func in enumerate(funcs):
            delta = esp_deltas(func, True, returns_value)
            regions = bounded_callee_frame_memory(func, delta, returns_value)
            if regions is None or any(cell < off + 4 and off < cell + 2
                                      for cell in cells for off in func.save_offs):
                return None
            if relocate_callee_regions(func, regions, fm if side == 0 else
                                       FrameMap([(floor, 0, 0)])) is None:
                return None
            blocked = list(func.callee_frame_indexed) + func.callee_frame_incoming
            seen = set()
            for call, source, base, spans in func.callee_frame_supplied:
                if (call, source, base) in allowed[side] and spans == ((0, 2),):
                    seen.add(base)
                else:
                    blocked.extend((base + offset, base + offset + size) for offset, size in spans)
            if any(cell < high and low < cell + 2 for cell in cells for low, high in blocked):
                return None
            for low, high in func.callee_frame_views:
                touched = [cell for cell in cells if cell < high and low < cell + 2]
                # A byte view stays at the same byte within its word. A
                # crossing or wider view would observe two different biases.
                if touched and (len(touched) != 1 or not touched[0] <= low < high <= touched[0] + 2):
                    return None
                seen.update(touched)
            observed.append(seen)
    finally:
        _JOIN_RECIPES = saved
    if any((cell in observed[0]) != (permutation[cell] in observed[1]) for cell in cells):
        return None
    return fm


def _frame_ranges(points, accesses, ends=None):
    """(FrameMap, None) from the learned points, or (None, defining points of the
    ranges that make it fail)"""
    # ranges of constant delta, each running up to the next point with another delta
    ranges = []
    pts = dict(points)
    for oc, end in (ends or {}).items():
        if oc in pts and end < 0 and end not in pts:
            pts[end] = 0                        # after a pinned store: unmoved until told otherwise
    for oc, dl in sorted(pts.items()):
        if ranges and ranges[-1][2] == dl and oc not in (ends or {}) and ranges[-1][0] not in (ends or {}):
            continue
        ranges.append([oc, None, dl])
    starts = [r[0] for r in ranges]
    for k in range(len(ranges)):
        ranges[k][1] = ranges[k + 1][0] if k + 1 < len(ranges) else 0
    # accesses below the lowest learned point belong to the lowest object
    lowest = min(off for off, _, _ in accesses)
    if lowest < ranges[0][0]:
        ranges[0][0] = lowest
    # every frame access of the draft must fall in a range
    for off, size, indexed in accesses:
        if not any(a <= off < b for a, b, _ in ranges):
            frame_map.why = 'offset %d not covered (ranges %s)' % (off, ranges)
            return None, []
    # injectivity over what the draft can touch
    images = []
    for k, (a, b, dl) in enumerate(ranges):
        inside = [(off, size, ix) for off, size, ix in accesses if a <= off < b]
        if not inside:
            continue
        end = b if any(ix for _, _, ix in inside) else max(off + max(size, 1) for off, size, _ in inside)
        images.append((a + dl, end + dl, k))
    images.sort()
    for (a1, b1, k1), (a2, b2, k2) in zip(images, images[1:]):
        if a2 < b1:
            frame_map.why = 'images overlap: %s' % [(x, y) for x, y, _ in images]
            return None, [starts[k1], starts[k2]]
    if all(dl == 0 for _, _, dl in ranges):
        frame_map.why = 'only the identity frame map inferred'
        return None, []                         # nothing to relocate
    return FrameMap([tuple(r) for r in ranges]), None


def common_binding_literal(left, right, left_facts, right_facts):
    """An exact scalar literal proved through both incoming bindings."""
    def values(binding, facts):
        if isinstance(binding, frozenset):
            literals = {facts[r] for r in binding if r in facts and is_c(facts[r])}
            if len(literals) > 1:
                raise Unsupported('inconsistent literals of equal holders')
            return literals
        if isinstance(binding, tuple) and binding[0] == 'rel' and binding[2][0] == 'ext':
            width = binding[2][1]
            if not 0 < width <= 32:
                return set()
            literals = {ext(facts[r], 0, width) for r in binding[1] if r in facts and is_c(facts[r])}
            if len(literals) > 1:
                raise Unsupported('inconsistent literal projections of equal holders')
            return literals
        if isinstance(binding, tuple) and binding[0] == '=' and is_c(binding[1]):
            return {binding[1]}
        return set()
    common = values(left, left_facts) & values(right, right_facts)
    return next(iter(common)) if common else None


def merge_closed_recipe_binding(x, y, xfacts, yfacts):
    """Express a closed edge value through exact holders on that same edge.

    Only the recipe survives the join. Its substituted closed loads, including
    their histories, prove this predecessor; they are not facts about later
    iterations. Partial facts and recursive holder recipes cannot ground it.
    """
    if not all(isinstance(v, tuple) and v for v in (x, y)):
        return None
    # An indirect field binding is the same current-holder recipe as an
    # expression load. Ground its initial fixed address on that edge only.
    def expression(binding):
        if binding[0] == 'ldat':
            address = add(C(binding[3]), *[scale(('in', r), k) for r, k in binding[2]])
            return ('expr', ('ld', binding[1], address, ()))
        return binding
    x, y = expression(x), expression(y)
    if x[0] == y[0] == 'expr' and D(x[1]) == D(y[1]):
        return x
    if {x[0], y[0]} != {'=', 'expr'}:
        return None
    value, recipe, facts = (x[1], y[1], xfacts) if x[0] == '=' else (y[1], x[1], yfacts)

    def contains_input(v):
        return isinstance(v, tuple) and (v[:1] == ('in',) or any(contains_input(q) for q in v))

    def entry(loc):
        fact = facts.get(loc)
        if loc not in REG32 or loc == 'esp' or not isinstance(fact, tuple) or not fact or \
                fact[0] not in ('c', 'imp', 'ld', 'lin', 'sx', 'zx', 'ext') or contains_input(fact):
            raise ValueError
        return fact

    try:
        rebuilt = rewrite_product(recipe, entry)
    except (ValueError, AssertionError):
        return None
    return ('expr', recipe) if D(rebuilt) == D(value) else None


def literal_fact_field(fact, width):
    """A literal low field explicitly proved by a full or partial holder fact."""
    if not 0 < width <= 32 or not isinstance(fact, tuple):
        return None
    mask = (1 << width) - 1
    if is_c(fact):
        return ext(fact, 0, width)
    if fact[0] == 'bits' and fact[1] >= width and is_c(fact[2]):
        return ext(fact[2], 0, width)
    if fact[0] == 'knownbits' and fact[1] & mask == mask:
        return C(fact[2] & mask)
    return None


def merge_narrow_holder_bindings(left, right, left_facts, right_facts, live_mask):
    """Prove one zero-extended holder field independently on both edges.

    Bindings may be literals or existing byte/word load/projection recipes.
    Every observed bit above the field must already be zero in each binding.
    The two histories stay separate; only common current holder names survive.
    """
    if not live_mask:
        return None
    def holders(binding, facts, width):
        if not isinstance(binding, tuple):
            return set()
        high = live_mask & (FULL ^ ((1 << width) - 1))
        if binding[0] == '=' and is_c(binding[1]):
            if binding[1][1] & high:
                return set()
            value = ext(binding[1], 0, width)
        elif binding[:1] == ('ldat',) and binding[1] in (1, 2) and binding[1] * 8 <= width and \
                all(r in REG32 and r != 'esp' for r, _ in binding[2]):
            value = ('ld', binding[1], add(C(binding[3]),
                     *[scale(('in', r), k) for r, k in binding[2]]), ())
        elif binding[0] == 'rel' and binding[2] == ('ext', width):
            # An older partial relation says nothing about live high bits.
            return set() if high else {r for r in binding[1] if r in REG32 and r != 'esp'}
        elif binding[0] == 'expr' and binding[1][:3] == ('ext', 0, width) and \
                binding[1][3][0] == 'in' and binding[1][3][1] in REG32 and binding[1][3][1] != 'esp':
            return {binding[1][3][1]}  # complete zero-extension expression
        else:
            return set()
        result = set()
        for loc, fact in facts.items():
            if loc not in REG32 or loc == 'esp':
                continue
            known = literal_fact_field(fact, width)
            if known is None and fact[0] == 'bits' and fact[1] >= width:
                known = ext(fact[2], 0, width)
            if known is None and fact[0] == 'expr':
                known = ext(fact[1], 0, width)
            if known == value:
                result.add(loc)
        return result
    for width in (8, 16):
        common = holders(left, left_facts, width) & holders(right, right_facts, width)
        if common:
            if live_mask & (FULL ^ ((1 << width) - 1)):
                # Unlike a partial relation, this records the independently
                # proved zero high bits as part of a complete expression.
                return ('expr', ('ext', 0, width, ('in', min(common, key=repr))))
            return ('rel', frozenset(common), ('ext', width))
    return None


def merge_held_load_binding(left, right, left_facts, right_facts):
    """Keep an existing full load recipe proved by a holder on the other edge.

    The holder's own complete current fact must be that exact load, including
    address, width and dependency history. No reads or equalities are inferred.
    """
    if isinstance(left, frozenset):
        holders, recipe, facts = left, right, left_facts
    elif isinstance(right, frozenset):
        holders, recipe, facts = right, left, right_facts
    else:
        return None
    if not isinstance(recipe, tuple) or recipe[:2] != ('ldat', 4) or \
            any(r not in REG32 or r == 'esp' for r, _ in recipe[2]):
        return None
    address = add(C(recipe[3]), *[scale(('in', r), k) for r, k in recipe[2]])
    required = ('expr', ('ld', 4, address, ()))
    return recipe if any(r in REG32 and r != 'esp' and facts.get(r) == required
                         for r in holders) else None


def affine_holder_recipe(value, held):
    """An integer affine relation using complete current register values.

    Every nonliteral term needs an exact exit holder. In particular, this
    cannot replace a held read by a new read or by a partial register fact.
    """
    if value[0] != 'lin' or has_sp(value):
        return None
    terms = {}
    for term, coefficient in value[2]:
        for loc, current in held:
            if loc in REG32 and loc != 'esp' and D(term) == D(current):
                key = ('in', loc)
                terms[key] = (terms.get(key, 0) + coefficient) & M32
                break
        else:
            return None
    return mk_lin(value[1], terms)


def merge_word_recipe_relation(x, y, xfacts, yfacts, live_mask):
    """Retain a word equality proved independently on every incoming edge."""
    if not live_mask or live_mask & ~0xffff or not all(isinstance(v, tuple) for v in (x, y)):
        return None
    if {x[0], y[0]} != {'rel', 'expr'}:
        return None
    relation, expression, facts = (x, y, yfacts) if x[0] == 'rel' else (y, x, xfacts)
    if relation[2] != ('ext', 16):
        return None
    value = ext(expression[1], 0, 16)
    def word_fact(loc):
        fact = facts.get(loc, (None,))
        if loc in REG32 and fact[0] == 'bits' and fact[1] == 16:
            return ext(fact[2], 0, 16)
        if (_PROJECTED_FRAME or _PRECISE_FRAME) and _is_slot(loc) and loc[2] == 2 and fact[0] == 'expr':
            return ext(fact[1], 0, 16)
        return None
    holders = frozenset(r for r in relation[1]
                       if word_fact(r) == value)
    return ('rel', holders, ('ext', 16)) if holders else None


def common_projection_holders(left, right, left_equal, right_equal):
    """Meet projected holders through full equalities proved on each edge."""
    if not all(isinstance(v, tuple) and v[0] == 'rel' for v in (left, right)) or \
            left[2] != right[2] or left[2][0] != 'ext' or not 0 < left[2][1] <= 32:
        return frozenset()

    def expand(holders, equal):
        out = set(holders)
        def scalar(loc):
            return loc in REG32 and loc != 'esp' if isinstance(loc, str) else _is_slot(loc) and loc[2] <= 4
        pairs = [(a, b) for a, b in equal if scalar(a) and scalar(b)]
        changed = True
        while changed:
            changed = False
            for a, b in pairs:
                if (a in out or b in out) and not {a, b} <= out:
                    out.update((a, b))
                    changed = True
        return out

    return frozenset(expand(left[1], left_equal) & expand(right[1], right_equal))


def _equivalent(cand, exe, addr, returns_value, remap=None, whole_frame=False, edge_context=True):
    global _JOIN_RECIPES
    if EXE_READ is not None:
        # always compare against the original function at its real size
        exe = EXE_READ(addr) or exe
    fc, fe = Func(cand, addr), Func(exe, addr)
    if remap is not None and not known_frame_map_constraints_valid(cand, addr, returns_value, remap, fe.frame_floor):
        raise Unsupported('frame relocation changes a proved callee or indexed extent')
    if returns_value and RETURN_MASK is not None:
        fc.return_mask = fe.return_mask = RETURN_MASK(addr)
    escaped = stack_escapes(fc) or stack_escapes(fe)
    joined_frame = _JOIN_RECIPES and _PRECISE_FRAME and any(
        i.mnemonic.startswith('f') for func in (fc, fe) for i in func.ins.values())
    if joined_frame and whole_frame:
        raise Unsupported('x87 integer joins require proved callee frame partitions')
    saved_join = _JOIN_RECIPES
    try:
        if joined_frame:
            # Derive pointers, indexed extents and callee footprints without
            # relying on the integer join refinement that this proof enables.
            _JOIN_RECIPES = False
        dc = esp_deltas(fc, escaped, returns_value)
        de = esp_deltas(fe, escaped, returns_value)
        if _EXTENDED:
            for func, delta in ((fc, dc), (fe, de)):
                func.decided_branches = decided_path_branches(func, delta, escaped, returns_value)
            if fc.decided_branches:
                dc = esp_deltas(fc, escaped, returns_value)
            if fe.decided_branches:
                de = esp_deltas(fe, escaped, returns_value)
        if escaped and _PRECISE_FRAME and not whole_frame:
            for func, delta in ((fc, dc), (fe, de)):
                func.callee_frame_ranges = bounded_callee_frame_memory(func, delta, returns_value)
        if whole_frame:
            fc.sp_rec = fe.sp_rec = None
        elif remap is not None:
            # Translate the draft's conservative memory boundary into the exe layout.
            made = fc.sp_rec
            if made is not None and None not in made:
                lows = [o for o in made if o < 0 and (fc.frame_floor is None or o >= fc.frame_floor)]
                if lows:
                    # Relocation can reorder ranges. Retain every image of the
                    # original memory region, including range starts with no LEA.
                    low = min(lows)
                    images = [max(a, low) + dl for a, b, dl in remap.ranges if b > low]
                    if images and fc.frame_floor is not None and min(images) < fc.frame_floor:
                        raise Unsupported('relocated frame memory below frame floor')
                    fc.sp_rec = {min(images)} if images else None
                else:
                    fc.sp_rec = set()
        if escaped and _PRECISE_FRAME and not whole_frame and remap is None:
            for func, delta in ((fc, dc), (fe, de)):
                regions = bounded_frame_memory(func, delta, returns_value)
                if regions is not None:
                    func.frame_memory_ranges = regions
        if escaped and _PRECISE_FRAME and not whole_frame:
            for func, mapping in ((fc, remap), (fe, None)):
                regions = func.callee_frame_ranges
                if regions is None:
                    continue
                if mapping is not None:
                    regions = relocate_callee_regions(func, regions, mapping)
                    if regions is None:
                        continue
                func.frame_memory_ranges = regions
                func.callee_frame_proof = True
    finally:
        _JOIN_RECIPES = saved_join
    if joined_frame and not all(getattr(func, 'callee_frame_proof', False) for func in (fc, fe)):
        raise Unsupported('x87 integer joins require proved callee frame partitions')
    if _DEAD_STORES:
        # Preserve the original holder facts for other globals. A target with
        # a deleted store cannot use them to remove its final visible write.
        fc.dead_global_addresses, fe.dead_global_addresses = set(), set()
    if escaped and _LOCAL_LIFETIMES and remap is None:
        for func, delta in ((fc, dc), (fe, de)):
            retired = local_retired_bytes(func, delta, returns_value)
            if retired is not None:
                func.local_retired_bytes = retired
    sc = summarize(fc, escaped, returns_value, dc, remap)
    se = summarize(fe, escaped, returns_value, de)
    if _EXTENDED:
        fc.global_holders = global_holder_facts(fc, sc, dc)
        fe.global_holders = global_holder_facts(fe, se, de)
        sc = summarize(fc, escaped, returns_value, dc, remap)
        se = summarize(fe, escaped, returns_value, de)
    edge_context = edge_context and _EXTENDED
    lc, le = liveness(sc), liveness(se)
    for f, live in ((fc, lc), (fe, le)):
        for blk, regs in getattr(f, 'ambiguous', {}).items():
            for r in sorted(regs & live.get(blk, set())):
                # with the frame's address taken its locals are memory, so a pointer into
                # them is an ordinary pointer from here on (loads and stores through it
                # alias the frame). The outgoing argument slots below the frame never are;
                # a pointer below esp (left over from copying a struct argument) points at
                # freed stack, which compiled code never reads through
                offs = f.ambiguous_offs.get(blk, {}).get(r, ())
                floor, sp = f.frame_floor, f.spr.get(blk, {}).get('esp')
                if escaped and all(o < 0 and (floor is None or o >= floor or (sp is not None and o < sp))
                                   for o in offs):
                    continue
                raise Unsupported('register %s holds a stack address on only some paths into %x' % (r, blk))
    evc = Evaluator(fc, escaped, returns_value, remap)
    eve = Evaluator(fe, escaped, returns_value)

    def is_entry(loc):
        return loc == 'esp'

    # binding: cand location -> the set of exe locations known to hold the same value
    # on entry (or ('=', const) when only the cand side keeps the constant). Blocks
    # reached along several edges get the meet of the edges' facts and are processed
    # again until nothing changes.
    # The walk is over pairs (cand block, exe block): a block may correspond to
    # several blocks on the other side when one compiler duplicated a tail the other
    # shares; every block of both functions must be reached.
    start = {'bind': {loc: frozenset([loc]) for loc in lc[addr]}, 'econst': {},
             'equal': frozenset(), 'fields': frozenset()}
    state = {(addr, addr, None, None): start}
    visited, exe_seen = set(), set()
    work = [(addr, addr, None, None)]
    rounds = 0
    while work:
        rounds += 1
        if rounds > 40 * (len(dc) + len(de) + 1):
            return False, 'binding analysis does not converge'
        context = work.pop()
        cb, eb = context[:2]
        stc = state[context]
        bindsets, econst = stc['bind'], stc['econst']

        # exe locations asserted equal to the same cand location share one symbol
        parent = {}

        def find(x):
            while parent.get(x, x) != x:
                x = parent[x]
            return x
        for v in bindsets.values():
            if isinstance(v, frozenset):
                m = sorted(v, key=repr)
                for x in m[1:]:
                    ra, rb = find(m[0]), find(x)
                    if ra != rb:
                        parent[max(ra, rb, key=repr)] = min(ra, rb, key=repr)
        for a, b in stc['equal']:
            ra, rb = find(a), find(b)
            if ra != rb:
                parent[max(ra, rb, key=repr)] = min(ra, rb, key=repr)
        ealias = {x: find(x) for x in parent}
        # Keep equality of low bits separate from full location equality.
        # A byte spill can relate several registers without equating their high
        # bits, which the executable may still use independently.
        bit_parent = {}
        def bit_find(x):
            while bit_parent.get(x, x) != x:
                x = bit_parent[x]
            return x
        for slot, register, width in stc['fields']:
            for bit in range(width):
                a = bit_find((ealias.get(slot, slot), bit))
                b = bit_find((ealias.get(register, register), bit))
                if a != b:
                    bit_parent[max(a, b, key=repr)] = min(a, b, key=repr)
        for v in bindsets.values():
            if isinstance(v, tuple) and v[0] == 'rel' and v[2][0] in ('ext', 'sx'):
                holders = sorted({ealias.get(r, r) for r in v[1]}, key=repr)
                for bit in range(v[2][1]):
                    for r in holders[1:]:
                        a, b = bit_find((holders[0], bit)), bit_find((r, bit))
                        if a != b:
                            bit_parent[max(a, b, key=repr)] = min(a, b, key=repr)
        bit_constants = constant_bit_classes(econst, ealias, bit_find) if _JOIN_RECIPES else {}
        canonical_constants = dict(econst)
        if _JOIN_RECIPES:
            for loc, value in econst.items():
                representative = ealias.get(loc, loc)
                if representative == loc:
                    continue
                fact = value if is_c(value) or value[0] == 'knownbits' else \
                    bounded_constant_bits(value[1]) if value[0] == 'expr' else None
                if fact is None:
                    continue
                previous = canonical_constants.get(representative)
                combined = intersect_constant_bits(previous, fact) if previous is not None and \
                    (is_c(previous) or previous[0] == 'knownbits') else fact
                if combined is not None:
                    canonical_constants[representative] = combined

        slot_widths = {}
        if _EXTENDED:
            for holder in le.get(eb, ()):
                if _is_slot(holder) and holder[2] <= 4 and not eve.in_frame_memory(holder[1], holder[2]):
                    slot_widths[holder[1]] = max(slot_widths.get(holder[1], 0), holder[2])

        def slot_entry(loc, first):
            # Stack views at the same byte address share their low bytes. Keep
            # the wider view's other bits independent of a narrow binding.
            if _is_slot(loc) and loc[2] < slot_widths.get(loc[1], 0):
                wide = ('s', loc[1], slot_widths[loc[1]])
                return ext(norm_entry(wide, first), 0, loc[2] * 8)
            return norm_entry(loc, first)

        def projected_entry(loc, first, overlap=True):
            width = loc[2] * 8 if _is_slot(loc) else 32
            requested = loc
            loc = ealias.get(loc, loc)
            resolved = {}
            def held(r, seen=frozenset()):
                r = ealias.get(r, r)
                if r in resolved:
                    return resolved[r]
                value = canonical_constants.get(r, slot_entry(r, first))
                if _EXTENDED and overlap and r not in canonical_constants and _is_slot(r) and 0 < r[2] <= 4:
                    # A wider instruction may cover a narrower input with
                    # literal, bit or expression facts proved on every incoming
                    # edge. Rebuild only those bytes; the rest stay independent
                    # inputs, including uninitialized high bytes.
                    slots = {}
                    masks = le.masks.get(eb, {})
                    for q, v in econst.items():
                        if not _is_slot(q) or not 0 < q[2] <= 4 or v[0] not in ('c', 'knownbits', 'expr') or q not in le.get(eb, ()):
                            continue
                        width = min(q[2], masks.get(q, FULL).bit_length() // 8)
                        if not width or masks.get(q, FULL) & ((1 << (8 * width)) - 1) != (1 << (8 * width)) - 1:
                            continue
                        if q[1] < r[1] + r[2] and r[1] < q[1] + width:
                            slots[q[1]] = (width, ext(projected_entry(q, first, overlap=False), 0, 8 * width))
                    covered = slot_value(slots, r[1], r[2], lambda view: slot_entry(view, first))
                    if covered is not None:
                        value = covered
                if value[0] == 'knownbits':
                    unknown = op('and', slot_entry(r, first), C(FULL ^ value[1]), comm=True)
                    return op('or', unknown, C(value[2]), comm=True) if value[2] else unknown
                if value[0] == 'bits':
                    if r in seen:
                        grounded = set().union(*(v for v in bindsets.values() if isinstance(v, frozenset)))
                        resolved.update(solve_edge_recipe_cycle(r, canonical_constants, ealias, grounded, first,
                                                               lambda q: held(q, seen)))
                        return resolved[r]
                    return ins(slot_entry(r, first), 0, value[1],
                               rewrite_product(value[2], lambda q: held(q, seen | {r})))
                if value[0] == 'expr':
                    # Full holder equalities can turn a copy recipe into its
                    # own identity, e.g. a register equal to a private spill.
                    # That fact needs no recursive recipe. Nonidentity cycles
                    # remain unsupported.
                    recipe = rewrite_product(value[1], lambda q: ('in', ealias.get(q, q)))
                    if recipe == ('in', r):
                        return slot_entry(r, first)
                    if recipe[0] == 'ext' and recipe[1] == 0 and recipe[3] == ('in', r):
                        # A self projection constrains only the holder's high
                        # zero bits. Keep its low bits as independent inputs.
                        return ext(slot_entry(r, first), 0, recipe[2])
                    if r in seen:
                        grounded = set().union(*(v for v in bindsets.values() if isinstance(v, frozenset)))
                        resolved.update(solve_edge_recipe_cycle(r, canonical_constants, ealias, grounded, first,
                                                               lambda q: held(q, seen)))
                        return resolved[r]
                    return rewrite_product(recipe, lambda q: held(q, seen | {r}))
                return value
            value = held(loc)
            bit = 0
            while bit < width:
                r, start = bit_find((loc, bit))
                end = bit + 1
                while end < width and bit_find((loc, end)) == (r, start + end - bit):
                    end += 1
                if r != loc or start != bit:
                    base = held(r)
                    value = ins(value, bit, end - bit, ext(base, start, end - bit))
                bit = end
            # Canonicalizing equal low fields must retain a literal proved in
            # any member. Only those exact bits move; the holder's unrelated
            # high bits remain independent inputs.
            mask = literal = 0
            for bit in range(width):
                fixed = bit_constants.get(bit_find((loc, bit)))
                if fixed is not None:
                    mask |= 1 << bit
                    literal |= fixed << bit
            if mask:
                value = op('and', value, C(FULL ^ mask), comm=True)
                if literal:
                    value = op('or', value, C(literal), comm=True)
            if _JOIN_RECIPES:
                proven = econst.get(requested, canonical_constants.get(loc))
                fact = proven if proven is not None and (is_c(proven) or proven[0] == 'knownbits') else \
                    bounded_constant_bits(proven[1]) if proven is not None and proven[0] == 'expr' else None
                if fact is not None:
                    if is_c(fact):
                        value = fact
                    else:
                        value = op('and', value, C(FULL ^ fact[1]), comm=True)
                        if fact[2]:
                            value = op('or', value, C(fact[2]), comm=True)
            return ext(value, 0, width) if width < 32 else value
        bind = {}
        for k, v in bindsets.items():
            if isinstance(v, frozenset):
                r = min(v, key=repr)
                bind[k] = ealias.get(r, r)
            else:
                bind[k] = v

        first = cb == addr and eb == addr

        def bound_ent_c(loc, bind=bind, m=dc[cb], econst=econst, first=first, overlap=True):
            if isinstance(loc, str) and loc in m:
                return add(SP, C(m[loc]))
            if loc in bind:
                b = bind[loc]
                if isinstance(b, tuple) and b[0] == '=':
                    return b[1]                 # constant carried in by every predecessor
                if isinstance(b, tuple) and b[0] == 'ldat':
                    def held(r):
                        r = ealias.get(r, r)
                        return projected_entry(r, first) if _EXTENDED else econst.get(r, norm_entry(r, first))
                    return ('ld', b[1], add(C(b[3]), *[scale(held(r), k) for r, k in b[2]]), ())
                if isinstance(b, tuple) and b[0] == 'expr':
                    return rewrite_product(b[1], lambda r: projected_entry(r, first))
                if isinstance(b, tuple) and b[0] == 'sum':
                    def held(r):
                        r = ealias.get(r, r)
                        return projected_entry(r, first) if _EXTENDED else econst.get(r, norm_entry(r, first))
                    return mk_lin(b[2], {}) if not b[1] else add(C(b[2]), *[scale(held(r), k) for r, k in b[1]])
                if isinstance(b, tuple) and b[0] == 'rel':
                    r = min(b[1], key=repr)
                    r = ealias.get(r, r)
                    base = projected_entry(r, first)
                    how, k = b[2]
                    if how == 'add':
                        return add(base, C(k))
                    return sx(k, base) if how == 'sx' else ext(base, 0, k)
                if not _EXTENDED and b in econst:
                    return econst[b]
                return projected_entry(b, first)
            if _EXTENDED and overlap and _is_slot(loc) and 0 < loc[2] <= 4:
                # An instruction can read a dword whose live input is only a
                # word or byte. Rebuild that view from established bindings at
                # the actual candidate offsets, leaving all other bytes unbound.
                slots = {}
                bytes_held = {}
                masks = lc.masks.get(cb, {})
                for held in bind:
                    if not _is_slot(held) or not 0 < held[2] <= 4:
                        continue
                    width = min(held[2], masks.get(held, FULL).bit_length() // 8)
                    if not width or masks.get(held, FULL) & ((1 << (8 * width)) - 1) != (1 << (8 * width)) - 1:
                        continue
                    if held[1] < loc[1] + loc[2] and loc[1] < held[1] + width:
                        value = ext(ent_c(held, overlap=False), 0, width * 8)
                        for byte in range(max(loc[1], held[1]), min(loc[1] + loc[2], held[1] + width)):
                            part = ext(value, 8 * (byte - held[1]), 8)
                            if byte in bytes_held and D(bytes_held[byte]) != D(part):
                                raise Unsupported('inconsistent overlapping stack inputs')
                            bytes_held[byte] = part
                        # Overlapping bytes agreed above. Keep the widest
                        # proved view so iteration order cannot drop its bytes.
                        previous = slots.get(held[1])
                        if previous is None or previous[0] < width:
                            slots[held[1]] = (width, value)
                value = slot_value(slots, loc[1], loc[2],
                                   lambda view: ('unbound', 'cand', view))
                if value is not None:
                    return value
            if first and loc in CALLEE_SAVED and loc != 'ebp':
                return ('preserved', loc)
            return ('unbound', 'cand', loc)

        def ent_c(loc, bound=bound_ent_c, overlap=True):
            value = bound(loc, overlap=overlap)
            # Mapping a word slot to a register does not widen the word read.
            return ext(value, 0, loc[2] * 8) if _is_slot(loc) and loc[2] < 4 else value

        def ent_e(loc, m=de[eb], econst=econst, first=first, ealias=ealias):
            if isinstance(loc, str) and loc in m:
                return add(SP, C(m[loc]))
            value = projected_entry(loc, first)
            if _EXTENDED:
                return value
            mask = le.masks.get(eb, {}).get(loc, FULL)
            width = min(mask.bit_length(), loc[2] * 8 if _is_slot(loc) else 32)
            return ext(value, 0, width) if width < 32 else value

        bc = evc.run(cb, ent_c)
        be = eve.run(eb, ent_e)
        why = compare_blocks(bc, be)
        pairs = None if why else succ_pairs(bc.term, be.term)
        if pairs is None and not why:
            why = 'terminators differ %s / %s' % (short(bc.term), short(be.term))
        absorbed_c, absorbed_e = (), ()
        if why:
            # one side may have split a block where the other runs straight on:
            # absorb the unconditional successor of either side and compare again
            def straight(b, f, s0, ev, ent):
                # up to three blocks the code runs straight on into
                out, cur = [], b
                while len(out) < 3:
                    t = term_succs(cur.term) if cur.term[0] in ('jmp', 'fall') else []
                    if len(t) != 1 or t[0] not in f.blocks or t[0] in (s0, addr) or t[0] in out:
                        break
                    out.append(t[0])
                    cur = ev.run(s0, ent, tuple(out))
                return out
            sc_, se_ = straight(bc, fc, cb, evc, ent_c), straight(be, fe, eb, eve, ent_e)
            combos = sorted(((i, j) for i in range(len(sc_) + 1) for j in range(len(se_) + 1) if i or j),
                            key=lambda ij: (ij[0] + ij[1], ij))
            for i_, j_ in combos:
                xc, xe = tuple(sc_[:i_]), tuple(se_[:j_])
                if not xc and not xe:
                    continue
                bc2 = evc.run(cb, ent_c, xc) if xc else bc
                be2 = eve.run(eb, ent_e, xe) if xe else be
                if compare_blocks(bc2, be2) is None and succ_pairs(bc2.term, be2.term) is not None:
                    bc, be, absorbed_c, absorbed_e = bc2, be2, xc, xe
                    pairs = succ_pairs(bc.term, be.term)
                    why = None
                    break
        if why:
            # a branch whose short arm only sets registers that are dead where the arms
            # meet (code the other compiler removed): the same as jumping to the meet
            def collapse(b, f, ev, d, live, entry):
                if b.term[0] != 'jcc':
                    return None

                def reaches(s, meet, depth, previous, previous_entry):
                    # blocks passed through on the way from s to meet without effects
                    # or live writes, or None
                    if s == meet:
                        return ()
                    if depth > 3 or s not in f.blocks or s == addr or s not in d:
                        return None
                    m = d[s]
                    ent = (lambda loc: add(SP, C(m[loc])) if loc in m else exit_value(previous.state, loc, previous_entry)) if _EXTENDED else stack_entry(m)
                    bs = ev.run(s, ent)
                    if bs.effects or (bs.state.written - ({'esp'} if _EXTENDED else {'esp', 'flags'})) & set(live.get(meet, ())):
                        return None
                    if bs.term[0] in ('jmp', 'fall'):
                        nxt = term_succs(bs.term)
                        if len(nxt) == 1:
                            r = reaches(nxt[0], meet, depth + 1, bs, ent)
                            return None if r is None else (s,) + r
                        return None
                    if bs.term[0] == 'jcc':
                        out = (s,)
                        for n2 in term_succs(bs.term):
                            r = reaches(n2, meet, depth + 1, bs, ent)
                            if r is None:
                                return None
                            out += r
                        return out
                    return None
                t = term_succs(b.term)
                for meet in t:
                    through = ()
                    for s2 in t:
                        r = reaches(s2, meet, 0, b, entry)
                        if r is None:
                            break
                        through += r
                    else:
                        nbk = Block()
                        nbk.effects, nbk.state, nbk.term = b.effects, b.state, ('jmp', meet)
                        return nbk, tuple(dict.fromkeys(through))
                # or every arm goes through effect-free blocks to one later block
                for meet in sorted(f.blocks):
                    if meet in t or meet <= max(t):
                        continue
                    through = ()
                    for s2 in t:
                        r = reaches(s2, meet, 0, b, entry)
                        if r is None:
                            break
                        through += r
                    else:
                        nbk = Block()
                        nbk.effects, nbk.state, nbk.term = b.effects, b.state, ('jmp', meet)
                        return nbk, tuple(dict.fromkeys(through))
                    break       # only the nearest following block
                return None
            ce = collapse(be, fe, eve, de, le, ent_e)
            cc = collapse(bc, fc, evc, dc, lc, ent_c)

            def run_on(b, absorbed, ev, s0, ent):
                # the collapsed block followed straight into its meet (one block on the
                # other side): the dead arms are skipped, the meet appended
                meet = b.term[1]
                if meet in (s0, addr):
                    return None
                b2 = ev.run(s0, ent, (meet,))
                return b2, absorbed + (meet,)
            ce2 = run_on(ce[0], ce[1], eve, eb, ent_e) if ce else None
            cc2 = run_on(cc[0], cc[1], evc, cb, ent_c) if cc else None
            for xc_, xe_ in ((None, ce), (cc, None), (cc, ce), (None, ce2), (cc2, None), (cc2, ce2)):
                if xc_ is None and xe_ is None:
                    continue
                bc2, ac = xc_ if xc_ else (bc, ())
                be2, ae = xe_ if xe_ else (be, ())
                if compare_blocks(bc2, be2) is None and succ_pairs(bc2.term, be2.term) is not None:
                    bc, be = bc2, be2
                    absorbed_c, absorbed_e = absorbed_c + ac, absorbed_e + ae
                    pairs = succ_pairs(bc.term, be.term)
                    why = None
                    break
        if why:
            return False, 'block %x/%x: %s' % (cb, eb, why)
        visited.add(cb)
        exe_seen.add(eb)
        visited.update(absorbed_c)
        exe_seen.update(absorbed_e)
        pairs = succ_pairs(bc.term, be.term)
        for c2, e2 in pairs:
            nb = {}
            ec2 = {}
            evals = {}
            evalv = {}
            for eloc in sorted(set(le.get(e2, ())) | (set(REG32) if _EXTENDED else set()), key=repr):
                if eloc != 'esp' and eloc not in de.get(e2, {}):
                    v = exit_value(be.state, eloc, ent_e)
                    if eloc in le.get(e2, ()):
                        evals[eloc] = D(v)
                    evalv[eloc] = v
                    if v[0] in ('c', 'imp'):
                        ec2[eloc] = v
                    elif v[0] == 'ld' and v[2][0] == 'c' and not unchanged_after(be, v):
                        pass
                    elif v[0] == 'ld' and v[2][0] == 'c':
                        # a global the exe keeps in a register where the draft reloads it
                        # (the mirror of the cand-side rule below)
                        ec2[eloc] = ('ld', v[1], v[2], ())
                    elif v[0] in ('lin', 'sx', 'zx') and closed_value(be, v) is not None:
                        # likewise a value computed only from constants and globals
                        # (an address into a global table indexed by a global)
                        ec2[eloc] = closed_value(be, v)
                    elif isinstance(eloc, str) and eloc in REG32:
                        # the same in the low bits of a register (ax after mov ax, [g]):
                        # only those bits are used downstream
                        emask = le.masks.get(e2, {}).get(eloc, FULL) if getattr(le, 'masks', None) else FULL
                        nb_ = emask.bit_length()
                        if nb_ in (8, 16):
                            low = ext(v, 0, nb_)
                            if low[0] == 'ld' and low[1] * 8 == nb_ and low[2][0] == 'c' and unchanged_after(be, low):
                                ec2[eloc] = ins(('in', eloc), 0, nb_, ('ld', low[1], low[2], ()))
            for eloc, value in evalv.items():
                # A dword pointer is subject to the same complete address and
                # unchanged-memory proof as a byte or word field.
                if _EXTENDED and value[0] == 'ld' and value[1] <= 4 and value[2][0] != 'c' and not has_sp(value[2]) and unchanged_after(be, value):
                    held = [(r, exit_value(be.state, r, ent_e)) for r in REG32
                            if r != eloc and r != 'esp' and r not in de.get(e2, {})]
                    try:
                        address = edge_expression(value[2], held, be)
                        ec2[eloc] = ('expr', ('ld', value[1], address, ()))
                    except ValueError:
                        pass
            for eloc, value in evalv.items():
                if _EXTENDED and value[0] == 'ext':
                    refs = {}
                    _uses(value, refs)
                    # These are exit values: even a register written in this
                    # block can hold the saved field. Equality is checked below.
                    holders = list(REG32)
                    holders += sorted((r for r in set(refs) | set(evalv)
                                       if _is_slot(r) and not eve.in_frame_memory(r[1], r[2])), key=repr)
                    held = [(r, exit_value(be.state, r, ent_e)) for r in holders
                            if r != eloc and r != 'esp' and r not in de.get(e2, {})]
                    try:
                        recipe = edge_expression(value, held, be)
                        # The next block already receives this exit holder.
                        # Replaying an identity adds no value information.
                        if recipe != ('in', eloc):
                            ec2[eloc] = ('expr', recipe)
                    except ValueError:
                        pass
            # Full register relations do not depend on private byte-view precision.
            # Exit evaluation has already applied every call's ABI clobbers.
            if _EXTENDED and (_PROJECTED_FRAME or _PRECISE_FRAME):
                for eloc, value in evalv.items():
                    if eloc in ec2 or eloc not in REG32 or eloc == 'esp':
                        continue
                    held = [(r, current) for r, current in evalv.items()
                            if r in REG32 and r not in (eloc, 'esp') and r not in de.get(e2, {})]
                    recipe = affine_holder_recipe(value, held)
                    if recipe is not None:
                        ec2[eloc] = ('expr', recipe)
            for eloc, value in evalv.items():
                width = le.masks.get(e2, {}).get(eloc, FULL).bit_length()
                if _EXTENDED and eloc in REG32 and 0 < width < 32:
                    low = ext(value, 0, width)
                    if width == 8 and is_c(low):
                        # A literal byte write proves only the live low byte.
                        # Preserve that fact when full and partial writes join.
                        ec2[eloc] = ('bits', width, low)
                    elif low[0] == 'ld' and not has_sp(low[2]) and unchanged_after(be, low):
                        held = [(r, exit_value(be.state, r, ent_e)) for r in REG32
                                if r != eloc and r != 'esp' and r not in de.get(e2, {})]
                        try:
                            address = edge_expression(low[2], held, be)
                            ec2[eloc] = ('bits', width, ('ld', low[1], address, ()))
                        except ValueError:
                            pass
            if _JOIN_RECIPES:
                for eloc, value in evalv.items():
                    if eloc in ec2 or not (eloc in REG32 or (_is_slot(eloc) and eloc[2] <= 4)):
                        continue
                    fact = bounded_constant_bits(value)
                    if fact is not None:
                        ec2[eloc] = fact
            cmask = lc.masks.get(c2, {}) if getattr(lc, 'masks', None) else {}
            for loc in sorted(lc.get(c2, ()), key=repr):
                if loc == 'esp' or loc in dc.get(c2, {}):
                    continue
                vc = exit_value(bc.state, loc, ent_c)
                mk = cmask.get(loc, FULL)
                narrow = (isinstance(loc, str) and loc in REG32) or _is_slot(loc)
                nbits = min(mk.bit_length(), loc[2] * 8 if _is_slot(loc) else 32) if narrow else 32
                if nbits < 32:
                    vc = ext(vc, 0, nbits)      # only these bits are ever used downstream
                dvc = D(vc)
                cands = []
                lookup_cands = []
                lookup_key = lookup_producer_key(vc) if _EXTENDED and loc in REG32 else None
                for eloc, dv in evals.items():
                    # a value may sit in a register on one side and a stack slot on the
                    # other (e.g. a callee-saved register saved later on one path)
                    if type(eloc) != type(loc) and not (_is_slot(loc) or _is_slot(eloc)):
                        continue
                    if (isinstance(loc, tuple) and loc[0] == 'f') != (isinstance(eloc, tuple) and eloc[0] == 'f') or \
                            (isinstance(loc, tuple) and loc[0] == 'f' and eloc != loc):
                        continue    # x87 stack positions are structural
                    if (loc == 'flags') != (eloc == 'flags'):
                        continue
                    if nbits < 32 and ((isinstance(eloc, str) and eloc in REG32) or _is_slot(eloc)):
                        dv = D(ext(evalv[eloc], 0, nbits))
                    if dv == dvc:
                        cands.append(eloc)
                    elif lookup_key is not None and eloc in REG32:
                        value = evalv[eloc]
                        if nbits < 32:
                            value = ext(value, 0, nbits)
                        if lookup_producer_key(value) == lookup_key:
                            lookup_cands.append(eloc)
                if lookup_cands:
                    # Both edge producers agree, including the full memory
                    # dependency tuple. Carry the actual register, not a reload.
                    cands.extend(lookup_cands)
                    for holder in lookup_cands:
                        ec2.pop(holder, None)
                    nb[loc] = frozenset(cands) if nbits == 32 else ('rel', frozenset(cands), ('ext', nbits))
                    continue
                if not cands:
                    if _EXTENDED and _is_slot(loc) and loc[2] == 4 and nbits == 32 and loc in evalv and \
                            not evc.in_frame_memory(loc[1], loc[2]) and not eve.in_frame_memory(loc[1], loc[2]):
                        # A private pointer spill can keep an adjacent field
                        # address while the other side keeps the object's base.
                        # Preserve the exact scalar difference even on an early
                        # constant iteration; every later edge checks it again.
                        difference = sub(vc, evalv[loc])
                        if is_c(difference):
                            nb[loc] = ('rel', frozenset([loc]), ('add', difference[1]))
                            continue
                    if loc in CALLEE_SAVED and vc == ('preserved', loc):
                        nb[loc] = frozenset([loc])
                        continue
                    if vc[0] in ('c', 'imp', 'preserved'):
                        # a constant, or the caller's value of a callee-saved register:
                        # the same known value on both sides, carried along
                        nb[loc] = ('=', vc)
                        continue
                    if vc[0] == 'ld' and vc[2][0] == 'c' and unchanged_after(bc, vc):
                        # a global kept in a register where the other side reloads it
                        nb[loc] = ('=', ('ld', vc[1], vc[2], ()))
                        continue
                    if vc[0] in ('lin', 'sx', 'zx', 'ext') and closed_value(bc, vc) is not None:
                        # likewise a value computed only from constants and globals
                        # (a widened global, an address into a table indexed by one)
                        nb[loc] = ('=', closed_value(bc, vc))
                        continue
                    if vc[0] == 'ld' and vc[2][0] != 'c' and SP not in lin_parts(vc[2])[1] and \
                            unchanged_after(bc, vc):
                        # an indirect load kept in a register where the other side reloads
                        # it: fine if every part of its address is still held by some exe
                        # register at the edge (the load is then re-expressed in them)
                        c0, terms = lin_parts(vc[2])
                        held = [(r, exit_value(be.state, r, ent_e)) for r in REG32
                                if r != 'esp' and r not in de.get(e2, {})]
                        # Prefer the complete object pointer to its expanded
                        # induction terms. This preserves the same field recipe
                        # when the first pointer is a literal and later ones vary.
                        if _PROJECTED_FRAME:
                            for r, value in held:
                                delta = sub(vc[2], value)
                                if is_c(delta):
                                    nb[loc] = ('ldat', vc[1], ((r, 1),), delta[1])
                                    break
                            if loc in nb:
                                continue
                        parts = []
                        for t, k in terms.items():
                            for r, value in held:
                                delta = sub(t, value)
                                if is_c(delta):
                                    parts.append((r, k))
                                    c0 = (c0 + delta[1] * k) & M32
                                    break
                            else:
                                break
                        else:
                            if parts:
                                nb[loc] = ('ldat', vc[1], tuple(parts), c0)
                                continue
                    # a value derived from another one: plus a constant (a pointer to a
                    # field) or its low bits zero-extended, kept in a register where the
                    # other side recomputes it from the original
                    derived = None
                    if vc[0] == 'lin' and vc[1]:
                        # (any linear base) + a constant
                        derived = (mk_lin(0, dict(vc[2])), ('add', vc[1]))
                    elif vc[0] == 'ext' and vc[1] == 0:
                        derived = (vc[3], ('ext', vc[2]))
                    if vc[0] == 'sx':
                        derived = (vc[2], ('sx', vc[1]))
                    if derived:
                        base = D(derived[0])
                        # any exe register holding the original at the edge will do (an
                        # equality at the successor's entry, live there or not)
                        holders = set(REG32) | {r for r in evalv if _is_slot(r)}
                        def derive(value):
                            how, width = derived[1]
                            return value if how == 'add' else (sx(width, value) if how == 'sx' else ext(value, 0, width))
                        rel = frozenset(r for r in holders if r != 'esp' and r not in de.get(e2, {}) and
                                        D(derive(exit_value(be.state, r, ent_e))) ==
                                        (base if derived[1][0] == 'add' else D(vc)))
                        if rel:
                            if _EXTENDED and _PRECISE_FRAME and derived[1][0] == 'ext' and \
                                    mk & (FULL ^ ((1 << derived[1][1]) - 1)):
                                # This is the candidate's complete zero-extension,
                                # unlike a partial equality chosen by liveness.
                                nb[loc] = ('expr', ('ext', 0, derived[1][1], ('in', min(rel, key=repr))))
                            else:
                                nb[loc] = ('rel', rel, derived[1])
                            continue
                    if nbits == 32:
                        # an exe register differing from the value by a constant (a pointer
                        # kept biased on one side)
                        off = {}
                        for r in REG32:
                            if r != 'esp' and r not in de.get(e2, {}):
                                dlt = sub(vc, exit_value(be.state, r, ent_e))
                                if is_c(dlt):
                                    off.setdefault(dlt[1], set()).add(r)
                        if off:
                            k = min(off)
                            nb[loc] = ('rel', frozenset(off[k]), ('add', k))
                            continue
                    if vc[0] == 'lin' and nbits == 32 and len(vc[2]) > 1:
                        # a sum of values the exe holds in registers (an address formed
                        # from two of them)
                        held = {}
                        for r in REG32:
                            if r != 'esp' and r not in de.get(e2, {}):
                                held.setdefault(D(exit_value(be.state, r, ent_e)), r)
                        parts = tuple((held.get(D(t)), k) for t, k in vc[2])
                        if all(r for r, _ in parts):
                            nb[loc] = integer_sum_recipe(parts, vc[1])
                            continue
                    if not _EXTENDED and vc[0] == 'mul':
                        holders = {D(exit_value(be.state, r, ent_e)): r for r in
                                   list(REG32) + sorted((r for r in evalv if _is_slot(r)), key=repr)
                                   if r != 'esp' and r not in de.get(e2, {})}
                        def represent_product(x):
                            r = holders.get(D(x))
                            if r is not None:
                                return ('in', r)
                            if x[0] == 'c':
                                return x
                            if x[0] in ('sx', 'ext', 'mul'):
                                return tuple(represent_product(y) if isinstance(y, tuple) else y for y in x)
                            raise ValueError
                        try:
                            nb[loc] = ('expr', represent_product(vc))
                            continue
                        except ValueError:
                            pass
                    # Reconstruct an integer value only from locations whose
                    # exit values agree, constants, and current memory reads.
                    # The recipe must agree literally on every incoming edge.
                    references = {}
                    _uses(vc, references)
                    slots = {r for r in set(evalv) | set(references)
                             if _is_slot(r) and not eve.in_frame_memory(r[1], r[2])}
                    held = [(r, exit_value(be.state, r, ent_e)) for r in
                            list(REG32) + sorted(slots, key=repr)
                            if r != 'esp' and r not in de.get(e2, {})]
                    try:
                        if not _EXTENDED:
                            raise ValueError
                        nb[loc] = ('expr', edge_expression(vc, held, bc))
                        continue
                    except ValueError:
                        pass
                    if _EXTENDED and narrow and (not _is_slot(loc) or loc[2] <= 4) and 0 < nbits <= 32:
                        try:
                            nb[loc] = ('expr', masked_edge_expression(vc, mk, held))
                            continue
                        except ValueError:
                            pass
                    return False, 'edge %x->%x: no counterpart for live %s = %s' % (cb, c2, loc, short(vc))
                if vc[0] == 'ld' and vc[2][0] == 'c' and unchanged_after(bc, vc):
                    # both sides still hold a global's current value: carry it as that
                    # load, so a later write-back of it is recognised as a no-op
                    lv = ('ld', vc[1], vc[2], ())
                    if all(ec2.get(e) == lv for e in cands):
                        nb[loc] = ('=', lv)
                        continue
                if _EXTENDED and vc[0] == 'ld' and vc[1] < 4 and vc[2][0] != 'c' and not has_sp(vc[2]) and unchanged_after(bc, vc):
                    held = [(r, exit_value(be.state, r, ent_e)) for r in REG32
                            if r != 'esp' and r not in de.get(e2, {})]
                    try:
                        address = edge_expression(vc[2], held, bc)
                        nb[loc] = ('expr', ('ld', vc[1], address, ()))
                        continue
                    except ValueError:
                        pass
                # A projected equality never asserts equal high register bits.
                nb[loc] = ('rel', frozenset(cands), ('ext', nbits)) if nbits < 32 else frozenset(cands)
            if _EXTENDED and bc.term[0] == be.term[0] == 'jcc' and be.term[2] != be.term[3]:
                cond = be.term[1]
                if cond[1] in ('e', 'ne') and cond[2][0:2] == ('fl', 'sub') and \
                        ((e2 == be.term[2]) == (cond[1] == 'e')):
                    a, b = cond[2][2:]
                    if is_c(b):
                        a, b = b, a
                    if is_c(a):
                        for r, value in evalv.items():
                            if value == b:
                                ec2[r] = a
                            elif r in REG32 and b[0] in ('ext', 'ld'):
                                width = b[2] if b[0] == 'ext' and b[1] == 0 else b[1] * 8 if b[0] == 'ld' else 32
                                if width < 32 and ext(value, 0, width) == b:
                                    ec2[r] = ('bits', width, a)
                        for loc in nb:
                            if exit_value(bc.state, loc, ent_c) == b:
                                nb[loc] = ('=', a)
            # exe-side constants only where no cand location carries the value
            path_constants = {r for r, v in ec2.items() if is_c(v)}
            used = set()
            for v in nb.values():
                if isinstance(v, frozenset):
                    used |= v
                elif v[0] == 'rel':
                    used |= v[1]
            for v in nb.values():
                if isinstance(v, tuple) and v[0] in ('sum', 'ldat'):
                    used |= {r for r, _ in (v[1] if v[0] == 'sum' else v[2])}
            ec2 = {k: v for k, v in ec2.items() if k not in used or (_EXTENDED and
                   (k in path_constants or v[0] in ('expr', 'bits', 'knownbits') or (v[0] == 'ld' and v[2][0] == 'c')))}
            # Preserve exact equalities between exe holders independently of a
            # candidate's partial binding. A low-byte relation must not discard
            # a separately proved equality of the holders' complete values.
            groups = {}
            for r, value in evalv.items():
                groups.setdefault(D(value), []).append(r)
            equal = frozenset((a, b) for group in groups.values()
                              for i, a in enumerate(group) for b in group[i + 1:])
            # A word slot and a register can keep equal low bytes while their
            # full values differ. Prove this from current exit expressions;
            # retain only fields that every predecessor independently proves.
            fields = frozenset((slot, register, slot[2] * 8)
                               for slot, value in evalv.items()
                               if _PROJECTED_FRAME and _is_slot(slot) and slot[2] in (1, 2)
                               and not eve.in_frame_memory(slot[1], slot[2])
                               for register in REG32 if register != 'esp' and register in evalv
                               and D(value) == D(ext(evalv[register], 0, slot[2] * 8)))
            source_c, source_e = absorbed_c[-1] if absorbed_c else cb, absorbed_e[-1] if absorbed_e else eb
            if edge_context and _EDGE_HISTORY:
                # Retain four correlated predecessor pairs across shared joins.
                # Each context must prove its full effects and every live value.
                # Longer histories meet by the existing must-equality rules.
                def history(previous, source):
                    previous = previous if isinstance(previous, tuple) else () if previous is None else (previous,)
                    return previous[-3:] + (source,)
                key = (c2, e2, history(context[2], source_c), history(context[3], source_e))
            else:
                key = (c2, e2, source_c if edge_context else None, source_e if edge_context else None)
            if key not in state:
                state[key] = {'bind': nb, 'econst': ec2, 'equal': equal, 'fields': fields}
                work.append(key)
                continue
            old = state[key]
            mb = {}
            for k in set(old['bind']) | set(nb):
                x, y = old['bind'].get(k), nb.get(k)
                if x is None or y is None:
                    return False, 'block %x: live %s bound on only some edges' % (c2, k)
                if _EXTENDED and (k in REG32 or (_is_slot(k) and k[2] <= 4)):
                    # When holders differ, a shared exact literal can be carried.
                    # A projection must cover every live bit of the location.
                    def full_holders(value, facts):
                        if isinstance(value, frozenset):
                            return value
                        if isinstance(value, tuple) and value[0] == '=':
                            return frozenset(r for r, fact in facts.items() if fact == value[1])
                        if isinstance(value, tuple) and value[0] == 'rel' and value[2][0] == 'ext':
                            width = value[2][1]
                            mask = lc.masks.get(c2, {}).get(k, FULL)
                            if 0 < width <= 32 and not mask & ~((1 << width) - 1):
                                return value[1]
                        return None
                    a = full_holders(x, old['econst'])
                    b = full_holders(y, ec2)
                    if a is not None and b is not None and not a & b:
                        literal = common_binding_literal(x, y, old['econst'], ec2)
                        if literal is not None:
                            mb[k] = ('=', literal)
                            continue
                if isinstance(x, frozenset) and isinstance(y, frozenset):
                    z = x & y
                    if not z:
                        return False, 'block %x: %s has no common counterpart on all edges' % (c2, k)
                    mb[k] = z
                elif x != y and all(isinstance(v, frozenset) or v[0] == '=' for v in (x, y)) and \
                        not (isinstance(x, frozenset) and isinstance(y, frozenset)):
                    # a known value on an edge stands for the exe locations holding that
                    # value there: keep the exe locations that agree on every edge
                    def holders(v, ec):
                        return v if isinstance(v, frozenset) else frozenset(e for e, w in ec.items() if w == v[1])
                    z = holders(x, old['econst']) & holders(y, ec2)
                    if not z and _EXTENDED and _PRECISE_FRAME and k in REG32:
                        projected = merge_narrow_holder_bindings(x, y, old['econst'], ec2,
                                                             lc.masks.get(c2, {}).get(k, FULL))
                        if projected is not None:
                            mb[k] = projected
                            continue
                    if not z:
                        return False, 'block %x: %s is %s on one edge, %s on another' % (c2, k, short(x, 60), short(y, 60))
                    mb[k] = z
                elif (isinstance(x, tuple) and isinstance(y, tuple) and
                      {x[0], y[0]} == {'=', 'rel'} and
                      (x if x[0] == 'rel' else y)[2][0] == 'ext'):
                    const, partial, ec = (x, y, old['econst']) if x[0] == '=' else (y, x, ec2)
                    width = partial[2][1]
                    mask = lc.masks.get(c2, {}).get(k, FULL)
                    if mask & (FULL ^ ((1 << width) - 1)) and ext(const[1], 0, width) != const[1]:
                        return False, 'block %x: %s has unproved live high bits at a projection join' % (c2, k)
                    z = partial[1] & frozenset(r for r, value in ec.items()
                                              if ext(value, 0, width) == ext(const[1], 0, width) or
                                              literal_fact_field(value, width) == ext(const[1], 0, width))
                    if not z:
                        return False, 'block %x: %s has no common constant projection' % (c2, k)
                    mb[k] = ('rel', z, partial[2])
                elif ((isinstance(x, frozenset) and isinstance(y, tuple) and y[0] == 'rel' and y[2][0] == 'ext') or
                      (isinstance(y, frozenset) and isinstance(x, tuple) and x[0] == 'rel' and x[2][0] == 'ext')):
                    full, partial = (x, y) if isinstance(x, frozenset) else (y, x)
                    z = full & partial[1]
                    if not z:
                        return False, 'block %x: %s has no common projected counterpart' % (c2, k)
                    mb[k] = ('rel', z, partial[2])
                elif isinstance(x, tuple) and isinstance(y, tuple) and x[0] == y[0] == 'rel' and x[2] == y[2]:
                    z = x[1] & y[1]
                    if not z and _PROJECTED_FRAME:
                        # Equality on one predecessor may add a holder that
                        # survives the other. Never pool facts across edges or
                        # promote a projected equality to a full-value fact.
                        z = common_projection_holders(x, y, old['equal'], equal)
                    if not z:
                        return False, 'block %x: %s has no common derived counterpart on all edges' % (c2, k)
                    mb[k] = ('rel', z, x[2])
                elif _EXTENDED and (k in REG32 or ((_PROJECTED_FRAME or _PRECISE_FRAME) and _is_slot(k) and k[2] == 2)) and merge_word_recipe_relation(
                        x, y, old['econst'], ec2, 0xffff if _is_slot(k) else lc.masks.get(c2, {}).get(k, FULL)) is not None:
                    mb[k] = merge_word_recipe_relation(
                        x, y, old['econst'], ec2, 0xffff if _is_slot(k) else lc.masks.get(c2, {}).get(k, FULL))
                elif _PROJECTED_FRAME and _EXTENDED and k in REG32 and k != 'esp' and merge_closed_recipe_binding(
                        x, y, old['econst'], ec2) is not None:
                    mb[k] = merge_closed_recipe_binding(x, y, old['econst'], ec2)
                elif _EXTENDED and _PRECISE_FRAME and k in REG32 and merge_narrow_holder_bindings(
                        x, y, old['econst'], ec2, lc.masks.get(c2, {}).get(k, FULL)) is not None:
                    mb[k] = merge_narrow_holder_bindings(x, y, old['econst'], ec2,
                                                       lc.masks.get(c2, {}).get(k, FULL))
                elif _EXTENDED and _PRECISE_FRAME and lc.masks.get(c2, {}).get(k, FULL) == FULL and \
                        (k in REG32 or (_is_slot(k) and k[2] == 4 and not evc.in_frame_memory(k[1], 4))) and \
                        merge_held_load_binding(x, y, old['econst'], ec2) is not None:
                    mb[k] = merge_held_load_binding(x, y, old['econst'], ec2)
                elif x == y:
                    mb[k] = x
                elif _JOIN_RECIPES and isinstance(x, tuple) and isinstance(y, tuple) and x[0] == y[0] == 'expr' and \
                        (k in REG32 or (_is_slot(k) and k[2] <= 4)):
                    # Recipes can retain different dead bits from an early
                    # constant iteration. Only their live fields must agree.
                    # This projects the candidate recipe, not the executable's
                    # holders. Their independent live bits remain observable
                    # in the full effects, branch and return comparison.
                    mask = lc.masks.get(c2, {}).get(k, FULL)
                    px, py = live_projection(x[1], mask), live_projection(y[1], mask)
                    if px != py:
                        return False, 'block %x: live fields of %s disagree across edges' % (c2, k)
                    mb[k] = ('expr', px)
                else:
                    return False, 'block %x: %s is %s on one edge, %s on another' % (c2, k, short(x, 60), short(y, 60))
            mec = {}
            for k, value in old['econst'].items():
                other = ec2.get(k)
                if value == other:
                    mec[k] = value
                elif _PROJECTED_FRAME and _EXTENDED and k in REG32 and k != 'esp' and other is not None:
                    # Ground the recipe on its closed predecessor independently,
                    # then retain only the same current-holder relation. Do not
                    # carry the first object's loads through subsequent loops.
                    left = value if value[0] == 'expr' else ('=', value)
                    right = other if other[0] == 'expr' else ('=', other)
                    merged = merge_closed_recipe_binding(left, right, old['econst'], ec2)
                    if merged is not None:
                        mec[k] = merged
                    elif _JOIN_RECIPES:
                        common = common_constant_bits(value, other)
                        if common is not None:
                            mec[k] = common
                elif _JOIN_RECIPES and other is not None:
                    common = common_constant_bits(value, other)
                    if common is not None:
                        mec[k] = common
            meq = old['equal'] & equal
            mf = old['fields'] & fields
            if mb != old['bind'] or mec != old['econst'] or meq != old['equal'] or mf != old['fields']:
                state[key] = {'bind': mb, 'econst': mec, 'equal': meq, 'fields': mf}
                if key not in work:
                    work.append(key)
    if len(visited) != len(dc) or len(exe_seen) != len(de):
        return False, 'unreached blocks (%d/%d vs %d/%d)' % (len(visited), len(dc), len(exe_seen), len(de))
    detail = ', stack escapes' if escaped else ''
    if getattr(fc, 'callee_frame_proof', False) or getattr(fe, 'callee_frame_proof', False):
        detail += ', binary callee footprints'
    if getattr(fc, 'decided_branches', {}) or getattr(fe, 'decided_branches', {}):
        detail += ', dominating path conditions'
    mask = getattr(fe, 'return_mask', FULL)
    if returns_value and mask != FULL:
        detail += ', binary return mask %#x' % mask
    return True, 'equivalent (%d blocks%s)' % (len(visited), detail)


def intersect_constant_bits(a, b):
    """Combine simultaneously proved constant fields of fully equal holders."""
    def fact(value):
        if is_c(value):
            return FULL, value[1]
        return value[1:] if value[0] == 'knownbits' else None
    left, right = fact(a), fact(b)
    if left is None or right is None:
        return None
    if left[0] & right[0] & (left[1] ^ right[1]):
        raise Unsupported('inconsistent constant fields of equal holders')
    mask = left[0] | right[0]
    fixed = (left[1] & left[0]) | (right[1] & right[0])
    return C(fixed) if mask == FULL else ('knownbits', mask, fixed)


def constant_bit_classes(facts, aliases, representative):
    """Carry already-proved literal bits through exact bit-equality classes.

    The facts and equalities belong to the same correlated predecessor state.
    A partial equality shares only its named bits, never a register's high bits.
    Conflicting facts refuse rather than selecting a convenient constant.
    """
    fixed = {}
    for loc, value in facts.items():
        fact = value if is_c(value) or value[0] == 'knownbits' else \
            bounded_constant_bits(value[1]) if value[0] == 'expr' else None
        if fact is None:
            continue
        mask, literal = (FULL, fact[1]) if is_c(fact) else fact[1:]
        for bit in range(32):
            if not mask & (1 << bit):
                continue
            key = representative((aliases.get(loc, loc), bit))
            current = (literal >> bit) & 1
            if key in fixed and fixed[key] != current:
                raise Unsupported('inconsistent constant bits of equal holders')
            fixed[key] = current
    return fixed


def bounded_constant_bits(value):
    """Constant high bits established by an integer expression's unsigned range."""
    intervals = address_intervals(value, 1)
    if len(intervals) != 1:
        return None
    low, end = intervals[0]
    mask = FULL ^ ((1 << (low ^ (end - 1)).bit_length()) - 1)
    if not mask:
        return None
    fixed = low & mask
    return C(fixed) if mask == FULL else ('knownbits', mask, fixed)


def common_constant_bits(a, b):
    """Bits fixed to the same constants on every incoming executable edge."""
    def fact(value):
        if is_c(value):
            return FULL, value[1]
        if value[0] == 'knownbits':
            return value[1:]
        if value[0] == 'expr':
            bounded = bounded_constant_bits(value[1])
            return fact(bounded) if bounded is not None else None
        return None
    left, right = fact(a), fact(b)
    if left is None or right is None:
        return None
    mask = left[0] & right[0] & ~(left[1] ^ right[1]) & FULL
    if not mask:
        return None
    value = left[1] & mask
    return C(value) if mask == FULL else ('knownbits', mask, value)


def live_projection(value, mask):
    """Retain exactly the selected integer bits, making every other bit zero."""
    result = C(0)
    bit = 0
    while bit < 32:
        if not mask & (1 << bit):
            bit += 1
            continue
        end = bit + 1
        while end < 32 and mask & (1 << end):
            end += 1
        result = ins(result, bit, end - bit, ext(value, bit, end - bit))
        bit = end
    return result


def masked_edge_expression(value, mask, held):
    """Rebuild exactly the live bits from equal fields of exit holders.

    Bits outside mask are arbitrary and may be zero: liveness excludes every
    downstream observation of them. Each live run needs a literal equality.
    """
    mask &= FULL
    result = C(0)
    bit = 0
    while bit < 32:
        if not mask & (1 << bit):
            bit += 1
            continue
        end = bit + 1
        while end < 32 and mask & (1 << end):
            end += 1
        width = end - bit
        field = ext(value, bit, width)
        if is_c(field):
            replacement = field
        else:
            replacement = None
            for loc, current in held:
                size = loc[2] * 8 if _is_slot(loc) else 32
                for offset in range(size - width + 1):
                    if D(field) == D(ext(current, offset, width)):
                        replacement = ext(('in', loc), offset, width)
                        break
                if replacement is not None:
                    break
            if replacement is None:
                raise ValueError('live field has no exit holder')
        result = ins(result, bit, width, replacement)
        bit = end
    return result


def edge_expression(x, held, block):
    """Express x in exit holders without inventing an entry-value equality."""
    for r, value in held:
        if D(x) == D(value):
            # This exit holder equals a zero-extended field. Keep that proven
            # projection in the recipe instead of equating unobserved high bits.
            return ext(('in', r), 0, x[2]) if x[0] == 'ext' and x[1] == 0 else ('in', r)
    if x[0] == 'c':
        return x
    if x[0] == 'ext':
        lo, n = x[1:3]
        for r, value in held:
            if D(x) == D(ext(value, lo, n)):
                return ext(('in', r), lo, n)
        return ('ext', lo, n, edge_expression(x[3], held, block))
    if x[0] == 'ins':
        base, lo, n, value = x[1:]
        # Only the bits outside the inserted field need a surviving holder.
        for r, v in held:
            if (not lo or D(ext(base, 0, lo)) == D(ext(v, 0, lo))) and \
                    (lo + n == 32 or D(ext(base, lo + n, 32 - lo - n)) ==
                     D(ext(v, lo + n, 32 - lo - n))):
                return ('ins', ('in', r), lo, n, edge_expression(value, held, block))
        return ('ins', edge_expression(base, held, block), lo, n, edge_expression(value, held, block))
    if x[0] in ('sx', 'mul', 'and'):
        return tuple(edge_expression(y, held, block) if isinstance(y, tuple) else y for y in x)
    if x[0] == 'lin':
        return ('lin', x[1], tuple((edge_expression(t, held, block), k) for t, k in x[2]))
    if x[0] == 'cat':
        return cat((n, edge_expression(v, held, block)) for n, v in x[1])
    if x[0] == 'ld' and not has_sp(x[2]) and unchanged_after(block, x):
        return ('ld', x[1], edge_expression(x[2], held, block), ())
    raise ValueError('no surviving representation')


def solve_edge_recipe_cycle(start, recipes, aliases, grounded, first, outside):
    """Resolve simultaneous must equalities, rather than recursively expanding them.

    A cyclic integer system needs full rank over the 32-bit modular ring. A
    redundant copy within an independently proved alias class can instead use
    that class's existing machine entry value, when a full candidate binding
    already observes it. No fresh value is invented for an unanchored cycle.
    """
    canon = lambda r: aliases.get(r, r)
    start = canon(start)
    definitions, members = {}, {}
    for r, value in recipes.items():
        key = canon(r)
        definitions.setdefault(key, []).append((r, value))
        members.setdefault(key, set()).add(r)
    for r, target in aliases.items():
        members.setdefault(canon(target), set()).update((r, target))

    def refs(value):
        if not isinstance(value, tuple) or not value:
            return set()
        if value[0] == 'in':
            return {canon(value[1])}
        return set().union(*(refs(v) for v in value if isinstance(v, tuple)))

    graph = {}
    for r, values in definitions.items():
        graph[r] = set().union(*(refs(v[1] if v[0] == 'expr' else v[2])
                                for _, v in values if v[0] in ('expr', 'bits')))
        graph[r] &= definitions.keys()
    reachable = set()
    def visit(r):
        if r in reachable:
            return
        reachable.add(r)
        for q in graph.get(r, ()):
            visit(q)
    visit(start)
    # The component is the reachable nodes that can also reach the start.
    reverse = {r: set() for r in reachable}
    for r in reachable:
        for q in graph.get(r, set()) & reachable:
            reverse[q].add(r)
    component, pending = set(), [start]
    while pending:
        r = pending.pop()
        if r not in component:
            component.add(r)
            pending.extend(reverse[r])
    nodes = sorted(component, key=repr)
    if len(nodes) > 8 or any(r not in REG32 or r == 'esp' for r in nodes):
        raise Unsupported('unsupported cyclic edge recipe locations')
    positions = {r: k for k, r in enumerate(nodes)}

    def affine(value):
        if value[0] == 'in':
            r = canon(value[1])
            if r in component:
                return {r: 1}, C(0)
            return {}, outside(r)
        if value[0] == 'lin':
            coefficients, remainder = {}, C(value[1])
            for term, factor in value[2]:
                parts, rest = affine(term)
                remainder = add(remainder, scale(rest, factor))
                for r, coefficient in parts.items():
                    coefficients[r] = (coefficients.get(r, 0) + factor * coefficient) & M32
            return {r: k for r, k in coefficients.items() if k}, remainder
        if refs(value) & component:
            raise Unsupported('nonlinear or partial cyclic edge recipe')
        return {}, rewrite_product(value, lambda r: outside(canon(r)))

    rows = []
    for r in nodes:
        values = definitions[r]
        for _, value in values:
            if value[0] in ('bits', 'knownbits'):
                raise Unsupported('partial cyclic edge recipe')
            if value[0] == 'expr':
                coefficients, remainder = affine(value[1])
            else:
                # Constants and closed loads are independent anchors. They are
                # entry facts, not recipes to expand in this simultaneous system.
                if refs(value) & component:
                    raise Unsupported('partial cyclic edge recipe anchor')
                coefficients, remainder = {}, value
            row = [(-coefficients.get(q, 0)) & M32 for q in nodes]
            row[positions[r]] = (row[positions[r]] + 1) & M32
            rows.append([row, remainder])
        copies = all(v[0] == 'expr' and v[1][0] == 'in' and canon(v[1][1]) == r for _, v in values)
        if copies and len(members.get(r, ())) > 1 and any(canon(q) == r for q in grounded):
            row = [int(q == r) for q in nodes]
            rows.append([row, norm_entry(r, first)])

    # An odd pivot is invertible modulo 2**32. Missing one means at least two
    # modular solutions, or an inconsistent system; both must fail closed.
    for column in range(len(nodes)):
        pivot = next((k for k in range(column, len(rows)) if rows[k][0][column] & 1), None)
        if pivot is None:
            raise Unsupported('nonunique cyclic edge recipe')
        rows[column], rows[pivot] = rows[pivot], rows[column]
        inverse = pow(rows[column][0][column], -1, 1 << 32)
        rows[column][0] = [(x * inverse) & M32 for x in rows[column][0]]
        rows[column][1] = scale(rows[column][1], inverse)
        for k, (row, remainder) in enumerate(rows):
            if k == column:
                continue
            factor = row[column]
            if factor:
                rows[k] = [[(x - factor * y) & M32 for x, y in zip(row, rows[column][0])],
                           sub(remainder, scale(rows[column][1], factor))]
    if any(any(row) or remainder != C(0) for row, remainder in rows[len(nodes):]):
        raise Unsupported('inconsistent cyclic edge recipe')
    return {r: rows[k][1] for k, r in enumerate(nodes)}


def lookup_producer_key(value):
    """Compare table producers while retaining their width and load history."""
    def address(x):
        if x[0] == 'in' and _is_slot(x[1]) and 0 < x[1][2] <= 4:
            return ('field', x[1][1] * 8, x[1][2] * 8)
        if x[0] == 'ext' and x[3][0] == 'in' and _is_slot(x[3][1]):
            slot = x[3][1]
            if 0 <= x[1] and 0 < x[2] and x[1] + x[2] <= slot[2] * 8 <= 32:
                return ('field', slot[1] * 8 + x[1], x[2])
        if x[0] == 'ld':
            return ('ld', x[1], address(x[2]), x[3])
        if x[0] == 'lin':
            terms = tuple(sorted(((address(t), k) for t, k in x[2]), key=repr))
            return ('lin', x[1], terms)
        if x[0] in ('mul', 'and'):
            return (x[0],) + tuple(sorted((address(y) for y in x[1:]), key=repr))
        return tuple(address(y) if isinstance(y, tuple) else y for y in x)
    if value[0] != 'ld' or has_sp(value[2]):
        return None
    return D(('ld', value[1], address(value[2]), value[3]))


def rewrite_product(x, entry):
    """Rebuild an edge recipe using the next block's entry values."""
    if x[0] == 'in':
        return entry(x[1])
    if x[0] == 'ext':
        return ext(rewrite_product(x[3], entry), x[1], x[2])
    if x[0] == 'sx':
        return sx(x[1], rewrite_product(x[2], entry))
    if x[0] == 'ins':
        return ins(rewrite_product(x[1], entry), x[2], x[3], rewrite_product(x[4], entry))
    if x[0] == 'lin':
        return add(C(x[1]), *[scale(rewrite_product(t, entry), k) for t, k in x[2]])
    if x[0] == 'cat':
        return cat((n, rewrite_product(v, entry)) for n, v in x[1])
    if x[0] == 'ld':
        return ('ld', x[1], rewrite_product(x[2], entry), x[3])
    if x[0] == 'and':
        return op('and', *[rewrite_product(v, entry) for v in x[1:]], comm=True)
    if x[0] == 'mul':
        values = [rewrite_product(v, entry) for v in x[1:]]
        value = values[0]
        for v in values[1:]:
            value = mul(value, v)
        return value
    assert x[0] == 'c'
    return x


def unchanged_after(blk, v):
    """the global read by load v still holds that value at the end of blk: every call
    and possibly aliasing store in the block precedes the load"""
    deps = set(v[3])
    return not [e for e in blk.effects if D(e) not in deps and
                (e[0] in ('call', 'str') or (e[0] == 'st' and may_alias(e[2], e[1], v[2], v[1])))]


def closed_value(blk, v):
    """v rebuilt from constants and global loads only (no block inputs, no stack),
    each load still holding its value at the end of blk; None otherwise. Such a
    value is the same wherever it is recomputed after blk, so one side may keep it
    in a register while the other recomputes it."""
    if v[0] in ('c', 'imp'):
        return v
    if v[0] == 'ld':
        addr = closed_value(blk, v[2])
        if addr is None or not unchanged_after(blk, v):
            return None
        return ('ld', v[1], addr, ())
    if v[0] == 'lin':
        terms = {}
        for t, k in v[2]:
            t2 = closed_value(blk, t)
            if t2 is None:
                return None
            terms[t2] = (terms.get(t2, 0) + k) & M32
        return mk_lin(v[1], terms)
    if v[0] in ('sx', 'zx'):
        inner = closed_value(blk, v[2])
        return None if inner is None else (v[0], v[1], inner)
    if v[0] == 'ext':
        inner = closed_value(blk, v[3])
        return None if inner is None else ext(inner, v[1], v[2])
    return None


def _is_slot(loc):
    return isinstance(loc, tuple) and loc[0] == 's'


def slot_value(slots, off, size, entry):
    """value of stack bytes off..off+size from the stores in this block, filling
    untouched gaps from entry(('s', off, size)); None if no store overlaps"""
    runs = []           # [kind, base, first byte, nbytes]: kind 'v' stored, 'c' constant, 'e' entry
    touched = False
    for b in range(off, off + size):
        hit = None
        for o, (s, v) in slots.items():
            if o <= b < o + s:
                hit = (o, v)
                break
        if hit is None:
            piece = ['e', None, b, 1]
        else:
            touched = True
            base, k = hit[1], b - hit[0]
            if base[0] == 'ext' and base[1] % 8 == 0 and base[2] % 8 == 0:
                if k < base[2] // 8:
                    base, k = base[3], k + base[1] // 8
                else:
                    base = C(0)
            if base[0] == 'c':
                piece = ['c', (base[1] >> (8 * k)) & 0xff, 0, 1]
            else:
                piece = ['v', base, k, 1]
        if runs:
            r = runs[-1]
            if r[0] == piece[0] == 'e':
                r[3] += 1
                continue
            if r[0] == piece[0] == 'v' and r[2] + r[3] == piece[2] and (r[1] is piece[1] or D(r[1]) == D(piece[1])):
                r[3] += 1
                continue
            if r[0] == piece[0] == 'c':
                r[1] |= piece[1] << (8 * r[3])
                r[3] += 1
                continue
        runs.append(piece)
    if not touched:
        return None
    parts = []
    for kind, base, first, n in runs:
        if kind == 'c':
            parts.append((n * 8, C(base)))
        elif kind == 'v':
            parts.append((n * 8, ext(base, first * 8, n * 8)))
        else:
            # untouched bytes come from the location's own entry value, so that the
            # same location (and binding) serves both sides
            parts.append((n * 8, ext(entry(('s', off, size)), (first - off) * 8, n * 8)))
    if len(parts) == 1:
        return parts[0][1]
    return cat(parts)


def exit_value(state, loc, entry):
    if loc == 'flags':
        f = state.flags
        return entry('flags') if f is FLAGS_IN else (('noflags',) if f is None else f)
    if isinstance(loc, str):
        return state.regs.get(loc, entry(loc))
    if loc[0] == 'f':
        k = loc[1]
        return state.fpu[-1 - k] if k < len(state.fpu) else entry(loc)
    off, size = loc[1], loc[2]
    v = slot_value(state.slots, off, size, entry)
    return entry(loc) if v is None else v


def compare_blocks(bc, be):
    ec = sorted(map(D, bc.effects))
    ee = sorted(map(D, be.effects))
    if ec != ee:
        dc = {D(e): e for e in bc.effects}
        de = {D(e): e for e in be.effects}
        # report the earliest differing effect on each side (deterministic)
        a = [e for e in bc.effects if D(e) in set(ec) - set(ee)]
        b = [e for e in be.effects if D(e) in set(ee) - set(ec)]
        global LAST_DIFF
        LAST_DIFF = (a[0] if a else None, b[0] if b else None)
        return 'effects differ: yours %s / exe %s' % (short(a[0]) if a else '-', short(b[0]) if b else '-')
    tc, te = bc.term, be.term
    if tc[0] == 'ret' and te[0] == 'ret':
        if D(tc[1:]) != D(te[1:]):
            return 'return values differ: %s / %s' % (short(tc[1]), short(te[1]))
    if tc[0] == 'tail' and te[0] == 'tail' and tc[1:] != te[1:]:
        return 'tail call differs'
    return None


def succ_pairs(tc, te):
    if tc[0] in ('ret', 'tail') or te[0] in ('ret', 'tail'):
        return [] if tc[0] == te[0] else None
    sc, se = term_succs(tc), term_succs(te)
    if tc[0] in ('jmp', 'fall') and te[0] in ('jmp', 'fall'):
        return [(sc[0], se[0])]
    if tc[0] == 'jcc' and te[0] == 'jcc':
        if D(tc[1]) == D(te[1]):
            return [(tc[2], te[2]), (tc[3], te[3])]
        if D(tc[1]) == D(neg_cond(te[1])):
            return [(tc[2], te[3]), (tc[3], te[2])]
        return None
    if tc[0] == 'switch' and te[0] == 'switch':
        if D(tc[1]) != D(te[1]) or len(tc[2]) != len(te[2]):
            return None
        return list(dict.fromkeys(zip(tc[2], te[2])))
    return None


def short(x, n=160):
    s = repr(x)
    return s if len(s) <= n else s[:n] + '...'



class BinaryReturnUse:
    """Demand on EAX at every statically referenced original call site.

    A function address must occur only as a direct call or a store into an
    absolute, unescaped callback slot. Unsupported references, tail transfers,
    exports, missing callers and undecodable continuations retain all bits.
    No source declaration or function name supplies a return type.
    """

    def __init__(self, pe, funcs, read):
        self.pe, self.funcs, self.read = pe, funcs, read
        self.starts = sorted(funcs)
        self.functions = {}
        self.cache = {}
        self.sections = [(sec, bytes(pe.read(sec.va, sec.rawsize))) for sec in pe.sections]
        self.direct = None

    def function(self, address):
        k = bisect.bisect_right(self.starts, address) - 1
        if k < 0:
            raise Unsupported('return-use site outside function inventory')
        start = self.starts[k]
        if address >= start + self.funcs[start]:
            raise Unsupported('return-use site outside recorded function')
        if start not in self.functions:
            self.functions[start] = Func(self.read(start), start)
        return self.functions[start]

    def instruction(self, address, containing=False):
        f = self.function(address)
        addresses = sorted(f.ins)
        k = bisect.bisect_right(addresses, address) - 1
        if k < 0:
            raise Unsupported('undecoded return-use site')
        i = f.ins[addresses[k]]
        if not i.address <= address < i.address + i.size or (not containing and i.address != address):
            raise Unsupported('return-use site is not an instruction')
        return f, i

    def references(self, value):
        word = (value & FULL).to_bytes(4, 'little')
        out = []
        for sec, raw in self.sections:
            if sec.name == '.reloc':
                continue
            off = raw.find(word)
            while off >= 0:
                out.append((sec, sec.va + off))
                off = raw.find(word, off + 1)
        return out

    def direct_calls(self):
        if self.direct is not None:
            return self.direct
        self.direct = {}
        for sec, raw in self.sections:
            if not sec.chars & 0x20000000:
                continue
            # Checking every possible encoding also catches references in code
            # missing from the function inventory. Such sites fail closed later.
            for opcode in (b'\xe8', b'\xe9'):
                off = raw.find(opcode)
                while off >= 0:
                    if off + 5 <= len(raw):
                        displacement = int.from_bytes(raw[off + 1:off + 5], 'little', signed=True)
                        target = (sec.va + off + 5 + displacement) & FULL
                        self.direct.setdefault(target, []).append(sec.va + off)
                    off = raw.find(opcode, off + 1)
        return self.direct

    @staticmethod
    def absolute(operand):
        return operand.type == X.X86_OP_MEM and not operand.mem.base and not operand.mem.index and \
            not operand.mem.segment and operand.size == 4

    def callback_calls(self, slot):
        calls = []
        for sec, reference in self.references(slot):
            if not sec.chars & 0x20000000:
                raise Unsupported('callback slot address escapes to data')
            f, i = self.instruction(reference, containing=True)
            ops = i.operands
            if reference != i.address + i.disp_offset or i.disp_size != 4:
                raise Unsupported('callback slot address escapes')
            if i.mnemonic == 'call' and self.absolute(ops[0]) and (ops[0].mem.disp & FULL) == slot:
                calls.append((f, i.address))
            elif i.mnemonic == 'mov' and self.absolute(ops[0]) and (ops[0].mem.disp & FULL) == slot and \
                    ops[1].type == X.X86_OP_IMM and (ops[1].imm & FULL) in self.funcs:
                continue
            else:
                raise Unsupported('unclassified callback slot access')
        if not calls:
            raise Unsupported('callback slot has no known calls')
        return calls

    def call_mask(self, f, call_address):
        token = 'callresult@%x' % call_address

        class ResultEvaluator(Evaluator):
            def call(self, st, eff, i, nxt):
                result = super().call(st, eff, i, nxt)
                if i.address == call_address:
                    st.set_reg('eax', ('in', token))
                return result

        escaped = stack_escapes(f)
        delta = esp_deltas(f, escaped, True)
        ev = ResultEvaluator(f, escaped, True)
        summ = {}
        call_block = None
        for address in f.blocks:
            if address not in delta:
                continue
            b = ev.run(address, stack_entry(delta[address]))
            if any(i.address == call_address for i in f.blocks[address]):
                call_block = address
            summ[address] = (b.state.read - set(delta[address]), b.state.written, term_succs(b.term), b)
        if call_block is None:
            raise Unsupported('unreachable return-use site')
        return liveness(summ).masks[call_block].get(token, 0) & FULL

    def mask(self, target):
        if target in self.cache:
            return self.cache[target]
        self.cache[target] = FULL
        # Synthetic callers without image sections, exported functions and an
        # externally entered image cannot establish a private return contract.
        if target not in self.funcs or not self.sections or target == getattr(self.pe, 'entry', None) or \
                (getattr(self.pe, 'dirs', ()) and self.pe.dirs[0] != (0, 0)):
            return FULL
        global _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES
        saved = _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES
        _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES = True, False, False, False, False
        try:
            calls = []
            for address in self.direct_calls().get(target, ()):
                f, i = self.instruction(address)
                if i.mnemonic != 'call' or i.operands[0].type != X.X86_OP_IMM or i.operands[0].imm != target:
                    raise Unsupported('tail or undecoded reference to return target')
                calls.append((f, address))
            for sec, reference in self.references(target):
                if sec.chars & 0x20000000:
                    f, i = self.instruction(reference, containing=True)
                    ops = i.operands
                    if i.mnemonic != 'mov' or not self.absolute(ops[0]) or ops[1].type != X.X86_OP_IMM or \
                            (ops[1].imm & FULL) != target or reference != i.address + i.imm_offset or i.imm_size != 4:
                        raise Unsupported('function address escapes')
                    calls.extend(self.callback_calls(ops[0].mem.disp & FULL))
                elif reference in getattr(self.pe, 'relocs', ()) and reference % 4 == 0:
                    calls.extend(self.callback_calls(reference))
                else:
                    raise Unsupported('unclassified function address in data')
            if not calls:
                return FULL
            observed = 0
            for f, address in set(calls):
                observed |= self.call_mask(f, address)
            self.cache[target] = observed
            return observed
        except Exception:
            return FULL
        finally:
            _EXTENDED, _ASSUME, _DEAD_STORES, _PRECISE_FRAME, _JOIN_RECIPES = saved


def configure(pe, funcs):
    """wire the call-convention oracles to an exe (tools.pe.PE) and its function table"""
    global X87_UNARY_POP, CALLEE_POP, IMPORT_NAME, EXE_READ, ARG_WIDTH, RETURN_MASK, FPU_OUT, THUNK_IAT, STACK_PROBE, PURE, CONST_READ, FRAME_INPUT, ADJ_FDIV
    # The symbol table identifies these CRT helpers. Their argument cleanup is
    # still taken from each executable's ret N, and the evaluator keeps their
    # result uninterpreted rather than assuming it equals the ordinary fdiv.
    adj_fdiv_args = {
        '_adj_fdiv_m32': 4, '_adj_fdiv_m64': 8,
        '_adj_fdivr_m32': 4, '_adj_fdivr_m64': 8,
        '_adj_fdiv_m16i': 4, '_adj_fdiv_m32i': 4,
        '_adj_fdivr_m16i': 4, '_adj_fdivr_m32i': 4,
    }
    ADJ_FDIV = {}
    sympath = ('config/net/symbols.tsv' if 'NET-WOXL.EXE' in getattr(pe, 'path', '').upper()
               else 'config/symbols.tsv')
    sympath = os.path.join(os.path.dirname(__file__), '..', sympath)
    if os.path.exists(sympath):
        with open(sympath) as symbol_file:
            symbol_lines = symbol_file.readlines()
        for line in symbol_lines:
            if not line.strip() or line.startswith('#'):
                continue
            fields = line.rstrip('\n').split('\t')
            if len(fields) < 3 or fields[1] not in adj_fdiv_args:
                continue
            address = int(fields[0], 16)
            size = funcs.get(address)
            if not size:
                continue
            code = pe.read(address, size)
            instructions = list(md.disasm(code, address))
            has_division = any(i.mnemonic.startswith(('fdiv', 'fdivr')) for i in instructions)
            has_division_call = any(i.mnemonic == 'call' and i.operands and
                                    i.operands[0].type == X.X86_OP_IMM and
                                    i.operands[0].imm in (0x4bcb66, 0x4bcb79)
                                    for i in instructions)
            expected_pop = adj_fdiv_args[fields[1]]
            if (has_division or has_division_call) and any(
                    i.mnemonic == 'ret' and i.op_str == '%x' % expected_pop for i in instructions):
                ADJ_FDIV[address] = (expected_pop, fields[1])
    cache = {}

    def pop(target):
        if target in cache:
            return cache[target]
        n = 0
        size = funcs.get(target)
        if size:
            code = pe.read(target, size)
            for i in capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32).disasm(code, target):
                if i.mnemonic == 'ret' and i.op_str:
                    n = int(i.op_str, 16)
                    break
        else:
            # import thunk: jmp [IAT]
            b = pe.read(target, 6)
            if b[:2] == b'\xff\x25':
                iat = int.from_bytes(b[2:6], 'little')
                if iat in pe.imports:
                    n = None
        cache[target] = n
        return n

    def imp(a):
        return pe.imports[a][1] if a in pe.imports else None

    def callee_pop(t):
        n = pop(t)
        return 0 if n is None else n

    starts = sorted(funcs)

    def exe_read(a):
        """the function's bytes up to the next known function: recorded sizes can stop
        at the last instruction and cut off a trailing switch table"""
        size = funcs.get(a)
        if not size:
            return None
        k = bisect.bisect_right(starts, a)
        nxt = starts[k] if k < len(starts) else a + size
        return pe.read(a, max(size, min(nxt - a, size + 256)))

    wcache = {}

    def arg_width(t):
        """bytes of each stack argument the callee ever reads (None: unknown)"""
        global ARG_WIDTH
        if t in wcache:
            return wcache[t]
        wcache[t] = None
        code = exe_read(t)
        if code is None:
            return None
        # widths of the callees this one forwards arguments to narrow its own
        # (recursion ends at the cache's None placeholder: unknown, i.e. whole)
        saved, ARG_WIDTH = ARG_WIDTH, arg_width
        try:
            for din in (0, 1):          # x87 helpers take a value on the x87 stack
                try:
                    f = Func(code, t)
                    f.entry_fdepth = din
                    esc = stack_escapes(f)
                    d = esp_deltas(f, esc, True)
                    summ = summarize(f, esc, True, d)
                    break
                except Unsupported:
                    if din == 1:
                        raise
            tail_widths = []
            if getattr(f, 'arg_address_taken', False):
                return None     # an argument is read through a pointer
            tails = [b for (_, _, _, b) in summ.values() if b.term[0] == 'tail']
            for b in tails:
                # a tail jump hands this function's own argument area to the target:
                # merge the target's widths (only when the frame is fully popped)
                if b.term[1][0] != 'c' or stack_off(b.state.regs.get('esp', SP)) != 0:
                    return None
                tw = arg_width(b.term[1][1])
                if tw is None:
                    return None
                tail_widths.append(tw)
            width = {}
            for tw in tail_widths:
                for k, v in tw.items():
                    width[k] = max(width.get(k, 0), v)
            try:
                # the bits of each incoming argument slot that anything depends on
                masks = liveness(summ).masks.get(t, {})
                used = []
                for loc, m in masks.items():
                    if isinstance(loc, tuple) and loc[0] == 's' and loc[1] + loc[2] > 4:
                        used += [loc[1] + i for i in range(loc[2]) if (m >> (8 * i)) & 0xff]
            except Unsupported:
                used = [b for r, _, _, _ in summ.values() for loc in r
                        if isinstance(loc, tuple) and loc[0] == 's' and loc[1] + loc[2] > 4
                        for b in range(loc[1], loc[1] + loc[2])]
            for byte in used:
                if byte >= 4:
                    k, j = divmod(byte - 4, 4)
                    width[k] = max(width.get(k, 0), j + 1)
            # an argument read whole somewhere and partially elsewhere counts whole
            wcache[t] = width
            return width
        except Exception:
            return None
        finally:
            ARG_WIDTH = saved

    fcache = {}

    def fpu_out(t, din):
        """x87 stack depth when the function returns, given din values on entry"""
        global FPU_OUT, ARG_WIDTH
        k = (t, din)
        if k in fcache:
            return fcache[k]
        fcache[k] = None
        code = exe_read(t)
        if code is None:
            return None
        saved = FPU_OUT, ARG_WIDTH
        FPU_OUT = ARG_WIDTH = None
        try:
            f = Func(code, t)
            f.entry_fdepth = din
            f.require_call_depth = True
            esc = stack_escapes(f)
            esp_deltas(f, esc, True)
            ev = Evaluator(f, esc, True)
            outs = set()
            for s0, d in f.spr.items():
                b = ev.run(s0, stack_entry(d))
                if b.term[0] == 'ret':
                    outs.add(len(b.state.fpu))
                elif b.term[0] == 'tail':
                    return None
            r = outs.pop() if len(outs) == 1 else None
            if r is not None and r not in (0, 1):
                r = None        # not a plausible x87 calling convention: don't trust it
            fcache[k] = r
            return r
        except Exception:
            return None
        finally:
            FPU_OUT, ARG_WIDTH = saved

    def thunk_iat(t):
        b = pe.read(t, 6)
        if b[:2] == b'\xff\x25':
            a = int.from_bytes(b[2:6], 'little')
            if a in pe.imports:
                return a
        return None

    probes = {}

    def stack_probe(t):
        """_chkstk: a short routine that ends by moving a computed value into esp"""
        if t not in probes:
            size = funcs.get(t) or 0
            code = pe.read(t, size) if 0 < size <= 64 else b''
            ins = list(capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32).disasm(code, t))
            probes[t] = any(i.mnemonic == 'mov' and i.op_str == 'esp, ecx' for i in ins) and \
                bool(ins) and ins[-1].mnemonic == 'ret'
        return probes[t]

    pure_cache = {}

    def pure(t):
        """the function reads and writes no memory (only registers, x87, its own
        non-escaping stack slots and the x87 control word, which it restores)"""
        global PURE
        if t in pure_cache:
            return pure_cache[t]
        pure_cache[t] = False
        code = exe_read(t)
        if code is None or len(code) > 512:
            return False
        saved, PURE = PURE, None
        try:
            for din in (0, 1):
                try:
                    f = Func(code, t)
                    if stack_escapes(f):
                        return False
                    f.entry_fdepth = din
                    d = esp_deltas(f, False, True)
                    ev = Evaluator(f, False, True)
                    ev.mem_read = False
                    blocks = [ev.run(s0, stack_entry(m)) for s0, m in d.items()]
                except Unsupported:
                    continue
                if ev.mem_read or any(b.term[0] == 'tail' or any(e[0] != 'fldcw' for e in b.effects) for b in blocks):
                    return False
                pure_cache[t] = True
                return True
            return False
        except Exception:
            return False
        finally:
            PURE = saved

    PURE = pure
    STACK_PROBE = stack_probe
    THUNK_IAT = thunk_iat
    CALLEE_POP = callee_pop
    IMPORT_NAME = imp
    EXE_READ = exe_read
    ARG_WIDTH = arg_width
    input_cache, input_active = {}, set()

    def frame_input(target, source):
        global _EXTENDED, _JOIN_RECIPES, _ASSUME, _DEAD_STORES
        key = (target, source)
        if key in input_cache:
            return input_cache[key]
        if key in input_active or len(input_active) >= 8 or len(input_cache) >= 2048:
            return None
        input_cache[key] = None
        code = exe_read(target)
        if code is None:
            return None
        input_active.add(key)
        saved = _EXTENDED, _JOIN_RECIPES, _ASSUME, _DEAD_STORES
        _EXTENDED = _JOIN_RECIPES = _ASSUME = _DEAD_STORES = False
        try:
            input_cache[key] = input_frame_footprint(code, target, source, frame_input)
        except (Unsupported, ValueError, KeyError, IndexError):
            pass
        finally:
            _EXTENDED, _JOIN_RECIPES, _ASSUME, _DEAD_STORES = saved
            input_active.remove(key)
        return input_cache[key]

    FRAME_INPUT = frame_input
    RETURN_MASK = BinaryReturnUse(pe, funcs, exe_read).mask
    ro = [(sec.va, sec.va + min(sec.vsize, sec.rawsize)) for sec in pe.sections if sec.readonly_data]

    def const_read(a, size):
        if a in pe.imports or not any(lo <= a and a + size <= hi for lo, hi in ro):
            return None
        return bytes(pe.read(a, size))
    CONST_READ = const_read
    FPU_OUT = fpu_out
    unary_cache = {}

    def unary_pop(target):
        """A pure body converts only TOP with fixed rounding and preserves lower cells.

        Probe one and two incoming cells. No pushes or value/status comparisons
        are allowed; CW updates must restore the saved entry word. The same
        sequence then preserves additional untouched lower cells. Derive registers
        from the body, never a calling-convention or library-name contract.
        """
        if target in unary_cache:
            return unary_cache[target]
        unary_cache[target] = None
        code = exe_read(target)
        if code is None or len(code) > 512 or not pure(target):
            return None
        try:
            f = Func(code, target)
            forbidden = {'call', 'fnstsw', 'fstsw', 'fnclex', 'fclex', 'fninit', 'finit',
                         'ftst', 'fcom', 'fcomp', 'fcompp', 'fucom', 'fucomp', 'fucompp'}
            if len(f.blocks) != 1 or any(i.mnemonic in forbidden for i in f.ins.values()):
                return None
            # A conversion's value must not depend on a caller's untracked
            # rounding mode, which an opaque callee may change. Restrict the
            # summary to integer pops with a body-proved fixed RC field.
            floating = {'fnstcw', 'fstcw', 'fldcw', 'fistp', 'fwait'}
            if any(i.mnemonic.startswith('f') and i.mnemonic not in floating for i in f.ins.values()):
                return None
            def fixed_bit(value, bit):
                kind = value[0]
                if kind == 'c':
                    return (value[1] >> bit) & 1
                if kind == 'ext':
                    return fixed_bit(value[3], bit + value[1]) if bit < value[2] else 0
                if kind == 'ins':
                    return fixed_bit(value[4], bit - value[2]) if value[2] <= bit < value[2] + value[3] else \
                        fixed_bit(value[1], bit)
                if kind in ('and', 'or', 'xor'):
                    bits = [fixed_bit(v, bit) for v in value[1:]]
                    if kind == 'and' and 0 in bits:
                        return 0
                    if kind == 'or' and 1 in bits:
                        return 1
                    if None not in bits:
                        return int(all(bits)) if kind == 'and' else int(any(bits)) if kind == 'or' else sum(bits) & 1
                return None
            reads_cw = [i.address for i in f.ins.values() if i.mnemonic in ('fnstcw', 'fstcw')]
            writes_cw = [i.address for i in f.ins.values() if i.mnemonic == 'fldcw']
            if writes_cw and (len(reads_cw) != 1 or reads_cw[0] >= min(writes_cw)):
                return None
            def contains_control(value):
                return value == ('fcw',) or (isinstance(value, tuple) and
                    any(contains_control(part) for part in value))
            results = []
            for depth in (1, 2):
                f = Func(code, target)
                f.entry_fdepth = depth
                delta = esp_deltas(f, False, True)
                class Probe(Evaluator):
                    def __init__(self, *args):
                        super().__init__(*args)
                        self.touched = set()
                        self.control = ('fcw',)
                    def fpu_op(self, state, effects, instruction):
                        if instruction.mnemonic == 'fldcw':
                            self.control = self.rd(state, effects, instruction.operands[0])
                        elif instruction.mnemonic == 'fistp' and any(
                                fixed_bit(self.control, bit) is None for bit in (10, 11)):
                            raise Unsupported('unary conversion uses caller rounding mode')
                        return super().fpu_op(state, effects, instruction)
                    def note(self, value):
                        used = {}
                        _uses(value, used)
                        self.touched.update(loc[1] for loc in used
                                            if isinstance(loc, tuple) and loc[0] == 'f')
                    def fst(self, state, index):
                        value = super().fst(state, index)
                        self.note(value)
                        return value
                    def fpop(self, state):
                        value = super().fpop(state)
                        self.note(value)
                        return value
                    def fpush(self, state, value):
                        raise Unsupported('unary callee grows x87 stack')
                ev = Probe(f, False, True)
                entry = stack_entry(delta[target])
                block = ev.run(target, entry)
                if block.term[0] != 'ret' or block.term[3] != 0 or ev.touched != {0} or \
                        block.state.fpu != [entry(('f', k)) for k in reversed(range(1, depth))]:
                    return None
                controls = [effect[1] for effect in block.effects if effect[0] == 'fldcw']
                if any(effect[0] != 'fldcw' for effect in block.effects) or \
                        (controls and controls[-1] != ext(('fcw',), 0, 16)):
                    return None
                registers = {r: exit_value(block.state, r, entry) for r in REG32 if r != 'esp'}
                if any(registers[r] != entry(r) for r in CALLEE_SAVED):
                    return None
                eax_uses = {}
                _uses(registers['eax'], eax_uses)
                if ('f', 0) not in eax_uses:
                    return None
                used = {}
                for r in ('eax', 'ecx', 'edx'):
                    if registers[r] != entry(r):
                        _uses(registers[r], used)
                if set(used) != {('f', 0)} or any(
                        registers[r] != entry(r) and (has_sp(registers[r]) or contains_control(registers[r]))
                        for r in ('eax', 'ecx', 'edx')):
                    return None
                results.append((registers, {r for r in ('ecx', 'edx') if registers[r] == entry(r)}))
            if results[0] == results[1]:
                unary_cache[target] = frozenset(results[0][1])
        except (Unsupported, ValueError, KeyError, IndexError):
            pass
        return unary_cache[target]
    X87_UNARY_POP = unary_pop

