# Toolchain image for ./build.sh and the verification tools:
#   docker build -t wipeoutxl .
# DOSBox runs the PsyQ CC1PSX.EXE compiler; the static i386 gcc 2.7.2 cpp/cc1 helpers in
# tools/gcc run natively on x86 hosts and through the host's i386 binfmt support elsewhere
# (Docker Desktop provides it).
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends binutils-mips-linux-gnu make python3 \
        python3-pycparser python3-toml cpp dosbox && rm -rf /var/lib/apt/lists/*
WORKDIR /work
