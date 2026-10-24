#!/bin/sh
# Build inside the Docker toolchain image (see ../Dockerfile), then verify against the retail EXEs.
set -e
cd "$(dirname "$0")"
docker run --rm -v "$PWD/../..:/src" -w "/src/$(basename "$(dirname "$PWD")")/xtro" ${WIPEOUT_IMAGE:-wipeoutxl} make -j${JOBS:-8} "$@"
if [ "$#" -eq 0 ]; then
    shasum -c ../config/checksum_xtro.sha1
fi
