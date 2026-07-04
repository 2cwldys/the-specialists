# TS bottom-left STATUS HUD — exact draw geometry (cont.87)

**Target binary:** `reference/client.dll` (Windows MSVC).
**Draw function:** `FUN_1004a3e0` @ **VA 0x1004a3e0 / RVA 0x4a3e0** (`CHudTSStatus::Draw`).
**Sprite loader:** `FUN_10049d10` @ VA 0x10049d10 (`CHudTSStatus::VidInit`).
Raw decompiles: `decomp/status_hud_draw.c`.

Found by xref on the layout strings: `"#HUD_FreeSlots"` (VA 0x10095ad4, pushed @0x1004a8b8, inside
`FUN_1004a3e0`) and the figure sprites (`sprites/player/stand.spr` etc, loaded @0x10049d1f in
`FUN_10049d10`). NOTE: `"Free Slots : %i"` (0x100929f8) and `"%i Kevlar  %i"` (0x10095080) that the
task cited belong to the **BUY MENU** (`FUN_10011a60` / `FUN_10037fe0`), *not* the status HUD — the
status free-slots label is the localized token **`#HUD_FreeSlots`** (renders lowercase "free slots",
matching the screenshot). The status HUD has **no Kevlar text**; armor is a Kevlar **sprite** only.

---

## Engine call table (resolved from the GOT / fn-ptr globals)

| ptr | function | signature used here |
|---|---|---|
| `DAT_100d5b28` | SPR_Load | `(path)` |
| `DAT_100d5b30` | SPR_Height | `(spr, frame)` |
| `DAT_100d5b34` | SPR_Width | `(spr, frame)` |
| `DAT_100d5b38` | SPR_Set | `(spr, r, g, b)` |
| `DAT_100d5b44` | SPR_DrawAdditive | `(frame, x, y, wrect*)` |
| `DAT_100d5b54` | **FillRGBA** | `(x, y, w, h, r, g, b, a)` |
| `DAT_100d5b90` | DrawCharacter | `(x, y, ch, r, g, b)` |
| `DAT_100d5bf4` | GetPlayerInfo (local team) | `() -> int* {teamidx,…}` |
| `FUN_1004c290` | **DrawHudNumber** (numbers.spr = `DAT_100cf920`) | `(spr, x, y, flags, val, r, g, b) -> xEnd` |
| `FUN_10021ce0` | **DrawConsoleString** | `(x, y, clipX, str, r, g, b) -> xEnd` |
| `FUN_100349a0` | Localize / LookupString | `("#token") -> char*` |
| `DAT_100d5908` | **ScreenWidth (SW)** | |
| `DAT_100d590c` | **ScreenHeight (SH)** | |

**Colors.** Three HUD-color cvars' `.value` (cvar+0xc) → `ftol` → `hudR,hudG,hudB`
(globals `1018a474 / 1018a598 / 1018a480`). Two derived tints are used everywhere:
- **halfHud** = `(hudR/2, hudG/2, hudB/2)` — normal element color (numbers, sprites, filled figure).
- **warnRed** = `(0xC3,0x2D,0x2D)` = **(195,45,45)** — the "low" color.
- Box backgrounds use the **full** `(hudR,hudG,hudB)`; the **team** box uses the **team color**.

