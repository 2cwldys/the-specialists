// rets_hold_probe: for a match plugin that holds players in place (FL_FROZEN, maxspeed clamped every
// frame): once a second, "[hold] t=<s> alive <n> frozen <n> moved <n> maxspeed <min>..<max>" over the live
// bots, where moved counts those more than 16 units from where they were a second before and teamed
// those with a team (pev_team). At 25 s (localinfo "ms_delay" for another) it forces the match live through the plugin's own console command (localinfo "ms_cmd", default
// amx_match_start), which starts its freeze. Run by scripts/test-win-server.sh with ADDONS=<suite zip>
// PROBE=rets_hold_probe EXTRA_SMA=<the match plugin> RCBOT=1.
#include <amxmodx>
#include <fakemeta>

#define FL_FROZEN_BIT	(1<<12)
#define TSM_FROZEN_BIT	(1<<26)	// the bit ts_matchmode sets as FL_FROZEN

new Float:g_last[33][3]
new bool:g_seen[33]
new g_t

public plugin_init()
{
	register_plugin("rets_hold_probe", "1", "reTS")
	set_task(1.0, "tick", 0, "", 0, "b")
	new d[8]
	get_localinfo("ms_delay", d, charsmax(d))
	set_task(d[0] ? str_to_float(d) : 25.0, "start_match")
}

public start_match()
{
	new cmd[64]
	get_localinfo("ms_cmd", cmd, charsmax(cmd))
	if (!cmd[0])
		copy(cmd, charsmax(cmd), "amx_match_start")
	server_print("[hold] players %d, sending '%s'", get_playersnum(1), cmd)
	server_cmd(cmd)
}

public tick()
{
	g_t++
	new alive, frozen, moved, teamed, Float:lo = 9999.0, Float:hi = -1.0
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id) || !is_user_bot(id) || !is_user_alive(id))
		{
			g_seen[id] = false
			continue
		}
		alive++
		if (pev(id, pev_team) > 0)
			teamed++
		if (pev(id, pev_flags) & (FL_FROZEN_BIT | TSM_FROZEN_BIT))
			frozen++
		new Float:ms
		pev(id, pev_maxspeed, ms)
		if (ms < lo) lo = ms
		if (ms > hi) hi = ms
		new Float:o[3]
		pev(id, pev_origin, o)
		if (g_seen[id] && get_distance_f(o, g_last[id]) > 16.0)
			moved++
		g_last[id] = o
		g_seen[id] = true
	}
	if (!alive)
		lo = 0.0
	server_print("[hold] t=%d alive %d teamed %d frozen %d moved %d maxspeed %.0f..%.0f", g_t, alive, teamed, frozen, moved,
		lo, hi)
}
