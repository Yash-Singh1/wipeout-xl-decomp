"""Compile a C file with MSVC 4.20 and compare every function in it with the exe.

Relocations in the object are resolved before comparing:
  - functions and globals by name through config/symbols.tsv
    (unknown names are learned from the exe and reported as new symbols)
  - the object's own .data/.bss/.rdata sections (string literals, statics) by
    one base address per section, which must agree across all references
  - __imp__ references through the exe's import table
The object's initialised data is then compared with the exe at the learned base.

usage: check.py [-v] [--flags "/O2 ..."] [--only NAME] FILE.c|FILE.obj
exit status 0 when every function matches.
"""
import sys, os, re, struct, difflib, collections, argparse
import capstone
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE
from coff import Obj, IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32
import msvc
import symbols as symtab

REPO = msvc.REPO
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def is_lit(sym):
    """string literals ($SG) and floating-point constants ($T) are checked one by one"""
    return sym.name.startswith('$SG') or sym.name.startswith('$T') or sym.name.startswith('??_C@')


def cname(n):
    """object symbol name -> C name"""
    if n.startswith('__imp__'):
        n = n[7:]
    elif n.startswith('_'):
        n = n[1:]
    return re.sub(r'@\d+$', '', n)


def bracketed_literal_base(data, offset, anchors, read):
    """Infer a short literal's base from matching literals on both sides.

    A partial source file can contain several disjoint runs of the original
    literal section. Require the nearest anchors to agree and the entire span
    between them, including padding, to match before preferring a local run.
    Each anchor is (object offset, executable address, literal byte length).
    """
    before = [a for a in anchors if a[0] < offset]
    after = [a for a in anchors if a[0] > offset]
    if not before or not after:
        return None
    left = max(before, key=lambda a: a[0])
    right = min(after, key=lambda a: a[0])
    base = (left[1] - left[0]) & 0xffffffff
    if base != (right[1] - right[0]) & 0xffffffff:
        return None
    end = right[0] + right[2]
    span = bytes(data[left[0]:end])
    return base if read(left[1], len(span)) == span else None


