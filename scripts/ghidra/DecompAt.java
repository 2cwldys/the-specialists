import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.util.*;
public class DecompAt extends GhidraScript {
    public void run() throws Exception {
        String[] raw = getScriptArgs(); java.util.List<String> al=new java.util.ArrayList<>(); for(String s:raw){for(String t:s.trim().split("\\s+")){if(!t.isEmpty())al.add(t);}} String[] args=al.toArray(new String[0]);
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        Set<String> seen = new HashSet<>();
        for (String a : args) {
            try {
                Address ad = currentProgram.getAddressFactory().getAddress(a);
                if (ad == null) { continue; }
                Function f = getFunctionContaining(ad);
                if (f == null) {
                    // Ghidra never defined a function here (e.g. a command-handler thunk it
                    // didn't reach). Disassemble + create one so we can decompile it.
                    try { disassemble(ad); } catch (Exception ignore) {}
                    try { f = createFunction(ad, null); } catch (Exception ignore) {}
                    if (f == null) f = getFunctionContaining(ad);
                }
                if (f == null) { println("// no function at " + a + " (could not create)"); continue; }
                if (f.hasNoReturn()) f.setNoReturn(false); // un-truncate this body
                String key = f.getEntryPoint().toString();
                if (seen.contains(key)) continue;
                seen.add(key);
                println("// ===== FUN @ " + key + " size=" + f.getBody().getNumAddresses() + " =====");
                DecompileResults r = di.decompileFunction(f, 900, monitor);
                if (r != null && r.decompileCompleted()) println(r.getDecompiledFunction().getC());
                else println("// decompile failed");
            } catch (Exception e) { println("// err " + a + ": " + e); }
        }
    }
}
