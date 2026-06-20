# ts/ — mod content directory

This is the runtime game directory for **mod-ts**. It is assembled onto a
server alongside the stock `valve/` directory and launched with `-game ts`.

```
ts/
  liblist.gam     # mod manifest (this repo)        — version-controlled
  dlls/hl.so      # server game dll, built from the SDK in this repo's root
                  #   via the hlds64-buildchain (make build-mod-ts-<arch>)
  ...             # any maps/models/sounds the mod overrides; everything
                  #   not present here falls back to valve/ at runtime
```

Only files this mod actually overrides need to live here — `fallback_dir
"valve"` in `liblist.gam` covers the rest. The client side currently falls
back to `valve/cl_dlls/client.so`; add a `cl_dlls/` here only when the mod
needs custom HUD/prediction.

Run:

```
hlds_linux -game ts +map crossfire +maxplayers 8 -port 27015 \
  +sv_lan 0 +sv_xashmaster master.goldsrc.net
```