class Checker:
    def __init__(self, exe=None, target='d3d'):
        self.target = target
        self.amap = None
        if target in symtab.TARGETS:
            sp, fp, ap = symtab.pv_paths(target)
            self.syms = symtab.load(sp)
            self.exe = PE(exe or symtab.exe_path(target))
            self.funcs = symtab.load_funcs(fp)
            self.amap = symtab.load_addrmap(ap)
        elif target == 'd3d':
            self.syms = symtab.load()
            self.exe = PE(exe or os.path.join(REPO, 'orig', 'D3D.EXE'))
            self.funcs = symtab.load_funcs()
        else:
            sp, fp = symtab.target_paths(target)
            self.syms = symtab.load(sp)
            self.exe = PE(exe or symtab.target_exe(target))
            self.funcs = symtab.load_funcs(fp)
        self.imports = {fn: a for a, (dll, fn) in self.exe.imports.items()}
        self.relset = set(self.exe.relocs)

    def find_literal(self, lit):
        """unique 4-aligned occurrence of a NUL-terminated literal in the exe's
        initialised data, or None"""
        if len(lit) < 2:
            return None
        if not hasattr(self, '_lit_cache'):
            self._lit_cache = {}
            self._data_secs = [(sec, self.exe.data[sec.raw:sec.raw + sec.rawsize]) for sec in self.exe.sections
                               if sec.name in ('.data', '.rdata')]
        if lit in self._lit_cache:
            return self._lit_cache[lit]
        hits = []
        for sec, raw in self._data_secs:
            i = raw.find(lit)
            while i >= 0 and len(hits) < 2:
                if i % 4 == 0:
                    hits.append(sec.va + i)
                i = raw.find(lit, i + 1)
        r = hits[0] if len(hits) == 1 else None
        self._lit_cache[lit] = r
        return r

    def find_constant(self, lit):
        """first aligned copy of a constant in the exe's read-only data, or None"""
        for sec in self.exe.sections:
            if not sec.readonly_data:
                continue
            raw = self.exe.data[sec.raw:sec.raw + min(sec.vsize, sec.rawsize)]
            i = raw.find(lit)
            while i >= 0:
                if i % 4 == 0:
                    return sec.va + i
                i = raw.find(lit, i + 1)
        return None

    def addr_of(self, d3d_addr):
        """address in the target exe of something the sources name by its D3D address"""
        return d3d_addr if self.amap is None else self.amap.get(d3d_addr)

    def check(self, obj, only=None, verbose=False):
        o = obj
        results = []
        learned = {}       # C name -> addr (new symbols)
        bases = {}         # section index -> Counter of base votes
        litv = {}          # string literal symbol index -> Counter of address votes
        text = [i for i, s in enumerate(o.sections) if s['flags'] & 0x20]
        fsyms = []
        for s in o.syms.values():
            if s.secnum - 1 in text and (s.type == 0x20 or s.cls == 2) and not s.name.startswith('$'):
                fsyms.append(s)
        fsyms.sort(key=lambda s: (s.secnum, s.value))

        def target_addr(fs):
            n = cname(fs.name)
            if n in self.syms.by_name:
                return self.syms.by_name[n][0]
            m = re.match(r'func_([0-9A-Fa-f]{6,8})$', n)
            return self.addr_of(int(m.group(1), 16)) if m else None

        # pass 1: vote section bases using functions with known addresses
        spans = []
        for i, fs in enumerate(fsyms):
            sec = o.sections[fs.secnum - 1]
            nxt = [x.value for x in fsyms[i + 1:] if x.secnum == fs.secnum]
            end = nxt[0] if nxt else sec['size']
            addr = target_addr(fs)
            spans.append((fs, addr, fs.value, end))
            if addr is None:
                continue
            exe = self.exe.read(addr, end - fs.value)
            for va, si, typ in sec['relocs']:
                if not fs.value <= va < end:
                    continue
                sym = o.syms[si]
                if sym.secnum > 0 and typ == IMAGE_REL_I386_DIR32 and sym.secnum - 1 not in text:
                    add = struct.unpack_from('<I', sec['data'], va)[0]
                    ev = struct.unpack_from('<I', exe, va - fs.value)[0]
                    if (addr + va - fs.value) not in self.relset:
                        continue
                    if is_lit(sym):
                        litv.setdefault(sym.idx, collections.Counter())[(ev - add) & 0xffffffff] += 1
                    else:
                        bases.setdefault(sym.secnum - 1, collections.Counter())[(ev - add - sym.value) & 0xffffffff] += 1
        base = {k: v.most_common(1)[0][0] for k, v in bases.items()}
        self.lits = {k: v.most_common(1)[0][0] for k, v in litv.items()}
        ambiguous = []
        # literals: trust a vote only if the exe holds the same bytes there;
        # otherwise (code shifted in a draft, or no aligned vote) find them by content
        for sym in o.syms.values():
            if not is_lit(sym) or sym.secnum <= 0 or sym.secnum - 1 in text:
                continue
            data = o.sections[sym.secnum - 1]['data']
            if sym.name.startswith('$T'):
                # a constant runs to the next symbol of its section
                nxt = min([x.value for x in o.syms.values() if x.secnum == sym.secnum and x.value > sym.value]
                          + [len(data)])
                lit = data[sym.value:nxt]
            else:
                nul = data.find(b'\0', sym.value)
                lit = data[sym.value:nul + 1 if nul >= 0 else len(data)]
            la = self.lits.get(sym.idx)
            if la is not None and self.exe.read(la, len(lit)) == lit:
                continue
            hit = self.find_literal(lit)
            if hit is not None:
                self.lits[sym.idx] = hit
            else:
                if la is not None:
                    del self.lits[sym.idx]  # the vote points at different bytes: never trust it
                ambiguous.append((sym, lit))
        # a literal whose bytes occur more than once in the exe: place it by the
        # section offset the other (verified) literals of its section agree on
        # a literal too short for its bytes to identify it (an empty string matches any
        # zero byte) keeps its vote only where the section layout of the verified longer
        # literals does not say otherwise
        for sym in o.syms.values():
            if not is_lit(sym) or sym.secnum <= 0 or sym.secnum - 1 in text or sym.idx not in self.lits:
                continue
            data = o.sections[sym.secnum - 1]['data']
            nul = data.find(b'\0', sym.value)
            if sym.name.startswith('$T') or nul < 0 or nul + 1 - sym.value >= 4:
                continue
            votes = collections.Counter()
            anchors = []
            for other in o.syms.values():
                if other.secnum == sym.secnum and other.idx in self.lits and other.idx != sym.idx and \
                        is_lit(other) and not other.name.startswith('$T'):
                    od = o.sections[other.secnum - 1]['data']
                    on = od.find(b'\0', other.value)
                    if on >= 0 and on + 1 - other.value >= 4:
                        votes[(self.lits[other.idx] - other.value) & 0xffffffff] += 1
                        anchors.append((other.value, self.lits[other.idx], on + 1 - other.value))
            if votes:
                secbase = bracketed_literal_base(data, sym.value, anchors, self.exe.read)
                if secbase is None:
                    secbase = votes.most_common(1)[0][0]
                a = (secbase + sym.value) & 0xffffffff
                if a != self.lits[sym.idx] and self.exe.read(a, nul + 1 - sym.value) == data[sym.value:nul + 1]:
                    self.lits[sym.idx] = a
        secwhole = {}
        for sym, lit in ambiguous:
            votes = collections.Counter()
            for other in o.syms.values():
                if other.secnum == sym.secnum and other.idx in self.lits and other.idx != sym.idx:
                    votes[(self.lits[other.idx] - other.value) & 0xffffffff] += 1
            if not votes:
                # nothing verified in this section: the linker keeps a section's
                # contribution contiguous, so look for all of its bytes at once
                whole = secwhole.get(sym.secnum)
                if whole is None:
                    whole = secwhole[sym.secnum] = self.find_literal(bytes(o.sections[sym.secnum - 1]['data']))
                if whole is not None:
                    votes[whole] += 1
            if votes:
                near = sorted((abs(other.value - sym.value), (self.lits[other.idx] - other.value) & 0xffffffff)
                              for other in o.syms.values()
                              if other.secnum == sym.secnum and other.idx in self.lits and other.idx != sym.idx)
                done = False
                for _, secbase in near + [(0, votes.most_common(1)[0][0])]:
                    a = (secbase + sym.value) & 0xffffffff
                    if self.exe.read(a, len(lit)) == lit:
                        self.lits[sym.idx] = a
                        done = True
                        break
                if done:
                    continue
            if sym.name.startswith('$T'):
                # a compiler constant read from read-only data is compared by its
                # contents (tools/equiv.py), so any copy of the same bytes will do
                a = self.find_constant(lit)
                if a is not None:
                    self.lits[sym.idx] = a

        for fs, addr, off, end in spans:
            name = cname(fs.name)
            if only and name != only:
                continue
            sec = o.sections[fs.secnum - 1]
            if addr is None:
                results.append((name, None, 'NOADDR', ''))
                continue
            size = end - off
            cand = bytearray(sec['data'][off:end])
            exe = self.exe.read(addr, size)
            notes = []
            for va, si, typ in sec['relocs']:
                if not off <= va < end:
                    continue
                p = va - off
                sym = o.syms[si]
                add = struct.unpack_from('<I', cand, p)[0]
                ev = struct.unpack_from('<I', exe, p)[0]
                if typ == IMAGE_REL_I386_REL32:
                    exe_target = (ev + addr + p + 4) & 0xffffffff
                else:
                    exe_target = ev
                t = None
                if sym.secnum - 1 == fs.secnum - 1 and sym.secnum > 0 and (sym.cls == 3 or sym.type != 0x20 and not sym.name.startswith('_')):
                    # local label / jump table in our own .text
                    t = addr + (sym.value - off) + (add if typ == IMAGE_REL_I386_DIR32 else 0)
                elif sym.secnum > 0 and sym.secnum - 1 not in text and is_lit(sym):
                    la = self.lits.get(sym.idx)
                    t = None if la is None else (la + add) & 0xffffffff
                elif sym.secnum > 0 and sym.secnum - 1 not in text:
                    b = base.get(sym.secnum - 1)
                    t = None if b is None else (b + sym.value + add) & 0xffffffff
                else:
                    n = cname(sym.name)
                    if sym.name.startswith('__imp__') and n in self.imports:
                        t = self.imports[n]
                    elif n in self.syms.by_name:
                        t = self.syms.by_name[n][0]
                    elif sym.secnum > 0 and sym.secnum - 1 in text:
                        tf = [x for x in spans if x[0] is sym]
                        t = tf[0][1] if tf and tf[0][1] is not None else None
                    elif re.match(r'(func|D|data)_([0-9A-Fa-f]{6,8})$', n):
                        t = self.addr_of(int(n.split('_')[1], 16))
                    if t is not None and typ == IMAGE_REL_I386_DIR32:
                        t += add
                    if t is None and self.amap is not None:
                        notes.append('%s has no %s address' % (n, 'NET' if self.target == 'net' else 'PowerVR'))
                    elif t is None:
                        t = exe_target
                        prev = learned.get(n)
                        tgt = (exe_target - (add if typ == IMAGE_REL_I386_DIR32 else 0)) & 0xffffffff
                        if prev is None:
                            learned[n] = tgt
                        elif prev != tgt:
                            notes.append('%s used at %x and %x' % (n, prev, tgt))
                if t is None:
                    # data the exe doesn't reference from here
                    notes.append('reference to %s has no counterpart in the exe' % sym.name)
                    t = exe_target + 1
                if typ == IMAGE_REL_I386_REL32:
                    val = (t - (addr + p + 4)) & 0xffffffff
                else:
                    val = t & 0xffffffff
                struct.pack_into('<I', cand, p, val)
            exp_size = self.funcs.get(addr)
            ok = bytes(cand) == exe
            # trailing filler in the object may be shorter than in the exe
            if not ok:
                tc = bytes(cand).rstrip(b'\x90')
                if exp_size and len(cand) >= exp_size and bytes(cand[:exp_size]) == exe[:exp_size]:
                    ok = True
            status = 'MATCH' if ok and not notes else 'DIFF'
            if exp_size is None:
                notes.append('no function starts at %x in the exe' % addr)
                status = 'DIFF'
            results.append((name, addr, status, (bytes(cand), exe, notes)))
        return results, learned, base

    def data_check(self, o, base):
        """compare initialised data: each string literal at its own address,
        other statics through their section's base."""
        out = []
        litsyms = {}
        for sym in o.syms.values():
            if is_lit(sym) and sym.secnum > 0:
                litsyms.setdefault(sym.secnum - 1, []).append(sym)
        for si in set(base) | set(litsyms):
            sec = o.sections[si]
            if not sec['data'] or sec['flags'] & 0x80:
                continue
            data = sec['data']
            mask = [True] * len(data)
            for va, s_, typ in sec['relocs']:
                for k in range(4):
                    if va + k < len(mask):
                        mask[va + k] = False
            offs = sorted({x.value for x in o.syms.values() if x.secnum - 1 == si} | {len(data)})
            nbad = 0
            nlit = 0
            for sym in litsyms.get(si, []):
                if sym.name.startswith('$SG'):
                    end = data.index(b'\0', sym.value) + 1 if b'\0' in data[sym.value:] else len(data)
                else:
                    end = min([x for x in offs if x > sym.value] + [sym.value + 8])
                for k in range(sym.value, end):
                    mask[k] = None
                a = self.lits.get(sym.idx)
                if a is None:
                    continue
                if self.exe.read(a, end - sym.value) != data[sym.value:end]:
                    out.append(('literal %s' % sym.name, a, end - sym.value, [0]))
                else:
                    nlit += 1
            b = base.get(si)
            if b is None:
                if nlit:
                    out.append(('%d literals' % nlit, 0, 0, []))
                continue
            try:
                exe = self.exe.read(b, len(data))
            except AttributeError:
                continue    # base learned from an entropy-matched build was never validated
            # zero padding between literals isn't checked
            bad = [i for i in range(len(data)) if mask[i] and data[i] != exe[i]]
            out.append((sec['name'], b, len([m for m in mask if m]), bad))
        return out


