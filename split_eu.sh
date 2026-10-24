#!/bin/sh
# Regenerate the European split (asm/eu, SLES_003.27.ld, undefined_*_eu.txt) from the retail
# SLES_003.27.  The US split (split.sh) is not touched.
set -e
cd "$(dirname "$0")"
rm -rf asm/eu build/eu SLES_003.27.ld undefined_funcs_auto_eu.txt undefined_syms_auto_eu.txt
python3 -m splat split sles_003.27.yaml
python3 tools/fix_asm.py --yaml sles_003.27.yaml $(find asm/eu -name '*.s')
# every source file is linked from src/NAME.c or from its all-asm stand-in (see Makefile)
python3 tools/eu_stubs.py
sed -i.bak 's|build/eu/asm/eu/stubs/|build/eu/src/|' SLES_003.27.ld
# GNU ld rounds the NOLOAD bss section up to the input's original alignment;
# the retail EXE starts .bss immediately after .data at 0x80095488.
sed -i.bak 's/^    \.main_bss (NOLOAD)/    .main_bss 0x80095488 (NOLOAD)/' SLES_003.27.ld && rm SLES_003.27.ld.bak
