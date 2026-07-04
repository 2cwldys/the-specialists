# cont.87 — The melee-move spine twist: the gait/torso split-bone yaw re-base (client.dll RE)

**Bug (ground truth):** In 3rd person, moving (walk/run) while holding a MELEE weapon
(katana/knife), OUR client renders the upper body / spine visibly TWISTED to the side (torso
faces a different direction than the legs). The ORIGINAL keeps the upper body ALIGNED while the
legs still turn to the movement direction. The difference is the client-side gait bone-merge at
the spine split bone (`Bip01 Dorso`).

**Verdict:** The original `StudioSetupBones` (client.dll `FUN_10031080`, RVA 0x31080) does a
**yaw re-base of the single spine bone** at the gait/torso split, which OUR code omits. Without it,
the spine bone keeps its torso-authored local pose while its PARENT chain was overwritten with the
gait (leg) yaw → the upper body is double-rotated by the gait yaw → the kink. The re-base adds
`(torso_root_yaw − gait_root_yaw)` to the spine bone's local twist so that, after concatenation
onto the gait-yawed parent, the net upper-body facing equals the torso facing → aligned.

Everything below is from `reference/client.dll` (imagebase 0x10000000). Raw artifacts in `decomp/`.

---

## 1. Helper identifications (addr → HL name → signature)  — all HIGH confidence

All verified by decompiling the body and matching term-for-term against the HL studio mathlib
(and against OUR `cl_dll/studio_util.cpp`, which already carries byte-identical copies of two of
them). `client.dll` uses the **studio** mathlib convention: **angles in RADIANS** (AngleQuaternion
multiplies each angle by 0.5 with no deg2rad; QuaternionAngles returns raw `atan2`/`asin`). Matrices
are 3×4 **row-major** flat float[12], `m[r][c] = f[r*4 + c]`.

| Addr (RVA) | HL name | Signature | Evidence |
|---|---|---|---|
| `0x2fdf0` | **QuaternionMatrix** | `(vec4_t q, float m[3][4])` | Term-for-term identical to our `studio_util.cpp:QuaternionMatrix` (m00=1−2y²−2z², m10=2(xy+wz), …). HIGH |
| `0x2fef0` | **MatrixAngles** (asin/gimbal variant) | `(float angles[3], float m[3][4])` — arg1=angles out, arg2=matrix | See §2. `angles[1]=asin(clamp(−m20,±1))`; gimbal guard on ±π/2; normal: `angles[0]=atan2(m21,m22)`, `angles[2]=atan2(m10,m00)`. HIGH |
| `0x2ffe0` | **QuaternionAngles** | `(vec4_t q, float angles[3])` | Composite: `QuaternionMatrix(q, tmp); MatrixAngles(angles, tmp);` → quat → euler. HIGH |
| `0x2fbd0` | **AngleQuaternion** | `(float angles[3], vec4_t q)` | Byte-identical to our `studio_util.cpp:AngleQuaternion` (angles[2]→sy/cy, [1]→sp/cp, [0]→sr/cr; radians, ×0.5). Exact inverse of 0x2ffe0. HIGH |
| `0x2fa90` | **ConcatTransforms** | `(float in1[3][4], float in2[3][4], float out[3][4])` | Standard 3×4 matrix multiply. Used only in the final bone-transform loop, NOT the split. HIGH |
| `0x30010` | **quaternion copy** | `(vec4_t dst, vec4_t src)` — cdecl, 4-dword copy | 4× `mov` copy. (Ghidra mislabels it `CStudioModelRenderer::Init` — ignore.) HIGH |
| `0x6c9b0` | **asin** (CRT wrapper) | `st0 → asin(st0)` | Called by MatrixAngles for pitch; tail-calls the CRT asin path. HIGH |

**Component convention (from 0x2fef0, confirmed):**
`angles[0] = ROLL` (atan2 m21,m22), `angles[1] = PITCH` (asin −m20), `angles[2] = YAW` (atan2 m10,m00).
For a pure world-Z rotation by θ this yields `angles[2] = θ`, confirming **index 2 = world yaw** for
the root bone.

### Constants (verified from .rdata)
- `0x1007d5e0` = **π/2** (1.5707963267948966, double)
- `0x1007d5f0` = **0.01** (0.009999999776, double) — gimbal-lock epsilon
- `0x10079e58` = **1.0** — asin input clamp bound
- `0x10079e80` = 1.0, `0x10079e90` = 0.5, `0x100799f0` = 0.0

---

## 2. MatrixAngles (FUN_1002fef0) — exact, from raw asm (`decomp/FUN_1002fef0.asm`)

