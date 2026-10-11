#!/usr/bin/env bash
# steam-pair.sh <commit>: build reTS's i386 server and client at <commit> and install them as a matching
# pair into a Steam Half-Life ts/ folder, for testing a build (or bisecting) in a real client under the
# native Linux engine: dlls/ts.so, ts_i386.so and ts_i686.so (the Steam engine loads ts.so, see CLAUDE.md
# on gamedll_linux) and cl_dlls/client.so. Windows files (mp.dll, client.dll) are left as they are, so
# test with hl_linux, not hl.exe. Restart Half-Life after each install.
#
# The build runs in its own detached worktree (build-steam-pair/wt, gitignored), so the main checkout
# stays as it is. Set aside the files you want to go back to before the first install.
#
#   STEAM_TS  the ts/ folder (default ~/.steam/steam/steamapps/common/Half-Life/ts)
#   IMAGE     the build image (default ghcr.io/goldsrc-net/build-containers/debian10:latest)
#
# Remove the worktree when done: git worktree remove --force build-steam-pair/wt
set -euo pipefail
C=${1:?usage: steam-pair.sh <commit>}
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
T="${STEAM_TS:-$HOME/.steam/steam/steamapps/common/Half-Life/ts}"
WT="$ROOT/build-steam-pair/wt"
[ -d "$T/dlls" ] && [ -d "$T/cl_dlls" ] || { echo "no dlls/ and cl_dlls/ in $T" >&2; exit 1; }
[ -d "$WT" ] || git -C "$ROOT" worktree add -q --detach "$WT" "$C"
git -C "$WT" checkout -q --detach "$C"
git -C "$WT" submodule update -q --init --recursive
( cd "$WT" && IMAGE="${IMAGE:-ghcr.io/goldsrc-net/build-containers/debian10:latest}" BUILD_DIR=build-i386 \
	scripts/build.sh i386 all >/dev/null )
for f in ts.so ts_i386.so ts_i686.so; do install -m 755 "$WT/build-i386/dlls/hl.so" "$T/dlls/$f"; done
install -m 755 "$WT/build-i386/cl_dll/client.so" "$T/cl_dlls/client.so"
echo "installed $(git -C "$WT" log --oneline -1)"
sha256sum "$T/dlls/ts.so" "$T/cl_dlls/client.so"
