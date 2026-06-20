#!/usr/bin/env bash
# dump-signatures.sh — extract recoverable C++ signatures from the original
# "ts" (The Specialists) game dlls, to seed reconstruction of the source.
#
# Two complementary symbol sources live next to this script:
#   mp.dll        Windows MSVC build — exports are MSVC-mangled (?name@Class@@...)
#                 which demangle to FULLY-TYPED signatures (return type, params,
#                 calling convention, const-ness). This is the rich source.
#   ts_i386.so    Linux GCC 2.96 build — NOT stripped: 2.2k-symbol .symtab gives
#                 the complete function inventory + addresses. GCC 2.x mangling
#                 is only partially recoverable with modern binutils c++filt, so
#                 we keep these raw (names already encode class::method) and use
#                 them for the inventory / disassembly address map.
#
# Output (all written next to this script):
#   signatures-msvc.txt   demangled, fully-typed signatures from mp.dll
#   exports-msvc.raw       raw mangled mp.dll export names
#   symbols-gcc2.txt       raw GCC2 symtab function names from ts_i386.so (+addr)
#   classes.txt            distinct class names seen, with method counts
set -euo pipefail
cd "$(dirname "$0")"

DLL=mp.dll
SO=ts_i386.so

# locate an MSVC demangler (llvm-undname); fall back across versioned names
UNDNAME=""
for c in llvm-undname llvm-undname-18 llvm-undname-17 llvm-undname-16 llvm-undname-15 llvm-undname-14; do
  if command -v "$c" >/dev/null 2>&1; then UNDNAME="$c"; break; fi
done
[ -n "$UNDNAME" ] || { echo "ERROR: no llvm-undname found (apt install llvm)"; exit 1; }
echo ">> using MSVC demangler: $UNDNAME"

# --- mp.dll : mangled exports -> typed signatures ---------------------------
echo ">> extracting MSVC exports from $DLL"
objdump -p "$DLL" | grep -oE '\?[A-Za-z0-9_@$?<>]+' | sort -u > exports-msvc.raw
echo "   $(wc -l < exports-msvc.raw) mangled exports"
"$UNDNAME" < exports-msvc.raw > signatures-msvc.txt 2>/dev/null || true
echo "   -> signatures-msvc.txt"

# --- ts_i386.so : full function inventory (addr + name) ---------------------
echo ">> extracting GCC2 symtab from $SO"
# 'T'/'t' = defined text (functions). Keep address for the disassembly map.
nm -C=none "$SO" 2>/dev/null | awk '$2 ~ /^[tT]$/ {print $1"  "$3}' | sort > symbols-gcc2.txt \
  || nm "$SO" | awk '$2 ~ /^[tT]$/ {print $1"  "$3}' | sort > symbols-gcc2.txt
echo "   $(wc -l < symbols-gcc2.txt) defined functions -> symbols-gcc2.txt"

# --- class inventory (from the richer MSVC signatures) ----------------------
echo ">> building class inventory"
# pull "Class::" qualifiers out of the demangled signatures
grep -oE '[A-Za-z_][A-Za-z0-9_]*::' signatures-msvc.txt \
  | sed 's/:://' | sort | uniq -c | sort -rn > classes.txt
echo "   $(wc -l < classes.txt) distinct classes -> classes.txt"

echo
echo "== summary =="
printf "  mp.dll typed signatures : %s\n" "$(grep -c '::' signatures-msvc.txt || echo 0)"
printf "  ts_i386.so functions    : %s\n" "$(wc -l < symbols-gcc2.txt)"
printf "  distinct classes        : %s\n" "$(wc -l < classes.txt)"
echo "  TS-specific classes (CTS* / weapon-ish):"
grep -iE 'CTS|Slow|Akimbo|Kungfu|Weapon|Flag|Grenade' classes.txt | head -25 | sed 's/^/    /'
