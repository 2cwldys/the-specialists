// rets_model_probe: on a teamplay server, change a fake client's "model" userinfo the way a player's
// "model <name>" does (set the key, then the game's ClientUserInfoChanged, as the engine calls it) and log
// what the game leaves in the key. Run by scripts/test-win-server.sh with ADDONS=1
// PROBE=rets_model_probe and a GAMECFG that turns teamplay on.
#include <amxmodx>
#include <fakemeta>

new g_bot
new g_step
new const g_try[][] = { "seal", "merc", "agent", "hitman", "gordon", "laurence", "castor", "mycustom" }

public plugin_init()
{
	register_plugin("rets_model_probe", "1", "reTS")
	set_task(3.0, "make_bot")
}

public make_bot()
{
	g_bot = engfunc(EngFunc_CreateFakeClient, "probe")
	new reject[128]
	dllfunc(DLLFunc_ClientConnect, g_bot, "probe", "127.0.0.1", reject)
	dllfunc(DLLFunc_ClientPutInServer, g_bot)
	set_task(2.0, "step", 0, "", 0, "b")
}

public step()
{
	if (g_step >= sizeof g_try) return
	new before[32], after[32], team[32]
	get_user_info(g_bot, "model", before, charsmax(before))
	set_user_info(g_bot, "model", g_try[g_step])
	dllfunc(DLLFunc_ClientUserInfoChanged, g_bot, engfunc(EngFunc_GetInfoKeyBuffer, g_bot))
	get_user_info(g_bot, "model", after, charsmax(after))
	get_user_info(g_bot, "team", team, charsmax(team))
	server_print("[model] force %d team '%s': had '%s', asked '%s', now '%s'",
		get_cvar_num("mp_teammodels_force"), team, before, g_try[g_step], after)
	g_step++
}
