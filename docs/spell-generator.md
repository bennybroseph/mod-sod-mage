# Spell generator

`tools/build_sod_mage_patch.py` is the single source of truth that produces both
halves of every spell ([why](architecture.md)): the client `Spell.dbc` (packed
into an MPQ) and the server SQL. Defining a spell once, in the `SPELLS` list,
keeps the two from drifting.

## What it does

1. **Extracts** the client's *effective* DBCs (`Spell.dbc` plus the index DBCs
   `SpellCastTimes`, `SpellDuration`, `SpellRange`, `SpellRadius`, `SpellIcon`,
   and `SkillLineAbility.dbc` + `Item.dbc`) from the client MPQs, respecting MPQ
   patch priority (see [Gotchas](gotchas.md)).
2. **Resolves indexes/icons at runtime** from those DBCs — e.g. "3000 ms cast" →
   the `SpellCastTimes` index that holds 3000, the icon ID for a texture name.
   Nothing is hardcoded, so the output is correct for whatever client it points at.
3. **Clones a real template spell** per new spell (so every index column is valid),
   applies the spec's `overrides`, sets the name/tooltip strings, and appends the
   row to a patched `Spell.dbc`.
4. **Emits SQL** to `data/sql/db-world/base/sod_mage_spell_dbc.sql` seeding three
   tables (idempotent, scoped to our IDs): `spell_dbc`, `spell_script_names`
   (binds C++ scripts — see [Gotchas](gotchas.md)), and `spell_proc`.
5. **Patches `SkillLineAbility.dbc`** so spells flagged with a `skill_line` land
   under the right spellbook tab (client-side grouping; e.g. Arcane = 237).
6. **Patches `Item.dbc`** with a row per custom item (the `ITEMS` list) so each
   resolves its bag inventory icon (itemId → DisplayInfoID). Without this, a custom
   item shows a red "?" icon in bags even though its name/tooltip (server-side) are
   fine — see [Gotchas](gotchas.md). `ITEMS` must mirror the `item_template` rows in
   `sod_mage_regeneration_unlock.sql`.
7. **Packs** the patched `Spell.dbc`, `SkillLineAbility.dbc`, and `Item.dbc` into
   the client patch MPQ via StormLib.

## Requirements

- **Python 3** with **`pympq`** (a StormLib binding, for MPQ read/write) and the
  read-only `mpyq` is optional. Install: `pip install pympq`.
- The **client `Data/` folder** must be readable — i.e. the WoW client must be
  **fully closed** (a running client locks its MPQs; see [Gotchas](gotchas.md)).

## Running it

```bash
python tools/build_sod_mage_patch.py --client "<path to WoW client root>"
# --dry-run   emit SQL + patched DBC but do not write the MPQ
```

`--client` defaults to a local path; override it for your install. The patch is
written as a high-priority locale patch (`patch-enus-z.mpq`) so it overrides
everything else; adjust for non-enUS clients.

## The spec (`SPELLS`)

Each entry is a dict. The important keys:

| Key | Meaning |
|-----|---------|
| `id` | spell ID |
| `client` | `True` → goes in the client `Spell.dbc`/MPQ; `False` → SQL only (hidden server spell) |
| `template` | a real spell ID to clone for a valid baseline (only for client spells) |
| `name` | `Name_Lang_enUS` |
| `desc` / `aura_desc` | optional client tooltip strings (`Description` / `AuraDescription`); support tooltip tokens like `$o1`, `$s1`, `$d` |
| `script` | C++ script class name → emitted as a `spell_script_names` row |
| `proc` | optional dict → emitted as a `spell_proc` row (required for proc auras) |
| `overrides` | `spell_dbc` column name → value; applied to both the DBC record and the SQL |

`overrides` keys are literal `spell_dbc` column names (e.g. `CastingTimeIndex`,
`EffectAura_1`, `SpellVisualID_1`). The DBC field index is resolved from the
core's `spell_dbc` table definition, so column names stay readable.

## The item spec (`ITEMS`)

A parallel `ITEMS` list declares the module's custom items for the `Item.dbc`
patch — one dict each with `id`, `class`, `subclass`, `material`, `display`
(an `ItemDisplayInfo` id whose icon you want; find one by its `InventoryIcon` in
`ItemDisplayInfo.dbc`), `invtype`, and `sheath`. Unlike spells, the item's
**server** rows (name, stats, vendor/loot, `ScriptName`) are **hand-written** in
`sod_mage_regeneration_unlock.sql` — `ITEMS` only feeds the client `Item.dbc`, so
keep the two in sync.

## Output

- `modules/mod-sod-mage/data/sql/db-world/base/sod_mage_spell_dbc.sql` — committed.
- `<client>/Data/enus/patch-enus-z.mpq` — the client patch (not committed; a
  generated client artifact) carrying the patched `Spell.dbc`,
  `SkillLineAbility.dbc`, and `Item.dbc`. Scratch extractions land in
  `tools/_work/` (ignored).

See [Adding a spell](adding-a-spell.md) for the full workflow around the spec.
