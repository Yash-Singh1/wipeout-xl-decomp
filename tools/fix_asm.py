"""Post-split fixups for GNU as.

Some original encodings can't be reproduced by GNU as, so emit them as raw
words using the little-endian bytes from spimdisasm's line comment:
  * PsyQ's divide-overflow check uses a trap that disassembles as the MIPS II
    `tge` instruction, which GNU as rejects for -march=r3000.
  * GTE (COP2) commands: splat's gte_macros.inc canonicalises the unused bits,
    while PsyQ's ASPSX encodes them differently.
"""
import pathlib
import re
import sys

# usage: fix_asm.py [--yaml CONFIG] [file.s ...]   (default config: scus_943.51.yaml)
args = sys.argv[1:]
yaml_path = "scus_943.51.yaml"
if args[:1] == ["--yaml"]:
    yaml_path, args = args[1], args[2:]
yaml_text = pathlib.Path(yaml_path).read_text()

# vram start of each C file's carved-out .rodata (from the splat yaml's main code segment)
_main = re.search(r"- name: main\n\s+type: code\n\s+start: (0x[0-9A-F]+)\n\s+vram: (0x[0-9A-F]+)", yaml_text)
MAIN_START, MAIN_VRAM = int(_main.group(1), 16), int(_main.group(2), 16)
CARVE = {
    m.group(2): int(m.group(1), 16) - MAIN_START + MAIN_VRAM
    for m in re.finditer(r"- \[(0x[0-9A-F]+), \.rodata, (\w+)\]", yaml_text)
}

LINE = re.compile(r"^(\s*/\* \w+ \w+ )([0-9A-F]{8})( \*/\s+)(\S+).*$")


def needs_raw(word, mnemonic):
    is_cop2_command = (word >> 26) == 0x12 and (word >> 25) & 1
    return mnemonic == "tge" or is_cop2_command


# the default is the US split; the European one (asm/eu) is fixed up by split_eu.sh
default = [str(p) for p in pathlib.Path("asm").rglob("*.s") if p.parts[:2] != ("asm", "eu")]
for path in map(pathlib.Path, args or default):
    text = path.read_text()
    out = []
    for line in text.splitlines(keepends=True):
        m = LINE.match(line)
        if m and m.group(4) != ".word":
            word = int.from_bytes(bytes.fromhex(m.group(2)), "little")
            if needs_raw(word, m.group(4)):
                line = f"{m.group(1)}{m.group(2)}{m.group(3)}.word 0x{word:08X} /* {m.group(4)} */\n"
        out.append(line)
    new = "".join(out)
    # Jump tables are `.align 3` relative to their C file's .rodata start (PsyQ
    # semantics; the link uses SUBALIGN(4)). splat picks the alignment from the
    # absolute address, so recompute it from the offset within the file's carve-out.
    start = CARVE.get(path.parent.name)
    def realign(m):
        rel = int(m.group(2), 16) - start if start is not None else 4
        return f"{m.group(1)}.align {3 if rel % 8 == 0 else 2}\nnonmatching jtbl_{m.group(2)}"
    new = re.sub(r"(^|\n)\.align [23]\nnonmatching jtbl_([0-9A-F]{8})", realign, new)
    if new != text:
        path.write_text(new)
