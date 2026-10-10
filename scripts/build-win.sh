#!/usr/bin/env bash
# build-win.sh — Win32 hl.dll (the game's dlls/mp.dll) and client.dll on Linux, without a Windows runner.
#
# clang-cl + lld-link give the MSVC ABI, so Valve's win32 vgui.lib links as it does under MSVC (mingw
# cannot). The MSVC CRT and Windows SDK come from xwin (cargo install xwin), about 300 MB to download:
#   xwin --accept-license --arch x86 splat --output ~/.cache/xwin/splat-x86
# XWIN_DIR overrides that path, LLVM_BIN the LLVM tools (default /usr/lib/llvm-21/bin).
# CI builds the same targets with MSVC on windows-latest (.github/workflows/ci-cd.yml).
#
# Usage: scripts/build-win.sh [hl|client|all]   (default all)
# Output: build-win32-clangcl/dlls/hl.dll, build-win32-clangcl/cl_dll/client.dll
# BUILDTYPE=Debug builds the Debug profile (with PDBs); BUILD_DIR=<dir under the repo> builds there.
set -euo pipefail

TARGET="${1:-all}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/${BUILD_DIR:-build-win32-clangcl}"
XWIN_DIR="${XWIN_DIR:-$HOME/.cache/xwin/splat-x86}"
export XWIN_DIR

case "$TARGET" in
  hl)     TARGETS=(--target hl) ;;
  client) TARGETS=(--target client) ;;
  all)    TARGETS=(--target hl --target client) ;;
  *) echo "unknown target: $TARGET (want hl|client|all)" >&2; exit 2 ;;
esac

if [ ! -d "$XWIN_DIR/crt/lib/x86" ]; then
  echo "ERROR: no x86 MSVC CRT under $XWIN_DIR. Fetch it first:" >&2
  echo "  xwin --accept-license --arch x86 splat --output $XWIN_DIR" >&2
  exit 1
fi

GEN="Unix Makefiles"; command -v ninja >/dev/null && GEN=Ninja
cmake -S "$ROOT" -B "$BUILD" -G "$GEN" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/WindowsClangClToolchain.cmake"
cmake --build "$BUILD" "${TARGETS[@]}" -j"$(nproc)"

for f in "$BUILD/dlls/hl.dll" "$BUILD/cl_dll/client.dll"; do
  [ -f "$f" ] && echo ">>> done: ${f#$ROOT/}"
done
