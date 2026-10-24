#!/usr/bin/env python3
"""Turn the stub sources splat writes for the European split (asm/eu/stubs/NAME.c, one
INCLUDE_ASM per function) into plain assembly files asm/eu/text/NAME.s.

The EU build links either src/NAME.c (compiled with -DVERSION_EU) or this all-asm
replacement for each source file.
usage: eu_stubs.py
"""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / 'asm/eu/text'
OUT.mkdir(parents=True, exist_ok=True)
ASM = re.compile(r'^INCLUDE_ASM\("([^"]+)", (\w+)\);$')
ROD = re.compile(r'^INCLUDE_RODATA\("([^"]+)", (\w+)\);$')
EMPTY = re.compile(r'^void (\w+)\(void\) \{$')

for stub in sorted((ROOT / 'asm/eu/stubs').glob('*.c')):
    name = stub.stem
    lines = ['.include "include/labels.inc"', '']
    for line in stub.read_text().splitlines():
        m = ASM.match(line) or EMPTY.match(line)
        if m:
            path = f'asm/eu/nonmatchings/{name}/{m.group(m.lastindex)}.s'
            lines += ['.section .text', '.set noat', '.set noreorder', f'.include "{path}"',
                      '.set reorder', '.set at', '']
            continue
        m = ROD.match(line)
        if m:
            lines += ['.section .rodata', f'.include "{m.group(1)}/{m.group(2)}.s"', '.section .text', '']
    (OUT / f'{name}.s').write_text('\n'.join(lines))
