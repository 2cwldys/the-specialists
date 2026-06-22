// ApplyTypes.java — make the server decompiles read with real types:
//   1. parse engine/progdefs.h so entvars_t (and globalvars_t) get REAL fields -> a typed
//      `entvars_t *pev` decompiles `*(float*)(pev+8)` as `pev->origin`.
//   2. apply the recovered gcc2 signatures (reference/signatures-gcc2.map) as prototypes:
//      the `this` pointer + each arg typed.
//   3. merge RETURN types from the MSVC exports (signatures-mp.txt / signatures-client.txt),
//      which the gcc2 demangle omits -> complete prototypes (return + params).
//
//   analyzeHeadless <proj> <name> -process ts_i386.so -noanalysis -scriptPath scripts/ghidra \
//     -postScript ApplyTypes.java <root>/reference/signatures-gcc2.map
//
// Args: [mapPath]  (progdefs.h + the MSVC sig files are derived from its reference/ dir).
// Conservative: classes we have no layout for stay opaque named structs (members show as
// offsets — gcc2 offsets differ from our SDK and there's no mod debug info); entvars_t is the
// one big struct we CAN populate safely (pure C, ABI-stable offsets).
import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParserUtils;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class ApplyTypes extends GhidraScript {
    DataTypeManager dtm;
    CategoryPath CP = new CategoryPath("/recovered");

    DataType prim(String t) {
        switch (t.trim()) {
            case "void": return VoidDataType.dataType;
            case "bool": return BooleanDataType.dataType;
            case "char": return CharDataType.dataType;
            case "unsigned char": case "byte": return ByteDataType.dataType;
            case "short": case "short int": return ShortDataType.dataType;
            case "unsigned short": return UnsignedShortDataType.dataType;
            case "int": case "long": case "long int": return IntegerDataType.dataType;
            case "unsigned int": case "unsigned": case "unsigned long": return UnsignedIntegerDataType.dataType;
            case "float": return FloatDataType.dataType;
            case "double": return DoubleDataType.dataType;
            default: return null;
        }
    }

    DataType named(String name) {  // reuse an existing type (e.g. parsed entvars_t) else opaque struct
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
        DataType base = prim(s);
        if (base == null) base = (s.isEmpty() ? VoidDataType.dataType : named(s));
        for (int i = 0; i < stars; i++) base = dtm.getPointer(base);
        return base;
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

    // ---- parse progdefs.h (with a tiny prelude for its 4 dependency types) -> entvars_t fields
    void parseProgdefs(String progdefs) {
        try {
            File prelude = File.createTempFile("sdk_prelude", ".h");
            try (PrintWriter w = new PrintWriter(prelude)) {
                w.println("typedef float vec_t;");
                w.println("typedef struct { vec_t x, y, z; } Vector;");
                w.println("typedef Vector vec3_t;");
                w.println("typedef int string_t;");
                w.println("typedef unsigned char byte;");
                w.println("typedef unsigned short word;");
                w.println("typedef int qboolean;");
                w.println("typedef int func_t;");
                w.println("struct edict_s; typedef struct edict_s edict_t;");
                w.println("typedef struct link_s { struct link_s *prev, *next; } link_t;");
            }
            // progdefs.h is a C++ header (constexpr etc. that Ghidra's C parser rejects), so
            // append ONLY the plain `typedef struct {...} name;` blocks (globalvars_t/entvars_t),
            // which need just the prelude types.
            StringBuilder src = new StringBuilder();
            BufferedReader pr = new BufferedReader(new FileReader(progdefs));
            String ln; StringBuilder cur = null; int depth = 0;
            while ((ln = pr.readLine()) != null) {
                if (cur == null && ln.matches("\\s*typedef\\s+struct\\b.*")) { cur = new StringBuilder(); }
                if (cur != null) {
                    cur.append(ln).append("\n");
                    for (char c : ln.toCharArray()) { if (c == '{') depth++; else if (c == '}') depth--; }
                    if (depth == 0 && ln.contains(";") && ln.contains("}")) { src.append(cur); cur = null; }
                }
            }
            pr.close();
            try (PrintWriter w = new PrintWriter(new FileOutputStream(prelude, true))) { w.println(src.toString()); }
            CParserUtils.parseHeaderFiles(new DataTypeManager[]{},
                new String[]{prelude.getAbsolutePath()}, new String[]{}, dtm, monitor);
            prelude.delete();
            List<DataType> hits = new ArrayList<>(); dtm.findDataTypes("entvars_t", hits);
            println("ApplyTypes: parsed progdefs.h — entvars_t " + (hits.isEmpty() ? "NOT found" :
                    "found (" + hits.get(0).getLength() + " bytes)"));
        } catch (Throwable e) {
            println("ApplyTypes: progdefs.h parse failed (" + e + ") — entvars_t stays opaque");
        }
    }

    // ---- MSVC return types: "public: <virtual> RET __thiscall Class::Method(args)" -> {Class::Method: RET}
    Map<String, String> msvcReturns(String... files) {
        Map<String, String> ret = new HashMap<>();
        Pattern P = Pattern.compile(
            "^\\s*(?:public|private|protected):\\s*(?:virtual\\s+|static\\s+)*(.+?)\\s+(?:__thiscall|__cdecl|__stdcall)\\s+([\\w:~]+)\\(");
        for (String f : files) {
            File ff = new File(f); if (!ff.exists()) continue;
            try (BufferedReader br = new BufferedReader(new FileReader(ff))) {
                String ln;
                while ((ln = br.readLine()) != null) {
                    Matcher m = P.matcher(ln);
                    if (m.find()) ret.putIfAbsent(m.group(2), m.group(1).trim());
                }
            } catch (IOException ignore) {}
        }
        return ret;
    }

    public void run() throws Exception {
        dtm = currentProgram.getDataTypeManager();
        String mapPath = getScriptArgs().length > 0 ? getScriptArgs()[0].trim()
                         : new File(currentProgram.getExecutablePath()).getParent() + "/../reference/signatures-gcc2.map";
        File ref = new File(mapPath).getParentFile();
        parseProgdefs(new File(ref, "../engine/progdefs.h").getPath());
        Map<String, String> rets = msvcReturns(new File(ref, "signatures-mp.txt").getPath(),
                                               new File(ref, "signatures-client.txt").getPath());
        println("ApplyTypes: " + rets.size() + " MSVC return types loaded.");

        Map<String, String> sig = new HashMap<>();
        try (BufferedReader br = new BufferedReader(new FileReader(mapPath))) {
            String ln;
            while ((ln = br.readLine()) != null) {
                int i = ln.indexOf("=>");
                if (i > 0) sig.put(ln.substring(0, i).trim(), ln.substring(i + 2).trim());
            }
        }
        Pattern P = Pattern.compile("^(?:([\\w:~<>]+)::)?([\\w~<>]+)\\s*\\((.*)\\)\\s*$");
        int applied = 0, retset = 0, skipped = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            String mangled = f.getSymbol() != null ? f.getSymbol().getName() : f.getName();
            String d = sig.get(mangled);
            if (d == null) { skipped++; continue; }
            Matcher m = P.matcher(d);
            if (!m.matches()) { skipped++; continue; }
            String cls = m.group(1), meth = m.group(2), args = m.group(3).trim();
            List<ParameterImpl> params = new ArrayList<>();
            try {
                if (cls != null && !cls.isEmpty())
                    params.add(new ParameterImpl("this", dtm.getPointer(named(cls)), currentProgram));
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
                String qn = (cls != null && !cls.isEmpty() ? cls + "::" : "") + meth;
                String rt = rets.get(qn);
                if (rt != null) { f.setReturnType(resolveType(rt), SourceType.USER_DEFINED); retset++; }
            } catch (Exception e) { skipped++; }
        }
        println("ApplyTypes: applied prototypes to " + applied + " functions (" + retset +
                " with MSVC return types; " + skipped + " unmatched).");
    }
}