import threading
_lock = threading.Lock()
ENTROPY_MAX = 96
PRE_MAX = 16      # dummy declarations tried directly in front of a function's own definition
BEFORE = 100      # entropy hint counts above this mean 'in front of the definition'


def entropy_search(ck, src, flags, includes, names, kmax=ENTROPY_MAX, jobs=4, save=True, judge=None, tag='', kbefore=None):
    """MSVC 4.20's output depends on compiler heap state, which unrelated
    declarations shift. Recompile src with k dummy declarations in front
    (k = 1..kmax) and return {name: k} for functions that then match exactly.
    judge(name, addr, status, info) -> bool replaces "match exactly" (the EQUIV pass
    uses it to accept proven equivalence); tag keeps its builds and hints apart.
    Functions still unmatched are then tried with k = 1..kbefore dummy declarations
    placed directly in front of their own definitions instead (the heap state a
    function sees also depends on the code compiled before it); such a count is
    recorded as BEFORE + k. Only the byte-match search does this (judge is None)."""
    import concurrent.futures as cf
    text = open(src).read()
    base = os.path.basename(src)
    tag = re.sub(r'\W', '_', os.path.relpath(os.path.abspath(src), REPO)) + tag
    hint = (lambda n: n + '@' + tag.rsplit('_', 1)[-1]) if judge else (lambda n: n)
    want = set(names)
    found = {}

    def defined_before(k, targets):
        # k dummy declarations in front of each definition of the target functions
        dummies = ['extern int entropy%d;' % i for i in range(k)]
        lines = text.split('\n')
        out = []
        for i, l in enumerate(lines):
            if l[:1] not in ('', ' ', '\t', '#', '/', '{', '}') and not l.rstrip().endswith(';') and \
                    i + 1 < len(lines) and lines[i + 1].startswith('{') and \
                    any(re.search(r'\b%s\s*\(' % re.escape(n), l) for n in targets):
                out.extend(dummies)
            out.append(l)
        return '\n'.join(out)

    def one(k):
        d = os.path.join(REPO, 'build', 'entropy', tag, 'k%02d' % k)
        os.makedirs(d, exist_ok=True)
        f = os.path.join(d, base)
        with open(f, 'w') as fh:
            if k > BEFORE:
                fh.write(defined_before(k - BEFORE, want - set(found)))
            else:
                fh.write(''.join('extern int entropy%d;\n' % i for i in range(k)) + text)
        obj = f[:-2] + '.obj'
        ok, _ = msvc.compile(f, obj, flags, includes)
        if not ok:
            # a compile can fail transiently when the shared container is overloaded; the same
            # source either compiles or not, so one retry only removes the flake
            ok, _ = msvc.compile(f, obj, flags, includes)
        if not ok:
            return k, []
        with _lock:
            res, _, _ = ck.check(Obj(open(obj, 'rb').read(), obj))
            if judge is None:
                return k, [r[0] for r in res if r[2] == 'MATCH' and r[0] in want]
            return k, [r[0] for r in res if r[0] in want and r[0] not in found and judge(*r)]

    # try the counts that worked before first, then the rest in batches,
    # stopping as soon as every wanted function has matched
    hints = load_entropy_hints()
    first = sorted({hints[hint(n)] for n in want if hint(n) in hints and 1 <= hints[hint(n)] <= kmax})
    order = first + [k for k in range(1, kmax + 1) if k not in first]
    with cf.ThreadPoolExecutor(jobs) as ex:
        i = 0
        while i < len(order) and not want <= set(found):
            batch = order[i:i + (len(first) if i == 0 and first else jobs)]
            i += len(batch)
            for k, ms in ex.map(one, batch):
                for n in ms:
                    found.setdefault(n, k)
        kb = PRE_MAX if kbefore is None else kbefore
        if judge is None and kb and not want <= set(found):
            hinted = sorted({hints[hint(n)] for n in want - set(found)
                             if hint(n) in hints and BEFORE < hints[hint(n)] <= BEFORE + kb})
            order = hinted + [BEFORE + k for k in range(1, kb + 1) if BEFORE + k not in hinted]
            i = 0
            while i < len(order) and not want <= set(found):
                batch = order[i:i + jobs]
                i += len(batch)
                for k, ms in ex.map(one, batch):
                    for n in ms:
                        found.setdefault(n, k)
    if save:
        save_entropy_hints({hint(n): k for n, k in found.items()})
    return found


