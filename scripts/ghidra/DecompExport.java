// DecompExport.java — Ghidra headless post-script.
// Decompiles functions in the loaded program and writes C to <outDir>,
// one file per owning class (parsed from the symbol name), so the output
// reads like a reconstructed translation unit.
//
// Args (via -postScript DecompExport.java <outDir> [classFilter]):
//   outDir       directory to write <Class>.c files into (created if missing)
//   classFilter  optional substring; only functions whose name contains it
//                are exported (e.g. "CTSSlowMotion"). Omitted => all functions.
//
// The .so is not stripped, so analysis applies the symtab and functions come
// out named. GCC2 names encode the class as __<len><ClassName>; we group by it.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.decompiler.DecompiledFunction;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DecompExport extends GhidraScript {

    // GCC2 mangling: name__<len><Class>...  capture the class chunk.
    private static final Pattern GCC2_CLASS = Pattern.compile("__(\\d+)([A-Za-z][A-Za-z0-9_]*)");

    private String ownerClass(String sym) {
        Matcher m = GCC2_CLASS.matcher(sym);
        if (m.find()) {
            int len = Integer.parseInt(m.group(1));
            String rest = m.group(2);
            if (rest.length() >= len) return rest.substring(0, len);
        }
        return "_free";   // non-member / unparseable
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outDir = args.length > 0 ? args[0] : "/tmp/ghidra-out";
        String filter = args.length > 1 && !args[1].isEmpty() ? args[1] : null;
        File od = new File(outDir);
        od.mkdirs();

        DecompInterface ifc = new DecompInterface();
        ifc.setOptions(new DecompileOptions());
        if (!ifc.openProgram(currentProgram)) {
            println("DecompExport: openProgram failed: " + ifc.getLastMessage());
            return;
        }

        int matched = 0, ok = 0, failed = 0;
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            if (monitor.isCancelled()) break;
            Function f = it.next();
            String name = f.getName();
            if (filter != null && !name.contains(filter)) continue;
            matched++;

            DecompileResults res = ifc.decompileFunction(f, 90, monitor);
            if (res == null || !res.decompileCompleted()) {
                failed++;
                println("  decompile failed: " + name);
                continue;
            }
            DecompiledFunction df = res.getDecompiledFunction();
            if (df == null || df.getC() == null) { failed++; continue; }

            File out = new File(od, ownerClass(name) + ".c");
            PrintWriter pw = new PrintWriter(new FileWriter(out, true));
            pw.println("/* " + name + "  @ " + f.getEntryPoint()
                       + "  (" + f.getBody().getNumAddresses() + " bytes) */");
            pw.println(df.getC());
            pw.close();
            ok++;
        }
        ifc.dispose();
        println("DecompExport: matched=" + matched + " exported=" + ok
                + " failed=" + failed + " -> " + outDir);
    }
}
