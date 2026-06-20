# reference/ — original "ts" (The Specialists) artifacts

Source-of-truth binaries from the original mod, kept for reconstructing the
game DLL as compilable C++ on top of this SDK fork. **Reference only — not
linked into any build.**

| File | What it is | Reconstruction value |
|---|---|---|
| `ts_i386.so` | Linux 32-bit server dll, GCC 2.96, **not stripped** | Full function inventory (2,426 funcs) + addresses; loads into Ghidra/IDA with names auto-applied. STABS present but only for libgcc/CRT — no source lines/locals for mod code. |
| `mp.dll` | Windows 32-bit server dll, MSVC, stripped of debug info | Exports **159 MSVC-mangled** symbols that demangle to **fully-typed** signatures (return type, params, `__thiscall`, const-ness). The richest signature source. |
| `ts_fgd.fgd` | Worldcraft/Hammer entity definitions | Entity classnames, keyvalues, spawnflags, I/O — maps editor-facing names to the `CTS*` classes for free. |

## Regenerating the signature data

```bash
./dump-signatures.sh        # needs llvm-undname (llvm) + binutils
```

Produces, in this folder:
- `signatures-msvc.txt` — demangled, fully-typed signatures from `mp.dll`
- `exports-msvc.raw` — raw mangled export names
- `symbols-gcc2.txt` — `addr  name` for every defined function in `ts_i386.so`
- `classes.txt` — distinct class names + method counts

## Notes / gotchas

- This binutils `c++filt` **cannot** demangle GCC 2.x (`gnu-v2`) names, so the
  `.so` names are kept raw. They're still readable — `Foo__3BarPi` = `Bar::Foo(int*)`
  — and the MSVC side gives the clean typed form for the same functions.
- `mp.dll` exports only 159 of the ~2,400 functions; use `ts_i386.so` for the
  **complete** inventory and the `.fgd` for the entity surface.
- The mod is built on the open HL SDK (this repo's base). Reconstruction =
  diff the symbol set against stock HLSDK, then decompile only the `CTS*` /
  weapon / slow-motion delta.

## Provenance

Copied from `~/Packages/xash3d-fwgs/rodir/ts/` (a prior iteration's runtime
tree). Third-party mod content — reconstruction work assumes proper rights.
