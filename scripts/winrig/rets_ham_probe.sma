// rets_ham_probe: are Ham Sandwich's slots right for the running game library? Hooks Spawn, Use, Touch,
// TakeDamage and Killed, then makes the GAME call each virtual (DispatchSpawn, DispatchUse, a
// trigger_hurt), never ExecuteHam: a hook registered at a wrong slot still fires when called
// through that same slot, so only the game's own call tells. Prints one "[ham] <name> fired|MISSING" line
// each. Run by scripts/test-win-server.sh with ADDONS=... PROBE=rets_ham_probe.
#include <amxmodx>
#include <fakemeta>
#include <hamsandwich>

new g_bot, g_button
new g_spawn, g_use, g_touch, g_damage, g_killed
new Float:g_dmg

public plugin_init()
{
	register_plugin("rets_ham_probe", "1", "reTS")
	// localinfo "hamhooks" (test-win-server.sh HAMHOOKS=) picks the hooks, e.g. "spawn.use" (the engine splits commas); default all
	new only[128]
	get_localinfo("hamhooks", only, charsmax(only))
	if (!only[0] || containi(only, "spawn") != -1) RegisterHam(Ham_Spawn, "player", "on_spawn", 1)
	if (!only[0] || containi(only, "takedamage") != -1) RegisterHam(Ham_TakeDamage, "player", "on_damage")
	if (!only[0] || containi(only, "killed") != -1) RegisterHam(Ham_Killed, "player", "on_killed")
	if (!only[0] || containi(only, "use") != -1) RegisterHam(Ham_Use, "func_button", "on_use")
	if (!only[0] || containi(only, "touch") != -1) RegisterHam(Ham_Touch, "trigger_hurt", "on_touch")
	server_print("[ham] hooks: %s", only[0] ? only : "all")
	set_task(3.0, "make_bot")
}

public on_spawn(id)    { if (id == g_bot) g_spawn++; }
public on_damage(id, inflictor, attacker, Float:dmg, bits) { if (id == g_bot) { g_damage++; g_dmg = dmg; } }
public on_killed(id, attacker, gib) { if (id == g_bot) g_killed++; }
public on_use(ent, caller, activator, type, Float:value) { if (ent == g_button) g_use++; }
public on_touch(ent, other) { if (other == g_bot) g_touch++; }

public make_bot()
{
	g_bot = engfunc(EngFunc_CreateFakeClient, "hamprobe")
	new reject[128]
	dllfunc(DLLFunc_ClientConnect, g_bot, "hamprobe", "127.0.0.1", reject)
	dllfunc(DLLFunc_ClientPutInServer, g_bot)
	set_task(1.0, "step1")
}

public step1()
{
	// Spawn: the game's DispatchSpawn calls CBasePlayer::Spawn through the vtable.
	dllfunc(DLLFunc_Spawn, g_bot)

	// Use: a func_button pressed through the game's DispatchUse.
	g_button = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "func_button"))
	dllfunc(DLLFunc_Spawn, g_button)
	dllfunc(DLLFunc_Use, g_button, g_bot)

	// Touch + TakeDamage: a trigger_hurt touching the player; CTriggerHurt::HurtTouch calls
	// pOther->TakeDamage through the vtable.
	new hurt = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "trigger_hurt"))
	set_pev(hurt, pev_dmg, 7.0)
	dllfunc(DLLFunc_Spawn, hurt)
	set_pev(g_bot, pev_takedamage, 2.0) // DAMAGE_AIM
	set_pev(g_bot, pev_health, 100.0)
	set_pev(g_bot, pev_deadflag, 0)
	dllfunc(DLLFunc_Touch, hurt, g_bot)

	set_task(1.0, "step2")
}

public step2()
{
	// Killed: a trigger_hurt the player cannot survive; CBasePlayer::TakeDamage calls Killed through
	// the vtable.
	new hurt = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "trigger_hurt"))
	set_pev(hurt, pev_dmg, 1000.0)
	dllfunc(DLLFunc_Spawn, hurt)
	set_pev(g_bot, pev_takedamage, 2.0)
	set_pev(g_bot, pev_health, 100.0)
	set_pev(g_bot, pev_deadflag, 0)
	dllfunc(DLLFunc_Touch, hurt, g_bot)
	set_task(1.0, "report")
}

public report()
{
	server_print("[ham] Spawn %s (%d)", g_spawn ? "fired" : "MISSING", g_spawn)
	server_print("[ham] Use %s (%d)", g_use ? "fired" : "MISSING", g_use)
	server_print("[ham] Touch %s (%d)", g_touch ? "fired" : "MISSING", g_touch)
	server_print("[ham] TakeDamage %s (%d, %.1f)", g_damage ? "fired" : "MISSING", g_damage, g_dmg)
	server_print("[ham] Killed %s (%d)", g_killed ? "fired" : "MISSING", g_killed)
}
