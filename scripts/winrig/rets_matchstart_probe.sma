// rets_matchstart_probe: for a match plugin that starts a round on ".ready" (models applied, players
// released): waits for bots to fill, forces the match live through the plugin's own console command
// (localinfo "ms_cmd", default amx_match_start), then prints "[ms] alive <n>" every second. A server that
// freezes stops printing; the last line says when. Run by scripts/test-win-server.sh with
// ADDONS=<suite zip> PROBE=rets_matchstart_probe EXTRA_SMA=<the match plugin> RCBOT=1.
#include <amxmodx>

new g_n

public plugin_init()
{
	register_plugin("rets_matchstart_probe", "1", "reTS")
	set_task(25.0, "start_match")
}

public start_match()
{
	new cmd[64]
	get_localinfo("ms_cmd", cmd, charsmax(cmd))
	if (!cmd[0])
		copy(cmd, charsmax(cmd), "amx_match_start")
	server_print("[ms] players %d, sending '%s'", get_playersnum(1), cmd)
	server_cmd(cmd)
	server_exec()
	server_print("[ms] sent")
	set_task(1.0, "alive", 0, "", 0, "b")
}

public alive()
{
	g_n++
	server_print("[ms] alive %d", g_n)
}
