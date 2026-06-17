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
   `SpellIcon`, plus `SkillLineAbility.dbc` and `Item.dbc`.
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
5. **Patch `Item.dbc`** with a row per custom item (the `ITEMS` list) so each
   resolves its **bag inventory icon** (itemId → DisplayInfoID). A custom item's
   name/stats come from the server, but the bag icon is client-side — without an
   `Item.dbc` row the item shows a red "?" in bags (the vendor frame is fine, as
   the vendor packet carries the displayid). `ITEMS` must mirror the
   `item_template` rows in `sod_mage_regeneration_unlock.sql`.
6. **Repack** the three patched DBCs into the client patch (`patch-enus-z.mpq`)
   via StormLib (the `pympq` binding) and drop it in the client locale `Data/`
   folder. (Implemented — no external MPQ tool needed.)

## Specs (single source of truth)

Spells live in the `SPELLS` list and custom items in the `ITEMS` list at the top
of `build_sod_mage_patch.py`. Player-facing spells (`401417` Regeneration,
`412510` Mass Regeneration, `400735` Temporal Beacon, `401405` Chronomantic
Healing) get a client `Spell.dbc` row; the hidden conversion proc (`900001`) is
**server-only** (SQL, no client DBC). The three custom items (`700200` charm,
`700201`/`700202` notes) get `Item.dbc` rows. See
[../docs/spell-generator.md](../docs/spell-generator.md) for the full spec keys
and [../docs/adding-a-spell.md](../docs/adding-a-spell.md) for the workflow.

> Built and validated against the real client DBCs, not blind — expect a couple
> of iterations confirming cast time / range / duration in-game.
