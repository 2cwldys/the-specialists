#!/usr/bin/env bash
# package.sh — build the reTS patch for a stock The Specialists 3.0 install at the current commit:
# the i386 server and client through the buildchain image (scripts/build.sh), the Win32 ones with
# clang-cl (scripts/build-win.sh), then scripts/assemble-dropin.sh lays out the package. CI builds the
# same binaries with its own jobs and runs assemble-dropin.sh on its artifacts.
#
# Usage: scripts/package.sh [--debug] [outdir]   (default dist/)
#   Release (default): reTS-<rev>, stripped, from build-i386 and build-win32-clangcl.
#   --debug: reTS-<rev>-debug from the Debug profile, not stripped, with PDBs, built apart in
#   build-i386-debug and build-win32-clangcl-debug. The Win32 Debug build needs xwin's debug CRT:
#     xwin --accept-license --arch x86 splat --include-debug-libs --output ~/.cache/xwin/splat-x86
# Refuses a tree with uncommitted changes (other than .serena/project.yml), so a package is a commit.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEBUG=()
if [ "${1:-}" = "--debug" ]; then DEBUG=(--debug); shift; fi
OUT="${1:-$ROOT/dist}"
cd "$ROOT"

if [ -n "$(git status --porcelain --untracked-files=no -- . ':!.serena/project.yml')" ]; then
  echo "ERROR: uncommitted changes; commit first so the package matches a commit" >&2
  git status --short --untracked-files=no -- . ':!.serena/project.yml' >&2
  exit 1
fi
REV="$(git rev-parse --short HEAD)"

if [ ${#DEBUG[@]} -eq 0 ]; then
  LIN=build-i386; WIN=build-win32-clangcl
  scripts/build.sh i386 all
  scripts/build-win.sh all
else
  LIN=build-i386-debug; WIN=build-win32-clangcl-debug
  BUILDTYPE=Debug BUILD_DIR=$LIN scripts/build.sh i386 all
  BUILDTYPE=Debug BUILD_DIR=$WIN scripts/build-win.sh all
fi

scripts/assemble-dropin.sh "${DEBUG[@]}" "$REV" \
  $LIN/dlls/hl.so $LIN/cl_dll/client.so \
  $WIN/dlls/hl.dll $WIN/cl_dll/client.dll \
  "$OUT"
