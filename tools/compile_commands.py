#!/usr/bin/env python3
"""Write .vscode/compile_commands.json so editors (VS Code's C/C++ extension, clangd) parse
every file with the include paths and defines of the build it belongs to: the PS1 game,
the intro players (xtro/) or the PC version (pc/). Paths are absolute, so the file is
generated per checkout (split.sh runs this) and not kept in git.
usage: compile_commands.py
"""
import json, os, pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
PS1 = ['-std=gnu89', '-nostdinc', '-Dpsx', '-D__psx__', '-Dmips', '-D__mips__',
       '-DLANGUAGE_C', '-D_LANGUAGE_C', '-D_MIPSEL', '-DVERSION_US']


def gcc(path, includes, defines=()):
    return ['gcc', '-x', 'c'] + ['-I' + str(ROOT / i) for i in includes] + PS1 + list(defines) + ['-c', str(path)]


def cl(path):
    rel = path.relative_to(ROOT / 'pc')
    args = ['cl.exe', '/TC', '/I' + str(ROOT / 'pc/include'), '/I' + str(path.parent),
            '/I' + str(ROOT / 'pc/tools/msvc420/include'), '/D_WIN32', '/D_M_IX86=500', '/D_X86_']
    if rel.parts[:2] == ('src', 'net'):
        args.append('/DNETPLAY')
    if rel.parts[:2] == ('src', 'powervr'):
        args.append('/DPOWERVR')
    return args + ['/c', str(path)]


entries = []
for path in sorted(ROOT.rglob('*.[ch]')):
    rel = path.relative_to(ROOT)
    top = rel.parts[0]
    if top == 'pc' and rel.parts[1] in ('src', 'include'):
        args = cl(path)
    elif top == 'xtro' and rel.parts[1] in ('src', 'include'):
        args = gcc(path, ['xtro/include'], ['-DMOVIE_NAME=xtro1', '-DMOVIE_FRAMES=630'])
    elif top in ('src', 'include'):
        args = gcc(path, ['include'])
    else:
        continue
    entries.append({'directory': str(ROOT), 'file': str(path), 'arguments': args})

out = ROOT / '.vscode' / 'compile_commands.json'
out.parent.mkdir(exist_ok=True)
out.write_text(json.dumps(entries, indent=1) + '\n')
print('%s: %d files' % (out.relative_to(ROOT), len(entries)))
