"""Turn NON_MATCHING drafts that tools/equiv.py proves equivalent into EQUIV blocks.

usage: promote_equiv.py [FILES...]   (default: every src/*.c)
For each `#ifdef NON_MATCHING ... #endif` block whose functions are all proven
equivalent (judged in the NON_MATCHING build), the guard becomes `#ifdef EQUIV`.
Then every touched file is rebuilt with /DEQUIV; blocks whose functions are not
exact-or-equivalent there are reverted. Run tools/progress.py afterwards.
"""
import sys, os, re, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import msvc, symbols as S, equiv
from coff import Obj
from check import Checker
from equivcheck import run, void_functions

REPO = S.REPO
FUNC = re.compile(r'^[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\([^;]*$', re.M)


def blocks(text):
    """(start line, end line, guard) of top-level #if blocks"""
    lines = text.split('\n')
    out, stack = [], []
    for i, l in enumerate(lines):
        t = l.strip()
        if t.startswith('#if'):
            stack.append((i, t))
        elif t.startswith('#endif') and stack:
            s, g = stack.pop()
            if not stack:
                out.append((s, i, g))
    return lines, out


def funcs_in(lines, s, e):
    names = []
    for k in range(s + 1, e):
        l = lines[k]
        m = re.match(r'^[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\(', l)
        if m and not l.rstrip().endswith(';') and not l.startswith(('typedef', 'extern', 'static const', '#')):
            j = k
            while j < e and '{' not in lines[j] and not lines[j].rstrip().endswith(';'):
                j += 1
            if j < e and '{' in lines[j]:
                names.append(m.group(1))
    return names


def set_guard(path, line_no, guard):
    lines = open(path).read().split('\n')
    lines[line_no] = guard
    open(path, 'w').write('\n'.join(lines))


def main():
    files = [os.path.abspath(f) for f in (sys.argv[1:] or sorted(glob.glob(os.path.join(REPO, 'src', '*.c'))))]
    ck = Checker()
    rows = run(files, ck=ck)
    proven = {(os.path.abspath(r[0]), r[1]) for r in rows if r[3] == 'EQUIV'}
    changed = {}
    for f in files:
        lines, bl = blocks(open(f).read())
        for s, e, g in bl:
            if g.replace(' ', '') not in ('#ifdefNON_MATCHING', '#ifNON_MATCHING', '#ifdefined(NON_MATCHING)'):
                continue
            names = funcs_in(lines, s, e)
            if names and all((f, n) in proven for n in names):
                set_guard(f, s, '#ifdef EQUIV')
                changed.setdefault(f, []).append((s, names))
    # verify each touched file in its EQUIV build; revert blocks that fail
    equiv.configure(ck.exe, ck.funcs)
    for f, bl in changed.items():
        obj = os.path.join(REPO, 'build', 'equiv', os.path.basename(f)[:-2] + '.obj')
        os.makedirs(os.path.dirname(obj), exist_ok=True)
        ok, msg = msvc.compile(f, obj, S.flags_for(f) + ['/DEQUIV', '/DNON_MATCHING'], [os.path.join(REPO, 'include'), os.path.dirname(f)])
        failed = set()
        why_failed = {}
        if not ok:
            failed = {n for _, names in bl for n in names}
        else:
            voids = void_functions(f)
            equiv.for_file(f)
            res, _, _ = ck.check(Obj(open(obj, 'rb').read(), obj))
            for name, addr, status, info in res:
                if status == 'DIFF':
                    good, why = equiv.equivalent(info[0], info[1], addr, returns_value=name not in voids)
                    if not good:
                        failed.add(name)
                        why_failed[name] = why
        for s, names in bl:
            if failed & set(names) or (failed and not ok):
                set_guard(f, s, '#ifdef NON_MATCHING')
                print('kept draft  %s: %s' % (os.path.basename(f), ', '.join(names)))
                for n in names:
                    if n in why_failed:
                        print('  %s: %s' % (n, why_failed[n][:200]))
            else:
                print('promoted    %s: %s' % (os.path.basename(f), ', '.join(names)))
        if failed - {n for _, names in bl for n in names}:
            print('  note: %s also not equivalent in the EQUIV build of %s' % (sorted(failed - {n for _, names in bl for n in names}), os.path.basename(f)))


if __name__ == '__main__':
    main()