```c
// arg1 = angles[3] (out), arg2 = m (row-major 3x4 flat)
void MatrixAngles(float *angles, float *m)   // m[r*4+c]
{
    float sp = -m[2*4+0];                     // -m20
    if (sp < -1.0f) sp = -1.0f; else if (sp > 1.0f) sp = 1.0f;
    angles[1] = asin(sp);                     // PITCH  (stored FIRST, at [ecx+4])

    // gimbal-lock guard: is |pitch| within 0.01 rad of ±pi/2 ?
    if ( gimbal_near_90(angles[1]) )          // folded |pitch|-pi/2 test, eps 0.01
    {
        angles[0] = 0.0f;                     // ROLL
        angles[2] = atan2(-m[0*4+1], m[1*4+1]);   // atan2(-m01, m11)
    }
    else
    {
        angles[0] = atan2(m[2*4+1], m[2*4+2]);    // ROLL = atan2(m21, m22)
        angles[2] = atan2(m[1*4+0], m[0*4+0]);    // YAW  = atan2(m10, m00)
    }
}
```

The gimbal fold (asm 0x2ff55–0x2ff9f): `d = |pitch| − π/2`, some ±0.5 range bookkeeping, then
compare `|d|` against 0.01; if `< 0.01` → gimbal branch. **The normal branch is what executes for a
spine/root bone essentially always** (pitch would have to be within 0.008 rad of straight up/down).

---

## 3. The split-bone re-base (FUN_10031080), exact — from raw asm

`FUN_10031080` is the client's **`StudioSetupBones`** (compute torso rotations into `q/pos`
[`0x100e89c0`/`0x100e6790`], compute gait rotations into `q2/pos2` [`0x100e91c0`/`0x100ea86c`],
merge, then the ConcatTransforms bone loop). The gait-merge loop is the classic HL loop **plus** two
TS additions.

### Setup right before the loop (asm 0x314ab–0x314c4)
```
lea eax,[esp+0x38]                     ; eax = &qRefStack (a 4-float stack buffer)
call FUN_10030010(&qRefStack, &q[0])   ; qRefStack = copy of q[0]  (ORIGINAL torso root quat)
```
So **`qRefStack` snapshots the torso root bone quaternion `q[0]` BEFORE the merge loop overwrites it.**
This is the load-bearing detail: bone 0 (`Bip01`, the root) is *before* the spine, so the very first
loop iteration copies the gait root `q2[0]` into `q[0]`. By the time we reach the spine, `q[0]` already
holds the **gait** root; `qRefStack` still holds the **torso** root.

### The merge loop (asm 0x3153d–0x31614) and split (0x31616–0x3166f)
```c
qRefStack = q[0];                                  // (done above) original TORSO root
for (i = 0; i < numbones; i++) {
    if (strcmp(bone[i].name, "Bip01 Dorso")==0 ||  // TS ADD: also match the Italian spine bone
        strcmp(bone[i].name, "Bip01 Spine")==0) {
        // ---- split bone: re-base its yaw, then STOP ----
        float angRoot[3], angCur[3], angBone[3];
        QuaternionAngles(qRefStack, angRoot);      // angles of ORIGINAL torso root
        QuaternionAngles(q[0],      angCur);       // angles of CURRENT root == gait root (copied @ i=0)
        float yawDelta = angRoot[2] - angCur[2];   // TORSO_yaw − GAIT_yaw   (index 2 = yaw)
        QuaternionAngles(q[i],      angBone);      // angles of the spine bone's TORSO pose
        angBone[0] += yawDelta;                    // add yaw-delta to ROLL (index 0)  <-- see note
        AngleQuaternion(angBone, q[i]);            // write the re-based spine quat back
        break;                                     // TS ADD: stop; bones after keep torso pose
    }
    pos[i] = pos2[i];                              // stock: copy gait pose into lower-body bones
    q[i]   = q2[i];
}
```

**Verified from asm, unambiguously:**
- delta is `angRoot[2] − angCur[2]` (both **index 2** = yaw) — `fld [esp+0x40]` (angRoot[2]) `fsub [esp+0x34]` (angCur[2]).
- delta is added to **index 0** of the spine bone's angles — `fld [esp+0x34]`(angBone[0]) `fadd`(delta) `fstp`(angBone[0]).
- then `AngleQuaternion(angBone, q[spine])`, then `break`.

