# Spells

Reference for the spells implemented so far. IDs and mechanics are defined in
`src/spell_sod_mage.h` / `src/spell_sod_mage.cpp` and the generator spec in
`tools/build_sod_mage_patch.py`.

## Regeneration — `401417`

A channeled heal-over-time that also stamps Temporal Beacon.

- **Cast:** 3s channel, 28% base mana, 40 yd, single friendly target.
- **Effect:** periodic heal (~1s tick) over the channel, *and* on apply it casts
  **Temporal Beacon** (`400735`) on the target and ensures the Mage carries the
  hidden **conversion aura** (`900002`).
- **Visual:** reuses Drain Life's beam (`SpellVisualID 12655`) — a green
  caster↔target channel tether. (No purple smooth beam exists in the client; see
  [Gotchas](gotchas.md).)
- **Tooltip:** `Heals the target for $o1 health over 3 sec and applies Temporal
  Beacon for 30 sec.` (`$o1` = client-computed total HoT.)
- **Script:** `spell_sod_mage_regeneration` (AuraScript, `AfterEffectApply` on the
  periodic-heal effect).

## Temporal Beacon — `400735`

The 30s buff. Two parts:

1. A small **passive HoT** (~8/sec) from its own periodic-heal effect.
2. The signature mechanic: while it's up, the caster's **Arcane** spell damage is
   converted to chronomantic healing on the beacon target (see `900002`).

- **Aura:** 30s, Magic dispel, applied via triggered cast from Regeneration.
- **Visual:** reuses Lightning Shield (`SpellVisualID 37`) — the orbiting orbs,
  plus its native sounds (incl. the impact on apply).
- **Tooltip / buff text:** `Records the subject's position in time and space.
  Damage done by caster will result in this subject receiving chronomantic
  healing.`
- **Script:** `spell_sod_mage_temporal_beacon` (AuraScript) — maintains the
  `caster GUID → beacon target GUIDs` registry on apply/remove and strips the
  conversion aura when the caster's last beacon ends.

## Temporal Beacon (conversion) — `900002`  *(server-only)*

Hidden passive proc aura on the Mage; the engine of the beacon mechanic.

- **Trigger:** the Mage dealing **Arcane** spell damage (`spell_proc` row with
  `SchoolMask = Arcane`, `AttributesMask = TRIGGERED_CAN_PROC` so Arcane Missiles'
  ticks count).
- **Effect:** heals each registered beacon target for a share of the damage via
  **Chronomantic Healing** (`900001`).
- **Reductions:** `SelfPct` when the beacon target is the caster; `MultiTargetPct`
  when the Arcane damage came from a multi-target spell (detected via
  `SpellInfo::IsTargetingArea()`).
- **Script:** `spell_sod_mage_temporal_conversion` (AuraScript, `DoCheckProc` +
  `OnEffectProc`), modeled on the core's Vampiric Embrace + Beacon of Light.

## Chronomantic Healing — `900001`

The instant triggered heal that the conversion aura casts. Heal amount is supplied
at cast time (`SPELLVALUE_BASE_POINT0`). Own ID so it shows correctly in the combat
log and benefits from healing mods. No script.

## Config (`conf/mod_sod_mage.conf.dist`)

| Key | Default | Meaning |
|-----|---------|---------|
| `SodMage.Enable` | 1 | Master switch; module is a no-op when off. |
| `SodMage.TemporalBeacon.ConversionPct` | 70 | % of Arcane damage converted to healing. |
| `SodMage.TemporalBeacon.SelfPct` | 50 | % of that healing kept when the target is the caster. |
| `SodMage.TemporalBeacon.MultiTargetPct` | 20 | % kept when the damage came from a multi-target spell. |

## Open tuning items

Heal coefficients (the per-tick base points for Regeneration and the beacon HoT)
and the "multi-target Arcane" detection are first-pass placeholders, not
SoD-exact. Adjust in the generator spec (DBC) and config.
