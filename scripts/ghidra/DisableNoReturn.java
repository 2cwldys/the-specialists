// DisableNoReturn.java — run as a -preScript BEFORE auto-analysis.
//
// Turns off the "Non-Returning Functions - Discovered" analyzer, which otherwise mis-marks
// normal functions (UTIL_WeaponTimeBase, Host_Say, ...) as noreturn and TRUNCATES the
// function bodies of every caller at analysis time (so DecompAt later emits a stub). With it
// off, function bodies flow through these calls and decompile in full. The "Known" analyzer
// (real libc exit/abort/_Unwind_Resume) is left on.
import ghidra.app.script.GhidraScript;

public class DisableNoReturn extends GhidraScript {
    public void run() throws Exception {
        setAnalysisOption(currentProgram, "Non-Returning Functions - Discovered", "false");
        println("DisableNoReturn: 'Non-Returning Functions - Discovered' = false");
    }
}
