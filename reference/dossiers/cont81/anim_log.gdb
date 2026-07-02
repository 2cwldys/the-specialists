set pagination off
set confirm off
set breakpoint pending on
handle SIG32 SIG33 SIG34 SIG35 SIG36 SIGPWR SIGXCPU nostop noprint pass
handle SIGSEGV nostop noprint pass
python
import gdb
LOG="/tmp/claude-1000/-home-stevenlafl-Projects-goldsrc-net-mod-ts/2f48a735-e255-40a6-aa65-94bdd93c9da6/scratchpad/anim_log.txt"
logf=open(LOG,"w")
def log(s):
    logf.write(s+"\n"); logf.flush()
class AnimBP(gdb.Breakpoint):
    def stop(self):
        try:
            gdb.newest_frame().select()
            e=lambda x: gdb.parse_and_eval(x)
            b="((CBasePlayer*)$rsi)"
            log("[A] act=%d last=%d dir=%d seqFin=%d seq=%d iuser4=0x%x legT=%.2f"%(
                int(e("(int)$edx")), int(e(b+"->m_TSAnim.m_iLastActivity")),
                int(e(b+"->m_TSAnim.m_iDirection")),
                int(e(b+"->m_fSequenceFinished")), int(e(b+"->pev->sequence")),
                int(e(b+"->pev->iuser4"))&0xffffffff,
                float(e(b+"->m_TSAnim.m_flLegTimer"))))
        except Exception as ex:
            log("[ERR] %s"%ex)
        return False
AnimBP("CTSAnimation::Animate")
log("[armed]")
end
run
