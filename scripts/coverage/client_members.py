#!/usr/bin/env python3
"""
client_members.py — recover member skeletons for the CLIENT (HUD/prediction) classes -> members_auto_client.tsv.

The server side gets member skeletons from the unstripped .so's gcc2 symbols (every method's class is known).
client.dll has NONE of that: no symbols, no RTTI (only .?AVtype_info@@), and the HUD classes aren't exported.
So we bring the class NAMES over from OUR reconstruction, which DOES have symbols:

  1. binmap (string-anchor) matches our build-i386/cl_dll/client.so (full symbols) -> client.dll RVAs, keyed
     by OUR class names -> reference/coverage/binmap_client.tsv.
  2. ApplyClientThis.java marks those client.dll functions __thiscall and renames them into class namespaces.
  3. DumpMembers.java (opaque + `(int)this` parse, class from namespace) -> members_auto_client.tsv.

Then curate_members can corroborate the cl_dll source names against this skeleton, same as the server loop.

  python3 scripts/coverage/client_members.py        # full pipeline (builds the i386 client.so if missing)
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
CSO = os.path.join(ROOT, "build-i386", "cl_dll", "client.so")
DLL = os.path.join(ROOT, "reference", "client.dll")
COV = os.path.join(ROOT, "reference", "coverage")
SP = os.path.join(ROOT, "scripts", "ghidra")
PROJ = os.path.join(ROOT, "recovered", "ghidra", "client")
GH = subprocess.run("command -v analyzeHeadless", shell=True, capture_output=True, text=True).stdout.strip() \
     or os.path.expanduser("~/Tools/ghidra/support/analyzeHeadless")


def sh(cmd, **kw):
    return subprocess.run(cmd, shell=True, text=True, **kw)


def main():
    # 0) our i386 client.so (32-bit PIC -> GOT-relative strings, like the server flow binmap expects)
    if not os.path.exists(CSO):
        print(">>> building i386 client.so …")
        sh(f"'{ROOT}/scripts/build.sh' i386 client", check=True)

    # 1) binmap match: our client.so (named) -> client.dll RVAs
    got = ""
    for ln in sh(f"nm '{CSO}'", capture_output=True).stdout.splitlines():
        if "_GLOBAL_OFFSET_TABLE_" in ln:
            got = "0x" + ln.split()[0]
            break
    demap = "/tmp/client_demap.txt"
    sh(f"nm '{CSO}' | awk '$2 ~ /[TtWwV]/ {{print $3}}' | grep '^_Z' > /tmp/cm_m")
    sh("c++filt < /tmp/cm_m > /tmp/cm_d")
    sh(f"paste -d'@' /tmp/cm_m /tmp/cm_d | awk -F'@' '{{print $1\"  =>  \"$2}}' > {demap}")
    binmap_out = os.path.join(COV, "binmap_client.tsv")
    env = dict(os.environ, BINMAP_SO=CSO, BINMAP_MP=DLL, BINMAP_GOT=got,
               BINMAP_MAP=demap, BINMAP_OUT=binmap_out)
    print(f">>> binmap: our client.so (GOT {got}) -> client.dll …")
    subprocess.run([sys.executable, os.path.join(HERE, "..", "binmap.py"), "--build"], env=env, check=True)

    # 2+3) type `this` from the match + DumpMembers, on a FRESH client.dll import (pristine functions)
    print(">>> ghidra: ApplyClientThis + DumpMembers (fresh import) …")
    sh(f"rm -rf '{PROJ}'; mkdir -p '{PROJ}'")
    cmd = (f"'{GH}' '{PROJ}' clientdll -import '{DLL}' -preScript DisableNoReturn.java -scriptPath '{SP}' "
           f"-postScript ApplyClientThis.java '{binmap_out}' "
           f"-postScript DumpMembers.java '{COV}' members_auto_client.tsv")
    out = sh(cmd, capture_output=True).stdout
    for ln in out.splitlines():
        if re.search(r"ApplyClientThis:|DumpMembers:", ln):
            print("   " + ln.split("> ", 1)[-1].replace(" (GhidraScript)", ""))

    tsv = os.path.join(COV, "members_auto_client.tsv")
    rows = open(tsv).read().splitlines()[1:] if os.path.exists(tsv) else []
    classes = sorted(set(r.split("\t")[0] for r in rows if "\t" in r))
    hud = [c for c in classes if c.startswith("CHud")]
    print(f"\nmembers_auto_client.tsv: {len(rows)} members / {len(classes)} classes "
          f"({len(hud)} CHud* classes: {', '.join(hud[:12])}{' …' if len(hud) > 12 else ''})")
    print("next: curate_members can corroborate cl_dll source names against this skeleton.")


if __name__ == "__main__":
    main()
