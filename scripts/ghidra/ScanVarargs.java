// ScanVarargs.java — list every call to a variadic function in a program typed by ApplyWinTypes.java, with
// the format string, the decompiled call statement and the raw instruction behind each variadic argument.
// Written to find reTS ports made from decompiles that dropped those arguments (scripts/ghidra/
// varargs-ports.py joins this list with the addresses cited in cl_dll/ and dlls/).
//
// Covers direct calls to functions with a "..." signature and indirect calls through a typed table member
// whose function type has one (gEngfuncs.Con_Printf, g_engfuncs.pfnAlertMessage, pmove->Con_DPrintf, ...).
//
//   analyzeHeadless <proj> <name> -process <prog> -noanalysis -readOnly -scriptPath scripts/ghidra \
//     -postScript ScanVarargs.java <out.tsv>
//
// Columns: caller  caller_end  caller_name  call  callee  fixed  format  conversions  varargs  computed  args_raw  decompiled
//   fixed       parameters before the "..."
//   conversions printf conversions in the format (%% excluded, a * width/precision counts one more)
//   varargs     stack slots pushed past the fixed ones
//   computed    how many of those were not immediates
//   args_raw    per variadic slot: the push, and for a register push the instruction that last set it
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.pcode.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class ScanVarargs extends GhidraScript {
    static final Pattern CONV = Pattern.compile(
        "%(%|[-+ #0]*(\\d+|\\*)?(\\.(\\d+|\\*))?(hh|h|ll|l|L|I64|I32|w)?[diouxXcsSpfFeEgGn])");

    // fixed-parameter count of a variadic callee, or -1
    int fixedOf(Instruction call) {
        Address[] fl = call.getFlows();
        if (fl.length > 0) {
            Function f = getFunctionAt(fl[0]);
            if (f != null && f.isThunk()) f = f.getThunkedFunction(true);
            return f != null && f.hasVarArgs() ? f.getParameterCount() : -1;
        }
        FunctionDefinition fd = indirectDef(call);
        return fd != null && fd.hasVarArgs() ? fd.getArguments().length : -1;
    }

    String calleeName(Instruction call) {
        Address[] fl = call.getFlows();
        if (fl.length > 0) { Function f = getFunctionAt(fl[0]); return f != null ? f.getName() : fl[0].toString(); }
        String m = memberName(call);
        return m != null ? m : call.toString();
    }

    // `call [table+off]`, or `call reg` after `mov reg,[table+off]` / `mov reg,[ptr]; call [reg+off]`
    Data memberData(Instruction call) {
        for (Reference r : call.getReferencesFrom()) {
            if (!r.getReferenceType().isRead()) continue;
            Data d = getDataContaining(r.getToAddress());
            if (d == null) continue;
            Data c = d.getPrimitiveAt((int) r.getToAddress().subtract(d.getAddress()));
            if (c != null) return c;
        }
        return null;
    }

    FunctionDefinition indirectDef(Instruction call) {
        Data c = memberData(call);
        DataType t = c != null ? c.getDataType() : null;
        if (t == null) t = viaRegister(call);
        if (t instanceof TypeDef) t = ((TypeDef) t).getBaseDataType();
        if (t instanceof Pointer && ((Pointer) t).getDataType() instanceof FunctionDefinition)
            return (FunctionDefinition) ((Pointer) t).getDataType();
        return null;
    }

    String memberName(Instruction call) {
        Data c = memberData(call);
        if (c != null) return pathOf(c);
        String v = viaRegisterName;
        return v;
    }

    String viaRegisterName;

    String pathOf(Data c) {
        String top = c.getRoot().getLabel(), path = c.getComponentPathName();
        return (top != null ? top : c.getRoot().getAddress().toString()) + (path != null && c != c.getRoot() ? "." + path : "");
    }

    // `mov r,[global ptr to struct]; ... call [r+off]` (pmove->Con_DPrintf, gEngfuncs.pEventAPI->...)
    DataType viaRegister(Instruction call) {
        viaRegisterName = null;
        Object[] o = call.getOpObjects(0);
        if (o.length != 2 || !(o[0] instanceof Register) || !(o[1] instanceof Scalar)) return null;
        Register r = (Register) o[0];
        long off = ((Scalar) o[1]).getValue();
        Instruction i = call.getPrevious();
        for (int n = 0; i != null && n < 12; n++, i = i.getPrevious()) {
            if (!i.getMnemonicString().equals("MOV") || !r.equals(i.getRegister(0))) continue;
            for (Reference ref : i.getReferencesFrom()) {
                Data d = getDataContaining(ref.getToAddress());
                if (d == null) continue;
                Data p = d.getPrimitiveAt((int) ref.getToAddress().subtract(d.getAddress()));
                if (p == null || !(p.getDataType() instanceof Pointer)) continue;
                DataType s = ((Pointer) p.getDataType()).getDataType();
                if (!(s instanceof Structure)) continue;
                DataTypeComponent c = ((Structure) s).getComponentContaining((int) off);
                if (c == null) continue;
                viaRegisterName = pathOf(p) + "->" + c.getFieldName();
                return c.getDataType();
            }
            return null;
        }
        return null;
    }

    String cstr(long v) {
        try {
            Address a = toAddr(v);
            if (currentProgram.getMemory().getBlock(a) == null || currentProgram.getMemory().getBlock(a).isExecute()) return null;
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 1024; i++) {
                int c = currentProgram.getMemory().getByte(a.add(i)) & 0xff;
                if (c == 0) return sb.toString();
                if (c < 0x20 && c != '\n' && c != '\r' && c != '\t') return null;
                sb.append((char) c);
            }
        } catch (Exception ig) {}
        return null;
    }

    // walk back from the call: the instruction that wrote each outgoing stack slot (slot 0 = first arg)
    Map<Integer, Instruction> slots(Instruction call, int want) {
        Map<Integer, Instruction> out = new HashMap<>();
        int off = 0;
        Function f = getFunctionContaining(call.getAddress());
        Instruction i = call.getPrevious();
        for (int n = 0; i != null && n < 240 && out.size() < want; n++, i = i.getPrevious()) {
            if (f != null && !f.getBody().contains(i.getAddress())) break;
            String m = i.getMnemonicString();
            if (i.getFlowType().isTerminal() || (i.getFlowType().isJump() && i.getFlowType().isUnConditional())) break;
            if (m.equals("PUSH")) {
                if (off >= 0 && off % 4 == 0) out.putIfAbsent(off / 4, i);
                off += 4;
            } else if (m.equals("POP")) off -= 4;
            else if ((m.equals("ADD") || m.equals("SUB")) && i.getRegister(0) != null && i.getRegister(0).getName().equals("ESP")
                    && i.getScalar(1) != null) off += (m.equals("SUB") ? 1 : -1) * (int) i.getScalar(1).getValue();
            else if (i.getRegister(0) == null && i.getNumOperands() >= 1 && (m.startsWith("MOV") || m.startsWith("FST") || m.startsWith("FIST"))) {   // mov/fstp [esp+d], x
                Object[] o = i.getOpObjects(0);
                if (o.length >= 1 && o[0] instanceof Register && ((Register) o[0]).getName().equals("ESP") && i.getRegister(0) == null) {
                    long d = o.length == 2 && o[1] instanceof Scalar ? ((Scalar) o[1]).getValue() : 0;
                    long s = off + d;
                    if (s >= 0 && s % 4 == 0) {
                        out.putIfAbsent((int) (s / 4), i);
                        if (storeSize(i) == 8) out.putIfAbsent((int) (s / 4) + 1, i);
                    }
                }
            }
        }
        return out;
    }

    String describe(Instruction push) {
        if (push == null) return "?";
        String s = push.toString();
        Register r = push.getMnemonicString().equals("PUSH") ? push.getRegister(0) : null;
        if (r == null && !push.getMnemonicString().equals("PUSH")) {
            Object[] o = push.getOpObjects(1);
            if (o.length == 1 && o[0] instanceof Register) r = (Register) o[0];
        }
        if (r != null) {
            Instruction i = push.getPrevious();
            for (int n = 0; i != null && n < 30; n++, i = i.getPrevious()) {
                Register w = i.getRegister(0);
                if (i.getFlowType().isCall() && r.getName().equals("EAX")) { s += " <= " + i.getAddress() + " " + i; break; }
                if (w != null && (w.equals(r) || w.contains(r) || r.contains(w)) && !i.getMnemonicString().equals("PUSH")
                        && !i.getMnemonicString().equals("CMP") && !i.getMnemonicString().equals("TEST")) {
                    s += " <= " + i.getAddress() + " " + i; break;
                }
            }
        }
        return push.getAddress() + " " + s;
    }

    void collect(ClangNode n, Map<Address, String> out) {
        if (n instanceof ClangStatement) {
            PcodeOp op = ((ClangStatement) n).getPcodeOp();
            StringBuilder sb = new StringBuilder();
            for (int k = 0; k < ((ClangStatement) n).numChildren(); k++) sb.append(((ClangStatement) n).Child(k).toString());
            if (op != null && (op.getOpcode() == PcodeOp.CALL || op.getOpcode() == PcodeOp.CALLIND))
                out.putIfAbsent(op.getSeqnum().getTarget(), sb.toString().replaceAll("\\s+", " ").trim());
            return;
        }
        if (n instanceof ClangTokenGroup)
            for (int k = 0; k < n.numChildren(); k++) collect(n.Child(k), out);
    }

    // bytes the instruction stores (8 for an fstp qword), from its p-code
    int storeSize(Instruction i) {
        for (ghidra.program.model.pcode.PcodeOp op : i.getPcode())
            if (op.getOpcode() == ghidra.program.model.pcode.PcodeOp.STORE) return op.getInput(2).getSize();
        return 4;
    }

    public void run() throws Exception {
        String outPath = getScriptArgs()[0].trim();
        Map<Function, List<Instruction>> sites = new TreeMap<>((a, b) -> a.getEntryPoint().compareTo(b.getEntryPoint()));
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext()) {
            Instruction i = it.next();
            if (!i.getFlowType().isCall() || fixedOf(i) < 0) continue;
            Function f = getFunctionContaining(i.getAddress());
            if (f != null) sites.computeIfAbsent(f, k -> new ArrayList<>()).add(i);
        }
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        int rows = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(outPath))) {
            w.println("caller\tcaller_end\tcaller_name\tcall\tcallee\tfixed\tformat\tconversions\tvarargs\tcomputed\targs_raw\tdecompiled");
            for (Map.Entry<Function, List<Instruction>> e : sites.entrySet()) {
                Function f = e.getKey();
                Map<Address, String> stmts = new HashMap<>();
                DecompileResults r = di.decompileFunction(f, 900, monitor);
                if (r != null && r.decompileCompleted()) collect(r.getCCodeMarkup(), stmts);
                for (Instruction call : e.getValue()) {
                    int fixed = fixedOf(call);
                    Map<Integer, Instruction> sl = slots(call, 24);
                    // the format is the last fixed parameter
                    Instruction fp = sl.get(fixed - 1);
                    String fmt = null;
                    if (fp != null) {
                        Scalar s = fp.getMnemonicString().equals("PUSH") ? fp.getScalar(0) : fp.getScalar(1);
                        if (s != null) fmt = cstr(s.getUnsignedValue());
                        if (fmt == null && fp.getRegister(0) != null) {    // push reg after mov reg, imm
                            Register rg = fp.getRegister(0);
                            Instruction j = fp.getPrevious();
                            for (int n = 0; j != null && n < 20; n++, j = j.getPrevious())
                                if (rg.equals(j.getRegister(0)) && !j.getMnemonicString().equals("PUSH")) {
                                    if (j.getMnemonicString().equals("MOV") && j.getScalar(1) != null) fmt = cstr(j.getScalar(1).getUnsignedValue());
                                    break;
                                }
                        }
                    }
                    int conv = 0;
                    if (fmt != null) {
                        Matcher m = CONV.matcher(fmt);
                        while (m.find()) {
                            if (m.group(1).equals("%")) continue;
                            conv++;
                            if ("*".equals(m.group(2))) conv++;
                            if ("*".equals(m.group(4))) conv++;
                        }
                    }
                    int want = fmt != null ? conv : 0;
                    int got = 0, computed = 0;
                    List<String> raw = new ArrayList<>();
                    for (int k = 0; k < want; k++) {
                        Instruction p = sl.get(fixed + k);
                        if (p != null) got++;
                        boolean imm = p != null && p.getMnemonicString().equals("PUSH") && p.getScalar(0) != null;
                        if (p != null && !imm) computed++;
                        raw.add(describe(p));
                    }
                    String callee = calleeName(call);
                    w.println(String.join("\t", f.getEntryPoint().toString(), f.getBody().getMaxAddress().toString(), f.getName(), call.getAddress().toString(), callee,
                        String.valueOf(fixed), fmt == null ? "?" : fmt.replace("\t", "\\t").replace("\n", "\\n"),
                        String.valueOf(conv), String.valueOf(got), String.valueOf(computed),
                        String.join(" | ", raw), stmts.getOrDefault(call.getAddress(), "?")));
                    rows++;
                }
            }
        }
        println("ScanVarargs: " + rows + " variadic call sites in " + sites.size() + " functions -> " + outPath);
    }
}
