# Installing reTS

reTS is a patch for The Specialists 3.0, not a standalone game. It replaces TS 3.0's game libraries,
`delta.lst` and one resource file, and leaves its maps, models, sounds and configs alone. You need
Half-Life and TS 3.0 installed first.

## Players

1. Download `reTS-<version>.zip`.
2. Copy its `ts` folder over your `Half-Life/ts` folder, replacing the files it asks about.
3. Restart Half-Life.

The zip's `README.txt` lists every file it replaces. Copy those files somewhere first if you want to go
back to TS 3.0; putting them back undoes the patch.

A reTS client plays on TS 3.0 servers and a TS 3.0 client plays on reTS servers, so nobody has to switch
at the same time.

## Servers

Install the same way into the server's `ts` folder and restart it. TS 3.0's `server.cfg`, `mapcycle.txt`
and cvars work unchanged.

The server library is in the zip three times because engines pick different names:

| File | Loaded by |
|---|---|
| `dlls/ts.so` | Steam's current Linux engine |
| `dlls/ts_i386.so` | older Linux engines (liblist.gam's `gamedll_linux`) |
| `dlls/ts_i686.so` | Metamod-R |
| `dlls/mp.dll` | Windows |

Metamod loads reTS the same way it loads TS 3.0. Metamod-R, AMX Mod X and RCBot run on it. reTSX, the TS
module in goldsrc-net's AMX Mod X fork, uses reTS's `ReTS_GetInterface` export instead of member offsets
when it finds it.

reTS-only server settings are listed in [reference/DEVIATIONS.md](reference/DEVIATIONS.md). Each defaults
to TS 3.0's behaviour. Set them in `server.cfg`.

## 64-bit engines

A zip that includes `*_amd64` and `*_arm64` files carries 64-bit builds for Xash3D and 64-bit ReHLDS,
named the way those engines build the file name from TS's liblist.gam. Steam's Half-Life is 32-bit and never loads them.

## Going back

Put the files you set aside back over the patch, or reinstall TS 3.0's libraries. reTS changes nothing
outside the files listed in the zip's `README.txt`.
