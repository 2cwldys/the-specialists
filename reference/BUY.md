# Buy / loadout-config command protocol — RE findings (cont.68h)

Recovered from `ts_i386.so` `ClientCommand` (the buy/config dispatch block, PIC base
ebx=0x165E10). These are the console commands the client's buy/config menu sends to the
server to assemble a loadout. The swap contract = the command **names + arg formats**.

## Commands (server ClientCommand dispatch)

| command | arg format | server action | our status |
|---|---|---|---|
| `rst_cfg` | — | `ResetArmament()` (clear the queued loadout) | ✅ renamed (was tsresetarmament) |
| `tkw` | `"%i_%i"` = `type_count` | `AddItemToArmament(type, count, false)` — "take weapon" (queue N of a weapon type) | ✅ renamed (was tsarm) |
| `tki` | `"%i"` = `type` | `AddItemToArmament(type, 1, false)` — "take item" (queue one) | ✅ added |
| `tka` | `"%i_%i"` = `caliber_count` | `AddAmmoToArmament(caliber, count)` — "take ammo" | ✅ renamed (was tsarmammo) |
| `cutwire_<n>` | prefix + `"%i"` | bomb wire cut (`strstr "cutwire_"` + sscanf) | ✅ renamed (was tscutwire) |
| `drop_<n>` | prefix + `"%i"` | drop a specific slot (`strstr "drop_"`) | ⬜ TODO (we have `drop` for the active weapon) |
| `cashala` | — | **CHEAT**: gated by `g_flWeaponCheat` (the engine `sv_cheats` mirror) → `GiveCash(player, 30000)`. NOT a config-apply. | ✅ wired (cont.69, gated on `g_psv_cheats`) |

Note: each token is `"%i_%i"` = `type_count` (split on `_`). **The client (apply FUN_10039400) BATCHES
the whole loadout into ONE space-separated command** — it builds `"tkw "` + a `"%i_%i "` token per
weapon (`tkw 2_1 5_1 9_1`) and sends it with a single ClientCmd; likewise one `tka …`/`tki …`. So the
SERVER handlers MUST loop over EVERY argv token (`CMD_ARGV(1..ARGC-1)`), not just `argv(1)` — parsing
only `argv(1)` drops all but the first weapon (the cont.69p loadout bug). Confirmed by RE of the apply
(`"tkw "` + `"%i_%i "` strings + the three ClientCmd calls at the end of FUN_10039400).

## The loadout flow (faithful model)

1. `rst_cfg` clears the queued armament.
2. `tkw`/`tki` queue weapons, `tka` queues ammo (into `m_armament[]`/`m_armamentAmmo[]`).
3. The queued config is applied to give the loadout (the "Use New Config" menu action).

**CORRECTION (cont.69):** `cashala` is NOT the config-apply command — RE of `ClientCommand`
@0x9dd5d shows it's a **cheat** (`g_flWeaponCheat`-gated → `GiveCash(player, 30000)`), and the
`weapon_%i` parse right after it (@0x9dd98, strstr `"weapon_"` + sscanf `"%i"`) is just the
numeric weapon-select transport (`weapon_<N>`), already handled. The original BUYS by **queue
(tkw/tki/tka) → apply**; **RESOLVED (cont.94):** there is NO apply-trigger command at all —
`BuyArmament` @0x83f10's ONLY caller is `CHalfLifeMultiplay::PlayerSpawn` @0x790ac (call
@0x79122): the queued loadout applies **on spawn** (which our `CTSGameRules::PlayerSpawn` →
`BuyArmament()` already mirrors). The `tsbuy`/`tsbuyammo`/`tsbuyarmament` dev stand-ins were
therefore STRIPPED (cont.94 server_strips) — `BuyWeapon` @0x845bc / `BuyAmmo` @0x841e8 are
called only from inside BuyArmament in the original too. `lasclr_` (laser colour) is
implemented server-side (client.cpp rebroadcast).

## Remaining work (the buy UI)

These commands are emitted by the client's buy/config menu — the full "2 Buy" weapon-category
submenu is still deferred (cont.66). The server command contract is now faithful + console-
testable; wiring the client UI to emit `tkw`/`tki`/`tka`/`rst_cfg` + the `cashala…` apply is the
follow-on. The exact `cashala…` command name + its config-string grammar need one more RE pass
(decompile the `cashala` block's strstr/sscanf chain fully).
</content>
