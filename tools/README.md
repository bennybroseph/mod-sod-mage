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

1. **Extract** from the client MPQs: `Spell.dbc`, `SpellCastTimes.dbc`,
   `SpellDuration.dbc`, `SpellRange.dbc`, `SpellRadius.dbc`, `SpellIcon.dbc`.
2. **Clone + override** (pure-Python WDBC reader/writer, no deps): for each new
   spell, clone a real template record (so all index fields are valid), then
   override only what changes. Resolve desired duration/range/cast-time by
   searching the aux DBCs for the wanted value and using that row's index.
3. **Emit** the patched `Spell.dbc` and the server SQL. The SQL seeds three
   tables (idempotent, scoped to our IDs): `spell_dbc` (the spells),
   `spell_script_names` (binds the C++ scripts to their spell IDs — without
   this the scripts never run), and `spell_proc` (the conversion proc).
4. **Repack** the patched DBC into `patch-?.MPQ` and drop it in the client
   `Data/` folder. (MPQ packer TBD — e.g. Ladik's MPQ Editor CLI.)

## Spell specs (single source of truth)

| ID | Clone template | Overrides |
|----|----------------|-----------|
| `401417` Regeneration | a 3s channeled periodic-heal spell | name, icon `inv_enchant_essencemysticalsmall`, single-target, 28% base mana, periodic-heal base points, 3s duration |
| `400735` Temporal Beacon | a single-target HoT | name, icon `spell_nature_timestop`, 30s duration, ~8/1s periodic-heal base points, Magic dispel |
| `900001` Chronomantic Heal | a simple instant heal | name, instant/no-GCD, base points supplied at cast time |

`900002` (caster conversion proc) is **server-side only** — it goes in the SQL
but **not** the client DBC/MPQ (hidden passive, no icon/tooltip needed).

> Built and validated against the real client DBCs, not blind — expect a couple
> of iterations confirming cast time / range / duration in-game.
