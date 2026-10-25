#!/bin/sh
# Regenerate asm/, the linker script and the auto symbol files from the retail EXE.
set -e
cd "$(dirname "$0")"
# keep the European and Japanese splits (asm/eu, asm/jp): they have their own scripts, split_eu.sh and split_jp.sh
for d in asm build; do
    [ -d $d ] && find $d -mindepth 1 -maxdepth 1 ! -name eu ! -name jp -exec rm -rf {} +
done
rm -f SCUS_943.51.ld undefined_funcs_auto.txt undefined_syms_auto.txt
python3 -m splat split scus_943.51.yaml
python3 tools/fix_asm.py
# GNU ld rounds the NOLOAD bss section up to the input's original alignment;
# the retail EXE starts .bss immediately after .data at 0x8009495C.
sed -i.bak 's/^    \.main_bss (NOLOAD)/    .main_bss 0x8009495C (NOLOAD)/' SCUS_943.51.ld && rm SCUS_943.51.ld.bak
# editor setup for this checkout
python3 tools/compile_commands.py
