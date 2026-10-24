# Wipeout XL (PC) decompilation

C source for the Windows version of Wipeout XL (Psygnosis, May 1997). Each function is
checked against the original executable: it either compiles with Microsoft Visual C++ 4.20
to exactly the original bytes, or is proven to compute the same thing. Functions that don't
match yet are kept as drafts under `#ifdef NON_MATCHING`.

| Executable | What |
|---|---|
| `D3D.EXE` | the Direct3D game (`src/*.c`) |
| `POWERVR.EXE` | the PowerVR build (shared sources plus `src/powervr/`) |
| `NET-WOXL.EXE` | the network build (shared sources plus `src/net/`) |
| `LAUNCHER.EXE`, `DXLINK.DLL` | the launcher (`src/launcher/`, `src/dxlink/`) |

Unlike the PlayStation build, this is not linked into a new executable yet; the checker
compiles every file and compares it function by function.

## Requirements

* Docker. Build the compiler image once: `docker build -t wipeoutxl-pc .`
* MSVC 4.20 in `tools/msvc420/` (with `bin/` and `include/`), not included.
* Python 3 with `capstone`: `pip install capstone`.
* Your own copies of the executables above in `orig/`.

## Checking

    python3 tools/progress.py                  # every file against D3D.EXE: must print OK
    python3 tools/progress.py --target net     # or powervr, launcher, dxlink
    python3 tools/check.py -v src/hud.c        # one file, with a diff of each function
    python3 tools/unmatched.py                 # what is left
    python3 tools/disasm.py NAME               # disassembly of one original function

`config/` holds the function list, the names and the compiler flags of each file.
