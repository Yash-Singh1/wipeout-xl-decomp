#!/bin/sh
# Regenerate the Japanese split (asm/jp, SIPS_600.10.ld, undefined_*_jp.txt) from the retail
# SIPS_600.10.  The US and EU splits are not touched.
set -e
cd "$(dirname "$0")"
rm -rf asm/jp build/jp SIPS_600.10.ld undefined_funcs_auto_jp.txt undefined_syms_auto_jp.txt
python3 -m splat split sips_600.10.yaml
python3 tools/fix_asm.py --yaml sips_600.10.yaml $(find asm/jp -name '*.s')
# every source file is linked from src/NAME.c or from its all-asm stand-in (see Makefile)
python3 tools/eu_stubs.py jp
sed -i.bak 's|build/jp/asm/jp/stubs/|build/jp/src/|' SIPS_600.10.ld
# GNU ld rounds the NOLOAD bss section up to the input's original alignment;
# the retail EXE starts .bss immediately after .data at 0x800945B4.
sed -i.bak 's/^    \.main_bss (NOLOAD)/    .main_bss 0x800945B4 (NOLOAD)/' SIPS_600.10.ld && rm SIPS_600.10.ld.bak
