#!/bin/sh
# Run a PsyQ DOS CC1PSX inside DOSBox (in the toolchain image, see Dockerfile).
# usage: doscc1.sh <psyq-version> <in.i> <out.s> [cc1 flags...]
set -e
VER=$1; IN=$2; OUT=$3; shift 3
# DOS can't redirect stderr, so cc1 errors are invisible under DOSBox. Catch them
# first with the native gcc 2.7.2 cc1 (same C front end).
if ! "$PWD/tools/gcc/2.7.2/cc1" -quiet "$@" "$IN" -o /dev/null; then
  echo "doscc1.sh: compile errors in $IN" >&2; exit 1
fi
W=$(mktemp -d)
cp "$PWD/tools/psyq/$VER/CC1PSX.EXE" "$W/"
cp "$IN" "$W/IN.I"
cat > "$W/RUN.BAT" <<EOB
CC1PSX.EXE $* IN.I -o OUT.S > LOG.TXT
EXIT
EOB
printf '[cpu]\ncore=normal\ncycles=max\n[sdl]\nnosound=true\n[mixer]\nnosound=true\n' > "$W/dosbox.conf"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 1800 dosbox -conf "$W/dosbox.conf" -c "mount c $W" -c "c:" -c "RUN.BAT" >/dev/null 2>&1 || true
[ -f "$W/OUT.S" ] || { cat "$W/LOG.TXT" 2>/dev/null; echo "no output" >&2; exit 1; }
# DOSBox doesn't propagate cc1's exit status, and cc1 leaves partial output on errors.
if grep -v -i 'warning' "$W/LOG.TXT" 2>/dev/null | grep -q -E ':[0-9]+: |error'; then
  tr -d '\r' < "$W/LOG.TXT" >&2; exit 1
fi
# DOSBox killed by the timeout (heavy load) leaves a truncated OUT.S: every
# .ent must have its .end
if [ "$(grep -c '^[[:space:]]*\.ent[[:space:]]' "$W/OUT.S")" != "$(grep -c '^[[:space:]]*\.end[[:space:]]' "$W/OUT.S")" ]; then
  echo "doscc1.sh: truncated output for $IN" >&2; exit 1
fi
cp "$W/OUT.S" "$OUT"; rm -rf "$W"
