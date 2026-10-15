// rets_infobuffer_probe: fakemeta's info buffer handles, as plugins use them. Hooks
// FM_ClientUserInfoChanged and reads "team" from the forward's buffer with EngFunc_InfoKeyValue; separately
// takes a buffer from EngFunc_GetInfoKeyBuffer, writes a key with EngFunc_SetClientKeyValue and reads it
// back. In the 64-bit fork before a03e1ba9 those natives used the handle as a raw pointer, so the engine
// was handed an address like 0x1 and crashed in Info_ValueForKey. Prints "[ib] ..." lines and
// "[ib] done". Run by scripts/test-win-server.sh with ADDONS=<suite zip> PROBE=rets_infobuffer_probe.
#include <amxmodx>
#include <fakemeta>

new g_bot

public plugin_init()
{
	register_plugin("rets_infobuffer_probe", "1", "reTS")
	register_forward(FM_ClientUserInfoChanged, "fw_userinfo")
	set_task(3.0, "make_bot")
}

public fw_userinfo(id, buffer)
{
	if (id != g_bot || !g_bot)
		return FMRES_IGNORED
	new got[32]
	engfunc(EngFunc_InfoKeyValue, buffer, "name", got, charsmax(got))
	server_print("[ib] forward buffer %d: name '%s'", buffer, got)
	return FMRES_IGNORED
}

public make_bot()
{
	g_bot = engfunc(EngFunc_CreateFakeClient, "ibprobe")
	new reject[128]
	dllfunc(DLLFunc_ClientConnect, g_bot, "ibprobe", "127.0.0.1", reject)
	dllfunc(DLLFunc_ClientPutInServer, g_bot)
	set_task(1.0, "step")
}

public step()
{
	new buf = engfunc(EngFunc_GetInfoKeyBuffer, g_bot)
	engfunc(EngFunc_SetClientKeyValue, g_bot, buf, "ibprobe", "42")
	new got[32]
	engfunc(EngFunc_InfoKeyValue, buf, "ibprobe", got, charsmax(got))
	new viaCore[32]
	get_user_info(g_bot, "ibprobe", viaCore, charsmax(viaCore))
	server_print("[ib] GetInfoKeyBuffer %d: set ibprobe=42, read back '%s', core reads '%s'", buf, got, viaCore)

	// The engine's own call, with the forward in the path (as a "model <name>" from a client raises it)
	dllfunc(DLLFunc_ClientUserInfoChanged, g_bot)
	server_print("[ib] done")
}