**IMPORTANT — coordinate system.** All **X** offsets are **ABSOLUTE PIXELS** (8, 72, 104, 136, 144),
NOT fractions of ScreenWidth. Only **Y** is screen-relative (bottom-anchored: `SH − k`). Box width is a
fixed **0x7f = 127px**. This is a fixed-pixel bottom-left layout — do **not** scale X by ScreenWidth
(our current code's `ScreenWidth/40` margins + auto-size are the bug). ScreenWidth is used only for the
text clip-X and to center the (out-of-scope) kungfu/movement icons at `SW/2`.

---

## Layout diagram (bottom-left; y grows down, SH = ScreenHeight)

```
 x=8                         x=135  x=136                      x=263
 ┌──────────────────────────────┐
 │ TEAM box  "humans"           │   y = SH-68  h=19  a=0x23(35)  ← team color   (only in team mode)
 ├──────────────────────────────┤
 │ [fig]    100  ♥              │   y = SH-48  h=39  a=0x0f(15)  ← hud color
 │  gauge   (num)(heart)        │                                (D CASH box sits here-ish, x≥136)
 │          [kevlar if armor]   │        ┌──────────────────────────────┐
 │                              │        │ 40  free slots               │  y = SH-28 h=19 a=0x0f
 └──────────────────────────────┘        └──────────────────────────────┘   FREE-SLOTS box (x=136)
                                                                              bottom-aligns w/ health box
```

## Every draw call (exact constants)

Order as emitted. `SH`=ScreenHeight, `SW`=ScreenWidth. Colors: **hud**=(hudR,hudG,hudB),
**half**=(hudR/2,…), **red**=(195,45,45).

### B. HEALTH box (always) — drawn FIRST
| # | call | x | y | w/rect | h | color | alpha | note |
|---|---|---|---|---|---|---|---|---|
| B1 | FillRGBA | 8 | SH−48 | 127 | 39 | hud | **0x0f=15** | main box |
| B2 | SPR figure (red top) | 8 | SH−52 | rect{0,W,0,missing} | — | red | — | only if health<100; "missing" portion |
| B3 | SPR figure (fill bottom) | 8 | SH−52+missing | rect{0,W,missing,figH} | — | half | — | health-fill silhouette gauge |
| B4 | DrawHudNumber(health) | 72 | SH−44 | flags=5 | — | health<50 ? red : half | — | value=health |
| B5 | SPR heart (+0x58) | 104 | SH−48 | — | — | health<50 ? red : half | — | |
| B6 | SPR kevlar (+0x5c) | 72 | SH−28 | — | — | armor<50 ? red : half | — | **only if armor>0** |

`missing = figH − min(health,100)*figH/100` (the gauge empties from the top). `figH`=SPR_Height,
`W`=SPR_Width of the current posture sprite (`+0x38` stand / `+0x3c` crouched / `+0x40` prone, indexed
by posture byte `+0x22`).

### A. TEAM box (only if in team mode; `DAT_100cf97c != 0`) — drawn SECOND
| # | call | x | y | w | h | color | alpha |
|---|---|---|---|---|---|---|---|
| A1 | FillRGBA | 8 | SH−68 | 127 | 19 | **team color** | **0x23=35** |
| A2 | DrawConsoleString(teamName) | 10 | SH−68 | clip=SW | — | team color | — |

Team color = `iTeamColors[teamnumber % iNumberOfTeamColors]`; teamName from the local player's info.

### C. FREE-SLOTS box (always) — drawn THIRD
| # | call | x | y | w | h | color | alpha |
|---|---|---|---|---|---|---|---|
| C1 | FillRGBA | 136 | SH−28 | 127 | 19 | hud | 0x0f=15 |
| C2 | DrawHudNumber(freeSlots) | 144 | SH−26 | flags=3, val=freeSlots | — | slots==0 ? red : half |
| C3 | DrawConsoleString(Localize("#HUD_FreeSlots")) | (C2 xEnd)+8 | SH−28 | clip=SW | half/red | — |

### D. CASH/CREDITS box (only if `teams>1` OR buy-enabled; `1<DAT_100cf97c \|\| DAT_1018d170!=0`)
| # | call | x | y | w | h | color |
|---|---|---|---|---|---|---|
| D1 | FillRGBA | 136 | SH−48 | 127 | 19 | hud @0x0f |
| D2 | DrawHudNumber(cash) | 136 | SH−46 | flags=0x11, val=cash | cash<1000 ? red : half |
| D3 | DrawConsoleString(Localize("#HUD_Credits")) | (D2 xEnd)+8 | SH−48 | — | half/red |

*(Out of scope: center-bottom kungfu `+0x64` @ `(SW/2−48, SH−56)` if `+0x29`, and movement `+0x68`
@ `(SW/2, SH−56)` if `+0x2a`.)*

---

## Value → our-field mapping

| original (offset) | value | our field |
|---|---|---|
| +0x20 (byte) | health | `m_iHealth` |
| +0x21 (byte) | armor  | `m_iArmor` |
| +0x22 (byte) | posture (0 stand/1 crouch/2 prone) | `m_iState` |
| +0x24 (int)  | cash   | `m_iCash` |
| +0x28 (byte) | **free slots** | `m_iSpace` |
| team name / color | — | `g_PlayerExtraInfo[localidx].teamname` / `iTeamColors[teamnumber % iNumberOfTeamColors]` |

Free-slots VALUE source = `m_iSpace` (TSSpace byte) — **confirmed** (`+0x28`, drawn with `#HUD_FreeSlots`).

---

## Confidence

| claim | conf |
|---|---|
| Draw fn = FUN_1004a3e0; all box X/Y/W/H + alphas (0x0f, 0x23) | **HIGH** (literal disasm constants) |
| Team box uses team color @ alpha 35, sits above health box | **HIGH** |
| Free-slots label = Localize("#HUD_FreeSlots") ("free slots"), value = m_iSpace, box at x=136 | **HIGH** |
| Figure drawn as a two-tone vertical health-fill gauge (red missing / half-hud filled) | **HIGH** |
| Heart x=104 / health num x=72 / kevlar sprite x=72,y=SH-28 (armor>0 only) | **HIGH** |
| Cash box gate = `teams>1 \|\| buy-enabled` | **MEDIUM** (global-flag interpretation) |
| DrawHudNumber flag bits (5/3/0x11) = digit-count/draw-zero; renders "100"/"40" naturally | **MEDIUM** (not geometry-critical) |
| Team-mode gate `DAT_100cf97c != 0` ≈ "teamname non-empty" in our drop-in | **MEDIUM** (approximation) |

---

## Drop-in `CHudTSStatus::Draw` body

Uses our helpers (`FillRGBA`, `SPR_*`, `TSHud_DrawNumber`, `TSHud_DrawString`, `TSHud_Color`) with the
original's absolute pixel coords. Needs `#include "vgui_TeamFortressViewport.h"` (for `iTeamColors` /
`iNumberOfTeamColors`) — `player_info.h` (g_PlayerExtraInfo) already included.

