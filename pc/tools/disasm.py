"""Disassemble exe functions with symbols, labels, imports and string literals.

usage: disasm.py [--target d3d|powervr|net] NAME|ADDR [...]       one or more functions
       disasm.py [--target d3d|powervr|net] --range START END     every function in [START, END)
"""
import sys, os, re, bisect, struct
import capstone
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE
import symbols as S
from datarefs import is_string

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


class Dis:
    def __init__(self, target='d3d'):
        self.target = target
        if target in S.TARGETS:
            sp, fp, _ = S.pv_paths(target)
            self.pe = PE(S.exe_path(target))
            self.syms = S.load(sp)
            self.funcs = S.load_func_rows(fp)
        elif target == 'd3d':
            self.pe = PE(os.path.join(S.REPO, 'orig', 'D3D.EXE'))
            self.syms = S.load()
            self.funcs = S.load_func_rows()
        else:
            sp, fp = S.target_paths(target)
            self.pe = PE(S.target_exe(target))
            self.syms = S.load(sp)
            self.funcs = S.load_func_rows(fp)
        self.starts = [f[0] for f in self.funcs]
        self.relset = set(self.pe.relocs)
        self.data = self.pe.section('.data')

    def name(self, a):
        if a in self.syms.by_addr:
            return self.syms.by_addr[a]
        if a in self.pe.imports:
            return '__imp_' + self.pe.imports[a][1]
        i = bisect.bisect_right(self.starts, a) - 1
        if i >= 0 and self.starts[i] == a:
            return 'func_%06x' % a
        if a in self.data:
            # inside a named global?
            return 'D_%06x' % a
        return None

    def comment(self, a):
        if a in self.data and is_string(self.pe, a):
            s = self.pe.cstr(a)
            return '"%s"' % s.encode('unicode_escape').decode()[:60]
        return None

    def function(self, addr):
        i = self.starts.index(addr)
        a, size = self.funcs[i][0], self.funcs[i][1]
        code = self.pe.read(a, size)
        out = []
        nm = self.name(a)
        out.append('; %s  %06x-%06x  (%d bytes)' % (nm, a, a + size, size))
        ins = []
        p = a
        jt = set()
        while p < a + size:
            # jump-table words inside the function
            if p in self.relset and a <= self.pe.u32(p) < a + size and (p in jt or p % 4 == 0 and not any(x.address <= p < x.address + x.size for x in ins[-1:])):
                ins.append(('dd', p, 4, self.pe.u32(p)))
                p += 4
                continue
            one = next(md.disasm(code[p - a:p - a + 16], p, 1), None)
            if one is None:
                ins.append(('db', p, 1, code[p - a]))
                p += 1
                continue
            ins.append(one)
            m = re.search(r'\[\w+\*4 \+ (0x[0-9a-f]+)\]', one.op_str)
            if one.mnemonic == 'jmp' and m:
                tb = int(m.group(1), 16)
                k = 0
                while tb + 4 * k in self.relset:
                    jt.add(tb + 4 * k)
                    k += 1
            p += one.size
        labels = set()
        for x in ins:
            if isinstance(x, tuple):
                if x[0] == 'dd':
                    labels.add(x[3])
                continue
            if (x.mnemonic.startswith('j') or x.mnemonic == 'loop') and x.op_str.startswith('0x'):
                t = int(x.op_str, 16)
                if a <= t < a + size:
                    labels.add(t)
        for x in ins:
            ad = x[1] if isinstance(x, tuple) else x.address
            if ad in labels:
                out.append('L_%06x:' % ad)
            if isinstance(x, tuple):
                if x[0] == 'dd':
                    out.append('    %06x:  dd L_%06x' % (ad, x[3]))
                else:
                    out.append('    %06x:  db 0x%02x' % (ad, x[3]))
                continue
            ops = x.op_str
            cmts = []
            def sub(m):
                v = int(m.group(0), 16)
                if a <= v < a + size and v in labels:
                    return 'L_%06x' % v
                n = self.name(v)
                c = self.comment(v)
                if c:
                    cmts.append(c)
                return n or m.group(0)
            ops = re.sub(r'0x[0-9a-f]{6,8}', sub, ops)
            line = '    %06x:  %-6s %s' % (ad, x.mnemonic, ops)
            if cmts:
                line = '%-60s ; %s' % (line, ' '.join(cmts))
            out.append(line)
        return '\n'.join(out)


def main():
    args = sys.argv[1:]
    target = 'd3d'
    if args and args[0] == '--target':
        target = args[1]
        if target not in ('d3d', 'powervr', 'net', 'launcher', 'dxlink'):
            raise SystemExit('target must be d3d, powervr, net, launcher or dxlink')
        args = args[2:]
    d = Dis(target)
    if args and args[0] == '--range':
        lo, hi = int(args[1], 16), int(args[2], 16)
        targets = [f[0] for f in d.funcs if lo <= f[0] < hi]
    else:
        targets = []
        for t in args:
            if t in d.syms.by_name:
                targets.append(d.syms.by_name[t][0])
            else:
                targets.append(int(t.replace('func_', ''), 16))
    for t in targets:
        print(d.function(t))
        print()


if __name__ == '__main__':
    main()