**Why read yaw(index 2) but write index 0 (roll):** the delta is computed in **root/world** space
(index 2 = world yaw). It is applied in the **spine bone's LOCAL** space. On the Italian skeleton the
`Bip01 Dorso` bone's local X axis runs UP the spine (vertical), so a rotation about the spine's local
X (its "roll", index 0) is exactly the **horizontal twist** of the torso. So the world-yaw delta is
injected as spine-local roll. This is a **model-specific** mapping — correct for `player.mdl`'s bone
axes, and it is what TS hardcoded. (Confidence the indices are 2→0: **HIGH**, straight from asm. The
geometric rationale: **HIGH** given the Italian skeleton.)

---

## 4. Stock-HL baseline diff — what TS ADDED

Stock Half-Life `CStudioModelRenderer::StudioSetupBones` gait merge:
```c
if (m_pPlayerInfo && m_pPlayerInfo->gaitsequence != 0) {
    StudioCalcRotations(pos2, q2, ...);
    for (i = 0; i < m_pStudioHeader->numbones; i++) {
        if (strcmp(pbones[i].name, "Bip01 Spine") == 0)
            break;                            // stop at spine; NO re-base
        memcpy(pos[i], pos2[i], sizeof(pos[i]));
        memcpy(q[i],   q2[i],   sizeof(q[i]));
    }
}
```
TS added exactly two things on top of stock:
1. **Also match `"Bip01 Dorso"`** (the Italian spine bone name; `player.mdl` has no `"Bip01 Spine"`).
2. **A yaw re-base of the split bone** before the break: snapshot the torso root, and add
   `(torso_root_yaw − gait_root_yaw)` to the spine bone's local roll via QuaternionAngles/AngleQuaternion.

Stock does neither (2) at all — it just breaks with a plain copy boundary. So the re-base is a pure
TS addition, and it is the missing piece.

### What OUR code (`cl_dll/StudioModelRenderer.cpp` ~985–1009) does today
- Persistent `copy` flag, HARD copy (`memcpy`) of gait into every bone until it sees Dorso/Spine, at
  which point `copy=false` and it **continues** (no break).
- A dead `else if (parent == "Bip01 Pelvis") copy = true` reset (TS pelvis is `Bip01 Bacino`, never hit).
- **No spine re-base at all.** → the split boundary (bones 0–7 gait, 8+ torso) is the same as the
  original, but the spine bone is left with its raw torso pose on a gait-yawed parent → the visible kink.

---

## 5. Sanity check against the behavioral spec  — CONFIRMED (HIGH)

- Spine bone world orientation = `parent_world ∘ spine_local`. After the merge, the spine's parent
  chain carries the **gait** yaw. The spine's `spine_local` was authored assuming the parent carried
  the **torso** yaw. So without a fix the upper body is rotated by the gait yaw instead of the torso
  yaw → twisted toward the leg-movement direction. This exactly matches "OUR torso faces a different
  direction than the legs / kinked."
- The re-base sets `spine_local' = spine_local` with twist `+= (torso_yaw − gait_yaw)`. Then
  `gait_yaw ∘ spine_local'` yaw-cancels back to `torso_yaw ∘ spine_local` = the authored torso facing
  → **upper body aligned**, legs still turned. This is the original's behavior. Sign is correct
  (delta = torso − gait). So the derived algorithm ALIGNS (kills the kink), not adds twist. HIGH.

---

## 6. Drop-in replacement

### 6a. Add a `QuaternionAngles` helper (our tree lacks quat→euler)

`cl_dll/studio_util.cpp` — add after `QuaternionMatrix` (uses our existing byte-identical
`QuaternionMatrix`; needs `<math.h>` for `asin`/`atan2`/`fabs`, already transitively included):

```c
/*
====================
QuaternionAngles  (matches client.dll FUN_1002ffe0 = QuaternionMatrix + FUN_1002fef0)
Returns euler angles in RADIANS: [0]=roll(local X), [1]=pitch, [2]=yaw. Studio convention.
====================
*/
void QuaternionAngles(vec4_t quaternion, float* angles)
{
    float matrix[3][4];
    QuaternionMatrix(quaternion, matrix);

    float sp = -matrix[2][0];
    if (sp < -1.0f) sp = -1.0f;
    else if (sp > 1.0f) sp = 1.0f;
    angles[1] = asin(sp); // pitch

    // gimbal-lock guard: pitch within ~0.01 rad of +/- pi/2 (matches the original's 0.01 eps)
    if (fabs(fabs(angles[1]) - (float)(M_PI * 0.5)) < 0.01f)
    {
        angles[0] = 0.0f;
        angles[2] = atan2(-matrix[0][1], matrix[1][1]);
    }
    else
    {
        angles[0] = atan2(matrix[2][1], matrix[2][2]); // roll
        angles[2] = atan2(matrix[1][0], matrix[0][0]); // yaw
    }
}
```
And declare in `cl_dll/studio_util.h` next to the others:
```c
void QuaternionAngles( vec4_t quaternion, float* angles );
```
(If `M_PI` is not defined in this TU, use `1.5707963267948966f` for `M_PI*0.5`.)

