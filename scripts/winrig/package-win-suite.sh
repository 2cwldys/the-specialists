#!/usr/bin/env bash
#
# package-win-suite.sh <amxmodx checkout> <reTS zip> [out dir] — the Windows (Win32) server suite for a
# TS 3.0 install that runs Metamod and AMX Mod X: reTS, Metamod-R and AMX Mod X with reTSX, laid out to
# extract over Half-Life/ts. Output: <out dir>/reTS-suite-<reTS rev>-win32.zip (default dist/).
#
#   <amxmodx checkout>  the fork's tsx-ts3 branch: reTSX is built from it (build-tsx-win.sh) and its
#                       gamedata/ goes over the drop-in's (reTS's Ham slots, the stock-TS gates)
#   <reTS zip>          a reTS package (scripts/package.sh); its Windows files and data are used
#   DROPIN              the buildchain's hlds-dropin-windows-i386.zip (default: release v1.0.9, fetched
#                       with gh into $HLDS_WIN/addons-src/zips). Metamod-R and every AMX Mod X file
#                       other than the TS module come from it, as CI built them with MSVC.
#
# A server's own configuration is left alone: AMX Mod X's stock configs go to
# addons/amxmodx/configs.default/, and Metamod's plugins.ini/config.ini are not shipped. Metamod-R goes
# where TS 3.0-era installs point liblist.gam: addons/metamod/dlls/metamod.dll.
set -euo pipefail
MODTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
AMXX="$(realpath "${1:?usage: package-win-suite.sh <amxmodx checkout> <reTS zip> [out dir]}")"
RETS_ZIP="$(realpath "${2:?reTS zip}")"
OUT="$(realpath -m "${3:-$MODTS/dist}")"
HLDS_WIN="${HLDS_WIN:-/media/Data3/Projects/goldsrc.net/hlds-win}"
DROPIN="${DROPIN:-$HLDS_WIN/addons-src/zips/hlds-dropin-windows-i386-v1.0.9.zip}"
if [ ! -f "$DROPIN" ]; then
  mkdir -p "$(dirname "$DROPIN")"
  gh release download v1.0.9 -R goldsrc-net/buildchain -p hlds-dropin-windows-i386.zip -O "$DROPIN"
fi

REV="$(basename "$RETS_ZIP" .zip)"; REV="${REV#reTS-}"
NAME="reTS-suite-$REV-win32"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT
PKG="$WORK/$NAME"; T="$PKG/ts"
mkdir -p "$T"

# reTS: the Windows libraries and the data files of the patch (no Linux libraries)
unzip -q "$RETS_ZIP" -d "$WORK/rets"
R="$(dirname "$(find "$WORK/rets" -path '*/ts/dlls/mp.dll' | head -1)")/.."
mkdir -p "$T/dlls" "$T/cl_dlls" "$T/resource"
cp "$R/dlls/mp.dll" "$T/dlls/"
cp "$R/cl_dlls/client.dll" "$T/cl_dlls/"
cp "$R/delta.lst" "$T/"
cp "$R/resource/trackerscheme.res" "$T/resource/"

# Metamod-R and AMX Mod X from the drop-in: the valve/ base, then its ts/ additions
unzip -q "$DROPIN" 'valve/addons/*' 'ts/addons/*' -d "$WORK/dropin"
mkdir -p "$T/addons/metamod/dlls"
cp "$WORK/dropin/valve/addons/metamod/dlls/metamod_i386.dll" "$T/addons/metamod/dlls/metamod.dll"
cp -r "$WORK/dropin/valve/addons/amxmodx" "$T/addons/"
cp -r "$WORK/dropin/ts/addons/amxmodx/." "$T/addons/amxmodx/"
mv "$T/addons/amxmodx/configs" "$T/addons/amxmodx/configs.default"
rm -rf "$T/addons/amxmodx/logs"

# reTSX and the reTS gamedata from the checkout
"$MODTS/scripts/winrig/build-tsx-win.sh" "$AMXX" "$WORK/tsx" > /dev/null
cp "$WORK/tsx/tsx_amxx.dll" "$T/addons/amxmodx/modules/tsx_amxx.dll"
cp -r "$AMXX/gamedata/." "$T/addons/amxmodx/data/gamedata/"
grep -q '"windows"' "$T/addons/amxmodx/data/gamedata/common.games/virtual.games/ts/offsets-rets.txt" \
  || { echo "offsets-rets.txt has no windows slots" >&2; exit 2; }

AMXX_REV="$(git -C "$AMXX" rev-parse --short HEAD)"
cat > "$PKG/README.txt" <<EOF
reTS $REV with Metamod-R and AMX Mod X (reTSX), for a Windows TS 3.0 server

Copy the ts folder over Half-Life/ts. Set aside the files it replaces first if you want to go back.

  ts/dlls/mp.dll, ts/cl_dlls/client.dll, ts/delta.lst, ts/resource/trackerscheme.res   reTS $REV
  ts/addons/metamod/dlls/metamod.dll   Metamod-R (buildchain drop-in, $(basename "$DROPIN"))
  ts/addons/amxmodx/                   AMX Mod X 1.10 (same drop-in), with reTSX's tsx_amxx.dll and the
                                       reTS gamedata from the AMX Mod X fork's tsx-ts3 ($AMXX_REV)

Your configuration is kept: addons/amxmodx/configs is not in this zip (AMX Mod X 1.10's stock files are
in configs.default to compare), nor are addons/metamod/plugins.ini and config.ini. Plugins compiled
for AMX Mod X 1.8.2 load in 1.10; addons/amxmodx/scripting has the 1.10 compiler and includes.

Why the whole set: stock AMX Mod X's TS module and Ham Sandwich use TS 3.0's memory offsets and
function slots, which reTS does not have. reTSX asks reTS for player data instead, and the reTS
gamedata gives Ham Sandwich reTS's own slots.

MANIFEST.tsv lists every file with its sha256.
EOF
( cd "$PKG" && find ts -type f | sort | while read -r f; do printf '%s\t%s\n' "$f" "$(sha256sum "$f" | cut -c1-64)"; done ) \
  > "$PKG/MANIFEST.tsv"
mkdir -p "$OUT"
rm -f "$OUT/$NAME.zip"
( cd "$PKG" && zip -qr "$OUT/$NAME.zip" ts README.txt MANIFEST.tsv )
echo ">>> $OUT/$NAME.zip ($(find "$T" -type f | wc -l) files; reTS $REV, amxmodx $AMXX_REV)"
