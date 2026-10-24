"""List game functions not yet matched in src/ (outside NON_MATCHING blocks),
with the file/draft that covers them if any.

usage: unmatched.py [--tsv]
"""
import os, re, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import symbols as S
from datarefs import GAME_END

DEF = re.compile(r'^[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\([^;{]*\)\s*\{?\s*$')


HEAD = re.compile(r'^(?:__declspec\(\w+\)\s+)?[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\(')


def scan(root=S.REPO):
    defined = {}   # name -> (file, draft?)
    for f in sorted(glob.glob(os.path.join(root, 'src', '**', '*.c'), recursive=True)):
        depth = []
        with open(f, errors='replace') as fh:
            lines = fh.read().splitlines()
        for i, l in enumerate(lines):
            s = l.strip()
            if s.startswith('#if'):
                depth.append('NON_MATCHING' in s and not s.startswith('#ifndef'))
            elif s.startswith('#endif') and depth:
                depth.pop()
            m = HEAD.match(l)
            if not m or l.lstrip().startswith(('typedef', 'return', '#')):
                continue
            # join the signature up to the closing parenthesis
            sig, j = l, i
            while sig.count('(') > sig.count(')') and j + 1 < len(lines) and j - i < 8:
                j += 1
                sig += ' ' + lines[j].strip()
            # the parameter list's own closing parenthesis (a one-line body has more)
            k, d = sig.index('(', m.start(1)), 0
            for k in range(k, len(sig)):
                d += {'(': 1, ')': -1}.get(sig[k], 0)
                if d == 0:
                    break
            rest = sig[k + 1:].strip() if d == 0 else ''
            if not rest and j + 1 < len(lines):
                rest = lines[j + 1].strip()
            # prototypes (extern or not) end in ';'; only a body makes a definition
            if not rest.startswith('{'):
                continue
            n = m.group(1)
            if n in ('if', 'while', 'for', 'switch', 'return', 'sizeof'):
                continue
            draft = any(depth)
            prev = defined.get(n)
            if prev is None or (prev[1] and not draft):
                defined[n] = (os.path.relpath(f, root), draft)
    return defined


def main():
    syms = S.load()
    defined = scan()
    rows = [r for r in S.load_func_rows() if r[0] < GAME_END]
    out = []
    for a, size, tu in rows[:]:
        name = syms.by_addr.get(a, 'func_%06x' % a)
        alt = 'func_%06x' % a
        d = defined.get(name) or defined.get(alt)
        if d and not d[1]:
            continue
        out.append((a, size, name, d[0] if d else '-', 'draft' if d else 'none'))
    tot = sum(r[1] for r in rows)
    um = sum(o[1] for o in out)
    for o in out:
        print('%06x\t%d\t%s\t%s\t%s' % o)
    print('# %d unmatched functions, %d of %d bytes (%.1f%% matched)' % (
        len(out), um, tot, 100.0 * (tot - um) / tot), file=sys.stderr)


if __name__ == '__main__':
    main()
