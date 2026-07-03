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
# delta.lst MUST track the repo's (round-7i-b: a stale deployed delta silently TRUNCATES
# prediction fields — m_iWeaponState 2-bit dropped the scope attach bit on the wire)
cp -f "$MODTS/network/delta.lst" "$TS/delta.lst"
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

# --- client runtime dep: vgui.so ---------------------------------------------
# The cont.46 client links DT_NEEDED `vgui.so` (the 64-bit openvgui). The engine
# ships the exact same lib as `libvgui.so`; expose it under the name the client
# wants so the dynamic loader resolves it (run with LD_LIBRARY_PATH=$XROOT).
# (libSDL2-2.0.so.0 already resolves from the system loader path.)
if [ -f "$XROOT/libvgui.so" ]; then
	ln -sfn libvgui.so "$XROOT/vgui.so"
else
	cp -f "$MODTS/linux/release/vgui64.so" "$XROOT/vgui.so"
fi

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
echo "    cd $XROOT && LD_LIBRARY_PATH=. ./xash3d -dev 2 -game ts +map tm_avana"
echo
echo "tm_avana is a team map (the round timer arms + sends RoundTime → the new HUD)."
echo "Add bots/players or run +sv_lan 1 and connect a second client to see the round run."
