// ApplyClientThis.java — type `this` on CLIENT (client.dll) functions so DumpMembers can recover their
// member skeletons. client.dll has no symbols/RTTI for the HUD classes and doesn't export them, so we can't
// name its functions the way the server's gcc2 symbols let us. Instead we bring the NAMES over from OUR
// reconstruction: scripts/coverage/client_members.py runs binmap (string-anchor) to match our built
// client.so (full symbols) -> client.dll RVAs, writing reference/coverage/binmap_client.tsv. This reads that
// map and, for each confident (class, rva), marks the function __thiscall with `this : <class> *` (an opaque
// named struct). DumpMembers (opaque-`this` parse) then recovers each class's member offsets+types.
//
//   analyzeHeadless recovered/ghidra/client clientdll -process client.dll -noanalysis \
//     -scriptPath scripts/ghidra -postScript ApplyClientThis.java <root>/reference/coverage/binmap_client.tsv \
//     -postScript DumpMembers.java <root>/reference/coverage members_auto_client.tsv
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class ApplyClientThis extends GhidraScript {
    DataTypeManager dtm;
    CategoryPath CP = new CategoryPath("/ts");
    Map<String, DataType> built = new HashMap<>();

    // A PADDED (not size-0) struct: a pointer to a size-0 opaque struct renders as `void *this`, so member
    // accesses can't deref. 0x2000 covers the HUD/vgui classes; few enough (~100) to stay under the
    // decompiler's datatype budget (the server's all-223-classes-at-0x8000 was what overflowed it). REPLACE
    // so a stale size-0 version from an earlier run is overwritten.
    DataType named(String name) {
        if (built.containsKey(name)) return built.get(name);
        DataType r = dtm.addDataType(new StructureDataType(CP, name, 0x2000), DataTypeConflictHandler.REPLACE_HANDLER);
        built.put(name, r);
        return r;
    }

    public void run() throws Exception {
        dtm = currentProgram.getDataTypeManager();
        long base = currentProgram.getImageBase().getOffset();
        File tsv = new File(getScriptArgs()[0].trim());
        int typed = 0, made = 0, named = 0, skipErr = 0;
        try (BufferedReader br = new BufferedReader(new FileReader(tsv))) {
            br.readLine();   // header: so_symbol  demangled  mp_rva  method  confidence
            String ln;
            while ((ln = br.readLine()) != null) {
                String[] p = ln.split("\t", -1);
                if (p.length < 4) continue;
                String dem = p[1], rvaS = p[2], method = p[3];
                if (method.equals("string?")) continue;            // skip ambiguous matches
                if (!dem.contains("::")) continue;
                String cls = dem.substring(0, dem.indexOf("::")).trim();
                String meth = dem.substring(dem.indexOf("::") + 2).replaceAll("\\(.*$", "").trim();
                if (cls.isEmpty()) continue;
                long rva;
                try { rva = Long.parseLong(rvaS.replace("0x", ""), 16); } catch (Exception e) { continue; }
                Address a = toAddr(base + rva);
                Function f = getFunctionAt(a);
                if (f == null) { f = createFunction(a, null); if (f != null) made++; }
                if (f == null) continue;
                DataType thisPtr = dtm.getPointer(named(cls));
                try {
                    // Replace the signature so `this` is a FORMAL param typed to the (padded) class struct —
                    // updateFunction can set it where setDataType can't (Ghidra's __thiscall `this` is an
                    // auto-param). The padded struct (vs size-0) is what makes it render `<Class> *this` and
                    // its derefs `this->field_0xNN` (a size-0 struct ptr collapses to `void *`).
                    f.updateFunction("__thiscall", null,
                        Arrays.asList(new ParameterImpl("this", thisPtr, currentProgram)),
                        Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
                    typed++;
                } catch (Exception ex) {
                    if (skipErr == 0) println("ApplyClientThis: first typing error on " + cls + " @ " + a + ": " + ex);
                    skipErr++;
                }
                // organize for readability: Class namespace + clean method name
                try {
                    Namespace ns = currentProgram.getSymbolTable().getNamespace(cls, currentProgram.getGlobalNamespace());
                    if (ns == null) ns = currentProgram.getSymbolTable().createNameSpace(currentProgram.getGlobalNamespace(), cls, SourceType.USER_DEFINED);
                    f.setParentNamespace(ns);
                    if (!meth.isEmpty()) f.setName(meth, SourceType.USER_DEFINED);
                    named++;
                } catch (Exception ig) {}
            }
        }
        println("ApplyClientThis: typed `this` on " + typed + " client functions (" + made
                + " newly created, " + named + " renamed into class namespaces).");
    }
}