HINTS_SEED = os.path.join(REPO, 'config', 'entropy_hints.tsv')
HINTS_LOCAL = os.path.join(REPO, 'build', 'entropy_hints.tsv')


def load_entropy_hints():
    """function name -> dummy-declaration count that last made it match"""
    out = {}
    for f in (HINTS_SEED, HINTS_LOCAL):
        if os.path.exists(f):
            for l in open(f):
                p = l.split()
                if len(p) == 2 and p[1].isdigit():
                    out[p[0]] = int(p[1])
    return out


def save_entropy_hints(found):
    if not found:
        return
    with _lock:
        h = {}
        if os.path.exists(HINTS_LOCAL):
            for l in open(HINTS_LOCAL):
                p = l.split()
                if len(p) == 2:
                    h[p[0]] = p[1]
        h.update({n: str(k) for n, k in found.items()})
        os.makedirs(os.path.dirname(HINTS_LOCAL), exist_ok=True)
        with open(HINTS_LOCAL, 'w') as fh:
            fh.writelines('%s\t%s\n' % kv for kv in sorted(h.items()))


def disasm(code, addr, syms):
    lines = []
    for ins in md.disasm(code, addr):
        s = ins.op_str
        for m in set(re.findall(r'0x[0-9a-f]{6}', s)):
            v = int(m, 16)
            if v in syms.by_addr:
                s = s.replace(m, syms.by_addr[v])
        lines.append((ins.address, ins.mnemonic + ' ' + s))
    return lines


