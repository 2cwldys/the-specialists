// FixNoReturn.java — clear wrongly-inferred "non-returning function" flags.
//
// Ghidra's auto-analysis ("Non-Returning Functions - Discovered") frequently marks normal
// functions (UTIL_WeaponTimeBase, Host_Say, strcmp thunks, ...) as noreturn, which makes the
// decompiler TRUNCATE every caller at the first such call (the "/* WARNING: Subroutine does
// not return */" cut we kept hitting). This clears noReturn on every function EXCEPT the
// genuinely-noreturn C runtime/engine ones, then the decompiler emits full bodies.
//
// Run once as a -postScript after import (it saves the program):
//   analyzeHeadless <proj> <name> -process <bin> -postScript FixNoReturn.java
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import java.util.*;

public class FixNoReturn extends GhidraScript {
    public void run() throws Exception {
        // Functions that REALLY never return — leave these marked.
        Set<String> keep = new HashSet<>(Arrays.asList(
            "exit", "_exit", "_Exit", "abort", "__stack_chk_fail", "__assert_fail",
            "__assert", "longjmp", "siglongjmp", "_Unwind_Resume", "__cxa_throw",
            "__cxa_rethrow", "pthread_exit", "Sys_Error", "Host_Error", "Host_EndGame",
            "__libc_start_main", "_ZSt9terminatev", "std::terminate", "quick_exit"));
        int cleared = 0, kept = 0;
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function f = it.next();
            if (!f.hasNoReturn()) continue;
            if (keep.contains(f.getName())) { kept++; continue; }
            f.setNoReturn(false);
            cleared++;
        }
        println("FixNoReturn: cleared " + cleared + " noreturn flag(s), kept " + kept + " genuine.");
    }
}
