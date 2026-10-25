#!/bin/sh
# Build inside the Docker toolchain image, then verify against the retail EXE.
#   ./build.sh [make args]       Wipeout XL (USA)     -> build/SCUS_943.51
#   ./build.sh eu [make args]    Wipeout 2097 (EU)    -> build/eu/SLES_003.27
#   ./build.sh jp [make args]    Wipeout XL (Japan)   -> build/jp/SIPS_600.10
set -e
cd "$(dirname "$0")"
DIR=$(basename "$PWD")
if [ "$1" = eu ] || [ "$1" = jp ]; then
    R=$1; shift
    [ $R = eu ] && EXE=SLES_003.27 || EXE=SIPS_600.10
    docker run --rm -v "$PWD/..:/src" -w "/src/$DIR" ${WIPEOUT_IMAGE:-wipeoutxl} make -j${JOBS:-8} VERSION=$R "$@"
    [ "$#" -eq 0 ] && shasum -c config/checksum_$R.sha1
    # stable symbol snapshot for the per-function checker (the ELF is rewritten mid-build)
    if [ "$#" -eq 0 ]; then
        docker run --rm -v "$PWD/..:/src" -w "/src/$DIR" ${WIPEOUT_IMAGE:-wipeoutxl} \
            sh -c "mips-linux-gnu-nm build/$R/$EXE.elf > syms_$R.nm.tmp && mv syms_$R.nm.tmp syms_$R.nm"
    fi
    exit 0
fi
docker run --rm -v "$PWD/..:/src" -w "/src/$DIR" ${WIPEOUT_IMAGE:-wipeoutxl} make -j${JOBS:-8} "$@"
[ "$#" -eq 0 ] && shasum -c config/checksum.sha1
# stable symbol snapshot for tools/check.py (the ELF itself is rewritten mid-build)
if [ "$#" -eq 0 ]; then
    docker run --rm -v "$PWD/..:/src" -w "/src/$DIR" ${WIPEOUT_IMAGE:-wipeoutxl} \
        sh -c 'mips-linux-gnu-nm build/SCUS_943.51.elf > syms.nm.tmp && mv syms.nm.tmp syms.nm'
fi
