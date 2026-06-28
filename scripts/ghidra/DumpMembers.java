// DumpMembers.java — recover the OFFSET + TYPE of every (incl. transient/unsaved) class member, by
// analyzing how each class's methods access `this`. Names aren't in the binary for unsaved members, so
// this emits a SKELETON (placeholder names) you curate into members_re.tsv.
//
// Run AFTER ApplyInventory (needs the class-struct `this` types + decompiler this-tracking):
//   analyzeHeadless <proj> <name> -process ts_i386.so -noanalysis -scriptPath scripts/ghidra \
//     -postScript ApplyInventory.java <root>/reference/coverage -postScript DumpMembers.java
// -> writes reference/coverage/members_auto.tsv (offsets NOT already in members.tsv).
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.Pointer;
import ghidra.program.model.listing.*;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class DumpMembers extends GhidraScript {
    public void run() throws Exception {
        File cov = new File(getScriptArgs().length > 0 ? getScriptArgs()[0]
                            : new File(currentProgram.getExecutablePath()).getParent() + "/../reference/coverage");

        // offsets already named from the save tables — skip those (authoritative)
        Set<String> saved = new HashSet<>();   // "Class@0xNN"
        File mtsv = new File(cov, "members.tsv");
        if (mtsv.exists()) try (BufferedReader br = new BufferedReader(new FileReader(mtsv))) {
            br.readLine(); String ln;
            while ((ln = br.readLine()) != null) { String[] p = ln.split("\t"); if (p.length > 1) saved.add(p[0] + "@" + p[1]); }
        }

        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        // class -> offset -> {type -> count}
        Map<String, Map<Integer, Map<String, Integer>>> fields = new TreeMap<>();
        Pattern CAST = Pattern.compile("\\*\\(([\\w ]+?) \\*\\)&this->field_0x([0-9a-fA-F]+)");
        Pattern BARE = Pattern.compile("this->field_0x([0-9a-fA-F]+)");
        // OPAQUE-`this` forms: when the class has no struct (named() pointer), accesses render as
        // `*(type *)(this + 0xNN)` / `(this + 0xNN)` instead of `this->field_0xNN`. Parsing these lets us
        // recover members for classes WITHOUT a struct (CTSStunt/CTSGameRules) — no giant padded struct needed.
        Pattern OCAST = Pattern.compile("\\*\\(([\\w ]+?) \\*\\)\\(this \\+ (0x[0-9a-fA-F]+)\\)");
        Pattern OBARE = Pattern.compile("\\(this \\+ (0x[0-9a-fA-F]+)\\)");

        int methods = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.getParameterCount() == 0) continue;
            Parameter p0 = f.getParameter(0);
            if (p0 == null || !"this".equals(p0.getName()) || !(p0.getDataType() instanceof Pointer)) continue;
            String cls = ((Pointer) p0.getDataType()).getDataType().getName();
            if (cls.endsWith("_vtbl")) continue;
            // No time budget — completeness over speed. The padded class structs (0x8000) slow the
            // decompiler, so a 25s cap silently dropped the bigger methods (CBaseAnimating etc.) and the
            // member inventory came back partial. 900s/fn means nothing real ever times out.
            DecompileResults r = dec.decompileFunction(f, 900, monitor);
            if (r == null || !r.decompileCompleted() || r.getDecompiledFunction() == null) continue;
            methods++;
            String c = r.getDecompiledFunction().getC();
            Map<Integer, String> typed = new HashMap<>();
            Matcher m = CAST.matcher(c);
            while (m.find()) typed.put(Integer.parseInt(m.group(2), 16), m.group(1).trim());   // *(float*)&this->field_0xNN
            m = OCAST.matcher(c);
            while (m.find()) typed.putIfAbsent(Integer.parseInt(m.group(2).substring(2), 16), m.group(1).trim()); // *(t*)(this + 0xNN)
            m = BARE.matcher(c);
            while (m.find()) { int off = Integer.parseInt(m.group(1), 16); typed.putIfAbsent(off, "undefined4"); }
            m = OBARE.matcher(c);
            while (m.find()) typed.putIfAbsent(Integer.parseInt(m.group(1).substring(2), 16), "undefined4"); // (this + 0xNN)
            for (Map.Entry<Integer, String> e : typed.entrySet())
                fields.computeIfAbsent(cls, k -> new TreeMap<>())
                      .computeIfAbsent(e.getKey(), k -> new HashMap<>())
                      .merge(e.getValue(), 1, Integer::sum);
        }

        int rows = 0;
        File out = new File(cov, "members_auto.tsv");
        try (PrintWriter w = new PrintWriter(out)) {
            w.println("Class\toffset\tfieldtype\tcpptype\tname\tcount");
            for (Map.Entry<String, Map<Integer, Map<String, Integer>>> ce : fields.entrySet()) {
                String cls = ce.getKey();
                for (Map.Entry<Integer, Map<String, Integer>> oe : ce.getValue().entrySet()) {
                    int off = oe.getKey();
                    if (saved.contains(cls + "@0x" + Integer.toHexString(off))) continue;   // already named
                    // pick the most-specific, most-frequent type
                    String best = "undefined4"; int bc = -1;
                    for (Map.Entry<String, Integer> te : oe.getValue().entrySet()) {
                        boolean spec = !te.getKey().startsWith("undefined");
                        boolean curspec = !best.startsWith("undefined");
                        if ((spec && !curspec) || (spec == curspec && te.getValue() > bc)) { best = te.getKey(); bc = te.getValue(); }
                    }
                    int total = oe.getValue().values().stream().mapToInt(Integer::intValue).sum();
                    String ft = best.startsWith("float") ? "FLOAT" : best.startsWith("double") ? "DOUBLE"
                              : best.contains("*") ? "POINTER" : best.startsWith("char") ? "CHARACTER" : "INTEGER";
                    w.printf("%s\t0x%x\t%s\t%s\tfield_0x%x\t%d%n", cls, off, ft, best, off, total);
                    rows++;
                }
            }
        }
        println("DumpMembers: scanned " + methods + " methods -> " + out.getPath() + " (" + rows
                + " unsaved member offsets across " + fields.size() + " classes; placeholder names — curate into members_re.tsv).");
    }
}
