// ApplyTypes.java — apply the recovered gcc2 signatures as REAL Ghidra function prototypes,
// so the decompiler propagates types (the `this` pointer is typed to its class, call-site args
// show their types) instead of `int param_1`. Postscript; pass the .map path as arg.
//
//   analyzeHeadless <proj> <name> -process ts_i386.so -noanalysis -scriptPath scripts/ghidra \
//     -postScript ApplyTypes.java reference/signatures-gcc2.map
//
// Type model (conservative — no member offsets, which differ under the gcc2 ABI and we have no
// debug info for): primitives map to built-ins; every class/struct name becomes a NAMED (empty)
// struct so it shows as `CTSGun *` / `entvars_t *` etc.; `*`/`&` -> pointer. Members still show
// as offsets (we don't fake a layout), but params + call args are now typed and readable.
import ghidra.app.script.GhidraScript;
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
        t = t.trim();
        switch (t) {
            case "void": return VoidDataType.dataType;
            case "bool": return BooleanDataType.dataType;
            case "char": return CharDataType.dataType;
            case "unsigned char": case "byte": return ByteDataType.dataType;
            case "short": return ShortDataType.dataType;
            case "int": case "long": return IntegerDataType.dataType;
            case "unsigned int": case "unsigned": case "unsigned long": return UnsignedIntegerDataType.dataType;
            case "float": return FloatDataType.dataType;
            case "double": return DoubleDataType.dataType;
            default: return null;
        }
    }

    DataType named(String name) {  // get-or-create a named (empty) struct for a class/struct type
        DataType e = dtm.getDataType(CP, name);
        if (e != null) return e;
        StructureDataType s = new StructureDataType(CP, name, 0);
        return dtm.addDataType(s, DataTypeConflictHandler.KEEP_HANDLER);
    }

    DataType resolveType(String s) {
        s = s.trim();
        int stars = 0;
        while (s.endsWith("*") || s.endsWith("&")) { stars++; s = s.substring(0, s.length() - 1).trim(); }
        if (s.startsWith("const ")) s = s.substring(6).trim();
        if (s.startsWith("struct ")) s = s.substring(7).trim();
        DataType base = prim(s);
        if (base == null) base = (s.isEmpty() ? VoidDataType.dataType : named(s));
        for (int i = 0; i < stars; i++) base = dtm.getPointer(base);
        return base;
    }

    // split "a, b<c,d>, e *" on top-level commas
    List<String> splitArgs(String s) {
        List<String> out = new ArrayList<>();
        int depth = 0, start = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '<' || c == '(') depth++;
            else if (c == '>' || c == ')') depth--;
            else if (c == ',' && depth == 0) { out.add(s.substring(start, i)); start = i + 1; }
        }
        if (start < s.length()) out.add(s.substring(start));
        return out;
    }

    public void run() throws Exception {
        dtm = currentProgram.getDataTypeManager();
        String mapPath = getScriptArgs().length > 0 ? getScriptArgs()[0].trim()
                         : "reference/signatures-gcc2.map";
        if (!new File(mapPath).isAbsolute())
            mapPath = new File(currentProgram.getExecutablePath()).getParent() + "/../" + mapPath;
        // demangled signature per mangled symbol
        Map<String, String> sig = new HashMap<>();
        try (BufferedReader br = new BufferedReader(new FileReader(mapPath))) {
            String ln;
            while ((ln = br.readLine()) != null) {
                int i = ln.indexOf("=>");
                if (i > 0) sig.put(ln.substring(0, i).trim(), ln.substring(i + 2).trim());
            }
        }
        Pattern P = Pattern.compile("^(?:([\\w:~<>]+)::)?([\\w~<>]+)\\s*\\((.*)\\)\\s*$");
        int applied = 0, skipped = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            String mangled = f.getSymbol() != null ? f.getSymbol().getName() : f.getName();
            String d = sig.get(mangled);
            if (d == null) { skipped++; continue; }
            Matcher m = P.matcher(d);
            if (!m.matches()) { skipped++; continue; }
            String cls = m.group(1), args = m.group(3).trim();
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
            } catch (Exception e) { skipped++; }
        }
        println("ApplyTypes: applied prototypes to " + applied + " functions (" + skipped + " unmatched).");
    }
}
