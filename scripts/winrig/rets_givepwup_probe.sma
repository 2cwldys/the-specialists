// rets_givepwup_probe: ts_givepwup on a powerup ts_createpwup has just made, with nopowerup 1, on a fake
// client that has spawned: kung fu (an instant powerup) and slow motion (a timed one, then usepowerup).
// Before reTSX called the game's CTSPowerUp::WaitTouch, neither was given. Run by
// scripts/test-win-server.sh with ADDONS=<suite zip> PROBE=rets_givepwup_probe UNTIL='\[probe\] done'.
#include <amxmodx>
#include <fakemeta>
#include <tsx>
#include <tsfun>

new g_bot
new g_alive
new g_step

public plugin_init()
{
	register_plugin("rets_givepwup_probe", "1", "reTS")
	set_task(3.0, "make_bot")
}

public make_bot()
{
	g_bot = engfunc(EngFunc_CreateFakeClient, "probe")
	if (!g_bot) { server_print("[probe] no fake client"); return; }
	new reject[128]
	dllfunc(DLLFunc_ClientConnect, g_bot, "probe", "127.0.0.1", reject)
	dllfunc(DLLFunc_ClientPutInServer, g_bot)
	set_task(1.0, "tick", 0, "", 0, "b")
}

public tick()
{
	if (!is_user_connected(g_bot)) { server_print("[probe] bot gone"); return; }
	if (!is_user_alive(g_bot))
	{
		engclient_cmd(g_bot, "respawn")
		return
	}
	// a couple of seconds alive first, so he is not still falling from the spawn point
	if (++g_alive < 2)
		return
	switch (g_step++)
	{
		case 0:
		{
			set_cvar_num("nopowerup", 1)
			new ent = ts_createpwup(TSPWUP_KUNGFU)
			new before = ts_has_fupowerup(g_bot)
			new given = ts_givepwup(g_bot, ent)
			server_print("[probe] nopowerup %d kung fu: had %d, givepwup %d, now %d, entity left %d",
				get_cvar_num("nopowerup"), before, given, ts_has_fupowerup(g_bot), pev_valid(ent))
			ent = ts_createpwup(TSPWUP_SLOWMO)
			given = ts_givepwup(g_bot, ent)
			engclient_cmd(g_bot, "usepowerup")
			server_print("[probe] slow motion: givepwup %d, usepowerup sent", given)
		}
		case 1:
		{
			server_print("[probe] slow motion running: %d (want %d)", ts_is_running_powerup(g_bot), TSPWUP_SLOWMO)
			server_print("[probe] done")
		}
	}
}
