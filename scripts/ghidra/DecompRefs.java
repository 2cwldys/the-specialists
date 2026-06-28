// DecompRefs.java — decompile every unique function that references any of the given address(es).
// Args: one or more addresses (space-separated). The static caller-finder: "who calls/uses this?".
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;
import java.util.*;

public class DecompRefs extends GhidraScript {
    public void run() throws Exception {
        java.util.List<String> args = new ArrayList<>();
        for (String s : getScriptArgs()) for (String t : s.trim().split("\\s+")) if (!t.isEmpty()) args.add(t);
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        ReferenceManager rm = currentProgram.getReferenceManager();
        Set<Address> seen = new HashSet<>();
        for (String a : args) {
            Address tgt = currentProgram.getAddressFactory().getAddress(a);
            if (tgt == null) continue;
            ReferenceIterator it = rm.getReferencesTo(tgt);
            while (it.hasNext()) {
                Function f = getFunctionContaining(it.next().getFromAddress());
                if (f == null || !seen.add(f.getEntryPoint())) continue;
                DecompileResults res = di.decompileFunction(f, 900, monitor);
                if (res != null && res.decompileCompleted()) {
                    println("// ===== " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " =====");
                    println(res.getDecompiledFunction().getC());
                }
            }
        }
        println("// DecompRefs: " + seen.size() + " referencing functions decompiled.");
    }
}
