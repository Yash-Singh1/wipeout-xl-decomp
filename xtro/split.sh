#!/bin/sh
# Regenerate asm/, the linker script and the auto symbol files from the retail XTRO1.EXE.
set -e
cd "$(dirname "$0")"
rm -rf asm build XTRO1.EXE.ld undefined_funcs_auto.txt undefined_syms_auto.txt
python3 -m splat split xtro1.yaml
python3 tools/fix_asm.py --yaml xtro1.yaml
# GNU ld rounds the NOLOAD bss section up to the input's original alignment;
# the retail EXE starts .bss immediately after .data at 0x80138ABC.
sed -i.bak 's/^    \.main_bss (NOLOAD)/    .main_bss 0x80138ABC (NOLOAD)/' XTRO1.EXE.ld && rm XTRO1.EXE.ld.bak