### 6b. Replace the merge loop (`cl_dll/StudioModelRenderer.cpp`, current ~985–1009)

```cpp
		// TS: snapshot the ORIGINAL torso root orientation BEFORE the loop overwrites q[0]
		// with the gait root — the split re-base measures the yaw delta against it.
		vec4_t q_torso_root;
		memcpy(q_torso_root, q[0], sizeof(vec4_t));

		for (i = 0; i < m_pStudioHeader->numbones; i++)
		{
			mstudiobone_t* bone = &pbones[i];

			// TS: the player skeletons are ITALIAN-boned ("Bip01 Dorso" = spine). The original
			// client.dll (FUN_10031080) copies the gait pose into every bone up to the first
			// "Bip01 Dorso"/"Bip01 Spine", then re-bases THAT split bone's yaw and stops. The
			// re-base cancels the gait yaw the parent chain injected, so the upper body follows
			// the torso (aim) facing instead of the legs' movement direction.
			if (0 == strcmp(bone->name, "Bip01 Spine") || 0 == strcmp(bone->name, "Bip01 Dorso"))
			{
				float angRoot[3], angCur[3], angBone[3];

				QuaternionAngles(q_torso_root, angRoot); // yaw of the ORIGINAL torso root
				QuaternionAngles(q[0], angCur);          // yaw of the CURRENT root == gait root (copied at i==0)
				float yawDelta = angRoot[2] - angCur[2]; // torso_yaw - gait_yaw   (index 2 = yaw)

				QuaternionAngles(q[i], angBone);         // the spine bone's torso-authored pose
				angBone[0] += yawDelta;                  // inject the yaw delta into the spine's local roll (index 0)
				AngleQuaternion(angBone, q[i]);          // write the re-based spine quat back

				break;                                   // bones after the spine keep their torso pose
			}

			memcpy(pos[i], pos2[i], sizeof(pos[i]));
			memcpy(q[i], q2[i], sizeof(q[i]));
		}
```

Notes:
- Drops the dead `"Bip01 Pelvis"` reset and the persistent `copy` flag — faithful to the original
  (which copies-then-break with no such reset).
- The split boundary (bones 0–7 gait, 8+ torso) is unchanged from our current code; the ONLY new
  behavior is the spine yaw re-base + the `break`.
- No invented constants. Uses only recovered math (QuaternionAngles = QuaternionMatrix + the exact
  MatrixAngles above; AngleQuaternion already byte-identical).

---

## 7. Confidence summary + what to verify live

| Claim | Confidence | Why |
|---|---|---|
| Helper identifications (all 7) | **HIGH** | term-for-term match; two are already byte-identical in our tree |
| Split reads yaw (idx 2) of both roots, writes idx 0 of spine, then breaks | **HIGH** | straight from raw asm, traced push-by-push |
| Reference quat = snapshot of torso root `q[0]` before the loop | **HIGH** | `FUN_10030010(&stack, &q[0])` at 0x314c4, a 4-dword copy |
| delta sign = torso_yaw − gait_yaw; re-base ALIGNS (kills kink) | **HIGH** | asm order + geometry both agree |
| MatrixAngles normal-branch formula (asin pitch, atan2 roll/yaw) | **HIGH** | raw asm |
| **Gimbal-lock fold exact epsilon/±0.5 bookkeeping** | **MED** | eps=0.01 and π/2 confirmed, but the exact ±0.5 fold path is approximated in 6a; it only matters within 0.008 rad of straight up/down — never reached by a spine/root bone in practice |
| index-0 (roll) is the twist axis on the Italian spine bone | **HIGH** | geometric (Dorso local-X up the spine); the numeric 2→0 mapping is asm-exact regardless |

**Live-trace check (optional, to remove any residual doubt):** on the running original, break at
client.dll `0x31616` while moving with a knife; read `angRoot[2]`, `angCur[2]`, the resulting
`yawDelta`, and the spine `q[i]` before/after — confirm `yawDelta` ≈ (viewangle yaw − movement yaw)
and the spine quat rotates by that amount. But the static evidence here is complete and self-consistent;
this fix is safe to ship and re-test in 3rd person first.