```cpp
bool CHudTSStatus::Draw(float flTime)
{
    if ((m_iFlags & HUD_ACTIVE) == 0)
        return true;
    if (gHUD.m_iHideHUDDisplay & (HIDEHUD_HEALTH | HIDEHUD_ALL))
        return true;
    if (gHUD.m_Spectate.IsActive())
        return true;

    int hr, hg, hb;
    TSHud_Color(hr, hg, hb);
    const int halfR = hr / 2, halfG = hg / 2, halfB = hb / 2; // element tint
    const int wR = 195, wG = 45, wB = 45;                     // warn/low color

    const int SW = ScreenWidth;
    const int SH = ScreenHeight;
    const int BOXW = 0x7f; // 127px fixed

    // Posture figure sprite (server TSState byte drives it).
    HSPRITE hFig = m_hStand; Rect* rcF = &m_rcStand;
    if (m_iState == 2 && m_hProne)  { hFig = m_hProne;  rcF = &m_rcProne; }
    else if (m_iState == 1 && m_hCrouch) { hFig = m_hCrouch; rcF = &m_rcCrouch; }

    // ---- B. HEALTH box (always) ----
    const int hbY = SH - 0x30;                 // SH-48
    FillRGBA(8, hbY, BOXW, 0x27, hr, hg, hb, 0x0f);

    if (hFig != 0)
    {
        const int figW = rcF->right - rcF->left;
        const int figH = rcF->bottom - rcF->top;
        const int cap = (m_iHealth < 100) ? m_iHealth : 100;
        const int missing = figH - (cap * figH) / 100; // empties from the top
        const int figX = 8, figY = hbY - 4;            // SH-52
        if (m_iHealth < 100)
        {
            Rect rt = { rcF->left, rcF->right, rcF->top, rcF->top + missing };
            SPR_Set(hFig, wR, wG, wB);
            SPR_DrawAdditive(0, figX, figY, &rt);
        }
        Rect rb = { rcF->left, rcF->right, rcF->top + missing, rcF->bottom };
        SPR_Set(hFig, halfR, halfG, halfB);
        SPR_DrawAdditive(0, figX, figY + missing, &rb);
    }

    // health number @72 + heart @104
    {
        const bool lo = m_iHealth < 50;
        const int nr = lo ? wR : halfR, ng = lo ? wG : halfG, nb = lo ? wB : halfB;
        TSHud_DrawNumber(0x48, hbY + 4, m_iHealth, nr, ng, nb); // (72, SH-44)
        if (m_hHeart != 0)
        {
            SPR_Set(m_hHeart, nr, ng, nb);
            SPR_DrawAdditive(0, 0x68, hbY, &m_rcHeart);          // (104, SH-48)
        }
    }
    // kevlar sprite (armor>0) @ (72, SH-28)
    if (m_iArmor > 0 && m_hKevlar != 0)
    {
        const bool lo = m_iArmor < 50;
        SPR_Set(m_hKevlar, lo ? wR : halfR, lo ? wG : halfG, lo ? wB : halfB);
        SPR_DrawAdditive(0, 0x48, hbY + 0x14, &m_rcKevlar);      // (72, SH-28)
    }

    // ---- A. TEAM box (team mode only), above the health box ----
    cl_entity_t* lp = gEngfuncs.GetLocalPlayer();
    if (lp != nullptr)
    {
        const char* team = g_PlayerExtraInfo[lp->index].teamname;
        if (team && team[0])
        {
            int tn = g_PlayerExtraInfo[lp->index].teamnumber;
            if (tn < 0) tn = 0;
            tn %= iNumberOfTeamColors;
            const int tr = iTeamColors[tn][0], tg = iTeamColors[tn][1], tb = iTeamColors[tn][2];
            const int tbY = SH - 0x44;                 // SH-68
            FillRGBA(8, tbY, BOXW, 0x13, tr, tg, tb, 0x23); // alpha 35, team color
            TSHud_DrawString(10, tbY, SW, team, tr, tg, tb);
        }
    }

    // ---- C. FREE-SLOTS box (always), to the right ----
    {
        const int fbY = SH - 0x1c;                     // SH-28
        FillRGBA(0x88, fbY, BOXW, 0x13, hr, hg, hb, 0x0f); // (136, SH-28)
        const bool lo = m_iSpace == 0;
        const int nr = lo ? wR : halfR, ng = lo ? wG : halfG, nb = lo ? wB : halfB;
        int xEnd = TSHud_DrawNumber(0x90, fbY + 2, m_iSpace, nr, ng, nb); // (144, SH-26)
        const char* lbl = gHUD.m_TextMessage.LookupString("#HUD_FreeSlots"); // -> "free slots"
        if (!lbl || !lbl[0]) lbl = "free slots";
        TSHud_DrawString(xEnd + 8, fbY, SW, lbl, nr, ng, nb);
    }

    return true;
}
```

**Notes for the implementer**
- `Rect` field order in our SDK is `{left, right, top, bottom}` (see `TSHud_DrawNumber`); the vertical
  gauge clips via `top`/`bottom`.
- Cash box (D) is omitted from the drop-in (needs the `usecash`/team-count gate; add later behind a gate
  matching `g_iTSUseCash` — MEDIUM confidence). The screenshot shows no cash box, so leaving it out
  matches the target.
- `TSHud_DrawNumber` renders natural digits (no leading zeros) → "100" and "40", matching the image.
- If `gHUD.m_TextMessage.LookupString` isn't wired for the token, the literal fallback "free slots"
  keeps parity.
```
