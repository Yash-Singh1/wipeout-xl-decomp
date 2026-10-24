# Intro movie players (XTRO1-4.EXE, NTSCANIM.EXE, PALANIM.EXE)

The small executables on the disc that play the `.AV` intro movies and return to the game.
They are one program built six times; the builds differ only in the movie file name, its
frame count and, for the European `PALANIM.EXE`, the video standard (`VERSION_EU` in
`include/avplay.h`).

| EXE | movie | frames |
|---|---|---|
| XTRO1.EXE | `\xtro1.AV` | 630 |
| XTRO2.EXE | `\xtro2.AV` | 600 |
| XTRO3.EXE | `\xtro3.AV` | 780 |
| XTRO4.EXE | `\xtro4.AV` | 960 |
| NTSCANIM.EXE | `\make.AV` | 2044 |
| PALANIM.EXE (Europe) | `\make.AV` | 1703 |

* `src/avplay.c`: the movie player.
* `src/malloc.c`: the game's heap allocator.
* `src/iso9660.c`: the CD file system reader.

The startup code and the PsyQ libraries come from the retail `XTRO1.EXE` as assembly.

    ./split.sh     # asm/ and the linker script from ../orig/XTRO1.EXE
    ./build.sh     # build all six and check them against ../config/checksum_xtro.sha1
