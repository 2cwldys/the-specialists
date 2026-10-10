#!/usr/bin/env bash
# package.sh — the reTS drop-in for a stock The Specialists 3.0 install: only the files that differ from
# TS 3.0, built at one commit. Extract over an existing ts/ (Steam's Half-Life/ts, a server's ts/).
#
#   dlls/ts.so ts_i386.so ts_i686.so   reTS server, i386. Steam's current engine loads ts.so (it cuts
#                                      liblist's gamedll_linux at the first "_"), older engines and
#                                      liblist name ts_i386.so, Metamod-R prefers ts_i686.so.
#   dlls/mp.dll                        reTS server, Win32 (scripts/build-win.sh)
#   cl_dlls/client.so, client.dll      reTS client, i386 and Win32
#   delta.lst                          game/ts: the 25th-anniversary engine's field widths
#   resource/trackerscheme.res         game/ts: loads Valve's Linux fonts (Fira Sans for Trebuchet MS)
#   MANIFEST.tsv                       path, source, sha256 of every file above
#
# Stock files stay stock: liblist.gam is TS's own (game/ts/liblist.gam is the development manifest).
# A file is added to the package only when it is confirmed working in a real client; list it below.
#
# Usage: scripts/package.sh [outdir]   (default dist/)
# Output: <outdir>/reTS-<commit>/ts/... and <outdir>/reTS-<commit>.zip
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

CONTENT=(delta.lst resource/trackerscheme.res)

scripts/build.sh i386 all
scripts/build-win.sh all

PKG="$OUT/reTS-$REV"
if [ -e "$PKG" ]; then
  echo "ERROR: $PKG exists; remove it or package another commit" >&2
  exit 1
fi
mkdir -p "$PKG/ts/dlls" "$PKG/ts/cl_dlls" "$PKG/ts/resource"

for name in ts.so ts_i386.so ts_i686.so; do
  cp build-i386/dlls/hl.so "$PKG/ts/dlls/$name"
done
cp build-win32-clangcl/dlls/hl.dll "$PKG/ts/dlls/mp.dll"
cp build-i386/cl_dll/client.so "$PKG/ts/cl_dlls/client.so"
cp build-win32-clangcl/cl_dll/client.dll "$PKG/ts/cl_dlls/client.dll"
for f in "${CONTENT[@]}"; do
  cp "game/ts/$f" "$PKG/ts/$f"
done

{
  printf 'path\tsource\tsha256\n'
  for name in ts.so ts_i386.so ts_i686.so; do
    printf 'dlls/%s\tbuild-i386 hl.so @%s\t%s\n' "$name" "$REV" "$(sha256sum "$PKG/ts/dlls/$name" | cut -d' ' -f1)"
  done
  printf 'dlls/mp.dll\tbuild-win32 hl.dll @%s\t%s\n' "$REV" "$(sha256sum "$PKG/ts/dlls/mp.dll" | cut -d' ' -f1)"
  printf 'cl_dlls/client.so\tbuild-i386 client.so @%s\t%s\n' "$REV" "$(sha256sum "$PKG/ts/cl_dlls/client.so" | cut -d' ' -f1)"
  printf 'cl_dlls/client.dll\tbuild-win32 client.dll @%s\t%s\n' "$REV" "$(sha256sum "$PKG/ts/cl_dlls/client.dll" | cut -d' ' -f1)"
  for f in "${CONTENT[@]}"; do
    printf '%s\tgame/ts/%s @%s\t%s\n' "$f" "$f" "$REV" "$(sha256sum "$PKG/ts/$f" | cut -d' ' -f1)"
  done
} > "$PKG/MANIFEST.tsv"

( cd "$PKG" && zip -qr "../reTS-$REV.zip" ts MANIFEST.tsv )
echo ">>> $PKG"
echo ">>> $OUT/reTS-$REV.zip"
column -t -s $'\t' "$PKG/MANIFEST.tsv"
