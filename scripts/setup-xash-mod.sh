#!/usr/bin/env bash
#
# setup-xash-mod.sh — assemble a native xash3d-fwgs game root that runs mod-ts
# (The Specialists) with OUR reconstructed client.so + hl.so, so the mod can be
# launched in the graphical engine for visual testing.
#
# It builds a `ts/` game dir (and completes the `valve/` fallback) inside the
# native xash3d-fwgs runtime root (dist/client-x64), wiring:
#   ts/dlls/hl.so          ← our amd64 server build
#   ts/cl_dlls/client.so   ← our amd64 client build  (+ client_amd64.so alias)
#   ts/<content>           ← symlinks to the original TS content
#   ts/liblist.gam         ← direct gamedll (no metamod), so it's a plain
#                            listen-server the client renders
#
# Re-run after rebuilding (scripts/build.sh amd64 all) to refresh the libs.
#
set -euo pipefail

MODTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
XASH="${XASH:-$HOME/Projects/goldsrc.net/xash3d-fwgs}"
XROOT="${XROOT:-$XASH/dist/client-x64}"          # native engine runtime root
TS_SRC="${TS_SRC:-$HOME/Containers/hlds/mods/ts}" # original TS content
VALVE_SRC="${VALVE_SRC:-$XASH/rodir/valve}"       # HL fallback content

HL_SO="$MODTS/build-amd64/dlls/hl.so"
CLIENT_SO="$MODTS/build-amd64/cl_dll/client.so"

echo ">>> mod-ts        : $MODTS"
echo ">>> xash root     : $XROOT"
echo ">>> TS content    : $TS_SRC"
echo ">>> valve content : $VALVE_SRC"

for f in "$HL_SO" "$CLIENT_SO"; do
	[ -f "$f" ] || { echo "!! missing $f — run scripts/build.sh amd64 all first"; exit 1; }
done
[ -x "$XROOT/xash3d" ] || { echo "!! no engine at $XROOT/xash3d"; exit 1; }
[ -d "$TS_SRC" ]   || { echo "!! no TS content at $TS_SRC"; exit 1; }
[ -d "$VALVE_SRC" ] || { echo "!! no valve content at $VALVE_SRC"; exit 1; }

# --- ts game dir --------------------------------------------------------------
TS="$XROOT/ts"
rm -rf "$TS"
mkdir -p "$TS/dlls" "$TS/cl_dlls"

# symlink every content entry except the ones we provide / don't want
for entry in "$TS_SRC"/*; do
	name="$(basename "$entry")"
	case "$name" in
		dlls|cl_dlls|liblist.gam|addons|addons2) continue ;;
	esac
	ln -sfn "$entry" "$TS/$name"
done

# our reconstructed libs (real copies so they can be refreshed independently).
# xash3d-fwgs on x86-64 looks up the ARCH-SUFFIXED name (hl_amd64.so /
# client_amd64.so) — without it, it silently falls back to valve/dlls/hl_amd64.so.
# Provide the suffixed name as primary + the bare name as an alias (GoldSrc compat).
cp -f "$HL_SO" "$TS/dlls/hl_amd64.so"
# delta.lst tracks the repo's for the xash client (our-client wire format). Use
# --remove-destination so we replace the symlink with a REAL file in the xash root —
# a plain `cp -f` would follow the ts/delta.lst symlink and write THROUGH into the shared
# original ~/Containers/hlds/mods/ts (clobbering the pristine original TS delta.lst).
cp -f --remove-destination "$MODTS/network/delta.lst" "$TS/delta.lst"
# NB: no generated sprites are deployed — the composed scope mask (ts_scopefull.spr) was an
# invented asset and has been removed; the scope now paints the ORIGINAL quarterscope.spr 4x
# rotated at render time (cl_dll/ts_scope.cpp), and quarterscope.spr resolves via the ts/sprites
# symlink into the original content. (Copying into ts/sprites would also write through that symlink.)
ln -sfn hl_amd64.so "$TS/dlls/hl.so"
cp -f "$CLIENT_SO" "$TS/cl_dlls/client_amd64.so"
ln -sfn client_amd64.so "$TS/cl_dlls/client.so"

cat > "$TS/liblist.gam" <<'EOF'
game "mod-ts (faithful reconstruction)"
gamedir "ts"
fallback_dir "valve"
type "multiplayer_only"
mpentity "info_player_deathmatch"
nomodels "0"
gamedll_linux "dlls/hl.so"
EOF

# --- client vgui: nothing to stage -------------------------------------------
# The modernize-build-sdl2-vgui client STATIC-links its own VGUI (openvgui,
# hidden visibility) and DT_NEEDEDs no external vgui.so — verified: client.so
# needs only libc/libm/libdl/libgcc. The engine also no longer ships an amd64
# libvgui.so (openvgui is wasm-only now), so VGui_LoadProgs(NULL) fails and the
# engine falls back to the client's InitVGUISupportAPI. Nothing to provision.

# --- complete the valve fallback ---------------------------------------------
mkdir -p "$XROOT/valve"
for entry in "$VALVE_SRC"/*; do
	name="$(basename "$entry")"
	[ -e "$XROOT/valve/$name" ] && continue   # keep anything already there (e.g. extras.pk3)
	ln -sfn "$entry" "$XROOT/valve/$name"
done

echo ">>> done."
echo
echo "Launch (graphical, OpenGL renderer) — LD_LIBRARY_PATH lets the client find vgui.so:"
echo "    cd $XROOT && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map ts_hammertime"
echo
echo "Real maps live in $XROOT/ts/maps (ts_* + ctb_deadheat). ctb_deadheat is a team/CTB map"
echo "(the round timer arms + sends RoundTime → the HUD). Add bots/players or run +sv_lan 1 and"
echo "connect a second client to see the round run."
