set pagination off
set confirm off
python
import gdb, struct

LOG = open('/tmp/setanim.log', 'w', buffering=1)
SETANIM = 0x1e12ffb0 + 0x46920

def rmem(addr, n):
    return bytes(gdb.selected_inferior().read_memory(addr, n))

class SetAnimBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__(f"*{SETANIM:#x}", type=gdb.BP_BREAKPOINT)
    def stop(self):
        try:
            esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff
            raw = rmem(esp + 8, 4)
            nameptr = struct.unpack('<I', raw)[0]
            name = rmem(nameptr, 32).split(b'\x00')[0].decode('latin1')
            flags = struct.unpack('<I', rmem(esp + 12, 4))[0]
            LOG.write(f"SetAnim name={name} flags={flags:#x}\n")
        except Exception as ex:
            LOG.write(f"SetAnim PROBE-ERR {ex}\n")
        return False

SetAnimBP()
LOG.write("ARMED\n")
end
continue
