# The Specialists 3.0 (RE)

reTS is a fan-made reconstruction of The Specialists 3.0, the Half-Life action mod, as C++ source on the
Half-Life SDK. It rebuilds both game libraries, the server and the client, from TS 3.0's shipped binaries,
so that each one drops into a stock TS 3.0 install and behaves the same. A reTS server plays with TS 3.0
clients and a reTS client plays on TS 3.0 servers.

reTS is not affiliated with or endorsed by the original TS team. The Specialists, its name, maps,
models and sounds are theirs; reTS replaces only the code. See [CREDITS.md](CREDITS.md) for everyone
who made the game.

## Installing

reTS is a patch on top of TS 3.0: copy the release zip's `ts` folder over your `Half-Life/ts` folder and
restart. [INSTALL.md](INSTALL.md) covers players, servers, the server library's file names, and going
back.

## Servers

reTS reads TS 3.0's cvars and config files unchanged, and Metamod-R, AMX Mod X and RCBot run on it.
Where reTS deliberately does something TS 3.0 does not, it is listed in
[reference/DEVIATIONS.md](reference/DEVIATIONS.md); each one keeps TS 3.0's behaviour by default.

## Building

The Linux libraries build in the [goldsrc-net/buildchain](https://github.com/goldsrc-net/buildchain)
Docker image (`scripts/build.sh`), the Windows ones with clang-cl (`scripts/build-win.sh`), and
`scripts/package.sh` makes the release zip. [BUILDING.md](BUILDING.md) has the steps.

## How it was made

The server's original `ts_i386.so` kept its symbols, which gives every function's name; the Windows
`mp.dll` and `client.dll` give typed signatures for the rest. Each class was decompiled with Ghidra,
rewritten on top of [Half-Life Updated](https://github.com/twhl-community/halflife-updated), and checked
by building, running and comparing against the original, including live traces of TS 3.0 running under
a debugger. `reference/` holds the originals and the derived inventories; `scripts/coverage/` checks the
reconstruction against them.

## License

reTS is built on the Half-Life SDK and is distributed under its license ([LICENSE](LICENSE)): free of
charge only. Half-Life Updated's own changes are in [CHANGELOG.md](CHANGELOG.md) and
[FULL_UPDATED_CHANGELOG.md](FULL_UPDATED_CHANGELOG.md), and its contributors in [CREDITS.md](CREDITS.md).
