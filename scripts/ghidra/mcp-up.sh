#!/usr/bin/env bash
# mcp-up.sh — preflight for driving the `re` project via the Ghidra MCP. The MCP attaches to a RUNNING
# Ghidra GUI instance (it can't be launched headless), so this can't fully automate the connect — but it
# removes the guesswork: it checks the project lock, finds the live MCP socket, and prints the exact
# connect_instance/open_program sequence to run from the agent.
#
#   scripts/ghidra/mcp-up.sh            # status + the connect steps
set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PROJ="$ROOT/recovered/ghidra/re"

echo "== Ghidra MCP preflight =="

# 1) the .lock gotcha — a GUI/headless can't both hold the project (CLAUDE.md §4a.3)
if compgen -G "$PROJ/re.rep/*.lock" >/dev/null 2>&1 || [ -f "$PROJ/re.rep/.lock" ]; then
  echo "  ⚠ re project is LOCKED — a headless run (re.sh) or another GUI holds it."
  echo "    Close it there first, or you'll get a lock error on open."
else
  echo "  ✓ re project not locked (safe to open in the GUI)."
fi

# 2) is a Ghidra MCP bridge socket live?
sock=$(ls /run/user/"$(id -u)"/ghidra-mcp/*.sock 2>/dev/null | head -1)
if [ -n "$sock" ]; then
  echo "  ✓ Ghidra MCP instance is UP ($(basename "$sock"))."
else
  echo "  ✗ no Ghidra MCP instance found. Start the Ghidra GUI (with the MCP bridge), then:"
fi

# 3) is the program even built?
if [ -d "$PROJ/re.rep" ]; then
  echo "  ✓ applied project on disk: $PROJ/re.rep"
else
  echo "  ✗ no applied project — run: scripts/ghidra/re.sh setup"
fi

cat <<'EOF'

In the Ghidra GUI: open  recovered/ghidra/re/re.gpr  and open the ts_i386.so program.
Then from the agent (MCP tools):
  list_instances                       # confirm the instance + project
  connect_instance("re")               # attach the bridge (loads ALL tool groups)
  open_program("/ts_i386.so")          # typed decompiles ready
  unload_tool_group("malware")         # drop the groups we never use for this work (less tool noise):
                                        #   malware (detect_malware/crypto/IOC/anti-analysis) — not relevant
                                        #   to reconstructing a game mod. Keep: function listing symbol xref
                                        #   datatype analysis comment program project.
Image base 0x10000  ⇒  Ghidra addr = (nm addr) + 0x10000.
Read-only on `re` (don't rename/mutate it — drifts from the re.sh-generated state).
EOF
