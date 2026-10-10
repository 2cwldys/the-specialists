#!/usr/bin/env bash
# package.sh — build the reTS patch for a stock The Specialists 3.0 install at the current commit:
# the i386 server and client through the buildchain image (scripts/build.sh), the Win32 ones with
# clang-cl (scripts/build-win.sh), then scripts/assemble-dropin.sh lays out the package. CI builds the
# same binaries with its own jobs and runs assemble-dropin.sh on its artifacts.
#
# Usage: scripts/package.sh [outdir]   (default dist/)
# Refuses a tree with uncommitted changes (other than .serena/project.yml), so a package is a commit.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/dist}"
cd "$ROOT"

if [ -n "$(git status --porcelain --untracked-files=no -- . ':!.serena/project.yml')" ]; then
  echo "ERROR: uncommitted changes; commit first so the package matches a commit" >&2
  git status --short --untracked-files=no -- . ':!.serena/project.yml' >&2
  exit 1
fi
REV="$(git rev-parse --short HEAD)"

scripts/build.sh i386 all
scripts/build-win.sh all

scripts/assemble-dropin.sh "$REV" \
  build-i386/dlls/hl.so build-i386/cl_dll/client.so \
  build-win32-clangcl/dlls/hl.dll build-win32-clangcl/cl_dll/client.dll \
  "$OUT"
