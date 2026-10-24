"""For each game function: .data addresses it references (via .reloc), split
into string literals and other data. Used to group functions into files:
each object file's literals are contiguous in .data, in link order."""
import sys, os, bisect, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE
import symbols as S

GAME_END = 0x473eb0


def is_string(pe, a):
    b = pe.read(a, 64)
    if a % 4 or b[0] == 0:
        return False
    n = b.find(b'\0')
    if n < 2 and n != -1:
        return False
    s = b[:n if n >= 0 else 64]
    return all(32 <= c < 127 or c in (9, 10, 13) for c in s)


def refs():
    pe = PE(os.path.join(S.REPO, 'orig', 'D3D.EXE'))
    data = pe.section('.data')
    rows = [r for r in S.load_func_rows() if r[0] < GAME_END]
    starts = [r[0] for r in rows]
    out = {r[0]: ([], []) for r in rows}
    for site in pe.relocs:
        if site >= GAME_END:
            continue
        i = bisect.bisect_right(starts, site) - 1
        if i < 0 or site >= rows[i][0] + rows[i][1]:
            continue
        t = pe.u32(site)
        if t in data:
            out[rows[i][0]][0 if is_string(pe, t) else 1].append(t)
    return pe, rows, out


if __name__ == '__main__':
    pe, rows, out = refs()
    syms = S.load()
    for a, size, tu in rows:
        st, ot = out[a]
        print('%06x %5d %-28s %s' % (a, size, syms.by_addr.get(a, ''),
              ' '.join('%06x' % x for x in sorted(set(st)))[:90]))
