// ApplyWinTypes.java — varargs signatures + typed engine tables for TS 3.0's client.dll and mp.dll.
//
// Why: both DLLs link the 2007 MSVC CRT statically and Ghidra's Function ID databases only know
// VS2012-2019, so sprintf & co. come out as FUN_ with a fixed two-parameter guess and the decompiler
// drops every argument after the format (client.dll 0x10001000 read `RandomLong(0,4); sprintf(buf, fmt)`
// for `sprintf(buf, "{shot%i", RandomLong(0, 4) + 1)`). The engine tables were untyped too
// (`(*DAT_100d5c38)(0,4)`). This script fixes both:
//
//  1. Variadic functions, found from their callers: every PUSH of a pointer to a string literal holding a
//     printf conversion is followed to its CALL; the push position is the format's parameter index. A
//     callee gets `f(p0, ..., const char *fmt, ...)` only when its own body takes the address of the
//     argument just past the format (va_start), so a function that merely receives a "%s" string is left
//     alone. The 2007 CRT entry points are then named from their bodies (string FILE flag 0x42, unbounded
//     count 0x7fffffff, read flag 0x49): sprintf, _snprintf, sscanf, and the non-variadic vsprintf and
//     _vsnprintf (same _output call, va_list passed in). A variadic function that formats with vsprintf/
//     _vsnprintf and hands the text to one engine-table function is named <member>_va (Con_Printf_va,
//     pfnAlertMessage_va, ...). Everything else keeps its FUN_ name with the new type.
//  2. Engine tables, typed from the SDK headers in this repo (engine/, common/, pm_shared/): the client's
//     gEngfuncs (cl_enginefunc_t, the copy Initialize makes), IEngineStudio (engine_studio_api_t, the copy
//     HUD_GetStudioModelInterface makes) and pmove (playermove_t *, set by PM_Init); mp.dll's g_engfuncs
//     (enginefuncs_t, GiveFnptrsToDll's copy), gpGlobals (globalvars_t *), gFunctionTable (DLL_FUNCTIONS,
//     what GetEntityAPI2 hands out) and pmove. Each table is cut to the size the binary copies (the 2007
//     engine's tables are shorter than halflife-updated's). The sub-tables reached through gEngfuncs
//     (pTriAPI, pEfxAPI, pEventAPI, pDemoAPI, pNetAPI, pVoiceTweak) are typed by the same header parse.
//
// Addresses are found from the exports (Initialize, HUD_GetStudioModelInterface, HUD_PlayerMoveInit,
// GiveFnptrsToDll, GetEntityAPI2), not hard-coded. Idempotent: types are replaced, signatures re-applied,
// and only FUN_ names (or the name this script gave) are changed, so it can be re-run on a project.
//
//   analyzeHeadless recovered/ghidra/client clientdll -process client.dll -noanalysis \
//     -scriptPath scripts/ghidra -postScript ApplyWinTypes.java <mod-ts root>
// (dc.sh setup client|mp runs it.)
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.pcode.HighFunctionDBUtil;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class ApplyWinTypes extends GhidraScript {
    DataTypeManager dtm;
    CategoryPath CP = new CategoryPath("/sdk");
    Listing listing;
    Memory mem;

    // ================================================================ SDK header parse
    Map<String, Long> defines = new HashMap<>();
    Map<String, String> structBody = new HashMap<>();     // canonical name -> body
    Map<String, Boolean> structIsUnion = new HashMap<>();
    Map<String, String> structCanon = new HashMap<>();    // tag or alias -> canonical name
    Map<String, String> aliases = new HashMap<>();        // typedef name -> type text
    Map<String, String[]> fnTypedefs = new HashMap<>();   // typedef name -> {ret, args}
    Set<String> enums = new HashSet<>();
    Map<String, DataType> built = new HashMap<>();
    Map<String, Integer> limits = new HashMap<>();        // struct -> byte limit (tables the binary copies short)
    Set<String> warned = new TreeSet<>();

    static final Set<String> PRIM_WORDS = new HashSet<>(Arrays.asList(
        "void", "bool", "char", "short", "int", "long", "float", "double", "unsigned", "signed"));

    void loadHeader(File f) throws IOException {
        String t = new String(Files.readAllBytes(f.toPath()), "ISO-8859-1");
        t = t.replaceAll("(?s)/\\*.*?\\*/", " ").replaceAll("//[^\\n]*", "").replace("\\\r\n", " ").replace("\\\n", " ");
        StringBuilder code = new StringBuilder();
        Pattern DEF = Pattern.compile("^\\s*#\\s*define\\s+(\\w+)\\s+\\(?\\s*(-?(?:0x[0-9a-fA-F]+|\\d+))[uUlL]*\\s*\\)?\\s*$");
        for (String ln : t.split("\n")) {
            if (ln.trim().startsWith("#")) {
                Matcher m = DEF.matcher(ln);
                if (m.matches()) defines.putIfAbsent(m.group(1), Long.decode(m.group(2)));
                continue;
            }
            code.append(ln).append('\n');
        }
        for (String st : statements(code.toString())) classify(st.replaceAll("\\s+", " ").trim());
    }

    // split at ';' on brace depth 0; a function body (`...) {...}`) ends its statement at the closing brace
    List<String> statements(String s) {
        List<String> out = new ArrayList<>();
        int depth = 0, start = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) {
                    String sofar = s.substring(start, i + 1);
                    int b = sofar.indexOf('{');
                    String head = sofar.substring(0, b).trim();
                    if (head.endsWith(")") || head.endsWith("const") || head.matches(".*\\b(namespace|extern \"C\")\\b.*")) {
                        out.add(sofar); start = i + 1;
                    }
                }
            } else if (c == ';' && depth == 0) { out.add(s.substring(start, i)); start = i + 1; }
        }
        return out;
    }

    void classify(String s) {
        if (s.isEmpty()) return;
        Matcher cm = Pattern.compile("^(?:static\\s+)?(?:constexpr|const)\\s+(?:unsigned\\s+)?(?:int|long|short|size_t)\\s+(\\w+)\\s*=\\s*\\(?\\s*(-?(?:0x[0-9a-fA-F]+|\\d+))[uUlL]*\\s*\\)?$").matcher(s);
        if (cm.matches()) { defines.putIfAbsent(cm.group(1), Long.decode(cm.group(2))); return; }
        int b = s.indexOf('{');
        if (b >= 0) {
            int e = s.lastIndexOf('}');
            if (e < b) return;
            String head = s.substring(0, b).trim(), body = s.substring(b + 1, e), tail = s.substring(e + 1).trim();
            Matcher m = Pattern.compile("^(typedef\\s+)?(struct|union)(?:\\s+(\\w+))?$").matcher(head);
            if (m.matches()) {
                String tag = m.group(3);
                List<String> names = new ArrayList<>();
                if (tag != null) names.add(tag);
                List<String> ptrAliases = new ArrayList<>();
                for (String a : tail.split(",")) {
                    a = a.trim(); if (a.isEmpty()) continue;
                    if (a.startsWith("*")) ptrAliases.add(a.replace("*", "").trim()); else if (a.matches("\\w+")) names.add(a);
                }
                if (names.isEmpty()) return;
                String canon = names.size() > 1 ? names.get(1) : names.get(0);   // prefer the typedef name
                if (structBody.containsKey(canon)) return;                        // first definition wins
                structBody.put(canon, body);
                structIsUnion.put(canon, m.group(2).equals("union"));
                for (String n : names) structCanon.putIfAbsent(n, canon);
                for (String p : ptrAliases) aliases.putIfAbsent(p, canon + "*");
                return;
            }
            m = Pattern.compile("^(typedef\\s+)?enum(?:\\s+(\\w+))?(?:\\s*:\\s*[\\w ]+)?$").matcher(head);
            if (m.matches()) {
                if (m.group(2) != null) enums.add(m.group(2));
                for (String a : tail.split(",")) if (a.trim().matches("\\w+")) enums.add(a.trim());
            }
            return;   // class / namespace / function body
        }
        if (!s.startsWith("typedef ")) return;
        String d = s.substring(8).trim();
        Matcher m = Pattern.compile("^(.+?)\\(\\s*\\*\\s*(\\w+)\\s*\\)\\s*\\((.*)\\)$").matcher(d);
        if (m.matches()) { fnTypedefs.putIfAbsent(m.group(2), new String[]{m.group(1).trim(), m.group(3)}); return; }
        if (d.contains("(")) return;
        m = Pattern.compile("^(.+?)\\s*(\\**)\\s*(\\w+)\\s*((?:\\[[^\\]]*\\]\\s*)*)$").matcher(d);
        if (m.matches()) {
            String name = m.group(3), base = m.group(1).trim() + m.group(2) + m.group(4).replace(" ", "");
            if (base.matches("(struct|union)\\s+" + name) || base.equals(name)) return;   // typedef struct X X;
            Matcher e = Pattern.compile("^enum\\s+(\\w+)$").matcher(base);
            if (e.matches()) { enums.add(name); return; }
            aliases.putIfAbsent(name, base);
        }
    }

    long evalDim(String s) {
        s = s.trim();
        if (s.isEmpty()) return 0;
        try { return Long.decode(s); } catch (Exception ig) {}
        if (defines.containsKey(s)) return defines.get(s);
        Matcher m = Pattern.compile("^(.+?)\\s*([-+*])\\s*(.+)$").matcher(s);
        if (m.matches()) {
            long a = evalDim(m.group(1)), c = evalDim(m.group(3));
            switch (m.group(2)) { case "+": return a + c; case "-": return a - c; default: return a * c; }
        }
        warned.add("array size '" + s + "' unknown, taken as 1");
        return 1;
    }

    DataType prim(String t) {
        switch (t) {
            case "void": return VoidDataType.dataType;
            case "bool": return BooleanDataType.dataType;
            case "char": case "signed char": return CharDataType.dataType;
            case "unsigned char": case "byte": case "uint8": case "BYTE": return UnsignedCharDataType.dataType;
            case "int8": return SignedByteDataType.dataType;
            case "short": case "short int": case "signed short": case "int16": return ShortDataType.dataType;
            case "unsigned short": case "unsigned short int": case "word": case "uint16": case "WORD": return UnsignedShortDataType.dataType;
            case "int": case "signed int": case "signed": case "long": case "long int": case "int32": case "qboolean":
            case "BOOL": case "string_t": case "HSPRITE": case "func_t": case "intp":
                return IntegerDataType.dataType;
            case "unsigned int": case "unsigned": case "unsigned long": case "unsigned long int": case "uint32":
            case "size_t": case "CRC32_t": case "uintp": case "DWORD":
                return UnsignedIntegerDataType.dataType;
            case "long long": case "__int64": case "int64": return LongLongDataType.dataType;
            case "unsigned long long": case "unsigned __int64": case "uint64": return UnsignedLongLongDataType.dataType;
            case "float": case "vec_t": return FloatDataType.dataType;
            case "double": case "long double": return DoubleDataType.dataType;
            case "va_list": return dtm.getPointer(CharDataType.dataType);
            default: return null;
        }
    }

    DataType vector() {
        if (built.containsKey("Vector")) return built.get("Vector");
        StructureDataType v = new StructureDataType(CP, "Vector", 0, dtm);
        v.setPackingEnabled(true);
        v.add(FloatDataType.dataType, 4, "x", null);
        v.add(FloatDataType.dataType, 4, "y", null);
        v.add(FloatDataType.dataType, 4, "z", null);
        DataType r = dtm.addDataType(v, DataTypeConflictHandler.REPLACE_HANDLER);
        built.put("Vector", r);
        return r;
    }

    DataType resolveType(String t) {
        t = t.trim();
        List<Long> dims = new ArrayList<>();
        Matcher am = Pattern.compile("\\[([^\\]]*)\\]\\s*$").matcher(t);
        while (am.find()) { dims.add(0, evalDim(am.group(1))); t = t.substring(0, am.start()).trim(); am = Pattern.compile("\\[([^\\]]*)\\]\\s*$").matcher(t); }
        int stars = 0;
        while (t.endsWith("*") || t.endsWith("&") || t.endsWith("const")) {
            if (t.endsWith("const")) t = t.substring(0, t.length() - 5).trim();
            else { stars++; t = t.substring(0, t.length() - 1).trim(); }
        }
        t = t.replaceAll("\\b(const|volatile|struct|union|enum|class|typename)\\b", " ").replaceAll("\\s+", " ").trim();
        DataType base = baseType(t);
        for (int i = 0; i < stars; i++) base = dtm.getPointer(base);
        for (int i = dims.size() - 1; i >= 0; i--) {
            long n = dims.get(i);
            if (n <= 0 || base.getLength() <= 0) { base = dtm.getPointer(base); continue; }
            base = new ArrayDataType(base, (int) n, base.getLength(), dtm);
        }
        return base;
    }

    DataType baseType(String t) {
        DataType p = prim(t);
        if (p != null) return p;
        if (t.equals("Vector") || t.equals("vec3_t")) return vector();
        if (t.isEmpty()) return VoidDataType.dataType;
        if (enums.contains(t)) return IntegerDataType.dataType;
        if (fnTypedefs.containsKey(t)) {
            String[] f = fnTypedefs.get(t);
            return dtm.getPointer(fnDef(t, f[0], f[1]));
        }
        if (structCanon.containsKey(t)) return buildStruct(structCanon.get(t));
        if (aliases.containsKey(t)) {
            String a = aliases.get(t);
            if (!a.equals(t)) return resolveType(a);
        }
        return opaque(t);
    }

    DataType opaque(String name) {
        if (built.containsKey(name)) return built.get(name);
        DataType r = dtm.addDataType(new StructureDataType(CP, name, 0, dtm), DataTypeConflictHandler.KEEP_HANDLER);
        built.put(name, r);
        return r;
    }

    List<String> splitTop(String s, char sep) {
        List<String> out = new ArrayList<>();
        int depth = 0, start = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '(' || c == '{' || c == '[' || c == '<') depth++;
            else if (c == ')' || c == '}' || c == ']' || c == '>') depth--;
            else if (c == sep && depth == 0) { out.add(s.substring(start, i)); start = i + 1; }
        }
        out.add(s.substring(start));
        return out;
    }

    static final Pattern FNPTR = Pattern.compile("(?s)^(.+?)\\(\\s*\\*\\s*(\\w*)\\s*\\)\\s*\\((.*)\\)$");
    static final Pattern DECL = Pattern.compile("^(.*?)(\\**)\\s*(\\w+)\\s*((?:\\[[^\\]]*\\]\\s*)*)$");

    FunctionDefinitionDataType fnDefRaw(String name, String ret, String args) {
        FunctionDefinitionDataType fd = new FunctionDefinitionDataType(CP, name, dtm);
        fd.setReturnType(resolveType(ret));
        List<ParameterDefinition> ps = new ArrayList<>();
        String a = args.trim();
        if (!a.isEmpty() && !a.equals("void")) {
            int n = 0;
            for (String arg : splitTop(a, ',')) {
                arg = arg.trim().replaceAll("\\s*=.*$", "");
                if (arg.equals("...")) { fd.setVarArgs(true); continue; }
                if (arg.isEmpty()) continue;
                Matcher fm = FNPTR.matcher(arg);
                if (fm.matches()) {
                    String pn = fm.group(2).isEmpty() ? "callback" : fm.group(2);
                    ps.add(new ParameterDefinitionImpl(pn, dtm.getPointer(fnDef(name + "__" + pn, fm.group(1), fm.group(3))), null));
                    n++; continue;
                }
                String pname = null, ptype = arg;
                Matcher dm = DECL.matcher(arg);
                if (dm.matches()) {
                    String id = dm.group(3), pre = dm.group(1).trim();
                    boolean isType = PRIM_WORDS.contains(id) || prim(id) != null || pre.isEmpty() || pre.equals("const")
                        || pre.matches("(struct|union|enum|class)") || pre.equals("unsigned") || pre.equals("signed");
                    if (!isType) {
                        pname = id;
                        ptype = pre + dm.group(2) + (dm.group(4).isEmpty() ? "" : "*");   // array param = pointer
                    }
                }
                DataType pt = resolveType(ptype);
                if (pt instanceof VoidDataType) continue;
                ps.add(new ParameterDefinitionImpl(pname != null ? pname : "p" + n, pt, null));
                n++;
            }
        }
        fd.setArguments(ps.toArray(new ParameterDefinition[0]));
        try { fd.setCallingConvention("__cdecl"); } catch (Throwable ig) {}
        return fd;
    }

    DataType fnDef(String name, String ret, String args) {
        String key = "fn:" + name;
        if (built.containsKey(key)) return built.get(key);
        DataType r = dtm.addDataType(fnDefRaw(name, ret, args), DataTypeConflictHandler.REPLACE_HANDLER);
        built.put(key, r);
        return r;
    }

    DataType buildStruct(String canon) {
        if (built.containsKey(canon)) return built.get(canon);
        String body = structBody.get(canon);
        boolean union = Boolean.TRUE.equals(structIsUnion.get(canon));
        Composite shell = union ? new UnionDataType(CP, canon, dtm) : new StructureDataType(CP, canon, 0, dtm);
        Composite c = (Composite) dtm.addDataType(shell, DataTypeConflictHandler.REPLACE_HANDLER);
        built.put(canon, c);
        // remove whatever a previous run left in the replaced type
        if (c instanceof Structure) ((Structure) c).deleteAll();
        else while (c.getNumComponents() > 0) c.delete(0);
        c.setPackingEnabled(true);
        int limit = limits.getOrDefault(canon, Integer.MAX_VALUE);
        for (String decl : splitTop(body, ';')) {
            decl = decl.trim();
            if (decl.isEmpty()) continue;
            if (decl.contains("{")) { warned.add(canon + ": nested aggregate member skipped: " + decl); continue; }
            if (decl.matches("^(static|typedef|using|friend|public:|private:|protected:)\\b.*") || decl.contains("operator")) continue;
            if (c.getLength() >= limit && !union) break;
            Matcher fm = FNPTR.matcher(decl);
            if (fm.matches()) {
                DataType fp = dtm.getPointer(fnDef(canon + "__" + fm.group(2), fm.group(1), fm.group(3)));
                c.add(fp, fp.getLength(), fm.group(2), null);
                continue;
            }
            if (decl.contains("(")) continue;   // member function
            decl = decl.replaceAll("\\s*=[^,]*", "").replaceAll("\\s*:\\s*\\d+\\s*$", "");
            List<String> parts = splitTop(decl, ',');
            Matcher dm = DECL.matcher(parts.get(0).trim());
            if (!dm.matches()) { warned.add(canon + ": unparsed member: " + decl); continue; }
            String baseT = dm.group(1).trim();
            List<String> decls = new ArrayList<>();
            decls.add(dm.group(2) + dm.group(3) + dm.group(4));
            for (int i = 1; i < parts.size(); i++) decls.add(parts.get(i).trim());
            for (String d1 : decls) {
                Matcher d2 = Pattern.compile("^(\\**)\\s*(\\w+)\\s*((?:\\[[^\\]]*\\]\\s*)*)$").matcher(d1);
                if (!d2.matches()) { warned.add(canon + ": unparsed declarator: " + d1); continue; }
                DataType mt = resolveType(baseT + d2.group(1) + d2.group(3).replace(" ", ""));
                if (mt.getLength() <= 0) {
                    warned.add(canon + "." + d2.group(2) + ": by-value type '" + baseT + "' has no layout (4 bytes assumed)");
                    mt = Undefined4DataType.dataType;
                }
                c.add(mt, mt.getLength(), d2.group(2), null);
            }
        }
        if (limit != Integer.MAX_VALUE)
            c.setDescription("cut to the " + limit + " bytes the TS 3.0 binary copies (the halflife-updated header is longer)");
        return c;
    }

    // ================================================================ program helpers
    // Resolve an export from the PE export directory itself: symbol names can't be trusted here
    // (ApplyClientThis's binmap renamed client.dll's HUD_GetStudioModelInterface export to CHudGeiger::Init).
    Map<String, Long> peExports = null;
    Map<String, Long> peExports() {
        if (peExports != null) return peExports;
        peExports = new HashMap<>();
        try {
            Address ib = currentProgram.getImageBase();
            int pe = mem.getInt(ib.add(0x3c));
            long edir = mem.getInt(ib.add(pe + 0x78)) & 0xffffffffL;
            if (edir == 0) return peExports;
            Address ed = ib.add(edir);
            int nNames = mem.getInt(ed.add(0x18));
            long funcs = mem.getInt(ed.add(0x1c)) & 0xffffffffL, names = mem.getInt(ed.add(0x20)) & 0xffffffffL,
                 ords = mem.getInt(ed.add(0x24)) & 0xffffffffL;
            for (int k = 0; k < nNames; k++) {
                long nr = mem.getInt(ib.add(names + 4L * k)) & 0xffffffffL;
                int ord = mem.getShort(ib.add(ords + 2L * k)) & 0xffff;
                long fr = mem.getInt(ib.add(funcs + 4L * ord)) & 0xffffffffL;
                StringBuilder sb = new StringBuilder();
                for (int c; (c = mem.getByte(ib.add(nr + sb.length())) & 0xff) != 0 && sb.length() < 256; ) sb.append((char) c);
                peExports.put(sb.toString(), ib.getOffset() + fr);
            }
        } catch (Exception e) { println("ApplyWinTypes: PE export directory unreadable (" + e + "), using symbols"); }
        return peExports;
    }

    Function exported(String name) {
        Long a = peExports().get(name);
        if (a != null) {
            Function f = getFunctionAt(toAddr(a));
            if (f == null) f = getFunctionContaining(toAddr(a));
            if (f != null) return f;
        }
        for (Symbol s : currentProgram.getSymbolTable().getSymbols(name)) {
            Function f = getFunctionAt(s.getAddress());
            if (f != null) return f;
        }
        return null;
    }

    List<Instruction> linear(Address a, int max) {
        List<Instruction> out = new ArrayList<>();
        Instruction i = getInstructionAt(a);
        Function f = getFunctionContaining(a);
        while (i != null && out.size() < max) {
            if (f != null && !f.getBody().contains(i.getAddress())) break;
            out.add(i);
            if (f == null && i.getFlowType().isTerminal()) break;
            i = i.getNext();
        }
        return out;
    }

    // dest/count/src of the first `mov edi,imm / mov ecx,imm / mov esi,imm ... rep movsd` in a function
    long[] repMovs(Function f) {
        long dst = -1, cnt = -1, src = -1;
        for (Instruction i : linear(f.getEntryPoint(), 40)) {
            String m = i.getMnemonicString();
            if (m.equals("MOV") && i.getRegister(0) != null && i.getScalar(1) != null) {
                String r = i.getRegister(0).getName();
                long v = i.getScalar(1).getUnsignedValue();
                if (r.equals("EDI")) dst = v; else if (r.equals("ECX")) cnt = v; else if (r.equals("ESI")) src = v;
            }
            if (m.startsWith("MOVSD") && (dst >= 0 || src >= 0) && cnt >= 0) return new long[]{dst, cnt, src};
        }
        return null;
    }

    Address firstWrite(Function f, boolean afterMovs) {
        boolean seen = !afterMovs;
        for (Instruction i : linear(f.getEntryPoint(), 40)) {
            if (i.getMnemonicString().startsWith("MOVSD")) seen = true;
            if (!seen || !i.getMnemonicString().equals("MOV")) continue;
            for (Reference r : i.getReferencesFrom())
                if (r.getReferenceType().isWrite() && r.getToAddress().isMemoryAddress()) return r.getToAddress();
        }
        return null;
    }

    Function firstCallee(Function f) {
        for (Instruction i : linear(f.getEntryPoint(), 20))
            if (i.getFlowType().isCall() && i.getFlows().length > 0) return getFunctionAt(i.getFlows()[0]);
        return null;
    }

    Map<String, Address[]> tables = new LinkedHashMap<>();   // label -> {start, end}

    void typeGlobal(Address a, DataType t, String label) throws Exception {
        clearListing(a, a.add(t.getLength() - 1));
        createData(a, t);
        SymbolTable st = currentProgram.getSymbolTable();
        Symbol have = null;
        for (Symbol s : st.getSymbols(a)) if (s.getName().equals(label)) have = s;
        if (have == null) have = st.createLabel(a, label, SourceType.USER_DEFINED);
        have.setPrimary();
        tables.put(label, new Address[]{a, a.add(t.getLength() - 1)});
        println("ApplyWinTypes: " + label + " @ " + a + " : " + t.getName() + " (" + t.getLength() + " bytes)");
    }

    DataType table(String canon, long dwords) {
        limits.put(canon, (int) (dwords * 4));
        DataType t = buildStruct(canon);
        if (t.getLength() != dwords * 4)
            println("ApplyWinTypes: WARNING " + canon + " is " + t.getLength() + " bytes, the binary copies " + dwords * 4);
        return t;
    }

    void typeTables() throws Exception {
        Function init = exported("Initialize"), give = exported("GiveFnptrsToDll");
        if (init != null) {                                                   // client.dll
            long[] r = repMovs(init);
            if (r != null) typeGlobal(toAddr(r[0]), table("cl_enginefunc_t", r[1]), "gEngfuncs");
            Function gsm = exported("HUD_GetStudioModelInterface");
            long[] s = gsm != null ? repMovs(gsm) : null;
            if (s != null) typeGlobal(toAddr(s[0]), table("engine_studio_api_t", s[1]), "IEngineStudio");
            Function pmi = exported("HUD_PlayerMoveInit");
            Function pmInit = pmi != null ? firstCallee(pmi) : null;
            Address pm = pmInit != null ? firstWrite(pmInit, false) : null;
            if (pm != null) {
                typeGlobal(pm, dtm.getPointer(buildStruct(structCanon.getOrDefault("playermove_s", "playermove_s"))), "pmove");
                nameFn(pmInit, "PM_Init");
            }
        }
        if (give != null) {                                                   // mp.dll
            long[] r = repMovs(give);
            if (r != null) typeGlobal(toAddr(r[0]), table("enginefuncs_t", r[1]), "g_engfuncs");
            Address gp = firstWrite(give, true);
            if (gp != null) typeGlobal(gp, dtm.getPointer(buildStruct(structCanon.get("globalvars_t"))), "gpGlobals");
            Function api2 = exported("GetEntityAPI2");
            long[] f = api2 != null ? repMovs(api2) : null;
            if (f != null && f[2] >= 0) {
                Structure df = (Structure) table("DLL_FUNCTIONS", f[1]);
                Address ft = toAddr(f[2]);
                typeGlobal(ft, df, "gFunctionTable");
                for (DataTypeComponent dc : df.getComponents()) {
                    if (!"pfnPM_Init".equals(dc.getFieldName())) continue;
                    Address pa = toAddr(mem.getInt(ft.add(dc.getOffset())) & 0xffffffffL);
                    Function pmInit = getFunctionAt(pa);
                    if (pmInit == null) { disassemble(pa); pmInit = createFunction(pa, null); }
                    Address pm = pmInit != null ? firstWrite(pmInit, false) : null;
                    if (pm == null) println("ApplyWinTypes: pfnPM_Init -> " + pa + " (" + pmInit + "): no pmove store found");
                    if (pm != null) {
                        typeGlobal(pm, dtm.getPointer(buildStruct(structCanon.getOrDefault("playermove_s", "playermove_s"))), "pmove");
                        nameFn(pmInit, "PM_Init");
                    }
                }
            }
        }
    }

    // ================================================================ variadic functions
    static final Pattern FMT = Pattern.compile(
        "%(?:%|[-+#0]*(?:\\d+|\\*)?(?:\\.(?:\\d+|\\*))?(?:hh|h|ll|l|L|I64|I32|w)?[diouxXcsSpfFeEgGn])");

    String cstr(long v) {
        try {
            Address a = toAddr(v);
            MemoryBlock b = mem.getBlock(a);
            if (b == null || !b.isInitialized() || b.isExecute()) return null;
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 1024; i++) {
                int c = mem.getByte(a.add(i)) & 0xff;
                if (c == 0) return sb.length() > 0 ? sb.toString() : null;
                if (c < 0x20 && c != '\n' && c != '\r' && c != '\t') return null;
                sb.append((char) c);
            }
        } catch (Exception ig) {}
        return null;
    }

    static class Site { Address push, call; int pos; String fmt; }
    Map<Function, List<Site>> sitesByCallee = new LinkedHashMap<>();

    // From the push of the format (or the push of a register loaded with it) to the CALL that takes it,
    // counting the argument slots pushed in between. An inner call whose own pushes are popped by an
    // `add esp,N` (cdecl) is stepped over.
    Site follow(Instruction push, String fmt) {
        int pos = 0;
        Instruction i = push.getNext();
        for (int n = 0; i != null && n < 64; n++, i = i.getNext()) {
            String m = i.getMnemonicString();
            if (i.getFlowType().isCall()) {
                Instruction nx = i.getNext();
                if (pos > 0 && nx != null && nx.getMnemonicString().equals("ADD") && nx.getRegister(0) != null
                        && nx.getRegister(0).getName().equals("ESP") && nx.getScalar(1) != null) {
                    int k = (int) (nx.getScalar(1).getValue() / 4);
                    if (k <= pos) { pos -= k; i = nx; continue; }
                }
                Site s = new Site(); s.push = push.getAddress(); s.call = i.getAddress(); s.pos = pos; s.fmt = fmt;
                return s;
            }
            if (i.getFlowType().isJump() || i.getFlowType().isTerminal()) return null;
            if (m.equals("PUSH")) pos++;
            else if (m.equals("POP")) pos--;
            else if ((m.equals("SUB") || m.equals("ADD")) && i.getRegister(0) != null && i.getRegister(0).getName().equals("ESP")
                    && i.getScalar(1) != null) pos += (m.equals("SUB") ? 1 : -1) * (int) (i.getScalar(1).getValue() / 4);
            if (pos < 0) return null;
        }
        return null;
    }

    void findFormatSites() {
        InstructionIterator it = listing.getInstructions(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction ins = it.next();
            String m = ins.getMnemonicString();
            Site s = null;
            if (m.equals("PUSH") && ins.getScalar(0) != null) {
                String f = cstr(ins.getScalar(0).getUnsignedValue());
                if (f != null && FMT.matcher(f).find()) s = follow(ins, f);
            } else if (m.equals("MOV") && ins.getRegister(0) != null && ins.getScalar(1) != null) {
                String f = cstr(ins.getScalar(1).getUnsignedValue());
                if (f != null && FMT.matcher(f).find()) {
                    Register r = ins.getRegister(0);
                    Instruction j = ins.getNext();
                    for (int n = 0; j != null && n < 24; n++, j = j.getNext()) {
                        if (j.getFlowType().isCall() || j.getFlowType().isJump()) break;
                        if (j.getMnemonicString().equals("PUSH") && r.equals(j.getRegister(0))) { s = follow(j, f); break; }
                    }
                }
            }
            if (s == null) continue;
            Instruction call = getInstructionAt(s.call);
            if (call.getFlows().length == 0) continue;               // indirect: engine tables are typed instead
            Function callee = getFunctionAt(call.getFlows()[0]);
            if (callee == null) continue;
            if (callee.isThunk()) callee = callee.getThunkedFunction(true);
            if (callee.isExternal()) continue;
            sitesByCallee.computeIfAbsent(callee, k -> new ArrayList<>()).add(s);
        }
    }

    // Argument index whose address the function takes (va_start): `lea r,[ebp+d]` in an ebp frame or
    // `lea r,[esp+d]` with the pushes so far accounted for. -1 if none.
    Set<Integer> vaStarts(Function f) {
        Set<Integer> out = new HashSet<>();
        int depth = 0, ebpAt = -1;
        long eaxImm = 0;
        for (Instruction i : linear(f.getEntryPoint(), 80)) {
            if (!f.getBody().contains(i.getAddress())) break;
            String m = i.getMnemonicString();
            if (m.equals("LEA")) {
                Object[] o = i.getOpObjects(1);
                if (o.length == 2 && o[0] instanceof Register && o[1] instanceof Scalar) {
                    String r = ((Register) o[0]).getName();
                    long d = ((Scalar) o[1]).getSignedValue();
                    if (r.equals("EBP") && ebpAt >= 0) { long k = d - ebpAt - 4; if (k >= 0 && k % 4 == 0) out.add((int) (k / 4)); }
                    if (r.equals("ESP")) { long k = d - depth - 4; if (k >= 0 && k % 4 == 0) out.add((int) (k / 4)); }
                }
            }
            if (m.equals("PUSH")) depth += 4;
            else if (m.equals("POP")) depth -= 4;
            else if ((m.equals("SUB") || m.equals("ADD")) && i.getRegister(0) != null && i.getRegister(0).getName().equals("ESP") && i.getScalar(1) != null)
                depth += (m.equals("SUB") ? 1 : -1) * (int) i.getScalar(1).getValue();
            else if (m.equals("MOV") && i.getRegister(0) != null && i.getRegister(0).getName().equals("EBP")
                    && i.getRegister(1) != null && i.getRegister(1).getName().equals("ESP")) ebpAt = depth;
            if (m.equals("MOV") && i.getRegister(0) != null && i.getRegister(0).getName().equals("EAX") && i.getScalar(1) != null)
                eaxImm = i.getScalar(1).getValue();
            if (i.getFlowType().isCall() && i.getFlows().length > 0 && chkstk != null && i.getFlows()[0].equals(chkstk.getEntryPoint()))
                depth += (int) eaxImm;   // __chkstk allocates EAX bytes of stack
        }
        return out;
    }

    Set<Long> scalars(Function f) {
        Set<Long> out = new HashSet<>();
        InstructionIterator it = listing.getInstructions(f.getBody(), true);
        while (it.hasNext()) {
            Instruction i = it.next();
            for (int k = 0; k < i.getNumOperands(); k++) {
                Scalar s = i.getScalar(k);
                if (s != null) out.add(s.getUnsignedValue());
                for (Object o : i.getOpObjects(k)) if (o instanceof Scalar) out.add(((Scalar) o).getUnsignedValue());
            }
        }
        return out;
    }

    List<Function> directCallees(Function f) {
        List<Function> out = new ArrayList<>();
        InstructionIterator it = listing.getInstructions(f.getBody(), true);
        while (it.hasNext()) {
            Instruction i = it.next();
            if (!i.getFlowType().isCall() || i.getFlows().length == 0) continue;
            Function g = getFunctionAt(i.getFlows()[0]);
            if (g != null) out.add(g);
        }
        return out;
    }

    // engine-table members called (indirect `call [table+off]` or `mov r,[table+off]; call r`) by f
    List<String> tableCalls(Function f) {
        List<String> out = new ArrayList<>();
        InstructionIterator it = listing.getInstructions(f.getBody(), true);
        while (it.hasNext()) {
            Instruction i = it.next();
            if (!(i.getFlowType().isCall() || i.getMnemonicString().equals("MOV"))) continue;
            for (Reference r : i.getReferencesFrom()) {
                if (!r.getReferenceType().isRead()) continue;
                Data d = getDataContaining(r.getToAddress());
                if (d == null || !tables.containsKey(d.getLabel())) continue;
                Data c = d.getComponentContaining((int) r.getToAddress().subtract(d.getAddress()));
                if (c != null && c.getFieldName() != null) out.add(c.getFieldName());
            }
        }
        return out;
    }

    Map<Function, String> named = new LinkedHashMap<>();
    Function chkstk;

    // MSVC's stack probe (`mov eax,N; call __chkstk` in the prologue of any function with over a page of
    // locals). Unnamed, Ghidra treats it as an ordinary call and loses the stack frame of every caller;
    // named __chkstk with the cspec's alloca_probe call-fixup, the decompiler knows it moves ESP by EAX.
    void findChkstk() {
        Map<Address, Integer> votes = new HashMap<>();
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            List<Instruction> p = linear(f.getEntryPoint(), 2);
            if (p.size() < 2 || !p.get(0).getMnemonicString().equals("MOV") || p.get(0).getRegister(0) == null
                    || !p.get(0).getRegister(0).getName().equals("EAX") || p.get(0).getScalar(1) == null
                    || !p.get(1).getFlowType().isCall() || p.get(1).getFlows().length == 0) continue;
            votes.merge(p.get(1).getFlows()[0], 1, Integer::sum);
        }
        for (Map.Entry<Address, Integer> v : votes.entrySet()) {
            Function g = getFunctionAt(v.getKey());
            if (g == null || g.getBody().getNumAddresses() > 64) continue;
            boolean page = scalars(g).contains(0x1000L), setsp = false;
            InstructionIterator it = listing.getInstructions(g.getBody(), true);
            while (it.hasNext()) {
                Instruction i = it.next();
                if (i.getMnemonicString().equals("MOV") && i.getRegister(0) != null && i.getRegister(0).getName().equals("ESP")
                        && i.getRegister(1) != null && i.getRegister(1).getName().equals("ECX")) setsp = true;
            }
            if (!page || !setsp) continue;
            chkstk = g;
            nameFn(g, "__chkstk");
            try { g.setCallFixup("alloca_probe"); } catch (Exception e) { println("ApplyWinTypes: call-fixup failed: " + e); }
            println("ApplyWinTypes: __chkstk @ " + g.getEntryPoint() + " (" + v.getValue() + " callers), alloca_probe call-fixup set");
            return;
        }
    }

    void nameFn(Function f, String name) {
        try {
            String cur = f.getName();
            if (cur.equals(name)) { named.put(f, name); return; }
            boolean ours = cur.matches("\\w+_va(_\\d+)?");   // a name an earlier run of this script gave
            if (!cur.startsWith("FUN_") && !ours && f.getSymbol().getSource() != SourceType.DEFAULT) {
                println("ApplyWinTypes: left " + cur + " @ " + f.getEntryPoint() + " (would be " + name + ")");
                return;
            }
            f.setName(name, SourceType.USER_DEFINED);
            named.put(f, name);
        } catch (Exception e) { println("ApplyWinTypes: could not name " + f.getEntryPoint() + " " + name + ": " + e); }
    }

    void signature(Function f, DataType ret, boolean varargs, Object[][] params) throws Exception {
        List<Variable> ps = new ArrayList<>();
        for (Object[] p : params) ps.add(new ParameterImpl((String) p[0], (DataType) p[1], currentProgram));
        ReturnParameterImpl rp = ret != null ? new ReturnParameterImpl(ret, currentProgram) : null;
        if (rp == null) {
            DataType cur = f.getReturnType();
            rp = new ReturnParameterImpl(cur, currentProgram);
        }
        f.updateFunction("__cdecl", rp, ps, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
        f.setVarArgs(varargs);
    }

    boolean pushes(Function f, long v) {
        InstructionIterator it = listing.getInstructions(f.getBody(), true);
        while (it.hasNext()) {
            Instruction i = it.next();
            if (i.getMnemonicString().equals("PUSH") && i.getScalar(0) != null && i.getScalar(0).getValue() == v) return true;
        }
        return false;
    }

    // the function's last `mov eax, imm` before RET loads a writable global's address
    boolean returnsGlobal(Function f) {
        long last = -1;
        InstructionIterator it = listing.getInstructions(f.getBody(), true);
        while (it.hasNext()) {
            Instruction i = it.next();
            if (i.getMnemonicString().equals("MOV") && i.getRegister(0) != null && i.getRegister(0).getName().equals("EAX"))
                last = i.getScalar(1) != null ? i.getScalar(1).getUnsignedValue() : -1;
            if (i.getFlowType().isTerminal() && last >= 0) {
                MemoryBlock b = mem.getBlock(toAddr(last));
                return b != null && b.isWrite();
            }
        }
        return false;
    }

    DataType cchar() { return dtm.getPointer(CharDataType.dataType); }

    void applyVariadics() throws Exception {
        findChkstk();
        findFormatSites();
        int verified = 0, rejected = 0, conflicted = 0;
        Function sprintf = null;
        List<Function> variadic = new ArrayList<>();
        Map<Function, Integer> fmtPos = new HashMap<>();
        for (Map.Entry<Function, List<Site>> e : sitesByCallee.entrySet()) {
            Function f = e.getKey();
            Map<Integer, Integer> cnt = new TreeMap<>();
            for (Site s : e.getValue()) cnt.merge(s.pos, 1, Integer::sum);
            int pos = -1, best = 0;
            for (Map.Entry<Integer, Integer> c : cnt.entrySet()) if (c.getValue() > best) { best = c.getValue(); pos = c.getKey(); }
            Set<Integer> va = vaStarts(f);
            String ex = e.getValue().get(0).fmt.replace("\n", "\\n");
            if (!va.contains(pos + 1)) {
                rejected++;
                println(String.format("ApplyWinTypes: not variadic  %s  fmt@%d (%d sites, va_start %s) e.g. \"%s\"",
                        f.getName() + "@" + f.getEntryPoint(), pos, e.getValue().size(), va, ex));
                continue;
            }
            if (cnt.size() > 1) { conflicted++; println("ApplyWinTypes: position conflict at " + f.getEntryPoint() + " " + cnt + " (took " + pos + ")"); }
            verified++;
            variadic.add(f);
            fmtPos.put(f, pos);
            Set<Long> sc = scalars(f);
            boolean str = sc.contains(0x42L), max = sc.contains(0x7fffffffL), rd = sc.contains(0x49L);
            DataType ret = null;
            String name = null;
            Object[][] params;
            if (pos == 1 && str && max) {
                name = "sprintf"; ret = IntegerDataType.dataType;
                params = new Object[][]{{"buffer", cchar()}, {"format", cchar()}};
                sprintf = f;
            } else if (pos == 2 && str && !max) {
                name = "_snprintf"; ret = IntegerDataType.dataType;
                params = new Object[][]{{"buffer", cchar()}, {"count", UnsignedIntegerDataType.dataType}, {"format", cchar()}};
            } else if (pos == 1 && rd) {
                name = "sscanf"; ret = IntegerDataType.dataType;
                params = new Object[][]{{"buffer", cchar()}, {"format", cchar()}};
            } else {
                params = new Object[pos + 1][];
                for (int k = 0; k < pos; k++) params[k] = new Object[]{"p" + k, Undefined4DataType.dataType};
                params[pos] = new Object[]{"format", cchar()};
            }
            signature(f, ret, true, params);
            if (name != null) nameFn(f, name);
            println(String.format("ApplyWinTypes: variadic  %-28s fmt@%d  %d call sites  e.g. \"%s\"",
                    (name != null ? name : f.getName()) + "@" + f.getEntryPoint(), pos, e.getValue().size(), ex));
        }
        // vsprintf/_vsnprintf: the other callers of sprintf's _output with a string FILE and va_list passed in
        if (sprintf != null) {
            Function output = null;
            for (Function g : directCallees(sprintf)) { output = g; break; }
            if (output != null) {
                nameFn(output, "_output");
                Set<Function> callers = output.getCallingFunctions(monitor);
                for (Function g : callers) {
                    if (g.equals(sprintf) || variadic.contains(g)) continue;
                    Set<Long> sc = scalars(g);
                    if (!sc.contains(0x42L)) continue;
                    if (sc.contains(0x7fffffffL)) {
                        signature(g, IntegerDataType.dataType, false, new Object[][]{{"buffer", cchar()}, {"format", cchar()}, {"args", cchar()}});
                        nameFn(g, "vsprintf");
                    } else {
                        signature(g, IntegerDataType.dataType, false, new Object[][]{{"buffer", cchar()}, {"count", UnsignedIntegerDataType.dataType}, {"format", cchar()}, {"args", cchar()}});
                        nameFn(g, "_vsnprintf");
                    }
                }
            }
        }
        // Wrappers: variadic, formats with vsprintf/_vsnprintf. In mp.dll these are the server's only three
        // variadic functions (ts_i386.so's symbols: UTIL_VarArgs, UTIL_LogPrintf, VoiceServerDebug), told
        // apart by what they do with the text: return it (UTIL_VarArgs), log it through pfnAlertMessage with
        // at_logged = 5 (UTIL_LogPrintf), or print it for the voice manager, every caller's format starting
        // "CVoiceGameMgr" (VoiceServerDebug). Anything else that hands the text to one engine-table function
        // is <member>_va.
        boolean server = exported("GiveFnptrsToDll") != null;
        Map<String, Integer> wrapperNames = new HashMap<>();
        for (Function f : variadic) {
            if (named.containsKey(f)) continue;
            boolean vfmt = false;
            for (Function g : directCallees(f)) { String n = named.get(g); if ("vsprintf".equals(n) || "_vsnprintf".equals(n)) vfmt = true; }
            if (!vfmt) continue;
            Set<String> tc = new LinkedHashSet<>(tableCalls(f));
            boolean voice = true;
            for (Site st : sitesByCallee.get(f)) if (!st.fmt.startsWith("CVoiceGameMgr")) voice = false;
            String name = null;
            if (server && tc.isEmpty() && fmtPos.get(f) == 0 && returnsGlobal(f)) name = "UTIL_VarArgs";
            else if (server && tc.equals(Collections.singleton("pfnAlertMessage")) && pushes(f, 5)) name = "UTIL_LogPrintf";
            else if (server && tc.equals(Collections.singleton("pfnAlertMessage")) && voice) name = "VoiceServerDebug";
            else if (tc.size() == 1) {
                String base = tc.iterator().next() + "_va";
                int k = wrapperNames.merge(base, 1, Integer::sum);
                name = k == 1 ? base : base + "_" + k;
            }
            if (name != null) nameFn(f, name);
        }
        println("ApplyWinTypes: " + verified + " variadic functions typed (" + conflicted + " with mixed format positions), "
                + rejected + " format-string callees left alone (no va_start at the argument after the format).");
        for (Map.Entry<Function, String> e : named.entrySet())
            println("ApplyWinTypes: named " + e.getValue() + " @ " + e.getKey().getEntryPoint());
    }

    // ================================================================ call-site overrides
    // A "..." prototype lets the decompiler take extra arguments at a call, but it only finds the ones it
    // can see on the stack at the CALL; when the pushes straddle branches (a kill log line with seventeen
    // arguments, a sprintf inside a large function) it shows fewer than were pushed. So every call to a
    // variadic function or variadic table member whose format is a string literal gets a call-site
    // override with exactly the arguments the format asks for (doubles take two slots, scanf conversions
    // are pointers). Ghidra's own Variadic Function Signature Override analyzer does this only for
    // functions named *printf*/*scanf*, which misses the engine tables and UTIL_* here.
    static final Pattern CONV = Pattern.compile(
        "%(%|[-+ #0]*(\\d+|\\*)?(\\.(\\d+|\\*))?(hh|h|ll|l|L|I64|I32|w)?([diouxXcsSpfFeEgGn]))");

    FunctionDefinition indirectDef(Instruction call) {
        DataType t = null;
        for (Reference r : call.getReferencesFrom()) {
            if (!r.getReferenceType().isRead()) continue;
            Data d = getDataContaining(r.getToAddress());
            Data c = d != null ? d.getPrimitiveAt((int) r.getToAddress().subtract(d.getAddress())) : null;
            if (c != null) { t = c.getDataType(); break; }
        }
        if (t == null) {   // mov reg,[global struct pointer]; call [reg+off]
            Object[] o = call.getOpObjects(0);
            if (o.length == 2 && o[0] instanceof Register && o[1] instanceof Scalar) {
                Register rg = (Register) o[0];
                Instruction i = call.getPrevious();
                for (int n = 0; i != null && n < 12 && t == null; n++, i = i.getPrevious()) {
                    if (!i.getMnemonicString().equals("MOV") || !rg.equals(i.getRegister(0))) continue;
                    for (Reference ref : i.getReferencesFrom()) {
                        Data d = getDataContaining(ref.getToAddress());
                        Data p = d != null ? d.getPrimitiveAt((int) ref.getToAddress().subtract(d.getAddress())) : null;
                        if (p == null || !(p.getDataType() instanceof Pointer)) continue;
                        DataType st = ((Pointer) p.getDataType()).getDataType();
                        if (!(st instanceof Structure)) continue;
                        DataTypeComponent c = ((Structure) st).getComponentContaining((int) ((Scalar) o[1]).getValue());
                        if (c != null) t = c.getDataType();
                    }
                    break;
                }
            }
        }
        if (t instanceof TypeDef) t = ((TypeDef) t).getBaseDataType();
        if (t instanceof Pointer && ((Pointer) t).getDataType() instanceof FunctionDefinition)
            return (FunctionDefinition) ((Pointer) t).getDataType();
        return null;
    }

    // the instruction that wrote each outgoing stack slot of a call (slot 0 = first argument)
    Map<Integer, Instruction> slots(Instruction call, int want) {
        Map<Integer, Instruction> out = new HashMap<>();
        int off = 0;
        Function f = getFunctionContaining(call.getAddress());
        Instruction i = call.getPrevious();
        for (int n = 0; i != null && n < 240 && out.size() < want; n++, i = i.getPrevious()) {
            if (f != null && !f.getBody().contains(i.getAddress())) break;
            if (i.getFlowType().isTerminal() || (i.getFlowType().isJump() && i.getFlowType().isUnConditional())) break;
            String m = i.getMnemonicString();
            if (m.equals("PUSH")) { if (off >= 0 && off % 4 == 0) out.putIfAbsent(off / 4, i); off += 4; }
            else if (m.equals("POP")) off -= 4;
            else if (i.getRegister(0) == null && i.getNumOperands() >= 1 && (m.startsWith("MOV") || m.startsWith("FST") || m.startsWith("FIST"))) {   // mov/fstp [esp+d]
                Object[] o = i.getOpObjects(0);
                if (o.length >= 1 && o[0] instanceof Register && ((Register) o[0]).getName().equals("ESP")
                        && (o.length == 1 || (o.length == 2 && o[1] instanceof Scalar))) {
                    long sd = off + (o.length == 2 ? ((Scalar) o[1]).getValue() : 0);
                    if (sd >= 0 && sd % 4 == 0) {
                        out.putIfAbsent((int) (sd / 4), i);
                        if (storeSize(i) == 8) out.putIfAbsent((int) (sd / 4) + 1, i);
                    }
                }
            }
            else if ((m.equals("ADD") || m.equals("SUB")) && i.getRegister(0) != null && i.getRegister(0).getName().equals("ESP")
                    && i.getScalar(1) != null) off += (m.equals("SUB") ? 1 : -1) * (int) i.getScalar(1).getValue();
        }
        return out;
    }

    String literalAt(Instruction p) {
        if (p == null) return null;
        if (p.getScalar(0) != null) return cstr(p.getScalar(0).getUnsignedValue());
        Register rg = p.getRegister(0);
        if (rg == null) return null;
        Instruction j = p.getPrevious();
        for (int n = 0; j != null && n < 20; n++, j = j.getPrevious())
            if (rg.equals(j.getRegister(0)) && !j.getMnemonicString().equals("PUSH"))
                return j.getMnemonicString().equals("MOV") && j.getScalar(1) != null ? cstr(j.getScalar(1).getUnsignedValue()) : null;
        return null;
    }

    void writeCallOverrides() throws Exception {
        int written = 0, mismatched = 0;
        SymbolTable st = currentProgram.getSymbolTable();
        InstructionIterator it = listing.getInstructions(true);
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction call = it.next();
            if (!call.getFlowType().isCall()) continue;
            Function caller = getFunctionContaining(call.getAddress());
            if (caller == null) continue;
            FunctionSignature sig = null;
            if (call.getFlows().length > 0) {
                Function g = getFunctionAt(call.getFlows()[0]);
                if (g != null && g.isThunk()) g = g.getThunkedFunction(true);
                if (g != null && g.hasVarArgs()) sig = g.getSignature();
            } else {
                FunctionDefinition fd = indirectDef(call);
                if (fd != null && fd.hasVarArgs()) sig = fd;
            }
            if (sig == null) continue;
            ParameterDefinition[] fixed = sig.getArguments();
            int nf = fixed.length;
            Map<Integer, Instruction> sl = slots(call, 48);
            String fmt = literalAt(sl.get(nf - 1));
            if (fmt == null) continue;
            boolean scan = sig.getName().contains("scanf");
            List<ParameterDefinition> ps = new ArrayList<>(Arrays.asList(fixed));
            int slot = nf, k = 0;
            Matcher m = CONV.matcher(fmt);
            while (m.find()) {
                if (m.group(1).equals("%")) continue;
                if ("*".equals(m.group(2))) { ps.add(new ParameterDefinitionImpl("width" + k, IntegerDataType.dataType, null)); slot++; }
                if ("*".equals(m.group(4))) { ps.add(new ParameterDefinitionImpl("prec" + k, IntegerDataType.dataType, null)); slot++; }
                String len = m.group(5) == null ? "" : m.group(5);
                char c = m.group(6).charAt(0);
                DataType t;
                if ("sScC".indexOf(c) >= 0) t = dtm.getPointer(CharDataType.dataType);
                else if (c == 'p') t = dtm.getPointer(VoidDataType.dataType);
                else if (c == 'n') t = dtm.getPointer(IntegerDataType.dataType);
                else if ("fFeEgG".indexOf(c) >= 0) t = scan ? dtm.getPointer(len.equals("l") || len.equals("L") ? DoubleDataType.dataType : FloatDataType.dataType) : DoubleDataType.dataType;
                else if (len.equals("ll") || len.equals("I64")) t = scan ? dtm.getPointer(LongLongDataType.dataType) : LongLongDataType.dataType;
                else t = scan ? dtm.getPointer(IntegerDataType.dataType) : ("uxXo".indexOf(c) >= 0 ? UnsignedIntegerDataType.dataType : IntegerDataType.dataType);
                if (scan && "sScC".indexOf(c) >= 0) t = dtm.getPointer(CharDataType.dataType);
                ps.add(new ParameterDefinitionImpl("a" + k, t, null));
                slot += t.getLength() > 4 ? 2 : 1;
                k++;
            }
            if (k == 0) continue;
            for (int q = nf; q < slot; q++)
                if (!sl.containsKey(q)) { mismatched++; println("ApplyWinTypes: override skipped at " + call.getAddress() + " (format wants " + (slot - nf) + " slots, found " + new TreeSet<>(sl.keySet()) + "): \"" + fmt.replace("\n", "\\n") + "\""); slot = -1; break; }
            if (slot < 0) continue;
            for (Symbol old : st.getSymbols(call.getAddress()))
                if (old.getParentNamespace() != null && old.getParentNamespace().getName().equals("override")
                        && old.getParentNamespace().getParentNamespace() != null
                        && old.getParentNamespace().getParentNamespace().getName().equals(caller.getName())) old.delete();
            FunctionDefinitionDataType o = new FunctionDefinitionDataType(sig.getName() + "_at_" + call.getAddress(), dtm);
            o.setReturnType(sig.getReturnType());
            o.setArguments(ps.toArray(new ParameterDefinition[0]));
            try { o.setCallingConvention(sig.getCallingConventionName()); } catch (Throwable ig) {}
            try { HighFunctionDBUtil.writeOverride(caller, call.getAddress(), o); written++; }
            catch (Exception e) { println("ApplyWinTypes: override failed at " + call.getAddress() + ": " + e); }
        }
        println("ApplyWinTypes: " + written + " variadic call sites given an exact call-site prototype from their format"
                + (mismatched > 0 ? " (" + mismatched + " skipped: fewer arguments pushed than the format asks for)" : "") + ".");
    }

    // bytes the instruction stores (8 for an fstp qword), from its p-code
    int storeSize(Instruction i) {
        for (ghidra.program.model.pcode.PcodeOp op : i.getPcode())
            if (op.getOpcode() == ghidra.program.model.pcode.PcodeOp.STORE) return op.getInput(2).getSize();
        return 4;
    }

    public void run() throws Exception {
        dtm = currentProgram.getDataTypeManager();
        listing = currentProgram.getListing();
        mem = currentProgram.getMemory();
        String root = getScriptArgs().length > 0 ? getScriptArgs()[0].trim()
                    : new File(currentProgram.getExecutablePath()).getParentFile().getParent();
        List<File> hs = new ArrayList<>();
        for (String d : new String[]{"engine", "common"}) {
            File[] fs = new File(root, d).listFiles((x, n) -> n.endsWith(".h"));
            if (fs != null) { Arrays.sort(fs); hs.addAll(Arrays.asList(fs)); }
        }
        for (String n : new String[]{"pm_shared/pm_defs.h", "pm_shared/pm_movevars.h", "pm_shared/pm_info.h"}) hs.add(new File(root, n));
        for (File h : hs) if (h.exists()) loadHeader(h);
        println("ApplyWinTypes: parsed " + hs.size() + " SDK headers: " + structBody.size() + " structs, "
                + fnTypedefs.size() + " function typedefs, " + defines.size() + " defines.");
        typeTables();
        applyVariadics();
        writeCallOverrides();
        for (String w : warned) println("ApplyWinTypes: note: " + w);
    }
}
