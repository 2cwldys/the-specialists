#!/usr/bin/env bash
#
# build-amxx-win.sh <amxmodx checkout> [out dir] — build AMX Mod X's core (amxmodx_mm.dll) and TS module
# (tsx_amxx.dll), Win32, from a checkout of the fork's tsx-ts3 branch (reTSX) with clang-cl, lld-link,
# NASM and the xwin CRT/SDK, the toolchain scripts/build-win.sh uses. AMBuild cannot cross-build Windows
# from Linux, so this compiles the sources AMBuildScript, amxmodx/AMBuilder, modules/ts/tsx/AMBuilder and
# the third_party/support AMBuilders list, with the flags they give an MSVC build.
#
# Both come from tsx-ts3, not the public drop-in: the core carries the gamedata condition
# "gamedll_export" (5a5e5e8b) that selects reTS's Ham Sandwich slots, and the module is reTSX. Every
# other module is unchanged from the public 64bit branch.
#
#   HLSDK    HL SDK headers (default ../hlds64-buildchain/hlsdk, the buildchain's pin)
#   METAMOD  metamod-hl1 headers (default ../hlds64-buildchain/metamod-hl1; AMX Mod X wants these, not
#            Metamod-R's)
#   XWIN_DIR xwin splat (default ~/.cache/xwin/splat-x86); LLVM_BIN (default /usr/lib/llvm-21/bin)
#
# Output: <out dir>/amxmodx_mm.dll and tsx_amxx.dll (default build-amxx-win/ in this repo).
set -euo pipefail
MODTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
AMXX="$(realpath "${1:?usage: build-amxx-win.sh <amxmodx checkout> [out dir]}")"
OUT="$(realpath -m "${2:-$MODTS/build-amxx-win}")"
HLSDK="$(realpath "${HLSDK:-$MODTS/../hlds64-buildchain/hlsdk}")"
METAMOD="$(realpath "${METAMOD:-$MODTS/../hlds64-buildchain/metamod-hl1}")"
XWIN_DIR="${XWIN_DIR:-$HOME/.cache/xwin/splat-x86}"
LLVM_BIN="${LLVM_BIN:-/usr/lib/llvm-21/bin}"
mkdir -p "$OUT/obj"

BASE=(--target=i686-pc-windows-msvc /nologo /c /O2 /MT /W0 -fms-compatibility-version=19.44
  /imsvc"$XWIN_DIR/crt/include" /imsvc"$XWIN_DIR/sdk/include/ucrt"
  /imsvc"$XWIN_DIR/sdk/include/um" /imsvc"$XWIN_DIR/sdk/include/shared"
  /DWIN32 /D_WINDOWS /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS
  /D_CRT_NONSTDC_NO_DEPRECATE /D_ITERATOR_DEBUG_LEVEL=0 /DAMX_NOPROPLIST /DPAWN_CELL_SIZE=32
  /DAMXMODX_BUILD /DHAVE_STDINT_H
  /I"$AMXX/public" /I"$AMXX/public/sdk" /I"$AMXX/public/amtl" /I"$AMXX/public/amtl/amtl"
  /I"$AMXX/public/memtools" /I"$AMXX/third_party" /I"$AMXX/third_party/hashing" /I"$AMXX/third_party/zlib"
  /I"$AMXX/third_party/utf8rewind" /I"$AMXX/support/versionlib")
HL=(/I"$METAMOD/metamod" /I"$HLSDK/common" /I"$HLSDK/dlls" /I"$HLSDK/engine" /I"$HLSDK/game_shared"
  /I"$HLSDK/public" /I"$HLSDK/pm_shared")

