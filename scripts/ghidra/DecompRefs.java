// DecompRefs.java — decompile every unique function that references the given address.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;
import java.util.*;

public class DecompRefs extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Address tgt = currentProgram.getAddressFactory().getAddress(args[0]);
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        ReferenceManager rm = currentProgram.getReferenceManager();
        ReferenceIterator it = rm.getReferencesTo(tgt);
        Set<Address> seen = new HashSet<>();
        while (it.hasNext()) {
            Reference r = it.next();
            Function f = getFunctionContaining(r.getFromAddress());
            if (f == null || seen.contains(f.getEntryPoint())) continue;
            seen.add(f.getEntryPoint());
            DecompileResults res = di.decompileFunction(f, 60, monitor);
            if (res != null && res.decompileCompleted()) {
                String c = res.getDecompiledFunction().getC();
                // only print functions that look like draw/HUD (call many funcs, reference scr)
                println("// ===== " + f.getName() + " @ " + f.getEntryPoint() + " size=" + f.getBody().getNumAddresses() + " =====");
                println(c);
            }
        }
    }
}
