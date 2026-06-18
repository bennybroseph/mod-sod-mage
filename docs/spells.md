# Spells

Reference for the spells implemented so far. IDs and mechanics are defined in
`src/spell_sod_mage.h` / `src/spell_sod_mage.cpp` and the generator spec in
`tools/build_sod_mage_patch.py`.

**Acquisition:** by default these are GM-`.learn`-only. With the optional
[`mod-rune-engraving`](../../mod-rune-engraving) engine installed, **Regeneration**
and **Mass Regeneration** are also acquirable as **engravable runes** — Regeneration
via the SoD item chain (Comprehension Charm + Spell Notes), Mass Regeneration via the
Awakened Lich drop (the shared `mod-sod-world` encounter). **Living Flame** is
registered as a guarded **Legs** rune (catalog only — no item/drop chain yet). See
the [README](../README.md) for the player-facing flow.

## Regeneration — `401417`

A channeled heal-over-time that also stamps Temporal Beacon.

- **Cast:** 3s channel, 28% base mana, 40 yd, single friendly target.
- **Effect:** periodic heal (~1s tick) over the channel, *and* on apply it casts
  **Temporal Beacon** (`400735`) on the target and ensures the Mage carries the
  hidden **conversion aura** (`900001`).
- **Visual:** reuses Drain Mana's beam (`SpellVisualID 12657`) — a blue
  caster↔target channel tether. (There's no purple smooth beam in the client; see
  [Gotchas](gotchas.md).)
- **Tooltip:** `Heals the target for an amount equal to 165% of your healing power
  over 3 sec and applies Temporal Beacon for 30 sec.` — static wording, since the
  3.3.5a client can't show a server-side coefficient ([Gotchas](gotchas.md)).
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
  so the server row carries them.
- **School:** `SchoolMask = 68` (Fire | Arcane). Because the damage carries **both**
  bits, it satisfies any Fire- *or* Arcane-gated proc — including the Mage's own
  **Chronomantic Healing** (the Temporal Beacon conversion, which checks
  `& Arcane`).
- **Mover:** SoD drives the flame with an **AreaTrigger**, which 3.3.5a doesn't
  have. We summon an invisible, immune trigger creature
  (`npc_sod_mage_living_flame`, entry `700200`, Fire Elemental model 1405 — a
  visible walking flame) that homes on the target at `SodMage.LivingFlame.SpeedPct`
  of run speed and lives 10 sec.
- **Damage:** the creature itself deals nothing. Once per second it makes its
  **summoner (the Mage)** recast the Spellfire AoE **`401558`** at its current
  position — so the damage is attributed to the caster (this is what lets it feed
  Chronomantic Healing and other procs). `401558` is cloned from Flamestrike
  (`2120`), so each tick renders a ground-fire patch; the moving sequence reads as
  the flame's trail. AoE radius 8 yd (`SpellRadius` index 14), `DefenseType = 1`
  (magic). **Per-tick damage is a level curve** — `13.828124 + 0.018012·L +
  0.044141·L²` (from wago/wowhead), set in script (`spell_sod_mage_living_flame_damage`,
  `OnEffectLaunchTarget` → `SetEffectValue`) so it works with no spellpower; gear
  spellpower adds on top via `spell_bonus_data direct 1.0` (the SoD tooltip's
  `×m1/100 = ×1.0`).
- **Scripts:** `spell_sod_mage_living_flame` (SpellScript, `OnEffectHitTarget`
  dummy → summon) and `npc_sod_mage_living_flame` (CreatureAI, chase + per-second
  owner cast), in `src/spell_sod_mage_living_flame.cpp`.

## Config (`conf/mod_sod_mage.conf.dist`)

| Key | Default | Meaning |
|-----|---------|---------|
| `SodMage.Enable` | 1 | Master switch; module is a no-op when off. |
| `SodMage.TemporalBeacon.ConversionPct` | 70 | % of Arcane damage converted to healing. |
| `SodMage.TemporalBeacon.SelfPct` | 50 | % of that healing kept when the target is the caster. |
| `SodMage.TemporalBeacon.MultiTargetPct` | 20 | % kept when the damage came from a multi-target spell. |
| `SodMage.LivingFlame.SpeedPct` | 40 | Living Flame creature move speed, % of normal run speed. |

## Accuracy vs. SoD (checked against wago.tools, build 1.15.8.x)

Verified against the real SoD DB2 data (see [Pulling SoD data](pulling-sod-data.md)):

- **Temporal Beacon — accurate.** Conversion 70% / self-reduced 50% / multi-target
  reduced 80% (our `ConversionPct=70`, `SelfPct=50`, `MultiTargetPct=20` kept all
  match `$s4/$s5/$s6`), passive HoT 8/sec, 30s, 100yd, instant, Arcane — all match.
- **Regeneration — one gap.** Cast/duration/range/mana(28%)/school all match. But
  SoD heals for **165% of healing power over 3 sec** (`$<healpower>*$m1/100*3`,
  `$m1=55`, *pure spellpower scaling, no flat base*), whereas ours is a flat
  ~55/tick with no spellpower coefficient. Closing this is a balance/scaling change
  (set the effect's healing coefficient); deferred while balance is out of scope.

The "multi-target Arcane" detection (`IsTargetingArea()`) remains a heuristic.

- **Living Flame — adapted.** Instant / 35 yd / 11% mana / 30s cooldown /
  Spellfire (school 68) / 10s / 1s damage tick all match wago. Damage is a **level
  curve** (`$<power>` in the SoD tooltip resolves to `13.828124 + 0.018012·L +
  0.044141·L²`, not gear spellpower — so it works without gear), reproduced in
  script; gear spellpower adds at `×1.0` on top. SoD's AreaTrigger mover has no
  3.3.5a analogue, so the homing chasing-creature is a deliberate adaptation
  (behavior-equivalent, not a 1:1 port).
