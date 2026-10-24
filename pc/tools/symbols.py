"""config/symbols.tsv (addr, name, kind) and config/functions.tsv access."""
import os, fnmatch

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS = os.path.join(REPO, 'config', 'symbols.tsv')
FUNCS = os.path.join(REPO, 'config', 'functions.tsv')
FLAGS = os.path.join(REPO, 'config', 'flags.tsv')


class Table:
    def __init__(self):
        self.by_name = {}
        self.by_addr = {}
        self.rows = []


def load(path=SYMS):
    t = Table()
    if not os.path.exists(path):
        return t
    for l in open(path):
        if not l.strip() or l.startswith('#'):
            continue
        f = l.rstrip('\n').split('\t')
        a, n, k = int(f[0], 16), f[1], f[2] if len(f) > 2 else ''
        t.rows.append((a, n, k))
        t.by_name[n] = (a, k)
        t.by_addr.setdefault(a, n)
    return t


def load_funcs(path=FUNCS):
    """addr -> size for every function the splitter found."""
    out = {}
    for l in open(path):
        if l.startswith('#') or not l.strip():
            continue
        f = l.split('\t')
        out[int(f[0], 16)] = int(f[1], 16)
    return out


def load_func_rows(path=FUNCS):
    rows = []
    for l in open(path):
        if l.startswith('#') or not l.strip():
            continue
        f = l.rstrip('\n').split('\t')
        rows.append((int(f[0], 16), int(f[1], 16), int(f[2])) + tuple(f[3:]))
    return rows


# the other executables decompiled against the same sources: target -> (exe, end of game code)
TARGETS = {
    'powervr': ('POWERVR.EXE', 0x47387c),
    'net': ('NET-WOXL.EXE', 0x4bc5b0),
}
TARGET_DEFINES = {'powervr': '/DPOWERVR', 'net': '/DNETPLAY'}
NET_DEFAULT_FLAGS = '/Od /G5 /QIfdiv'     # what the src/net files and the NET build's plain functions use


def exe_path(target='d3d'):
    return os.path.join(REPO, 'orig', TARGETS[target][0] if target in TARGETS else 'D3D.EXE')


def game_end(target='d3d'):
    if target in TARGETS:
        return TARGETS[target][1]
    from datarefs import GAME_END
    return GAME_END


def target_flags_path(target):
    return os.path.join(REPO, 'config', target, 'flags.tsv')


def flags_for(src, target='d3d'):
    """per-file compiler flags from config/flags.tsv (file name, flags)."""
    if target in TARGETS:
        pv_flags = target_flags_path(target)
        rel = os.path.relpath(src, REPO).replace(os.sep, '/')
        if os.path.exists(pv_flags):
            for l in open(pv_flags):
                if l.startswith('#') or not l.strip():
                    continue
                f = l.rstrip('\n').split('\t')
                if f[0] == rel or (target == 'net' and fnmatch.fnmatchcase(rel, f[0])):
                    return ['/nologo', '/c'] + f[1].split()
        if target == 'net' and rel.startswith('src/net/'):
            return ['/nologo', '/c'] + NET_DEFAULT_FLAGS.split()
    base = os.path.basename(src)
    default = None
    flags_path = FLAGS
    rel = os.path.relpath(os.path.abspath(src), REPO).replace('\\', '/')
    if rel.startswith('src/launcher/'):
        flags_path = os.path.join(REPO, 'config', 'launcher', 'flags.tsv')
    elif rel.startswith('src/dxlink/'):
        flags_path = os.path.join(REPO, 'config', 'dxlink', 'flags.tsv')
    if os.path.exists(flags_path):
        for l in open(flags_path):
            if l.startswith('#') or not l.strip():
                continue
            f = l.rstrip('\n').split('\t')
            if f[0] in (base, '*'):
                default = ['/nologo', '/c'] + f[1].split()
                if f[0] == base:
                    return default
    return default or ['/nologo', '/c', '/O2', '/Oa', '/G5']


def pv_paths(target='powervr'):
    d = os.path.join(REPO, 'config', target)
    return os.path.join(d, 'symbols.tsv'), os.path.join(d, 'functions.tsv'), os.path.join(d, 'addrmap.tsv')


def target_paths(target):
    """Return symbol and function tables for a standalone executable target."""
    if target == 'd3d':
        return SYMS, FUNCS
    d = os.path.join(REPO, 'config', target)
    return os.path.join(d, 'symbols.tsv'), os.path.join(d, 'functions.tsv')


def target_exe(target):
    return os.path.join(REPO, 'orig', {'launcher': 'LAUNCHER.EXE', 'dxlink': 'DXLINK.DLL'}[target])


def load_addrmap(path):
    """D3D address -> PowerVR address (functions, then data)."""
    out = {}
    if not os.path.exists(path):
        return out
    for l in open(path):
        if l.startswith('#') or not l.strip():
            continue
        f = l.split('\t')
        out[int(f[0], 16)] = int(f[1], 16)
    return out
