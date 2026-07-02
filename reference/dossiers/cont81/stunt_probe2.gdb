set pagination off
set breakpoint pending on
python
import gdb
LOG = open('/tmp/stunt_probe2.log', 'w', buffering=1)
def plog(m): LOG.write(m + '\n')

class LineBP(gdb.Breakpoint):
    def __init__(self, spec, tag, exprs=None):
        super().__init__(spec)
        self.tag = tag; self.exprs = exprs or []
    def stop(self):
        try:
            gdb.newest_frame().select()
            vals = []
            for n, e in self.exprs:
                try: vals.append(f"{n}={gdb.parse_and_eval(e)}")
                except Exception as ex: vals.append(f"{n}=ERR")
            plog(f"{self.tag} " + ' '.join(vals))
        except Exception as ex:
            plog(f"{self.tag} PROBE-ERR {ex}")
        return False

# GoRoll: entry gates snapshot / fail-arm / success
LineBP('ts_stunt.cpp:291', 'GOROLL-ENTRY', [
    ('iuser4', '((CTSStunt*)$rdi)->m_pPlayer->pev->iuser4'),
    ('flags', '((CTSStunt*)$rdi)->m_pPlayer->pev->flags & 512'),
    ('rollNext', '((CTSStunt*)$rdi)->m_flRollNextTime'),
    ('t', 'gpGlobals->time'),
    ('vx', '((CTSStunt*)$rdi)->m_pPlayer->pev->velocity.x'),
    ('vy', '((CTSStunt*)$rdi)->m_pPlayer->pev->velocity.y'),
    ('rollTime', '((CTSStunt*)$rdi)->m_flRollTime'),
    ('cool', '((CTSStunt*)$rdi)->m_flStunCooldown'),
])
LineBP('ts_stunt.cpp:319', 'GOROLL-FAILARM')
LineBP('ts_stunt.cpp:332', 'GOROLL-SUCCESS')
LineBP('ts_stunt.cpp:421', 'GOWHEEL-WINDOWFAIL')
LineBP('ts_stunt.cpp:450', 'GOWHEEL-HIGH')
LineBP('ts_stunt.cpp:477', 'GOWHEEL-LOW')
plog('PROBE2 ARMED')
end
run
quit
