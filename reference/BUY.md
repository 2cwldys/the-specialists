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
| `cashala…` | config string | `GiveCash(...)` + APPLY a config string: parses `weapon_%i` (each queued weapon) and `lasclr_%i` (laser colour) out of the string | 🟡 our `tsbuyarmament`→`BuyArmament()` is the apply stand-in; the faithful trigger is this config-string command (exact name truncated in recovery — `cashala` + suffix) |

Note: `tkw`/`tka` take a SINGLE argv token `"5_2"` (sscanf `"%i_%i"`), not two argv — e.g.
`tkw 5_2` queues 2× weapon type 5. Our handlers parse `argv(1)` splitting on `_`.

## The loadout flow (faithful model)

1. `rst_cfg` clears the queued armament.
2. `tkw`/`tki` queue weapons, `tka` queues ammo (into `m_armament[]`/`m_armamentAmmo[]`).
3. The `cashala…` config command applies the queued config (and the `weapon_%i`/`lasclr_%i`
   string it carries), giving cash + the loadout — the "Use New Config" menu action.

So the original BUYS by **queue (tkw/tki/tka) → apply (cashala config)**, NOT a direct
per-weapon "buy". Our `tsbuy`/`tsbuyammo` (immediate `BuyWeapon`/`BuyAmmo`) and `tsbuyarmament`
(apply) are dev/console stand-ins with no 1:1 original command — kept for testing, flagged as
"extra" by the coverage harness (honest). `lasclr_` (laser colour) is unimplemented (a cl_dll
cosmetic).

## Remaining work (the buy UI)

These commands are emitted by the client's buy/config menu — the full "2 Buy" weapon-category
submenu is still deferred (cont.66). The server command contract is now faithful + console-
testable; wiring the client UI to emit `tkw`/`tki`/`tka`/`rst_cfg` + the `cashala…` apply is the
follow-on. The exact `cashala…` command name + its config-string grammar need one more RE pass
(decompile the `cashala` block's strstr/sscanf chain fully).
</content>
