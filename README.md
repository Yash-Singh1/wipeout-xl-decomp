# Wipeout XL / Wipeout 2097 (PlayStation) decompilation

C source that rebuilds the PlayStation executables of Wipeout XL (USA) and Wipeout 2097
(Europe) byte for byte, with the original compiler. This repository also contains a partial
decompilation of the original PC release. If you're looking for the rewrite you will find it
in the rewrite repo: https://github.com/Yash-Singh1/wipeout-xl-rewrite.

| Executable | Release | SHA-1 |
|---|---|---|
| `SCUS_943.51` | Wipeout XL (USA) | `4c620967bce0cf543fb1fef4d34b20389c4a028c` |
| `SLES_003.27` | Wipeout 2097 (Europe) | see `config/checksum_eu.sha1` |
| `XTRO1-4.EXE`, `NTSCANIM.EXE`, `PALANIM.EXE` | intro movie players | see `config/checksum_xtro.sha1` |

Every game function is compiled from C; only the PsyQ SDK libraries are taken from the
retail executable as assembly. Both releases build from the same sources: the European
build defines `VERSION_EU`, and the differences (50 Hz timing, the 256-line screen, the
memory card format) are expressed with the macros in `include/version.h`.

## Requirements

* Docker. Build the toolchain image once: `docker build -t wipeoutxl .`
* Python 3 with splat: `pip install splat64==0.50.0`.
* Your own copies of the retail executables in `orig/`: `SCUS_943.51`, `SLES_003.27` and,
  for the intro players, `XTRO1.EXE`.
* The compilers in `tools/` (not included):
  * `tools/psyq/3.6/CC1PSX.EXE`: the PsyQ 3.6 C compiler the game was built with. It
    runs under DOSBox inside the image.
  * `tools/gcc/2.7.2/` (`cpp`, `cc1`): static i386 GCC 2.7.2 from decompals/old-gcc.
  * `tools/maspsx/`: [maspsx](https://github.com/mkst/maspsx).

## Building

    ./split.sh                 # asm/ and the linker script from orig/SCUS_943.51
    ./build.sh                 # build/SCUS_943.51, prints "OK" when it matches

    ./split_eu.sh              # the same for orig/SLES_003.27
    ./build.sh eu              # build/eu/SLES_003.27

    (cd xtro && ./split.sh && ./build.sh)   # the six intro players

`split.sh` also writes `.vscode/compile_commands.json` (`tools/compile_commands.py`), so the
editor parses each file with the include paths and defines of its build.

## Layout

| Path | What |
|---|---|
| `src/` | the game, one file per original source file |
| `include/` | headers; `version.h` holds the US/European differences |
| `xtro/` | the intro movie player, built six times with different movie settings |
| `pc/` | the Windows version (in progress), see [pc/README.md](pc/README.md) |
| `*.yaml` | splat configuration |
| `config/` | symbol names (`symbol_addrs*.txt`), expected SHA-1s, extra linker symbols |
| `tools/` | build helpers |
