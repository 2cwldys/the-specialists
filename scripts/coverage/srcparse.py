#!/usr/bin/env python3
"""
srcparse.py — a brace-aware scanner of our dlls/ + cl_dll/ sources. Unlike the old header-only
member scan in audit.py, this walks BOTH .cpp and .h with real brace tracking, so it sees:
  - members of classes declared in a .cpp (CFuncPlat/CBaseDoor/CMultiManager/...), not just headers;
  - the class -> base hierarchy (to resolve inherited methods);
  - inline methods defined inside the class body (which carry no out-of-line nm symbol);
  - free functions defined at file scope.
Used by audit.py to make MISSING/INVENTED trustworthy (no .cpp / inline / inheritance false positives).

Run standalone to inspect one class:  python3 scripts/coverage/srcparse.py CBaseDoor
"""
import os, re, sys
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC_DIRS = [os.path.join(ROOT, "dlls"), os.path.join(ROOT, "cl_dll")]

CLASSRE = re.compile(r"\b(?:class|struct)\s+([A-Za-z_]\w*)\s*(?::\s*(?:public|private|protected|virtual|\s)+([A-Za-z_]\w*))?")
# a member declaration line at class scope: <type...> name [array] (= | ; | {)
DECLRE = re.compile(r"^\s*(?:static\s+|const\s+|mutable\s+|volatile\s+)*[A-Za-z_][\w:<>,\s\*&]*?\b(m_[A-Za-z0-9_]+|[a-z][a-z0-9_]+)\s*(?:\[[^\]]*\])?\s*(?:=|;|\{)")
# a pointer-to-member data member:  <ret> (Class::*name)(...)  or  <ret> (*name)(...)  — the name is
# buried in parens so DECLRE/INLINE_METHODRE both miss it (function-pointer save fields like m_pfnThink).
PMEMBERRE = re.compile(r"\(\s*(?:[A-Za-z_]\w*\s*::\s*)?\*\s*(m_[A-Za-z0-9_]+|[a-z][a-z0-9_]+)\s*\)\s*\(")
# an inline method declared/defined in the class body:  <stuff> name( ... ) ... ; or {
INLINE_METHODRE = re.compile(r"^\s*(?:virtual\s+|static\s+|inline\s+|explicit\s+)*[A-Za-z_][\w:<>,\s\*&]*?\b(~?[A-Za-z_]\w*)\s*\(")
# a free function definition at file scope:  <type> name( ... ) {  (definition, not a call)
FREEDEFRE = re.compile(r"^[A-Za-z_][\w:<>,\s\*&]*?\b([A-Za-z_]\w*)\s*\([^;]*\)\s*(?:const)?\s*\{")
TYPEWORD = {"int","bool","char","float","double","void","short","long","unsigned","string_t",
            "vec_t","byte","entvars_t","return","else","case","public","private","protected",
            "virtual","static","const","struct","class","enum","typedef","using","friend","Vector"}


def strip_comments(txt):
    """Single-pass tokenizer: blank out line comments, block comments, and string/char contents
    (keeping the delimiters), preserving newlines so braces never desync. A naive regex leaks quotes
    from // comments across newlines and eats real code — this state machine does not."""
    out = []
    i, n, state = 0, len(txt), "code"
    while i < n:
        c = txt[i]
        nxt = txt[i + 1] if i + 1 < n else ""
        if state == "code":
            if c == "/" and nxt == "/":
                state = "line"; i += 2; continue
            if c == "/" and nxt == "*":
                state = "block"; i += 2; continue
            if c == '"':
                state = "string"; out.append('"'); i += 1; continue
            if c == "'":
                state = "char"; out.append("'"); i += 1; continue
            out.append(c); i += 1
        elif state == "line":
            if c == "\n":
                state = "code"; out.append("\n")
            i += 1
        elif state == "block":
            if c == "*" and nxt == "/":
                state = "code"; i += 2; continue
            if c == "\n":
                out.append("\n")
            i += 1
        elif state == "string":
            if c == "\\":
                out.append(" "); i += 2; continue
            if c == '"':
                out.append('"'); state = "code"; i += 1; continue
            out.append("\n" if c == "\n" else " "); i += 1
        elif state == "char":
            if c == "\\":
                out.append(" "); i += 2; continue
            if c == "'":
                out.append("'"); state = "code"; i += 1; continue
            out.append(" "); i += 1
    return "".join(out)


def parse():
    members = defaultdict(set)      # class -> set(member name)
    hierarchy = {}                  # class -> base class
    inline_methods = defaultdict(set)  # class -> set(method name defined inline)
    decl_file = {}                  # class -> relative file it is declared in
    free_funcs = defaultdict(set)   # relfile -> set(free function names defined there)
    for d in SRC_DIRS:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not (fn.endswith(".cpp") or fn.endswith(".h")):
                continue
            rel = os.path.relpath(os.path.join(d, fn), ROOT)
            txt = strip_comments(open(os.path.join(d, fn), errors="replace").read())
            depth = 0
            stack = []         # (class, depth_at_open)
            pending = None      # (class, base) seen, awaiting its opening brace
            for code in txt.splitlines():
                code = code.split("//")[0]
                start_depth = depth
                # capture at the immediate body scope of the innermost class
                if stack and start_depth == stack[-1][1] + 1:
                    cls = stack[-1][0]
                    if "(" not in code:
                        dm = DECLRE.match(code)
                        if dm and dm.group(1) not in TYPEWORD:
                            members[cls].add(dm.group(1))
                    else:
                        pm = PMEMBERRE.search(code)
                        if pm and pm.group(1) not in TYPEWORD:
                            members[cls].add(pm.group(1))
                        else:
                            im = INLINE_METHODRE.match(code)
                            if im and im.group(1) not in TYPEWORD:
                                inline_methods[cls].add(im.group(1))
                elif not stack and start_depth == 0 and "(" in code and "{" in code:
                    fm = FREEDEFRE.match(code)
                    if fm and fm.group(1) not in TYPEWORD:
                        free_funcs[rel].add(fm.group(1))
                # note a class/struct declaration (the latest one wins until its '{' or ';')
                cm = CLASSRE.search(code)
                if cm:
                    pending = (cm.group(1), cm.group(2))
                # walk braces / semicolons
                for ch in code:
                    if ch == ";" and pending is not None:
                        pending = None  # forward declaration / class-typed decl — never opened a body
                    elif ch == "{":
                        if pending is not None:
                            stack.append((pending[0], depth))
                            if pending[1]:
                                hierarchy[pending[0]] = pending[1]
                            decl_file.setdefault(pending[0], rel)
                            pending = None
                        depth += 1
                    elif ch == "}":
                        depth -= 1
                        if stack and depth == stack[-1][1]:
                            stack.pop()
    return {"members": members, "hierarchy": hierarchy, "inline_methods": inline_methods,
            "decl_file": decl_file, "free_funcs": free_funcs}


if __name__ == "__main__":
    r = parse()
    if len(sys.argv) > 1:
        c = sys.argv[1]
        print(f"{c}: declared in {r['decl_file'].get(c, '?')}, base = {r['hierarchy'].get(c, '?')}")
        print(f"  members ({len(r['members'].get(c, set()))}): {sorted(r['members'].get(c, set()))}")
        print(f"  inline methods ({len(r['inline_methods'].get(c, set()))}): {sorted(r['inline_methods'].get(c, set()))}")
    else:
        print(f"classes: {len(r['decl_file'])}, with members: {len(r['members'])}, "
              f"hierarchy edges: {len(r['hierarchy'])}, free-func files: {len(r['free_funcs'])}")