# cc <obj dir> <flags...> -- <sources...>: compile each source (C as C, C++ with /EHsc /GR- /TP)
cc() {
  local dir="$1"; shift; local flags=(); while [ "$1" != -- ]; do flags+=("$1"); shift; done; shift
  mkdir -p "$OUT/obj/$dir"
  for s in "$@"; do
    local o="$OUT/obj/$dir/$(echo "${s#$AMXX/}" | tr / _).obj"
    if [[ "$s" == *.c ]]; then
      "$LLVM_BIN/clang-cl" "${BASE[@]}" "${flags[@]}" /TC /Fo"$o" -- "$s"
    else
      "$LLVM_BIN/clang-cl" "${BASE[@]}" "${flags[@]}" /EHsc /GR- /TP /Fo"$o" -- "$s"
    fi
  done
}
lib() { "$LLVM_BIN/llvm-lib" /nologo /OUT:"$OUT/$1.lib" "$OUT/obj/$1"/*.obj; }
link() {  # link <out dll> <objs/libs...>
  local dll="$1"; shift
  "$LLVM_BIN/lld-link" /nologo /DLL /MACHINE:X86 /SUBSYSTEM:WINDOWS /SECTION:.data,RW \
    '/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1' \
    /libpath:"$XWIN_DIR/crt/lib/x86" /libpath:"$XWIN_DIR/sdk/lib/um/x86" /libpath:"$XWIN_DIR/sdk/lib/ucrt/x86" \
    /OUT:"$dll" "$@" kernel32.lib user32.lib
}

# static libraries the core links (zlib, hashing, utf8rewind, versionlib)
Z="$AMXX/third_party/zlib"
cc zlib -- "$Z"/{adler32,compress,crc32,deflate,gzclose,gzlib,gzread,gzwrite,infback,inffast,inflate,inftrees,trees,uncompr,zutil}.c
H="$AMXX/third_party/hashing"
cc hashing -- "$H/hashing.cpp" "$H"/hashers/{crc32,keccak,md5,sha1,sha3,sha256}.cpp
U="$AMXX/third_party/utf8rewind"
cc utf8rewind /DUTF8REWINDS_EXPORTS -- "$U"/{utf8rewind,unicodedatabase}.c \
  "$U"/internal/{casemapping,codepoint,composition,database,decomposition,seeking,streaming}.c
cc versionlib -- "$AMXX/support/versionlib/versionlib.cpp"
for l in zlib hashing utf8rewind versionlib; do lib "$l"; done

# core: the i386 NASM JIT, interpreter helpers and native gateway, then the C++ sources
A="$AMXX/amxmodx"
mkdir -p "$OUT/obj/core"
for a in helpers-x86 natives-x86 amxexecn amxjitsn; do
  extra=(); [ "$a" = amxjitsn ] && extra=(-O0)  # opcode sizes must stay maximal for patching
  nasm -I"$A/" "$A/$a.asm" -f win32 -o "$OUT/obj/core/$a-asm.o" "${extra[@]}"
done
CORE=(meta_api CVault vault float file modules CMisc CTask string amxmodx CEvent CCmd CLogEvent srvcmd
  strptime amxcore amxtime power amxxlog fakemeta amxxfile CLang emsg CForward CPlugin CModule CMenu util
  amx amxdbg natives newmenus debugger optimizer format messages libraries vector sorting nongpl_matches
  CFlagManager datastructs trie_natives CDataPack datapacks stackstructs CTextParsers textparse CvarManager
  cvars CLibrarySys CGameConfigs gameconfigs CoreConfig sm_crc32)
srcs=(); for s in "${CORE[@]}"; do srcs+=("$A/$s.cpp"); done
cc core /DJIT /DASM32 /DAMXX_USE_VERSIONLIB /I"$A" "${HL[@]}" -- "${srcs[@]}" \
  "$AMXX/public/memtools/MemoryUtils.cpp" "$AMXX/public/memtools/CDetour/detours.cpp" \
  "$AMXX/public/memtools/CDetour/asm/asm.c" "$AMXX/public/resdk/mod_rehlds_api.cpp"
link "$OUT/amxmodx_mm.dll" "$OUT/obj/core"/*.obj "$OUT/obj/core"/*-asm.o \
  "$OUT/zlib.lib" "$OUT/hashing.lib" "$OUT/utf8rewind.lib" "$OUT/versionlib.lib"

# reTSX
T="$AMXX/modules/ts/tsx"
cc tsx /I"$T" /I"$T/sdk" "${HL[@]}" -- "$AMXX/public/sdk/amxxmodule.cpp" \
  "$T"/{CMisc,CRank,NBase,NRank,Utils,moduleconfig,usermsg}.cpp
link "$OUT/tsx_amxx.dll" "$OUT/obj/tsx"/*.obj

echo ">>> $OUT/amxmodx_mm.dll, tsx_amxx.dll ($(git -C "$AMXX" rev-parse --short HEAD))"
