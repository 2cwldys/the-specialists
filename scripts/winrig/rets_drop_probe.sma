// rets_drop_probe: for "drop on the last gun leaves it in hand". On a player's "drop" command, logs the
// held weapon (TSX), the view and player models and the dropped guns on the floor (ts_groundweapon), then
// the same 0.2 s later. Kung fu is weapon 0 with an empty view model. Run by scripts/test-win-server.sh
// with ADDONS=<suite zip> PROBE=rets_drop_probe.
#include <amxmodx>
#include <fakemeta>
#include <tsfun>

public plugin_init()
{
	register_plugin("rets_drop_probe", "1", "reTS")
}

public client_command(id)
{
	new cmd[16]
	read_argv(0, cmd, charsmax(cmd))
	if (equal(cmd, "drop"))
	{
		report(id, "before")
		new p[1]
		p[0] = id
		set_task(0.2, "after", 0, p, 1)
	}
	return PLUGIN_CONTINUE
}

public after(p[])
{
	report(p[0], "after")
}

report(id, const when[])
{
	if (!is_user_connected(id))
		return
	new clip, ammo, mode, extra
	new w = ts_getuserwpn(id, clip, ammo, mode, extra)
	new vm[64], pm[64]
	pev(id, pev_viewmodel2, vm, charsmax(vm))
	pev(id, pev_weaponmodel2, pm, charsmax(pm))
	new n, ent = -1
	while ((ent = engfunc(EngFunc_FindEntityByString, ent, "classname", "ts_groundweapon")) > 0)
		n++
	server_print("[drop] %s %d: weapon %d clip %d, view '%s', player '%s', guns on floor %d",
		when, id, w, clip, vm, pm, n)
	log_amx("[drop] %s %d: weapon %d clip %d, view '%s', player '%s', guns on floor %d",
		when, id, w, clip, vm, pm, n)
}
