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
        // `*(type *)(this + 0xNN)` / `(this + 0xNN)` instead of `this->field_0xNN`. The `(int)this`/`(uint)this`
        // variant appears when `this` stays `void *` (the MSVC __thiscall auto-`this` on the CLIENT). Parsing
        // these recovers members for classes WITHOUT a real struct (CTSStunt server-side; the HUD classes client-side).
        Pattern OCAST = Pattern.compile("\\*\\(([\\w ]+?) \\*\\)\\((?:\\((?:int|uint)\\))?this \\+ (0x[0-9a-fA-F]+)\\)");
        Pattern OBARE = Pattern.compile("\\((?:\\((?:int|uint)\\))?this \\+ (0x[0-9a-fA-F]+)\\)");

        int methods = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.getParameterCount() == 0) continue;
            Parameter p0 = f.getParameter(0);
            if (p0 == null || !"this".equals(p0.getName()) || !(p0.getDataType() instanceof Pointer)) continue;
            // class name: prefer the function's CLASS NAMESPACE (set by ApplyInventory server / ApplyClientThis
            // client) — robust even when `this` stays an untyped void* (client __thiscall auto-`this`). Fall
            // back to the `this` pointed-type name (server, where `this` is the real class struct).
            String cls;
            ghidra.program.model.symbol.Namespace pns = f.getParentNamespace();
            if (pns != null && !pns.isGlobal() && !(pns instanceof ghidra.program.model.listing.Library)
                && !pns.getName().isEmpty())
                cls = pns.getName();
            else
                cls = ((Pointer) p0.getDataType()).getDataType().getName();
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
        // optional 2nd arg = output filename (the CLIENT run writes members_auto_client.tsv so it doesn't
        // clobber the server skeleton).
        String outName = getScriptArgs().length > 1 ? getScriptArgs()[1].trim() : "members_auto.tsv";
        File out = new File(cov, outName);
        try (PrintWriter w = new PrintWriter(out)) {
            w.println("Class\toffset\tfieldtype\tcpptype\tname\tcount");
            for (Map.Entry<String, Map<Integer, Map<String, Integer>>> ce : fields.entrySet()) {
                String cls = ce.getKey();
                for (Map.Entry<Integer, Map<String, Integer>> oe : ce.getValue().entrySet()) {
                    int off = oe.getKey();
                    if (saved.contains(cls + "@0x" + Integer.toHexString(off))) continue;   // already named
                    // DETERMINISTIC canonical type. Ghidra's decompiler infers the integer WIDTH/SIGNEDNESS of
                    // an unsaved field non-deterministically (short<->undefined4, uint<->undefined4 across runs —
                    // internal decompiler threading, NOT fixable via -max-cpu). But float/double/pointer detection
                    // IS stable. So decide by PRESENCE (not the wobbly frequency winner) and collapse every
                    // integer/char/undefined guess to a canonical `int`: reproducible output, and the only thing
                    // dropped is the unreliable int-width guess (the real per-member type comes from curation anyway).
                    boolean anyFloat = false, anyDouble = false, anyPtr = false;
                    for (String t : oe.getValue().keySet()) {
                        if (t.startsWith("float")) anyFloat = true;
                        else if (t.startsWith("double")) anyDouble = true;
                        else if (t.contains("*")) anyPtr = true;
                    }
                    String best; String ft;
                    if (anyFloat)       { best = "float";  ft = "FLOAT"; }
                    else if (anyDouble) { best = "double"; ft = "DOUBLE"; }
                    else if (anyPtr)    { best = "void *"; ft = "POINTER"; }
                    else                { best = "int";    ft = "INTEGER"; }
                    int total = oe.getValue().values().stream().mapToInt(Integer::intValue).sum();
                    w.printf("%s\t0x%x\t%s\t%s\tfield_0x%x\t%d%n", cls, off, ft, best, off, total);
                    rows++;
                }
            }
        }
        println("DumpMembers: scanned " + methods + " methods -> " + out.getPath() + " (" + rows
                + " unsaved member offsets across " + fields.size() + " classes; placeholder names — curate into members_re.tsv).");
    }
}
