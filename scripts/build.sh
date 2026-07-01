#!/usr/bin/env bash
# build.sh — build mod-ts's hl.so portably, using the hlds64-buildchain image.
#
# Clean separation: mod-ts is NOT a buildchain submodule. It only *consumes*
# the buildchain's Docker image (the glibc-2.28 cross-toolchain) so binaries
# run on any glibc >= 2.28. The buildchain stays provenance-only for the
# public server stack; mod-ts owns its own build here.
#
# Prereq: the image must exist locally. Build it once from the buildchain:
#   ( cd ../hlds64-buildchain && make image )
#
# Usage:
#   scripts/build.sh [i386|amd64|aarch64] [hl|client|all]   (defaults: amd64 hl)
#
# Output: build-<arch>/dlls/hl.so  (+ build-<arch>/cl_dll/client.so for client/all)
#
# The client links VGUI: 32-bit (i386) uses Valve's bundled vgui.so; 64-bit (amd64) uses
# openvgui's ABI-compatible libvgui.so (vendored as linux/release/vgui64.so) — see cl_dll/CMakeLists.txt.
set -euo pipefail

ARCH="${1:-amd64}"
TARGET="${2:-hl}"
IMAGE="${IMAGE:-hlds64-buildchain:debian10}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

case "$TARGET" in
  hl)     BUILD_TARGETS="hl" ;;
  client) BUILD_TARGETS="client" ;;
  all)    BUILD_TARGETS="hl client" ;;
  *) echo "unknown target: $TARGET (want hl|client|all)" >&2; exit 2 ;;
esac

case "$ARCH" in
  i386)    TC="-DCMAKE_TOOLCHAIN_FILE=/work/mod-ts/cmake/LinuxToolchain.cmake";          ENV="" ;;
  amd64)   TC="";                                                                        ENV="" ;;
  aarch64) TC="-DCMAKE_TOOLCHAIN_FILE=/work/mod-ts/cmake/LinuxToolchain-aarch64.cmake";  ENV="CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++" ;;
  *) echo "unknown arch: $ARCH (want i386|amd64|aarch64)" >&2; exit 2 ;;
esac

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "ERROR: image '$IMAGE' not found. Build it first:" >&2
  echo "  ( cd ../hlds64-buildchain && make image )" >&2
  exit 1
fi

TARGET_FLAGS=""; for t in $BUILD_TARGETS; do TARGET_FLAGS="$TARGET_FLAGS --target $t"; done

echo ">>> mod-ts ($ARCH) [$BUILD_TARGETS] via $IMAGE"
docker run --rm -i \
  --platform=linux/amd64 \
  --user "$(id -u):$(id -g)" \
  -v "$ROOT":/work/mod-ts \
  "$IMAGE" \
  bash -lc "set -e; \
    mkdir -p /work/mod-ts/build-$ARCH; \
    cd /work/mod-ts/build-$ARCH; \
    $ENV cmake -S /work/mod-ts -B . -DCMAKE_BUILD_TYPE=${BUILDTYPE:-Release} $TC; \
    cmake --build . -j\$(nproc)$TARGET_FLAGS"

for t in $BUILD_TARGETS; do
  case "$t" in
    hl)     SO="$ROOT/build-$ARCH/dlls/hl.so" ;;
    client) SO="$ROOT/build-$ARCH/cl_dll/client.so" ;;
  esac
  [ -f "$SO" ] || continue
  echo ">>> done: ${SO#$ROOT/}"
  echo "    glibc floor: $(objdump -T "$SO" 2>/dev/null | grep -oE 'GLIBC_[0-9.]+' | sort -V | tail -1)"
done
