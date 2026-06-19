# Spells

Reference for the spells implemented so far. IDs and mechanics are defined in
`src/spell_sod_mage.h` / `src/spell_sod_mage.cpp` and the spell spec in
`tools/sod_spells.py`.

**Acquisition:** by default these are GM-`.learn`-only. With the optional
[`mod-rune-engraving`](../../mod-rune-engraving) engine installed, **Regeneration**,
**Mass Regeneration**, and **Living Flame** are also acquirable as **engravable
runes** — Regeneration and Living Flame via the SoD item chain (Comprehension Charm +
scrambled Spell Notes dropped by Mage-only sources, decoded then used), Mass
Regeneration via the Awakened Lich drop (the shared `mod-sod-world` encounter). Living
Flame's scrambled notes (*Spell Notes: MILEGIN VALF*, `203752`) drop at ~20% from
low-level caster mobs (Kobold Geomancer, Frostmane Shadowcaster/Seer, Scarlet
Warrior/Missionary/Zealot, Burning Blade Thug/Neophyte/Fanatic/Apprentice/Cultist);
decoding yields *Spell Notes: Living Flame* (`203746`) which unlocks the **Legs** rune.
**Enlightenment** (Chest) is earned via the **Servant of Azora** event (Alliance):
Polymorph the disguised critters scattered across Elwynn to free the
apprentices, collect all four *Azora Apprentice Notes* pages they drop, and read
them together into *Spell Notes: Enlightenment* (`203749`) — see
[Enlightenment](#enlightenment-412324-chest-rune) below. See the
[README](../README.md) for the player-facing flow.

## Regeneration — `401417`

A channeled heal-over-time that also stamps Temporal Beacon.

- **Cast:** 3s channel, 28% base mana, 40 yd, single friendly target.
- **Effect:** periodic heal (~1s tick) over the channel, *and* on apply it casts
  **Temporal Beacon** (`400735`) on the target and ensures the Mage carries the
  hidden **conversion aura** (`900001`).
- **Visual:** reuses Drain Mana's beam (`SpellVisualID 12657`) — a blue
  caster↔target channel tether. (There's no purple smooth beam in the client; see
  [Gotchas](gotchas.md).)
- **Tooltip:** `Heals the target for $o1 health over $d sec and applies Temporal
  Beacon for 30 sec.` — **dynamic**: the client computes `$o1` from a client-only
  linear fit of the heal curve (see [Tooltips](#dynamic-tooltips)), so it scales
  with the player's level (and the client adds healing power).
- **Script:** `spell_sod_mage_regeneration` (AuraScript, `AfterEffectApply` on the
  periodic-heal effect).

## Mass Regeneration — `412510`

The AoE sibling of Regeneration.

- **Cast:** instant → 3s channel, 40 yd, **45% base mana**, Arcane. Drain Mana visual.
- **Targeting:** `ImplicitTargetA = TARGET_UNIT_TARGET_ALLY` + `ImplicitTargetB =
  TARGET_UNIT_LASTTARGET_AREA_PARTY` (10 yd) — heals the chosen ally **and their
  party** in range (same pattern as Prayer of Healing). The core applies the
  periodic-heal aura to each, so the script's apply hook fires once per target.
- **Effect:** 165% of healing power over 3s to each (`spell_bonus_data dot 0.55`,
  base 0, `SpellLevel 0`), and applies **Temporal Beacon for 15 sec** to each
  target — half of Regeneration's 30s, set via `ApplyTemporalBeacon(...,
  SOD_MAGE_BEACON_MS_MASS_REGEN)`.
- **Script:** `spell_sod_mage_mass_regeneration` (AuraScript), mirrors
  Regeneration but with the 15s beacon constant.

## Temporal Beacon — `400735`

The 30s buff. Two parts:

1. A small **passive HoT** (~8/sec) from its own periodic-heal effect.
2. The signature mechanic: while it's up, the caster's **Arcane** spell damage is
   converted to chronomantic healing on the beacon target (see `900001`).

- **Aura:** 30s, Magic dispel, applied via triggered cast from Regeneration.
- **Visual:** reuses Lightning Shield (`SpellVisualID 37`) — the orbiting orbs,
  plus its native sounds (incl. the impact on apply).
- **Tooltip / buff text:** `Records the subject's position in time and space.
  Damage done by caster will result in this subject receiving chronomantic
  healing.`
- **Script:** `spell_sod_mage_temporal_beacon` (AuraScript) — maintains the
  `caster GUID → beacon target GUIDs` registry on apply/remove and strips the
  conversion aura when the caster's last beacon ends.

## Temporal Beacon (conversion) — `900001`  *(server-only)*

Hidden passive proc aura on the Mage; the engine of the beacon mechanic.

- **Trigger:** the Mage dealing **Arcane** damage (`spell_proc` row with
  `SchoolMask = Arcane`, `AttributesMask = TRIGGERED_CAN_PROC` so Arcane Missiles'
  ticks count). The proc flags include ranged auto-attack, so an **Arcane wand**
  also feeds the beacon; the Arcane school filter keeps non-Arcane wands out.
- **Effect:** heals each registered beacon target for a share of the damage via
  **Chronomantic Healing** (`401405`).
- **Reductions:** `SelfPct` when the beacon target is the caster; `MultiTargetPct`
  when the Arcane damage came from a multi-target spell (detected via
  `SpellInfo::IsTargetingArea()`).
- **Script:** `spell_sod_mage_temporal_conversion` (AuraScript, `DoCheckProc` +
  `OnEffectProc`), modeled on the core's Vampiric Embrace + Beacon of Light.

## Chronomantic Healing — `401405`

The instant triggered heal that the conversion aura casts (the real SoD ID). Heal
amount is supplied at cast time (`SPELLVALUE_BASE_POINT0`). Its own ID so it shows
correctly in the combat log and benefits from healing mods. No script.

## Living Flame — `401556`

A spellfire flame that creeps toward the target, dealing **Spellfire (Fire +
Arcane)** damage to nearby enemies along its trail.

- **Cast:** instant, 35 yd, 11% base mana, **30s cooldown** (SoD), requires a
  hostile target. Cloned from Fire Blast (`2136`) for an instant enemy-target fire
  cast; cooldown/GCD set explicitly (`RecoveryTime 30000`, `StartRecoveryTime 1500`)
  so the server row carries them. Carries `SPELL_ATTR1_NO_THREAT`, so the **cast
  doesn't enter combat** — only the trail's damage does. Uses a **custom cast
  visual** (`SpellVisual 700556` — a clone of Fire Blast's visual `143` with the
  on-target impact kit removed; `sod-client` patches `SpellVisual.dbc`): the Mage
  plays the instant fire-cast gesture but **nothing renders on the target** (the
  only fire is the ground patch). Note: the "3.3.5a HD" client ships a modified
  `SpellVisual.dbc` in a **base** archive (`Data/patch-s.mpq`), which outranks the
  locale patch — so `sod-client` writes our `SpellVisual.dbc` to **both**
  `Data/patch-z.mpq` (base) and `patch-enus-z.mpq`, or the override is shadowed.
- **School:** `SchoolMask = 68` (Fire | Arcane). Because the damage carries **both**
  bits, it satisfies any Fire- *or* Arcane-gated proc — including the Mage's own
  **Chronomantic Healing** (the Temporal Beacon conversion, which checks
  `& Arcane`).
- **Mover:** SoD drives the flame with an **AreaTrigger**, which 3.3.5a doesn't
  have. We summon an immune, non-attackable trigger creature
  (`npc_sod_mage_living_flame`, entry `700200`, Vazruden Fire Trap model `17522`
  at 0.25 scale, sunk to the ground with `HoverHeight -1.0` — a self-contained
  ground-fire patch, so it reads as fire creeping along the ground) that homes on
  the target at `SodMage.LivingFlame.SpeedPct` of run speed and lives 10 sec.
- **Damage:** the creature itself deals nothing. Once per second it makes its
  **summoner (the Mage)** recast the Spellfire AoE **`401558`** at its current
  position — so the damage is attributed to the caster (this is what lets it feed
  Chronomantic Healing and other procs). `401558` is cloned from Flamestrike
  (`2120`), so each tick renders a ground-fire patch; the moving sequence reads as
  the flame's trail. AoE radius 3 yd (`SpellRadius` index 15 — a narrow trail), `DefenseType = 1`
  (magic). **Per-tick damage is a level curve** — `13.828124 + 0.018012·L +
  0.044141·L²` (from wago/wowhead), set in script (`spell_sod_mage_living_flame_damage`,
  `OnEffectLaunchTarget` → `SetEffectValue`) so it works with no spellpower; gear
  spellpower adds on top via `spell_bonus_data direct 1.0` (the SoD tooltip's
  `×m1/100 = ×1.0`).
- **Scripts:** `spell_sod_mage_living_flame` (SpellScript, `OnEffectHitTarget`
  dummy → summon) and `npc_sod_mage_living_flame` (CreatureAI, chase + per-second
  owner cast), in `src/spell_sod_mage_living_flame.cpp`. Living Flame's tooltip
  uses the cross-spell token `$401558s1` to show the trail's per-second damage.

## Enlightenment — `412324`  *(Chest rune)*

A passive that rewards mana management with two mutually-exclusive states:

- **Above 70% mana:** `+10% spell damage` (sub-aura `412326`,
  `MOD_DAMAGE_PERCENT_DONE` over all magic schools).
- **Below 30% mana:** `10%` of your mana regeneration continues while casting
  (sub-aura `412325`, `MOD_MANA_REGEN_INTERRUPT`).
- **Between 30–70%:** neither — both sub-auras are stripped.

SoD's *"Gain the Enlightenment ability"* entry (`415729`) just points at `412324`,
so the rune teaches `412324` directly and no `415729` row is shipped.

- **Driver:** `412324` is a passive whose only effect is a **1s periodic dummy**.
  Each tick the script reads the Mage's mana % and toggles the two sub-auras; both
  carry permanent duration and are applied/removed by the driver (so they persist
  between polls and vanish cleanly on unlearn).
- **The Five Second Rule (the subtle half):** the low-mana effect is *not* custom
  regen code — it's `MOD_MANA_REGEN_INTERRUPT`, the same aura retail **Meditation**
  uses. The "lockout" is the FSR: in AzerothCore it's triggered by **spending
  mana** (`Spell::TakePower`), not by *casting* per se — a free/instant spell never
  trips it — and it only ever throttles the **spirit/intellect** portion of regen
  (flat mp5 from gear always flows). This aura sets how much of that spirit regen
  survives the 5s window, and the engine recomputes regen on aura apply/remove.
- **Icon:** `spell_arcane_mindmastery` (SoD's Mind Mastery art), shared by the
  passive, both sub-buffs, and the rune panel.
- **Tuning:** the thresholds and poll cadence are config keys (see below) — SoD
  re-evaluates every 5s; the default keeps that but admins can poll faster.
- **Script:** `spell_sod_mage_enlightenment` (AuraScript — `OnEffectPeriodic` +
  apply/remove), in `src/spell_sod_mage_enlightenment.cpp`. The mana-state decision
  is the pure, unit-tested `SodMageRules::EnlightenmentStateFor`.

### Acquisition — the Servant of Azora event (Alliance)

Enlightenment is **earned**, not free by class. Disguised critters named
*Polymorphed Apprentice* (custom creature `700210`) wander **all over Elwynn**,
each wearing a random non-Elwynn critter model and the **Wild Polymorph** debuff
(`23603`, applied permanently). Casting **Polymorph** frees the apprentice: the
**same creature** clears its auras (cancelling the sheep morph), snaps to a human
model in a **dust-cloud poof** (`12244`) — so it reads as the Polymorph
transforming it, no second NPC — thanks you, plays the **Blink** animation in place
(`15993`, which reuses Blink's visual without moving), and drops a paper (gameobject
`700220`). Clicking the paper grants the
next of four **Azora Apprentice Notes** pages (`203756` / `203960` / `203961` /
`203962` — real SoD ids). *Using* any page while holding all four assembles **Spell
Notes: Enlightenment** (`203749`); using that unlocks the rune via the engine's
`item_rune_unlock` (mapping `203749 → 7000004`, which flips the rune from
free-by-class to gated).

The event logic is in `src/sod_mage_azora_event.cpp` (critter seeding + reveal,
apprentice sequence, paper grant, page combine); the data is in
`data/sql/db-world/base/sod_mage_enlightenment_unlock.sql`. There are **no fixed
spawns** — `sod_mage_critter_seeder` (an `AllCreatureScript`) converts a share of
real Elwynn critters in place as they spawn, so the originals are untouched in the
DB and return on respawn. The chance **scales with distance from the Tower of
Azora** (`SodMage.Enlightenment.Critter*` — `CritterChanceMaxPct` 15% within
`InnerRadius`, falling off to `CritterChanceMinPct` 5% at `OuterRadius`), via the
unit-tested `SodMageRules::SeedChanceForDistance` — so apprentices cluster around
Azora's tower and thin out across the zone. The disguise model is rolled in script
(a creature is capped at 4 models, and `23603` would otherwise force its own
transform). Horde uses a different zone/NPC — a later pass.

## Dynamic tooltips

Regeneration, Mass Regeneration, and Living Flame deal a **level-scaled** amount
(a quadratic curve in the caster's level), computed exactly server-side in script.
The 3.3.5a client computes tooltips itself and can only scale **linearly**
(`EffectRealPointsPerLevel`), so it can't render the quadratic.

The generator works around this by writing a **client-only** linear fit of each
curve into the client `Spell.dbc` (the `client_overrides` / `client_overrides_float`
keys → `EffectBasePoints_1`, `EffectRealPointsPerLevel_1`, `BaseLevel 1`,
`MaxLevel 80`), never into the server `spell_dbc`. The fit spans the full server
range — **exact at levels 1 and 80**, scaling the whole way — so the tooltip is
dynamic across all levels and accurate at the cap, but because the client can only
scale linearly it reads **high mid-range** (a straight line over a quadratic). The
actual heal/damage is the exact SoD quadratic at every level — only the tooltip is
approximate. Tooltip text uses the client tokens `$o1` (periodic heal total) /
`$d` (duration) / `$401558s1` (Living Flame's trail damage); the client adds the
player's spell/healing power on top.

Knobs live in `tools/sod_spells.py`: the `living_flame_tick` / `regen_tick`
curves and `tooltip_fit(curve, lo=1, hi=80)` — the fit anchors (full server range).

## Config (`conf/mod_sod_mage.conf.dist`)

| Key | Default | Meaning |
|-----|---------|---------|
| `SodMage.Enable` | 1 | Master switch; module is a no-op when off. |
| `SodMage.TemporalBeacon.ConversionPct` | 70 | % of Arcane damage converted to healing. |
| `SodMage.TemporalBeacon.SelfPct` | 50 | % of that healing kept when the target is the caster. |
| `SodMage.TemporalBeacon.MultiTargetPct` | 20 | % kept when the damage came from a multi-target spell. |
| `SodMage.LivingFlame.SpeedPct` | 40 | Living Flame creature move speed, % of normal run speed. |
| `SodMage.Enlightenment.HighManaPct` | 70 | Above this mana %, Enlightenment grants +10% spell damage. |
| `SodMage.Enlightenment.LowManaPct` | 30 | Below this mana %, part of mana regen continues through the FSR. |
| `SodMage.Enlightenment.PollMs` | 5000 | How often (ms) Enlightenment re-evaluates mana state (1000ms resolution). |

## Accuracy vs. SoD (checked against wago.tools, build 1.15.8.x)

Verified against the real SoD DB2 data (see [Pulling SoD data](pulling-sod-data.md)):

- **Temporal Beacon — accurate.** Conversion 70% / self-reduced 50% / multi-target
  reduced 80% (our `ConversionPct=70`, `SelfPct=50`, `MultiTargetPct=20` kept all
  match `$s4/$s5/$s6`), passive HoT 8/sec, 30s, 100yd, instant, Arcane — all match.
- **Regeneration / Mass Regeneration — accurate.** Cast/duration/range/mana/school
  all match. The heal is the SoD **level curve** — `$<power>` resolves to
  `38.258376 + 0.904195·L + 0.161311·L²`, × `m1/100` (`m1=55` → 0.55/tick × 3
  ticks) — so it heals without gear; healing power adds on top via
  `spell_bonus_data dot 0.55`. Set per tick in script (`OnEffectCalcAmount` →
  `SetAmount(SodMageRules::RegenTickHeal(level))`).

The "multi-target Arcane" detection (`IsTargetingArea()`) remains a heuristic.

- **Living Flame — adapted.** Instant / 35 yd / 11% mana / 30s cooldown /
  Spellfire (school 68) / 10s / 1s damage tick all match wago. Damage is a **level
  curve** (`$<power>` in the SoD tooltip resolves to `13.828124 + 0.018012·L +
  0.044141·L²`, not gear spellpower — so it works without gear), reproduced in
  script; gear spellpower adds at `×1.0` on top. SoD's AreaTrigger mover has no
  3.3.5a analogue, so the homing chasing-creature is a deliberate adaptation
  (behavior-equivalent, not a 1:1 port).

- **Enlightenment — accurate.** Sourced from `412324`: `+10%` spell damage above
  `70%` mana (`412326`, `MOD_DAMAGE_PERCENT_DONE`, magic schools), and `10%` of
  mana regen continuing while casting below `30%` mana (`412325`,
  `MOD_MANA_REGEN_INTERRUPT`) — matching the resolved tooltip `$s1/$s2/$s3/$s4`.
  SoD drives the state with a 5s periodic dummy; we reproduce that (poll cadence
  configurable). The modern aura *type* enums (226/79/134) were mapped to their
  3.3.5a meanings, not copied by number.
