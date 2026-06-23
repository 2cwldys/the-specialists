import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
public class FindStr extends GhidraScript {
  public void run() throws Exception {
    String needle = getScriptArgs()[0];
    Listing l = currentProgram.getListing();
    DataIterator di = l.getDefinedData(true);
    while (di.hasNext()) {
      Data d = di.next();
      Object v = d.getValue();
      if (v != null && v.toString().contains(needle)) {
        println("STR " + d.getAddress() + " = " + v.toString());
        ReferenceManager rm = currentProgram.getReferenceManager();
        ReferenceIterator it = rm.getReferencesTo(d.getAddress());
        while (it.hasNext()) {
          Reference r = it.next();
          Function f = getFunctionContaining(r.getFromAddress());
          println("  ref from " + r.getFromAddress() + (f!=null?(" in "+f.getName()+" @"+f.getEntryPoint()):""));
        }
      }
    }
  }
}
