#!/usr/bin/env bash
#
# build-tsx-win.sh <amxmodx checkout> [out dir] — build AMX Mod X's TS module (tsx_amxx.dll, Win32) from a
# checkout of the fork's tsx-ts3 branch (reTSX) with clang-cl and the xwin CRT/SDK, the same toolchain as
# scripts/build-win.sh. AMBuild cannot cross-build Windows from Linux; the module is eight sources, so this
# compiles them directly with the flags AMBuildScript and modules/ts/tsx/AMBuilder give an MSVC build.
#
#   HLSDK    HL SDK headers (default ../hlds64-buildchain/hlsdk, the buildchain's pin)
#   METAMOD  metamod-hl1 headers (default ../hlds64-buildchain/metamod-hl1; AMX Mod X wants these, not
#            Metamod-R's)
#   XWIN_DIR xwin splat (default ~/.cache/xwin/splat-x86); LLVM_BIN (default /usr/lib/llvm-21/bin)
#
# Output: <out dir>/tsx_amxx.dll (default build-tsx-win/ in this repo).
set -euo pipefail
MODTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
AMXX="$(realpath "${1:?usage: build-tsx-win.sh <amxmodx checkout> [out dir]}")"
OUT="$(realpath -m "${2:-$MODTS/build-tsx-win}")"
HLSDK="$(realpath "${HLSDK:-$MODTS/../hlds64-buildchain/hlsdk}")"
METAMOD="$(realpath "${METAMOD:-$MODTS/../hlds64-buildchain/metamod-hl1}")"
XWIN_DIR="${XWIN_DIR:-$HOME/.cache/xwin/splat-x86}"
LLVM_BIN="${LLVM_BIN:-/usr/lib/llvm-21/bin}"
TSX="$AMXX/modules/ts/tsx"
mkdir -p "$OUT/obj"

CFLAGS=(--target=i686-pc-windows-msvc /nologo /c /O2 /MT /EHsc /GR- /TP /W0
  -fms-compatibility-version=19.44
  /imsvc"$XWIN_DIR/crt/include" /imsvc"$XWIN_DIR/sdk/include/ucrt"
  /imsvc"$XWIN_DIR/sdk/include/um" /imsvc"$XWIN_DIR/sdk/include/shared"
  /DWIN32 /D_WINDOWS /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS
  /D_CRT_NONSTDC_NO_DEPRECATE /D_ITERATOR_DEBUG_LEVEL=0 /DAMX_NOPROPLIST /DPAWN_CELL_SIZE=32
  /DAMXMODX_BUILD /DHAVE_STDINT_H
  /I"$TSX" /I"$TSX/sdk" /I"$AMXX/public" /I"$AMXX/public/sdk" /I"$AMXX/public/amtl"
  /I"$AMXX/public/amtl/amtl" /I"$AMXX/public/memtools" /I"$METAMOD/metamod"
  /I"$HLSDK/common" /I"$HLSDK/dlls" /I"$HLSDK/engine" /I"$HLSDK/game_shared" /I"$HLSDK/public"
  /I"$HLSDK/pm_shared")
objs=()
for s in "$AMXX/public/sdk/amxxmodule.cpp" "$TSX"/{CMisc,CRank,NBase,NRank,Utils,moduleconfig,usermsg}.cpp; do
  o="$OUT/obj/$(basename "$s" .cpp).obj"
  "$LLVM_BIN/clang-cl" "${CFLAGS[@]}" /Fo"$o" -- "$s"
  objs+=("$o")
done
"$LLVM_BIN/lld-link" /nologo /DLL /MACHINE:X86 /SUBSYSTEM:WINDOWS /SECTION:.data,RW \
  '/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1' \
  /libpath:"$XWIN_DIR/crt/lib/x86" /libpath:"$XWIN_DIR/sdk/lib/um/x86" /libpath:"$XWIN_DIR/sdk/lib/ucrt/x86" \
  /OUT:"$OUT/tsx_amxx.dll" "${objs[@]}" kernel32.lib user32.lib
echo ">>> $OUT/tsx_amxx.dll ($(git -C "$AMXX" rev-parse --short HEAD))"
