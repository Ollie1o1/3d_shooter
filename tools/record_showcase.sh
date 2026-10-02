#!/bin/sh
# Re-record docs/overdrive.gif: four scripted shots rendered frame by frame
# (fixed 1/30 s steps, so it doesn't matter how fast the machine is), then
# encoded with gifski (brew install gifski).
#
#   tools/record_showcase.sh [OUT.gif]
#
# Each shot is the game's own footage mode (see main.cpp): --record FRAMES
# SKIP PREFIX saves frames after a warm-up, --cam/--campath glide the camera,
# --spawn puts enemies in front of it, --autoaim turns onto the nearest one and
# fires, --kite backs off from whatever gets close, --clean hides title cards.
set -e
cd "$(dirname "$0")/.."
OUT="${1:-docs/overdrive.gif}"
TMP="$(mktemp -d)"
make -s

# 1. Sunset Yard: Rippers and Husks charge into the shotgun
./shooter --arena 1 --god --cam 0 1.7 14 -90 -2 --campath -3 1.7 11 -95 -2 \
  --spawn 1 --spawn 1 --spawn 0 --spawn 1 --spawn 0 --autoaim --clean --weapon 2 --res 720 --record 85 30 "$TMP/a" >/dev/null 2>&1
# 2. The Core at night: a Shieldbearer, a Juggernaut and a Brute
./shooter --arena 4 --god --cam 0 1.7 -208 -90 -2 --campath 4 1.7 -211 -95 -2 \
  --spawn 9 --spawn 6 --spawn 4 --autoaim --clean --weapon 1 --res 720 --record 85 35 "$TMP/b" >/dev/null 2>&1
# 3. Into the Sanctum under the eclipse
./shooter --arena 5 --god --cam 0 2.4 -293 -90 7 --campath 0 5 -314 -90 2 \
  --clean --weapon 1 --res 720 --record 55 5 "$TMP/c" >/dev/null 2>&1
# 4. The Sovereign
./shooter --arena 5 --god --cam 0 1.8 -310 -90 2 --spawn 8 \
  --autoaim --kite --clean --weapon 1 --res 720 --record 180 30 "$TMP/d" >/dev/null 2>&1

# Every other frame (15 fps), scaled to 720 wide, then one GIF
n=0
for f in $(ls "$TMP"/a_*.bmp "$TMP"/b_*.bmp "$TMP"/c_*.bmp "$TMP"/d_*.bmp); do
  n=$((n + 1))
  [ $((n % 2)) -eq 0 ] && sips -s format png -Z 720 "$f" --out "$TMP/f_$(printf %04d $n).png" >/dev/null 2>&1
done
gifski --fps 15 --width 640 --quality 65 --lossy-quality 60 -o "$OUT" "$TMP"/f_*.png >/dev/null 2>&1
rm -rf "$TMP"
echo "wrote $OUT ($(du -h "$OUT" | cut -f1))"
