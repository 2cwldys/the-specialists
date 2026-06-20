#!/usr/bin/env bash
# dump-signatures.sh — extract recoverable C++ signatures from the original
# "ts" (The Specialists) game dlls, to seed reconstruction of the source.
#
# The goal is an ABI-compatible rebuild of BOTH the server and client libs, so
# the MSVC PE exports (which pin types, vtables and __thiscall) are the spine.
#
# Symbol sources living next to this script:
#   mp.dll        Windows MSVC server dll — MSVC-mangled exports (?name@Class@@..)
#                 demangle to FULLY-TYPED signatures (return type, params,
#                 calling convention, const-ness). The ABI source for the server.
#   client.dll    Windows MSVC client dll — same, ~1k exports incl. the HL client
#                 engine interface (HUD_*) and VGUI. The ABI source for the client.
#   ts_i386.so    Linux GCC 2.96 server dll — NOT stripped: 2.4k-symbol .symtab
#                 gives the complete function inventory + addresses for the
#                 disassembly map. GCC 2.x mangling isn't recoverable with this
#                 binutils c++filt, so names are kept raw (still readable).
#
# Output (all written next to this script):
#   signatures-<base>.txt   demangled, fully-typed signatures per MSVC dll
#   exports-<base>.raw      raw mangled export names per MSVC dll
#   symbols-gcc2.txt        raw GCC2 symtab function names from ts_i386.so (+addr)
#   classes.txt             distinct class names seen across all dlls, w/ counts
set -euo pipefail
cd "$(dirname "$0")"

PE_DLLS="mp.dll client.dll"   # MSVC PE binaries (server + client)
SO=ts_i386.so

# locate an MSVC demangler (llvm-undname); fall back across versioned names
UNDNAME=""
for c in llvm-undname llvm-undname-18 llvm-undname-17 llvm-undname-16 llvm-undname-15 llvm-undname-14; do
  if command -v "$c" >/dev/null 2>&1; then UNDNAME="$c"; break; fi
done
[ -n "$UNDNAME" ] || { echo "ERROR: no llvm-undname found (apt install llvm)"; exit 1; }
echo ">> using MSVC demangler: $UNDNAME"

# --- MSVC PE dlls : mangled exports -> typed signatures ---------------------
: > classes.tmp
for DLL in $PE_DLLS; do
  [ -f "$DLL" ] || { echo ">> SKIP $DLL (missing)"; continue; }
  base="${DLL%.dll}"
  echo ">> extracting MSVC exports from $DLL"
  objdump -p "$DLL" | grep -oE '\?[A-Za-z0-9_@$?<>]+' | sort -u > "exports-$base.raw"
  echo "   $(wc -l < "exports-$base.raw") mangled exports"
  "$UNDNAME" < "exports-$base.raw" > "signatures-$base.txt" 2>/dev/null || true
  echo "   -> signatures-$base.txt"
  cat "signatures-$base.txt" >> classes.tmp
done

# --- ts_i386.so : full function inventory (addr + name) ---------------------
echo ">> extracting GCC2 symtab from $SO"
# 'T'/'t' = defined text (functions). Keep address for the disassembly map.
nm -C=none "$SO" 2>/dev/null | awk '$2 ~ /^[tT]$/ {print $1"  "$3}' | sort > symbols-gcc2.txt \
  || nm "$SO" | awk '$2 ~ /^[tT]$/ {print $1"  "$3}' | sort > symbols-gcc2.txt
echo "   $(wc -l < symbols-gcc2.txt) defined functions -> symbols-gcc2.txt"

# --- class inventory (from the richer MSVC signatures, server+client) -------
echo ">> building class inventory"
# pull "Class::" qualifiers out of the demangled signatures of every dll
grep -oE '[A-Za-z_][A-Za-z0-9_]*::' classes.tmp \
  | sed 's/:://' | sort | uniq -c | sort -rn > classes.txt
rm -f classes.tmp
echo "   $(wc -l < classes.txt) distinct classes -> classes.txt"

echo
echo "== summary =="
for DLL in $PE_DLLS; do
  base="${DLL%.dll}"; [ -f "signatures-$base.txt" ] || continue
  printf "  %-12s typed signatures : %s\n" "$DLL" "$(grep -c '::' "signatures-$base.txt" || echo 0)"
done
printf "  ts_i386.so   functions      : %s\n" "$(wc -l < symbols-gcc2.txt)"
printf "  distinct classes (all dlls) : %s\n" "$(wc -l < classes.txt)"
echo "  TS-specific classes (CTS* / weapon-ish):"
grep -iE 'CTS|Slow|Akimbo|Kungfu|Weapon|Flag|Grenade' classes.txt | head -25 | sed 's/^/    /'
