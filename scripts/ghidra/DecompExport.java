// DecompExport.java — Ghidra headless post-script.
// Decompiles functions in the loaded program and writes C to <outDir>,
// one file per owning class (parsed from the symbol name), so the output
// reads like a reconstructed translation unit.
//
// Args (via -postScript DecompExport.java <outDir> [classFilter]):
//   outDir       directory to write <Class>.c files into (created if missing)
//   classFilter  optional substring; only functions whose namespaced name
//                (CTSGun::Deploy) contains it are exported. Omitted => all functions.
//
// On an ApplyInventory'd project methods are renamed into class namespaces, so
// matching and grouping use the namespaced name; functions never renamed keep their
// GCC2 name (__<len><ClassName>) and are grouped by parsing it. Each <Class>.c is
// truncated on its first write in a run, so repeated exports don't accumulate.
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
import java.io.BufferedReader;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Map;
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

    // mangled-name => demangled C++ prototype (from gnv2demangle.py)
    private Map<String,String> loadSigMap(String path) throws Exception {
        Map<String,String> m = new HashMap<>();
        if (path == null) return m;
        File f = new File(path);
        if (!f.exists()) { println("sig map not found: " + path); return m; }
        BufferedReader br = new BufferedReader(new FileReader(f));
        String line;
        while ((line = br.readLine()) != null) {
            int k = line.indexOf("  =>  ");
            if (k > 0) m.put(line.substring(0, k).trim(), line.substring(k + 6).trim());
        }
        br.close();
        println("loaded " + m.size() + " demangled signatures");
        return m;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outDir = args.length > 0 ? args[0] : "/tmp/ghidra-out";
        String filter = args.length > 1 && !args[1].isEmpty() ? args[1] : null;
        String sigPath = args.length > 2 && !args[2].isEmpty() ? args[2] : null;
        Map<String,String> sigs = loadSigMap(sigPath);
        File od = new File(outDir);
        od.mkdirs();

        DecompInterface ifc = new DecompInterface();
        ifc.setOptions(new DecompileOptions());
        if (!ifc.openProgram(currentProgram)) {
            println("DecompExport: openProgram failed: " + ifc.getLastMessage());
            return;
        }

        int matched = 0, ok = 0, failed = 0;
        java.util.Set<String> started = new java.util.HashSet<>();
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            if (monitor.isCancelled()) break;
            Function f = it.next();
            if (f.isThunk() || f.isExternal()) continue;   // 6-byte PLT stubs carry the same names
            String name = f.getName(true);   // CTSGun::Deploy once renamed, else the raw symbol
            if (filter != null && !name.contains(filter)) continue;
            matched++;

            DecompileResults res = ifc.decompileFunction(f, 900, monitor);
            if (res == null || !res.decompileCompleted()) {
                failed++;
                println("  decompile failed: " + name);
                continue;
            }
            DecompiledFunction df = res.getDecompiledFunction();
            if (df == null || df.getC() == null) { failed++; continue; }

            String owner = f.getParentNamespace().isGlobal()
                ? ownerClass(f.getName()) : f.getParentNamespace().getName();
            File out = new File(od, owner + ".c");
            PrintWriter pw = new PrintWriter(new FileWriter(out, !started.add(owner)));
            String proto = sigs.get(f.getName());
            if (proto != null) pw.println("// " + proto);
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
