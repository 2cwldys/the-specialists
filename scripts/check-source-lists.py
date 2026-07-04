#!/usr/bin/env python3
"""
check-source-lists.py — verify the hand-maintained build source lists agree with
the authoritative CMake lists.

We build hl.so (server) and client.so (client) through FOUR parallel build systems,
each carrying its own explicit .cpp source list:

  authority : dlls/CMakeLists.txt        cl_dll/CMakeLists.txt   (amd64/aarch64/win64 CI)
  legacy    : linux/Makefile.hldll       linux/Makefile.hl_cdll  (linux-i386 CI: `cd linux && make`)
  msvc      : projects/vs2019/hldll.vcxproj  .../hl_cdll.vcxproj  (win32 CI: msbuild)

When a reconstructed TS file is added to CMakeLists only, the Makefile/MSBuild CI jobs
silently stop matching and fail. This script normalizes every .cpp each build system
COMPILES to a repo-relative path and set-compares the legacy + msvc lists against the
CMake authority, per target. Exit 1 (with a MISSING/EXTRA report) on any drift.

Usage: scripts/check-source-lists.py [-v]    (run from anywhere; repo root is auto-located)
CI/pre-commit friendly: no deps beyond the stdlib.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def norm(p):
    """Collapse a repo-relative path with .. segments to a clean posix path."""
    return os.path.normpath(p).replace(os.sep, "/")


# --- extractors: each returns a set of repo-relative .cpp paths a build system compiles ---

def from_cmake(cmake_rel):
    """target_sources() .cpp entries, resolved relative to the CMakeLists' own dir."""
    base = os.path.dirname(cmake_rel)  # e.g. "dlls" or "cl_dll"
    text = open(os.path.join(ROOT, cmake_rel)).read()
    # only the target_sources(...) block(s)
    out = set()
    for m in re.finditer(r"target_sources\([^)]*\)", text, re.S):
        for cpp in re.findall(r'([A-Za-z0-9_./\\-]+\.cpp)\b', m.group(0)):
            out.add(norm(os.path.join(base, cpp)))
    return out


# Makefile obj-dir variable -> source directory it compiles from.
MK_HLDLL_DIRS = {
    "HLDLL_OBJ_DIR": "dlls",
    "PM_OBJ_DIR": "pm_shared",
    "GAME_SHARED_OBJ_DIR": "game_shared",
    "PUBLIC_OBJ_DIR": "public",
    "COMMON_OBJ_DIR": "common",
}
MK_CLDLL_DIRS = {
    "HL1_OBJ_DIR": "cl_dll",
    "HL1_PARTICLEMAN_OBJ_DIR": "cl_dll/particleman",
    "HL1_SERVER_OBJ_DIR": "dlls",
    "PUBLIC_OBJ_DIR": "public",
    "COMMON_OBJ_DIR": "common",
    "GAME_SHARED_OBJ_DIR": "game_shared",
    "PM_SHARED_OBJ_DIR": "pm_shared",
}


def from_makefile(mk_rel, dirmap):
    """$(SOME_OBJ_DIR)/<subpath>.o entries -> <srcdir>/<subpath>.cpp."""
    text = open(os.path.join(ROOT, mk_rel)).read()
    out = set()
    for var, srcdir in dirmap.items():
        for sub in re.findall(r"\$\(" + var + r"\)/([A-Za-z0-9_./-]+)\.o\b", text):
            out.add(norm(os.path.join(srcdir, sub + ".cpp")))
    return out


def from_vcxproj(vcx_rel):
    """<ClCompile Include="..\\..\\dir\\file.cpp" /> -> dir/file.cpp (proj is at projects/vs2019)."""
    projdir = os.path.dirname(vcx_rel)  # projects/vs2019
    text = open(os.path.join(ROOT, vcx_rel)).read()
    out = set()
    for inc in re.findall(r'<ClCompile\s+Include="([^"]+\.cpp)"', text):
        p = inc.replace("\\", "/")
        out.add(norm(os.path.join(projdir, p)))
    return out


# target -> (authority CMake, [(label, extracted-set) ...])
def build_targets():
    return {
        "server (hl)": (
            from_cmake("dlls/CMakeLists.txt"),
            [
                ("linux/Makefile.hldll", from_makefile("linux/Makefile.hldll", MK_HLDLL_DIRS)),
                ("projects/vs2019/hldll.vcxproj", from_vcxproj("projects/vs2019/hldll.vcxproj")),
            ],
        ),
        "client (client)": (
            from_cmake("cl_dll/CMakeLists.txt"),
            [
                ("linux/Makefile.hl_cdll", from_makefile("linux/Makefile.hl_cdll", MK_CLDLL_DIRS)),
                ("projects/vs2019/hl_cdll.vcxproj", from_vcxproj("projects/vs2019/hl_cdll.vcxproj")),
            ],
        ),
    }


def main():
    verbose = "-v" in sys.argv
    ok = True
    for target, (authority, others) in build_targets().items():
        print(f"== {target}: {len(authority)} .cpp in CMake authority ==")
        for label, other in others:
            missing = sorted(authority - other)   # in CMake, not in this build file -> CI will skip it
            extra = sorted(other - authority)      # in this build file, not in CMake -> stale/invented
            if not missing and not extra:
                print(f"   OK  {label}  ({len(other)} .cpp)")
                if verbose:
                    pass
                continue
            ok = False
            print(f"  DRIFT {label}  ({len(other)} .cpp)")
            for m in missing:
                print(f"        MISSING (add to {os.path.basename(label)}): {m}")
            for e in extra:
                print(f"        EXTRA   (in {os.path.basename(label)}, not CMake): {e}")
    if ok:
        print("\nAll source lists agree with the CMake authority.")
        return 0
    print("\nSource lists DRIFTED — sync the legacy Makefile / vcxproj to dlls|cl_dll/CMakeLists.txt.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
