# mod-sod-mage client/server spell generator

The SoD spells use IDs the stock 3.3.5a client doesn't have. Two artifacts must
agree exactly, so both are produced from **one** generator run:

1. **Client `Spell.dbc`** rows (packed into a `patch-?.MPQ` for the client).
2. **Server `spell_dbc` SQL** (`data/sql/db-world/base/sod_mage_spell_dbc.sql`).

## Why generate instead of hand-write

`Spell.dbc` records reference other DBCs by **index**, not value:
`CastingTimeIndex` → `SpellCastTimes.dbc`, `DurationIndex` → `SpellDuration.dbc`,
`RangeIndex` → `SpellRange.dbc`, `EffectRadiusIndex` → `SpellRadius.dbc`,
`SpellIconID` → `SpellIcon.dbc`. Those indices differ between builds and can't be
guessed. The generator reads the **real client DBCs** (which the server also
loads), so every index is correct and identical on both sides.

## Pipeline (run once the client `Data/` path is known)

1. **Extract** from the client MPQs (respecting MPQ patch priority): `Spell.dbc`,
   the index DBCs `SpellCastTimes`/`SpellDuration`/`SpellRange`/`SpellRadius`/
   `SpellIcon`, plus `SkillLineAbility.dbc`.
2. **Clone + override** (pure-Python WDBC reader/writer, no deps): for each new
   spell, clone a real template record (so all index fields are valid), then
   override only what changes. Resolve desired duration/range/cast-time by
   searching the aux DBCs for the wanted value and using that row's index.
3. **Emit** the patched `Spell.dbc` and the server SQL. The SQL seeds three
   tables (idempotent, scoped to our IDs): `spell_dbc` (the spells),
   `spell_script_names` (binds the C++ scripts to their spell IDs — without
   this the scripts never run), and `spell_proc` (the conversion proc).
4. **Patch `SkillLineAbility.dbc`** so a spell flagged with a `skill_line` lands
   under the right spellbook tab (client-side grouping; e.g. Arcane = 237).
5. **Repack** the two patched DBCs (`Spell.dbc`, `SkillLineAbility.dbc`) into the
   client patch (`patch-enus-z.mpq`) via StormLib (the `pympq` binding) and drop
   it in the client locale `Data/` folder. (Implemented — no external MPQ tool
   needed.)

Custom-item **bag icons** are NOT built here. WoW replaces whole DBCs per patch,
so every SoD module's `Item.dbc` rows must share one file; that consolidated patch
is owned by [`mod-sod-world`](../../mod-sod-world). This module lists its items in
`tools/client_items.json`, which that tool aggregates.

## Specs (single source of truth)

Spells live in the `SPELLS` list at the top of `build_sod_mage_patch.py`.
Player-facing spells (`401417` Regeneration, `412510` Mass Regeneration, `400735`
Temporal Beacon, `401405` Chronomantic Healing) get a client `Spell.dbc` row; the
hidden conversion proc (`900001`) is **server-only** (SQL, no client DBC). Custom
item rows live in `tools/client_items.json` (consumed by `mod-sod-world`'s patch
builder). See [../docs/spell-generator.md](../docs/spell-generator.md) for the
full spec keys and [../docs/adding-a-spell.md](../docs/adding-a-spell.md) for the
workflow.

> Built and validated against the real client DBCs, not blind — expect a couple
> of iterations confirming cast time / range / duration in-game.
