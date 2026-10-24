"""Check NON_MATCHING drafts for semantic equivalence with the exe.

usage: equivcheck.py [--target d3d|powervr|net] [-v] [--only NAME] [FILES...]
By default compiles with /DNON_MATCHING /DEQUIV, then asks tools/equiv.py to
prove each function that differs in bytes. DIFF can also mean an unsupported
proof; a failed comparison alone does not establish an observable difference.
"""
import sys, os, re, glob, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import msvc, symbols as S, equiv
from coff import Obj
from check import Checker

REPO = S.REPO


def void_functions(src):
    text = open(src, errors='replace').read()
    return set(re.findall(r'^\s*(?:static\s+)?(?:__inline\s+)?void\s+(?:__\w+\s+)?(\w+)\s*\(', text, re.M))


def run(files, only=None, verbose=False, ck=None, define='NON_MATCHING,EQUIV', target='d3d'):
    ck = ck or Checker(target=target)
    equiv.configure(ck.exe, ck.funcs)
    out = []
    for src in files:
        tag = target + '/equiv' if target != 'd3d' else 'eq'
        obj = os.path.join(REPO, 'build', tag, os.path.relpath(src, os.path.join(REPO, 'src'))[:-2] + '.obj')
        os.makedirs(os.path.dirname(obj), exist_ok=True)
        flags = S.flags_for(src, target) + ['/D' + d for d in define.split(',')]
        if target in S.TARGET_DEFINES:
            flags.append(S.TARGET_DEFINES[target])
        ok, msg = msvc.compile(src, obj, flags,
                               [os.path.join(REPO, 'include'), os.path.dirname(os.path.abspath(src))])
        if not ok:
            out.append((src, None, None, 'COMPILE', msg.strip().splitlines()[-1] if msg.strip() else ''))
            continue
        voids = void_functions(src)
        equiv.for_file(src, target)
        res, _, _ = ck.check(Obj(open(obj, 'rb').read(), obj))
        for name, addr, status, info in res:
            if only and name != only:
                continue
            if status == 'MATCH':
                out.append((src, name, addr, 'MATCH', ''))
            elif status == 'DIFF':
                cand, exe, notes = info
                if len(cand) != len(exe) and False:
                    pass
                good, why = equiv.equivalent(cand, exe, addr, returns_value=name not in voids)
                out.append((src, name, addr, 'EQUIV' if good else 'DIFF', why))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('files', nargs='*')
    ap.add_argument('-v', action='store_true')
    ap.add_argument('--only')
    ap.add_argument('-D', dest='define', default='NON_MATCHING,EQUIV', help='guards to build with, comma-separated (default: both, as the gate verifies EQUIV)')
    ap.add_argument('--target', choices=('d3d', 'powervr', 'net'), default='d3d')
    a = ap.parse_args()
    files = a.files or sorted(glob.glob(os.path.join(REPO, 'src', '*.c')))
    rows = run(files, a.only, a.v, define=a.define, target=a.target)
    n = {}
    for src, name, addr, st, why in rows:
        n[st] = n.get(st, 0) + 1
        if st in ('EQUIV', 'COMPILE') or (st == 'DIFF' and a.v):
            print('%-7s %s %s %s' % (st, os.path.basename(src), name or '', why))
    print(' '.join('%s=%d' % kv for kv in sorted(n.items())))


if __name__ == '__main__':
    main()
