/***
*  ts_buymenu.h — The Specialists REAL buy/config menu (cont.69l).
*
*  A faithful reconstruction of the menu the game actually draws — the HUD-drawn cascade
*  (Config | Categories | Weapons | Detail columns all visible at once), RE'd from
*  client.dll's 0x10037xxx cluster (orchestrator FUN_10036750, builders FUN_10037540/
*  100377d0/10037960/10037ba0, header+preview FUN_10036af0, input FUN_10038840, mouse
*  FUN_10038360/10038410, apply FUN_10039400). See reference/MENUS.md "★★ THE REAL BUY MENU".
*
*  NOT vgui: it draws with TSHud_DrawString + the TS HUD font and reads number keys (via
*  HUD_Key_Event) AND mouse clicks (via IN_MouseEvent) — exactly as the original. "Use New
*  Config" emits the swap-contract commands rst_cfg / tkw (reference/BUY.md).
***/
#pragma once

class CHudTSBuyMenu : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float flTime) override;
	void Reset() override;

	void Open();
	void Close();
	bool IsUp() const { return m_active; }

	// slotN while the menu is up (client.dll 0x1004f220 -> 0x10038840(obj, current level, N)):
	// drives the current column.
	void HandleSlot(int slot);
	// A mouse button state changed (IN_MouseEvent) — FUN_10038360: left-click an entry fires its key
	// (miss -> key 1), right-click -> key 10 (Go Back). Hit-test = FUN_10038410.
	void HandleMouse(int buttons);

private:
	enum
	{
		LVL_CONFIG = 0,
		LVL_CATEGORY = 1,
		LVL_WEAPONS = 2,
		LVL_DETAIL = 3,
		LVL_ACCESSORY = 4,
		LVL_REMOVE = 5,
		LVL_SAVE = 6,
		LVL_LOAD = 7,
		LVL_CHANGETEAM = 8
	};

	struct Slot
	{
		bool used;
		bool accessory;
		int weaponId;
		int attachMask;
		int cost;
		bool akimbo;
		int count;
	};

	enum { MAX_SLOTS = 16, SAVE_SLOTS = 9 };

	bool m_active = false;     // obj+0
	int m_level = LVL_CONFIG;  // obj+4
	int m_cat = 0;             // obj+8
	int m_attachMask = 0;      // obj+0xc
	int m_weapon = -1;         // obj+0x10
	Slot m_slots[MAX_SLOTS];   // obj+0x94..
	int m_spent = 0;           // obj+0x414 (weapons+attachments+auto-ammo, the displayed Total)
	int m_slotsUsed = 0;       // obj+0x418
	bool m_useNewConfig = true; // obj+0x2f08
	int m_ammoRounds[16] = {}; // obj+0x14 : auto-bought rounds per caliber (for the preview)
	int m_ammoUnits[16] = {};  // obj+0x18 : units bought per caliber (emitted as `tka cal_units`)
	Slot m_saved[SAVE_SLOTS][MAX_SLOTS]; // obj+0x41c.. : the 9 save/load config slots
	bool m_savedFull[SAVE_SLOTS] = {};
	Slot m_lastConfig[MAX_SLOTS] = {};   // obj+0x41c slot 0 : the auto "last config" (open loads it; apply saves it)
	bool m_lastConfigValid = false;
	bool m_firstOpen = true;             // obj+0x2f10 : first open per map -> randomize ("Use New Config")

	int m_prevButtons = 0; // obj+0x2f18 : mouse-button edge tracking

	// preview sprites (the bought-weapon icons in the header, FUN_10036af0) — same files the
	// weapon-select bar loads (sprites/weapons/<name>.spr).
	HSPRITE m_hIcon[40];
	Rect m_rcIcon[40];

	// helpers
	int  TeamCount() const;
	bool Restricted(int id) const;
	bool AttachAvailable(int id, int bit) const;
	int  WeaponCost(int id, int attachMask, bool akimboOrFull) const;
	int  WeaponSlotCost(int id, bool akimboOrFull) const;
	bool QueueWeapon(int id, int attachMask, bool akimboOrFull, bool accessory);
	void RemoveSlot(int i);
	void ResetConfig();
	void BuyRandom();
	void Apply();
	void Recount();

	// the unified input handler (FUN_10038840) — dispatch `key` at `level`. Keyboard passes the
	// current level; the mouse passes the clicked column's level.
	void Dispatch(int level, int key);
	// FUN_10038410 — map a cursor pos to (column-level, key); sets level = -1 on a miss.
	void MouseToEntry(int mx, int my, int& level, int& key);

	// rendering
	struct Cursor { int x, y, rowH, width; };
	void SetCursor(Cursor& c, int x, int y, int rowH, int width);
	void DrawEntry(Cursor& c, const char* text, int r, int g, int b);
	void DrawHeader();
	void DrawLoadoutPreview(); // FUN_10036af0 : the 5 bought-weapon columns at the top
	void DrawColumnConfig();
	void DrawColumnCategory();
	void DrawColumnWeapons();
	void DrawColumnDetail();
	void DrawColumnAccessory();
	void DrawColumnSaveLoad(bool save);
	void DrawColumnRemove();
	void DrawColumnChangeTeam();
	void DrawWeaponPreview(int id);
};
