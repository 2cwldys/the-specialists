// rets_slots_probe: an M60, a grenade and a Glock (87 slots) on a fake client that has spawned, with
// the 81 free slots of a spawn: the Glock is refused (4 left). Then ts_setuserslots(200) and the same
// again: the M60 is owned already, a second grenade and the Glock fit (183 left), carrying 94. Before
// reTS dropped its own 81 cap, the Glock was refused there too. Run by scripts/test-win-server.sh with
// ADDONS=<suite zip> PROBE=rets_slots_probe UNTIL='\[probe\] done'.
#include <amxmodx>
#include <fakemeta>
#include <tsx>
#include <tsfun>

#define M60		32
#define M61		24
#define GLOCK18		1

new g_bot
new g_alive
new g_step

public plugin_init()
{
	register_plugin("rets_slots_probe", "1", "reTS")
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

GiveHeavyLoadout()
{
	ts_giveweapon(g_bot, M60, 0, 0)
	ts_giveweapon(g_bot, M61, 1, 0)
	ts_giveweapon(g_bot, GLOCK18, 0, 0)
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
			new before = ts_getuserslots(g_bot)
			GiveHeavyLoadout()
			server_print("[probe] 81 slots: had %d, now %d (want 4)", before, ts_getuserslots(g_bot))
		}
		case 1:
		{
			ts_setuserslots(g_bot, 200)
			new before = ts_getuserslots(g_bot)
			GiveHeavyLoadout()
			server_print("[probe] more slots: had %d, now %d (want 183), grenades %d (want 2)",
				before, ts_getuserslots(g_bot), ts_getuserammo(g_bot, M61))
			server_print("[probe] done")
		}
	}
}