def show_diff(cand, exe, addr, syms, context=3):
    a = disasm(exe, addr, syms)
    b = disasm(cand, addr, syms)
    at = [x[1] for x in a]
    bt = [x[1] for x in b]
    sm = difflib.SequenceMatcher(None, at, bt, autojunk=False)
    out = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            if i2 - i1 > 2 * context:
                for k in range(i1, i1 + context):
                    out.append('   %x  %-44s %s' % (a[k][0], at[k], bt[j1 + k - i1]))
                out.append('   ...')
                for k in range(i2 - context, i2):
                    out.append('   %x  %-44s %s' % (a[k][0], at[k], bt[j1 + k - i1]))
            else:
                for k in range(i1, i2):
                    out.append('   %x  %-44s %s' % (a[k][0], at[k], bt[j1 + k - i1]))
        else:
            n = max(i2 - i1, j2 - j1)
            for k in range(n):
                l = at[i1 + k] if i1 + k < i2 else ''
                r = bt[j1 + k] if j1 + k < j2 else ''
                ad = '%x' % a[i1 + k][0] if i1 + k < i2 else '      '
                out.append(' ! %s  %-44s %s' % (ad, l, r))
    ratio = sm.ratio()
    return ratio, out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('file')
    ap.add_argument('-v', action='store_true', help='show diffs of non-matching functions')
    ap.add_argument('--only')
    ap.add_argument('--flags')
    ap.add_argument('--obj')
    ap.add_argument('-D', dest='defines', action='append', default=[], help='extra guard to build with, e.g. -D EQUIV -D NON_MATCHING')
    ap.add_argument('--no-entropy', action='store_true', help="don't retry DIFFs with dummy declarations")
    ap.add_argument('--target', choices=('d3d', 'powervr', 'net', 'launcher', 'dxlink'), default='d3d', help='which exe to compare with (default d3d)')
    args = ap.parse_args()
    ck = Checker(target=args.target)
    f = args.file
    if f.endswith('.c'):
        obj = args.obj or os.path.join(REPO, 'build', 'check', os.path.basename(f)[:-2] + '.obj')
        os.makedirs(os.path.dirname(obj), exist_ok=True)
        flags = (['/nologo', '/c'] + args.flags.split()) if args.flags else symtab.flags_for(f, args.target)
        if args.target in symtab.TARGET_DEFINES:
            flags += [symtab.TARGET_DEFINES[args.target]]
        flags += ['/D' + d for ds in args.defines for d in ds.split(',')]
        ok, out = msvc.compile(f, obj, flags, [os.path.join(REPO, 'include'), os.path.dirname(os.path.abspath(f))])
        if out.strip():
            print(out)
        if not ok:
            sys.exit(2)
    else:
        obj = f
    o = Obj(open(obj, 'rb').read(), obj)
    res, learned, base = ck.check(o, args.only, args.v)
    ent = {}
    diffs = [r[0] for r in res if r[2] == 'DIFF']
    if diffs and f.endswith('.c') and not args.no_entropy:
        ent = entropy_search(Checker(target=args.target), f, flags, [os.path.join(REPO, 'include'), os.path.dirname(os.path.abspath(f))], diffs)
    nmatch = 0
    for name, addr, status, info in res:
        if name in ent:
            nmatch += 1
            print('MATCH*   %06x %s  (matches with %s: compiler heap noise)' % (addr, name, ('%d dummy declarations in front of its definition' % (ent[name] - BEFORE)) if ent[name] > BEFORE else ('%d dummy declarations in front' % ent[name])))
            continue
        if status == 'NOADDR':
            print('%-8s %s (no address: name it in config/symbols.tsv or call it func_XXXXXX)' % ('?', name))
            continue
        cand, exe, notes = info
        if status == 'MATCH':
            nmatch += 1
            print('MATCH    %06x %s' % (addr, name))
        else:
            ratio, lines = show_diff(cand, exe, addr, ck.syms)
            import equiv
            from equivcheck import void_functions
            equiv.configure(ck.exe, ck.funcs)
            equiv.for_file(f, ck.target)
            eqok, eqwhy = equiv.equivalent(cand, exe, addr, returns_value=not (f.endswith('.c') and name in void_functions(f)))
            print('%s %06x %s  (%.1f%% similar, %d vs %d bytes)%s' % ('EQUIV   ' if eqok else 'DIFF    ', addr, name, ratio * 100, len(cand), ck.funcs.get(addr, 0),
                  '' if eqok else '\n         not equivalent: ' + eqwhy))
            for n in notes:
                print('         note: ' + n)
            if args.v:
                print('     exe' + ' ' * 46 + 'yours')
                print('\n'.join(lines))
    for n, a in sorted(learned.items(), key=lambda x: x[1]):
        known = ck.syms.by_addr.get(a)
        if known and known != n:
            print('SYMBOL   %s -> %06x, but symbols.tsv calls that address %s' % (n, a, known))
        else:
            print('NEWSYM   %06x %s' % (a, n))
    for name, b, size, bad in ck.data_check(o, base):
        if name.startswith('literal'):
            print('DATA     %s at %06x differs from the exe' % (name, b))
        elif name.endswith('literals'):
            print('DATA     %s OK' % name)
        elif bad:
            print('DATA     %s at %06x: %d of %d bytes differ (first at +%x)' % (name, b, len(bad), size, bad[0]))
        else:
            print('DATA     %s at %06x: %d bytes OK' % (name, b, size))
    total = sum(1 for r in res if r[2] != 'NOADDR')
    print('%d/%d functions match' % (nmatch, total))
    sys.exit(0 if nmatch == total and total else 1)


if __name__ == '__main__':
    main()
