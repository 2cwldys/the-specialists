set pagination off
set breakpoint pending on
python
import gdb

LOG = open('/tmp/stunt_probe.log', 'w', buffering=1)

def plog(msg):
    LOG.write(msg + '\n')

class HitBP(gdb.Breakpoint):
    def __init__(self, spec, tag, expr=None):
        super().__init__(spec)
        self.tag = tag
        self.expr = expr
    def stop(self):
        try:
            gdb.newest_frame().select()
            extra = ''
            if self.expr:
                vals = []
                for name, e in self.expr:
                    try:
                        vals.append(f"{name}={gdb.parse_and_eval(e)}")
                    except Exception as ex:
                        vals.append(f"{name}=ERR({ex})")
                extra = ' ' + ' '.join(vals)
            plog(f"{self.tag}{extra}")
        except Exception as ex:
            plog(f"{self.tag} PROBE-ERR {ex}")
        return False

# this = rdi at entry for methods
HitBP('CBasePlayer::Duck', 'DUCK', [
    ('pressedDUCK', '((CBasePlayer*)$rdi)->m_afButtonPressed & 4'),
])
HitBP('CTSStunt::GoRoll', 'GOROLL', [
    ('rollNext', '((CTSStunt*)$rdi)->m_flRollNextTime'),
    ('time', 'gpGlobals->time'),
    ('rollTime', '((CTSStunt*)$rdi)->m_flRollTime'),
    ('stuntTime', '((CTSStunt*)$rdi)->m_flStuntTime'),
    ('cooldown', '((CTSStunt*)$rdi)->m_flStunCooldown'),
])
HitBP('CTSStunt::GoWheel', 'GOWHEEL', [
    ('wheelNext', '((CTSStunt*)$rdi)->m_flWheelNextTime'),
    ('time', 'gpGlobals->time'),
    ('rollTime', '((CTSStunt*)$rdi)->m_flRollTime'),
    ('stuntTime', '((CTSStunt*)$rdi)->m_flStuntTime'),
    ('cooldown', '((CTSStunt*)$rdi)->m_flStunCooldown'),
    ('lastStunt', '((CTSStunt*)$rdi)->m_flLastStunt'),
])
HitBP('CTSStunt::CloseAttack', 'CLOSEATTACK', [
    ('cooldown', '((CTSStunt*)$rdi)->m_flStunCooldown'),
])
plog('PROBE ARMED')
end
run
quit
