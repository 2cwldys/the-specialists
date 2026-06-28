// ApplyInventory.java — drive Ghidra from the recovered inventory so server decompiles read like source.
// Superset of ApplyTypes.java: it ALSO builds a REAL struct per class from the recovered save-table
// members (reference/coverage/members.tsv) merged up the inheritance chain (hierarchy.tsv), and types
// each method's `this` with it — so `*(int *)(this + 0x84)` decompiles as `this->poweruptype`.
//
//   1. parse engine/progdefs.h            -> typed entvars_t (pev->origin, ...)
//   2. build {class -> struct} from members.tsv + hierarchy.tsv (pev @0 + merged ancestry members)
//   3. apply gcc2 prototypes (signatures-gcc2.map) with `this` = the real class struct *, + MSVC returns
//
//   analyzeHeadless <proj> <name> -process ts_i386.so -noanalysis -scriptPath scripts/ghidra \
//     -postScript ApplyInventory.java <root>/reference/coverage
//
// Args: [coverageDir]  (defaults to <exe>/../reference/coverage; reference/ is its parent's parent).
import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParserUtils;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class ApplyInventory extends GhidraScript {
    DataTypeManager dtm;
    CategoryPath CP = new CategoryPath("/recovered");
    Map<String, List<String[]>> members = new HashMap<>();   // class -> [{offset,fieldtype,cpptype,name,count}]
    Map<String, String> base = new HashMap<>();              // class -> base class
    Map<String, DataType> built = new HashMap<>();           // class -> its struct (cached)
    Map<String, List<String[]>> vtbl = new HashMap<>();      // class -> [{slotHex, owner, method, args}]
    Map<String, DataType> builtVt = new HashMap<>();         // class -> its vtable struct (cached)
    Map<String, String> rets = new HashMap<>();              // Class::method -> MSVC return type
    Map<String, String> funcFile = new HashMap<>();          // symbol -> original .cpp
    Map<String, List<String[]>> fnRefs = new HashMap<>();    // symbol -> [{symbol, kind, value}] (assets/cvars/cmds)
    static final int VPTR = 0x60;                            // gcc2 old-ABI vtable ptr offset (this SDK)

    String plateFor(String sym) {
        StringBuilder b = new StringBuilder();
        String file = funcFile.get(sym);
        if (file != null) b.append(file);
        List<String[]> refs = fnRefs.get(sym);
        if (refs != null) {
            List<String> assets = new ArrayList<>(), cvars = new ArrayList<>(), cmds = new ArrayList<>();
            for (String[] r : refs) {
                if (r.length < 3) continue;
                if (r[1].equals("cvar")) cvars.add(r[2]);
                else if (r[1].equals("command")) cmds.add(r[2]);
                else assets.add(r[2]);
            }
            if (!assets.isEmpty()) b.append("\nassets: ").append(String.join(", ", assets));
            if (!cvars.isEmpty()) b.append("\ncvars: ").append(String.join(", ", cvars));
            if (!cmds.isEmpty()) b.append("\ncommands: ").append(String.join(", ", cmds));
        }
        return b.length() > 0 ? b.toString() : null;
    }

    DataType prim(String t) {
        switch (t.trim()) {
            case "void": return VoidDataType.dataType;
            case "bool": case "BOOL": return BooleanDataType.dataType;
            case "char": return CharDataType.dataType;
            case "unsigned char": case "byte": return ByteDataType.dataType;
            case "short": case "short int": return ShortDataType.dataType;
            case "unsigned short": return UnsignedShortDataType.dataType;
            case "int": case "long": case "long int": case "string_t": return IntegerDataType.dataType;
            case "unsigned int": case "unsigned": case "unsigned long": return UnsignedIntegerDataType.dataType;
            case "float": return FloatDataType.dataType;
            case "double": return DoubleDataType.dataType;
            default: return null;
        }
    }

    DataType named(String name) {
        List<DataType> hits = new ArrayList<>();
        dtm.findDataTypes(name, hits);
        for (DataType d : hits) if (!(d instanceof Pointer)) return d;
        return dtm.addDataType(new StructureDataType(CP, name, 0), DataTypeConflictHandler.KEEP_HANDLER);
    }

    DataType resolveType(String s) {
        s = s.trim();
        int stars = 0;
        while (s.endsWith("*") || s.endsWith("&")) { stars++; s = s.substring(0, s.length() - 1).trim(); }
        if (s.startsWith("const ")) s = s.substring(6).trim();
        if (s.startsWith("class ")) s = s.substring(6).trim();
        if (s.startsWith("struct ")) s = s.substring(7).trim();
        if (s.startsWith("enum ")) s = s.substring(5).trim();
        DataType b = prim(s);
        if (b == null) b = (s.isEmpty() ? VoidDataType.dataType : named(s));
        for (int i = 0; i < stars; i++) b = dtm.getPointer(b);
        return b;
    }

    List<String> splitArgs(String s) {
        List<String> out = new ArrayList<>(); int depth = 0, start = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '<' || c == '(') depth++;
            else if (c == '>' || c == ')') depth--;
            else if (c == ',' && depth == 0) { out.add(s.substring(start, i)); start = i + 1; }
        }
        if (start < s.length()) out.add(s.substring(start));
        return out;
    }

    void loadTsv(File f, Map<String, List<String[]>> into, int keyCol) throws IOException {
        if (!f.exists()) return;
        try (BufferedReader br = new BufferedReader(new FileReader(f))) {
            br.readLine();  // header
            String ln;
            while ((ln = br.readLine()) != null) {
                String[] p = ln.split("\t", -1);
                if (p.length > keyCol) into.computeIfAbsent(p[keyCol], k -> new ArrayList<>()).add(p);
            }
        }
    }

    // Build (and cache) a struct for `cls`: pev @0 + every saved member of the class AND its ancestors,
    // each at its recovered byte offset. Gaps stay undefined; the named fields are the win.
    DataType classStruct(String cls) {
        if (built.containsKey(cls)) return built.get(cls);
        TreeMap<Integer, String[]> fields = new TreeMap<>();   // offset -> member row
        Set<String> seen = new HashSet<>();
        String c = cls;
        while (c != null && seen.add(c)) {
            for (String[] m : members.getOrDefault(c, Collections.emptyList())) {
                int off = (int) Long.parseLong(m[1].replace("0x", ""), 16);
                fields.putIfAbsent(off, m);   // derived wins over base on offset clash (shouldn't happen)
            }
            c = base.get(c);
        }
        int size = 4;   // at least room for pev
        for (Map.Entry<Integer, String[]> e : fields.entrySet())
            size = Math.max(size, e.getKey() + 4);
        if (vtbl.containsKey(cls)) size = Math.max(size, VPTR + 4);
        // PAD generously: the save table only names SAVED members, so the real object is larger. Without
        // padding, an access to an unsaved member (e.g. m_flExpire) past the struct end wraps to
        // `this[1].<some named field>` — actively WRONG. Padding makes it honest `this->field_0xNN`.
        // (Over-padding is safe & cheap: undefined tail bytes aren't stored, and the code never reads past
        // the real object. 0x8000 covers even the biggest object, TSGameRules @ 0x5894.)
        size = Math.max(size, 0x8000);
        StructureDataType s = new StructureDataType(CP, cls, size);
        built.put(cls, s);   // cache early to break member-type cycles
        try { s.replaceAtOffset(0, dtm.getPointer(named("entvars_t")), 4, "pev", ""); } catch (Exception ig) {}
        for (Map.Entry<Integer, String[]> e : fields.entrySet()) {
            if (e.getKey() == 0) continue;
            String[] m = e.getValue();
            DataType dt = resolveType(m[3]);                    // cpptype column
            try { s.replaceAtOffset(e.getKey(), dt, Math.max(1, dt.getLength()), m[4], ""); }
            catch (Exception ig) {}
        }
        // type the vtable pointer @0x60 so virtual calls resolve to this->vtbl->Method(...)
        DataType vt = vtableStruct(cls);
        if (vt != null) try { s.replaceAtOffset(VPTR, dtm.getPointer(vt), 4, "vtbl", ""); } catch (Exception ig) {}
        DataType reg = dtm.addDataType(s, DataTypeConflictHandler.REPLACE_HANDLER);
        built.put(cls, reg);
        return reg;
    }

    // Build (and cache) a vtable struct for `cls`: one typed function pointer per slot, named the method,
    // so an indirect `this->vtbl[0x28](...)` decompiles as `this->vtbl->Precache(...)`.
    DataType vtableStruct(String cls) {
        if (builtVt.containsKey(cls)) return builtVt.get(cls);
        List<String[]> slots = vtbl.get(cls);
        if (slots == null) { builtVt.put(cls, null); return null; }
        int vsize = 4;   // a size-0 struct can't take replaceAtOffset — size it to the last slot
        for (String[] sl : slots) {
            try { vsize = Math.max(vsize, (int) Long.parseLong(sl[0].replace("0x", ""), 16) + 4); } catch (Exception ig) {}
        }
        StructureDataType vt = new StructureDataType(CP, cls + "_vtbl", vsize);
        builtVt.put(cls, vt);   // cache early (break cycles)
        for (String[] sl : slots) {
            int off;
            try { off = (int) Long.parseLong(sl[0].replace("0x", ""), 16); } catch (Exception e) { continue; }
            String owner = sl[1], meth = sl[2], args = sl[3];
            FunctionDefinitionDataType fd = new FunctionDefinitionDataType(CP, cls + "__" + meth + "_" + off);
            String rt = rets.get((owner != null ? owner + "::" : "") + meth);
            fd.setReturnType(rt != null ? resolveType(rt) : (DataType) Undefined4DataType.dataType);
            List<ParameterDefinition> pd = new ArrayList<>();
            pd.add(new ParameterDefinitionImpl("this", dtm.getPointer(named(owner != null ? owner : cls)), ""));
            if (args != null && !args.trim().isEmpty() && !args.trim().equals("void")) {
                int n = 0;
                for (String a : splitArgs(args)) {
                    a = a.trim();
                    if (a.isEmpty() || a.equals("...")) continue;
                    pd.add(new ParameterDefinitionImpl("a" + (n++), resolveType(a), ""));
                }
            }
            try { fd.setArguments(pd.toArray(new ParameterDefinition[0])); } catch (Exception ig) {}
            DataType fp = dtm.getPointer(dtm.addDataType(fd, DataTypeConflictHandler.KEEP_HANDLER));
            try { vt.replaceAtOffset(off, fp, 4, meth, ""); } catch (Exception ig) {}
        }
        DataType reg = dtm.addDataType(vt, DataTypeConflictHandler.REPLACE_HANDLER);
        builtVt.put(cls, reg);
        return reg;
    }

    void loadVtablesMd(File f) throws IOException {
        if (!f.exists()) return;
        Pattern S = Pattern.compile("`(0x[0-9a-fA-F]+)`.*?([A-Za-z_]\\w*)::(~?\\w+)\\((.*)\\)");
        try (BufferedReader br = new BufferedReader(new FileReader(f))) {
            String ln, cls = null;
            while ((ln = br.readLine()) != null) {
                if (ln.startsWith("## ")) { cls = ln.substring(3).trim(); continue; }
                if (cls == null) continue;
                Matcher m = S.matcher(ln);
                if (m.find())
                    vtbl.computeIfAbsent(cls, k -> new ArrayList<>())
                        .add(new String[]{m.group(1), m.group(2), m.group(3), m.group(4)});
            }
        }
    }

    // Parse the plain `typedef struct {...} name;` blocks out of the given C++ SDK headers (progdefs.h:
    // globalvars_t/entvars_t; eiface.h: enginefuncs_t — the engine API table). A shared prelude + broad
    // forward-decls let the C parser accept the function-pointer members so g_engfuncs/gpGlobals/pev type.
    void parseSdkTypes(String... headers) {
        try {
            File prelude = File.createTempFile("sdk_prelude", ".h");
            try (PrintWriter w = new PrintWriter(prelude)) {
                w.println("typedef float vec_t;");
                w.println("typedef struct { vec_t x, y, z; } Vector;");
                w.println("typedef Vector vec3_t;");
                w.println("typedef int string_t; typedef int qboolean; typedef int func_t;");
                w.println("typedef unsigned char byte; typedef unsigned short word;");
                w.println("typedef struct link_s { struct link_s *prev, *next; } link_t;");
                // forward declarations so pointer members in enginefuncs_t parse
                for (String s : new String[]{"edict_s", "entvars_s", "globalvars_s", "TraceResult",
                        "KeyValueData", "cvar_s", "usercmd_s", "netadr_s", "customization_s", "weapon_data_s",
                        "playermove_s", "model_s", "physent_s", "pmtrace_s", "msurface_s", "trace_t",
                        "sentenceentry_s", "SAVERESTOREDATA", "TYPEDESCRIPTION"})
                    w.println("struct " + s + ";");
                w.println("typedef struct edict_s edict_t; typedef struct entvars_s entvars_t;");
                w.println("typedef struct globalvars_s globalvars_t; typedef struct cvar_s cvar_t;");
                w.println("typedef struct TraceResult TraceResult; typedef struct KeyValueData KeyValueData;");
            }
            StringBuilder src = new StringBuilder();
            for (String h : headers) {
                File hf = new File(h); if (!hf.exists()) continue;
                BufferedReader pr = new BufferedReader(new FileReader(hf));
                String ln; StringBuilder cur = null; int depth = 0;
                while ((ln = pr.readLine()) != null) {
                    if (cur == null && ln.matches("\\s*typedef\\s+struct\\b.*")) cur = new StringBuilder();
                    if (cur != null) {
                        cur.append(ln).append("\n");
                        for (char c : ln.toCharArray()) { if (c == '{') depth++; else if (c == '}') depth--; }
                        if (depth == 0 && ln.contains(";") && ln.contains("}")) { src.append(cur); cur = null; }
                    }
                }
                pr.close();
            }
            // resolve the array-size macros so the bone arrays size right (else entvars_t drifts 4 bytes
            // and every field past ~scale — health/takedamage/... — gets MISLABELLED). HL values: 4, 2.
            String fixed = src.toString().replace("[NUM_ENT_CONTROLLERS]", "[4]").replace("[NUM_ENT_BLENDERS]", "[2]");
            try (PrintWriter w = new PrintWriter(new FileOutputStream(prelude, true))) { w.println(fixed); }
            CParserUtils.parseHeaderFiles(new DataTypeManager[]{},
                new String[]{prelude.getAbsolutePath()}, new String[]{}, dtm, monitor);
            prelude.delete();
            List<DataType> h = new ArrayList<>(); dtm.findDataTypes("enginefuncs_t", h);
            println("ApplyInventory: SDK types parsed — enginefuncs_t " + (h.isEmpty() ? "NOT found" : "found"));
        } catch (Throwable e) { println("ApplyInventory: SDK type parse failed (" + e + ")"); }
    }

    // Build enginefuncs_t in Java (the real eiface.h is too gnarly for Ghidra's C parser): grab the
    // ordered pfn* member names by regex -> a struct of named function pointers (4 bytes each). Order =
    // declaration order = the binary's layout, so g_engfuncs.pfnPrecacheModel(...) resolves.
    DataType buildEnginefuncs(String path) {
        File f = new File(path);
        if (!f.exists()) return null;
        List<String> names = new ArrayList<>();
        Pattern P = Pattern.compile("\\(\\s*\\*\\s*(pfn\\w+)\\s*\\)");
        boolean in = false;
        try (BufferedReader br = new BufferedReader(new FileReader(f))) {
            String ln;
            while ((ln = br.readLine()) != null) {
                if (ln.contains("enginefuncs_s")) in = true;
                if (in) { Matcher m = P.matcher(ln); while (m.find()) names.add(m.group(1)); }
                if (in && ln.contains("enginefuncs_t")) break;
            }
        } catch (IOException e) { return null; }
        if (names.isEmpty()) return null;
        StructureDataType s = new StructureDataType(CP, "enginefuncs_t", names.size() * 4);
        FunctionDefinitionDataType fn = new FunctionDefinitionDataType(CP, "engine_fn");
        DataType fp = dtm.getPointer(dtm.addDataType(fn, DataTypeConflictHandler.KEEP_HANDLER));
        for (int i = 0; i < names.size(); i++)
            try { s.replaceAtOffset(i * 4, fp, 4, names.get(i), ""); } catch (Exception ig) {}
        println("ApplyInventory: enginefuncs_t built (" + names.size() + " engine fns).");
        return dtm.addDataType(s, DataTypeConflictHandler.REPLACE_HANDLER);
    }

    // type the GOT pointer slots (the data references TO `name`) with `slotType` (= pointer-to-target).
    int typeGotSlots(String name, DataType slotType) {
        if (slotType == null) return 0;
        int n = 0;
        try {
            ghidra.program.model.symbol.Symbol[] syms =
                currentProgram.getSymbolTable().getGlobalSymbols(name).toArray(new ghidra.program.model.symbol.Symbol[0]);
            if (syms.length == 0) return 0;
            for (ghidra.program.model.symbol.Reference r : getReferencesTo(syms[0].getAddress())) {
                if (!r.getReferenceType().isData()) continue;     // the GOT pointer slot, not code
                Address from = r.getFromAddress();
                try { clearListing(from, from.add(3)); createData(from, slotType); n++; } catch (Exception ig) {}
            }
        } catch (Exception ig) {}
        return n;
    }

    // Type the .rodata FP constant pool so DAT_xxx shows its VALUE (0.1, 0.25, ...). The x87 load that
    // references each constant tells the size by opcode: d8/d9 = m32 (float), dc/dd = m64 (double) — so we
    // never have to reverse a float from raw bytes or read the wrong width on a double.
    void typeFpConstants() {
        MemoryBlock ro = currentProgram.getMemory().getBlock(".rodata");
        if (ro == null) return;
        Address lo = ro.getStart(), hi = ro.getEnd();
        Set<Address> done = new HashSet<>();
        int nF = 0, nD = 0;
        for (Instruction insn : currentProgram.getListing().getInstructions(true)) {
            int b;
            try { b = insn.getByte(0) & 0xff; } catch (Exception e) { continue; }
            DataType dt;
            if (b == 0xd8 || b == 0xd9) dt = FloatDataType.dataType;
            else if (b == 0xdc || b == 0xdd) dt = DoubleDataType.dataType;
            else continue;
            for (Reference r : insn.getReferencesFrom()) {
                Address to = r.getToAddress();
                if (to == null || to.compareTo(lo) < 0 || to.compareTo(hi) > 0 || done.contains(to)) continue;
                try {
                    clearListing(to, to.add(dt.getLength() - 1));
                    createData(to, dt);
                    done.add(to);
                    // rename to encode the value so the decompile reads `dc_0_1` (=0.1) not DOUBLE_xxx
                    double dv = (dt == DoubleDataType.dataType)
                        ? Double.longBitsToDouble(currentProgram.getMemory().getLong(to))
                        : Float.intBitsToFloat(currentProgram.getMemory().getInt(to));
                    ghidra.program.model.symbol.Symbol s = currentProgram.getSymbolTable().createLabel(
                        to, (dt == DoubleDataType.dataType ? "dc_" : "fc_") + fmtNum(dv), SourceType.USER_DEFINED);
                    if (s != null) s.setPrimary();
                    if (dt == DoubleDataType.dataType) nD++; else nF++;
                } catch (Exception ig) {}
            }
        }
        println("ApplyInventory: typed FP constants — " + nF + " float, " + nD + " double.");
    }

    // value -> a label-safe token: 0.1 -> "0_1", 64 -> "64", -1300 -> "neg1300", 0.0666667 -> "0_0666667"
    String fmtNum(double v) {
        String s = (v == Math.rint(v) && Math.abs(v) < 1e15) ? String.valueOf((long) v) : String.valueOf(v);
        return s.replace("-", "neg").replace(".", "_").replace("E", "e").replace("+", "");
    }

    // type a global data symbol (by name) with a datatype; clears whatever's there first
    void typeGlobal(String name, DataType dt) {
        try {
            ghidra.program.model.symbol.Symbol[] syms =
                currentProgram.getSymbolTable().getGlobalSymbols(name).toArray(new ghidra.program.model.symbol.Symbol[0]);
            if (syms.length == 0 || dt == null) return;
            Address a = syms[0].getAddress();
            clearListing(a, a.add(Math.max(1, dt.getLength()) - 1));
            createData(a, dt);
        } catch (Exception ig) {}
    }

    Map<String, String> msvcReturns(String... files) {
        Map<String, String> ret = new HashMap<>();
        Pattern P = Pattern.compile(
            "^\\s*(?:public|private|protected):\\s*(?:virtual\\s+|static\\s+)*(.+?)\\s+(?:__thiscall|__cdecl|__stdcall)\\s+([\\w:~]+)\\(");
        for (String f : files) {
            File ff = new File(f); if (!ff.exists()) continue;
            try (BufferedReader br = new BufferedReader(new FileReader(ff))) {
                String ln;
                while ((ln = br.readLine()) != null) { Matcher m = P.matcher(ln); if (m.find()) ret.putIfAbsent(m.group(2), m.group(1).trim()); }
            } catch (IOException ignore) {}
        }
        return ret;
    }

    public void run() throws Exception {
        dtm = currentProgram.getDataTypeManager();
        String covPath = getScriptArgs().length > 0 ? getScriptArgs()[0].trim()
                         : new File(currentProgram.getExecutablePath()).getParent() + "/../reference/coverage";
        File cov = new File(covPath);
        File ref = cov.getParentFile();                    // reference/
        parseSdkTypes(new File(ref, "../engine/progdefs.h").getPath());   // entvars_t/globalvars_t (C parser)
        DataType ef = buildEnginefuncs(new File(ref, "../engine/eiface.h").getPath());  // enginefuncs_t (in Java)
        typeGlobal("g_engfuncs", ef);                                // engine API table -> g_engfuncs.pfnXxx(...)
        typeGlobal("gpGlobals", dtm.getPointer(named("globalvars_t")));  // gpGlobals->time, ...
        // engine calls go through PIC GOT slots (data pointers to g_engfuncs/gpGlobals); type those slots
        // so `(*slot).pfnPrecacheModel(...)` resolves instead of `*(code**)(PTR + 8)`.
        int slots = 0;
        slots += typeGotSlots("g_engfuncs", ef != null ? dtm.getPointer(ef) : null);
        slots += typeGotSlots("gpGlobals", dtm.getPointer(dtm.getPointer(named("globalvars_t"))));
        println("ApplyInventory: typed " + slots + " engine GOT pointer slots.");
        typeFpConstants();   // .rodata float/double constants -> their literal value in the decompile

        loadTsv(new File(cov, "members.tsv"), members, 0);
        // RE-curated transient members (m_flExpire, … — verified by decompile, but NOT in any save table,
        // so not auto-recoverable). These are OUR RE-grounded names layered on top of the save-table ones.
        loadTsv(new File(cov, "members_re.tsv"), members, 0);
        // auto-recovered transient members (DumpMembers): OFFSET+TYPE only, placeholder field_0xNN names.
        // Lowest priority (classStruct's putIfAbsent keeps the first = saved > curated > auto), so this only
        // TYPES the members we haven't named yet -> `this->field_0x94` (a float) instead of a raw cast.
        loadTsv(new File(cov, "members_auto.tsv"), members, 0);
        Map<String, List<String[]>> hier = new HashMap<>();
        loadTsv(new File(cov, "hierarchy.tsv"), hier, 0);
        for (Map.Entry<String, List<String[]>> e : hier.entrySet())
            base.put(e.getKey(), e.getValue().get(0)[1]);
        loadVtablesMd(new File(cov, "VTABLES.md"));
        loadTsv(new File(cov, "precache.tsv"), fnRefs, 0);   // function -> asset/cvar/command refs
        File ffile = new File(cov, "funcfile.tsv");
        if (ffile.exists()) try (BufferedReader br = new BufferedReader(new FileReader(ffile))) {
            br.readLine(); String ln;
            while ((ln = br.readLine()) != null) { String[] p = ln.split("\t", -1); if (p.length > 1) funcFile.put(p[0], p[1]); }
        }
        rets = msvcReturns(new File(ref, "signatures-mp.txt").getPath(),
                           new File(ref, "signatures-client.txt").getPath());
        // pre-build every class struct (+ vtable struct) so class-typed PARAMS resolve to the real layout.
        Set<String> allcls = new HashSet<>(members.keySet());
        allcls.addAll(base.keySet());
        allcls.addAll(vtbl.keySet());
        for (String c : allcls) classStruct(c);
        println("ApplyInventory: " + members.size() + " classes w/ members, " + base.size() + " base relations, "
                + vtbl.size() + " vtables, " + builtVt.size() + " vtable structs.");
        Map<String, String> sig = new HashMap<>();
        try (BufferedReader br = new BufferedReader(new FileReader(new File(ref, "signatures-gcc2.map")))) {
            String ln;
            while ((ln = br.readLine()) != null) { int i = ln.indexOf("=>"); if (i > 0) sig.put(ln.substring(0, i).trim(), ln.substring(i + 2).trim()); }
        }

        Pattern P = Pattern.compile("^(?:([\\w:~<>]+)::)?([\\w~<>]+)\\s*\\((.*)\\)\\s*$");
        int applied = 0, structs = 0, retset = 0, skipped = 0, renamed = 0, commented = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            String mangled = f.getSymbol() != null ? f.getSymbol().getName() : f.getName();
            String d = sig.get(mangled);
            if (d == null) { skipped++; continue; }
            Matcher m = P.matcher(d);
            if (!m.matches()) { skipped++; continue; }
            String cls = m.group(1), meth = m.group(2), args = m.group(3).trim();
            List<ParameterImpl> params = new ArrayList<>();
            try {
                if (cls != null && !cls.isEmpty()) {
                    // Give the receiver its real (padded) class struct when we have member/hierarchy data for it;
                    // otherwise an OPAQUE named pointer. (Padding EVERY class to 0x8000 — 223 of them — overflowed
                    // the decompiler's datatype budget and made it FAIL on many functions, gutting the member
                    // inventory. Classes of pure transient state like CTSStunt still get `this` named + an opaque
                    // pointer, so their accesses read `*(t*)(this + 0xNN)` — DumpMembers parses that opaque form
                    // directly, no giant struct needed.)
                    DataType self = members.containsKey(cls) || base.containsKey(cls) ? classStruct(cls) : named(cls);
                    if (self instanceof Structure && ((Structure) self).getNumDefinedComponents() > 0) structs++;
                    params.add(new ParameterImpl("this", dtm.getPointer(self), currentProgram));
                }
                if (!args.isEmpty() && !args.equals("void")) {
                    int n = 0;
                    for (String a : splitArgs(args)) {
                        a = a.trim();
                        if (a.equals("...") || a.isEmpty()) continue;
                        params.add(new ParameterImpl("a" + (n++), resolveType(a), currentProgram));
                    }
                }
                f.replaceParameters(params, Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
                                    true, SourceType.USER_DEFINED);
                applied++;
                String rt = rets.get((cls != null && !cls.isEmpty() ? cls + "::" : "") + meth);
                if (rt != null) { f.setReturnType(resolveType(rt), SourceType.USER_DEFINED); retset++; }
                // organize: put the function in a `Class` namespace + give it the clean demangled name
                if (cls != null && !cls.isEmpty()) {
                    try {
                        Namespace ns = currentProgram.getSymbolTable().getNamespace(cls, currentProgram.getGlobalNamespace());
                        if (ns == null) ns = currentProgram.getSymbolTable().createNameSpace(currentProgram.getGlobalNamespace(), cls, SourceType.USER_DEFINED);
                        f.setParentNamespace(ns);
                        f.setName(meth, SourceType.USER_DEFINED);
                        renamed++;
                    } catch (Exception ig) {}
                }
                String plate = plateFor(mangled);   // file + assets/cvars/commands this fn references
                if (plate != null) { setPlateComment(f.getEntryPoint(), plate); commented++; }
            } catch (Exception e) { skipped++; }
        }
        // type the message-id globals (gmsg*) as int so REG_USER_MSG assignments read cleanly
        int gtyped = 0;
        File gf = new File(cov, "globals.tsv");
        if (gf.exists()) try (BufferedReader br = new BufferedReader(new FileReader(gf))) {
            br.readLine(); String ln;
            while ((ln = br.readLine()) != null) {
                String[] p = ln.split("\t", -1);
                if (p.length >= 3 && p[0].equals("global") && p[1].startsWith("gmsg")) {
                    try { Address a = toAddr(Long.parseLong(p[2].replace("0x", ""), 16));
                          clearListing(a); createData(a, IntegerDataType.dataType); gtyped++; } catch (Exception ig) {}
                }
            }
        }
        println("ApplyInventory: prototypes -> " + applied + " functions (" + structs +
                " with a real class struct `this`; " + retset + " MSVC returns; " + skipped + " unmatched). "
                + built.size() + " class structs, " + builtVt.size() + " vtable structs. "
                + renamed + " renamed into class namespaces, " + commented + " plate-commented, " + gtyped + " gmsg* typed.");
    }
}
