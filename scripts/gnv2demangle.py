#!/usr/bin/env python3
"""gnv2demangle.py — demangle GNU g++ 2.x ("gnu-v2") C++ symbol names.

Modern binutils c++filt dropped this dialect, but the original ts_i386.so was
built with gcc-2.96, so its 2426 symbols carry full type info we can recover
(class, method, parameter types) — for ALL functions, not just the 159 that
mp.dll exports. Scope: the subset the HL SDK actually uses (no templates).

Reads symbols on stdin (one per line, optionally "addr  name"), prints
"name  =>  demangled" on stdout. Pass --sig to print just the demangled form.

Grammar handled: member funcs (meth__<len>Class<args>), global funcs
(func__F<args>), constructors (__<len>Class<args>), destructors (_$_<len>Class),
type prefixes P/R/C/V/U/S, builtins, <len>Name classes, and T<n>/N<count><n>
backrefs (indexed as libiberty cplus-dem does; checked against mp.dll's MSVC signatures).
"""
import sys, re

BUILTIN = {'v':'void','i':'int','l':'long','s':'short','c':'char','w':'wchar_t',
           'b':'bool','f':'float','d':'double','r':'long double','x':'long long',
           'e':'...'}

class Fail(Exception): pass

def get_count(s, i):
    """libiberty cplus-dem get_count: one digit, unless a longer digit run is closed by '_'
    (so T18USE_TYPE is T1 + 8USE_TYPE, while T12_ is T12)."""
    if i >= len(s) or not s[i].isdigit(): raise Fail('count')
    j = i + 1
    while j < len(s) and s[j].isdigit(): j += 1
    if j > i + 1 and j < len(s) and s[j] == '_':
        return int(s[i:j]), j + 1
    return int(s[i]), i + 1

def parse_type(s, i, seen):
    """Parse one type starting at s[i]; return (type_string, new_i)."""
    pre = []
    # 'G' marks a class type passed by value — consume, no effect on the type text
    while i < len(s) and s[i] == 'G':
        i += 1
    # qualifier/pointer prefixes (outermost first in mangling)
    while i < len(s) and s[i] in 'PRCVUSA':
        c = s[i]
        if c == 'P': pre.append('*')
        elif c == 'R': pre.append('&')
        elif c == 'C': pre.append('const')
        elif c == 'V': pre.append('volatile')
        elif c == 'U': pre.append('unsigned')
        elif c == 'S': pre.append('signed')
        elif c == 'A':
            # array Ann_  — rare in HL; bail to keep things honest
            raise Fail('array')
        i += 1
    if i >= len(s): raise Fail('eof')
    ch = s[i]
    if ch == 'T':                      # backref T<n>: same type as entry n of the type list
        # The list starts with the class for member functions and constructors (so T1 is the first
        # argument there) and is empty for free functions (T0 is the first argument). That resolves
        # CTSKnife::CreateKnife's T2 (= Vector) and AddToFullPack's T2 (= edict_s *) alike.
        k, i = get_count(s, i + 1)
        if not 0 <= k < len(seen): raise Fail('T idx')
        t = seen[k]
        seen.append(t)
        return t, i
    elif ch.isdigit():                 # <len><name> class
        n = ''
        while i < len(s) and s[i].isdigit(): n += s[i]; i += 1
        ln = int(n)
        base = s[i:i+ln]; i += ln
        if len(base) != ln: raise Fail('name')
    elif ch == 'Q':                    # nested name Q<count><len><name>...
        i += 1
        cnt = int(s[i]); i += 1
        parts = []
        for _ in range(cnt):
            n = ''
            while i < len(s) and s[i].isdigit(): n += s[i]; i += 1
            ln = int(n); parts.append(s[i:i+ln]); i += ln
        base = '::'.join(parts)
    elif ch in BUILTIN:
        base = BUILTIN[ch]; i += 1
    else:
        raise Fail('type %r' % ch)
    # assemble: handle const/unsigned as part of base, * and & as suffixes
    unsigned = 'unsigned' in pre or 'signed' in pre
    if unsigned:
        base = ('unsigned ' if 'unsigned' in pre else 'signed ') + base
    if 'const' in pre and ('*' in pre or '&' in pre):
        base = 'const ' + base       # pointer/ref to const
    elif 'const' in pre:
        base = 'const ' + base
    suff = ''.join(' *' if p=='*' else ' &' for p in pre if p in ('*','&'))
    t = (base + suff).strip()
    seen.append(t)
    return t, i

def parse_args(s, i, cls=None):
    """cls: the owning class for member functions/constructors; it is entry 0 of the type list."""
    seen = [cls] if cls else []
    args = []
    while i < len(s):
        if s[i] == '_':              # return-type marker (rare); stop
            break
        if s[i] == 'N':              # N<count><index>: repeat type-list entry <index> <count> times
            count, i = get_count(s, i + 1)
            idx, i = get_count(s, i)
            if not 0 <= idx < len(seen): raise Fail('N idx')
            args.extend([seen[idx]] * count)
            seen.extend([seen[idx]] * count)
            continue
        t, i = parse_type(s, i, seen)
        args.append(t)
    if args == ['void']: args = []
    return args

def demangle(sym):
    sym = sym.strip()
    # destructor _$_<len>Class  (note: contains no '__', so check before the guard)
    m = re.match(r'_[$.]_(\d+)(.+)$', sym)   # gcc2 writes both _$_ and _._
    if m:
        cls = m.group(2)[:int(m.group(1))]
        return '%s::~%s()' % (cls, cls)
    if '__' not in sym: raise Fail('no __')
    # split on the LAST '__' that begins the signature
    # member:  name__<len>Class<args>   global: name__F<args>   ctor: __<len>Class<args>
    idx = sym.find('__')
    # constructor: starts with __
    if idx == 0:
        rest = sym[2:]
        mm = re.match(r'(\d+)(.+)$', rest)
        if not mm: raise Fail('ctor')
        ln = int(mm.group(1)); cls = rest[len(mm.group(1)):len(mm.group(1))+ln]
        argstr = rest[len(mm.group(1))+ln:]
        args = parse_args(argstr, 0, cls)
        return '%s::%s(%s)' % (cls, cls, ', '.join(args))
    name = sym[:idx]; rest = sym[idx+2:]
    if rest.startswith('F'):                 # global function
        args = parse_args(rest[1:], 0)
        return '%s(%s)' % (name, ', '.join(args))
    m = re.match(r'(\d+)', rest)             # member function
    if m:
        ln = int(m.group(1)); cls = rest[len(m.group(1)):len(m.group(1))+ln]
        argstr = rest[len(m.group(1))+ln:]
        args = parse_args(argstr, 0, cls)
        return '%s::%s(%s)' % (cls, name, ', '.join(args))
    raise Fail('shape')

def main():
    sigonly = '--sig' in sys.argv[1:]
    ok = bad = 0
    for line in sys.stdin:
        line = line.rstrip('\n')
        if not line.strip(): continue
        parts = line.split()
        sym = parts[-1]
        try:
            d = demangle(sym); ok += 1
        except Fail:
            d = None; bad += 1
        if sigonly:
            if d: print(d)
        else:
            print('%s  =>  %s' % (sym, d if d else '(unparsed)'))
    sys.stderr.write('gnv2demangle: ok=%d unparsed=%d\n' % (ok, bad))

if __name__ == '__main__':
    main()
