# Spell spec

The SoD spells use IDs the stock 3.3.5a client doesn't have, so two artifacts must
agree exactly ([why](architecture.md)): the client `Spell.dbc` (packed into an MPQ)
and the server `spell_dbc` SQL. Both are produced from **one** declaration — this
module's spell spec — so they never drift.

## Where the spec lives

`tools/sod_spells.py` exports `build_spells(idx)`, returning the list of spells this
module defines. It's pure data plus a little math (tooltip curves), with the shared
WoW enum constants imported from `sod-client`.

The **build itself** — extracting the client DBCs, cloning template rows, resolving
indices/icons, packing the MPQ, and emitting the SQL — is **not** in this module.
It's owned by the shared [`sod-client`](https://github.com/mod-sod/sod-client)
pipeline, which consolidates every class module's spells into one client patch.
See its [architecture doc](https://github.com/mod-sod/sod-client/blob/main/docs/architecture.md)
for the pipeline and the full spec-key reference.

## The spec keys

Each entry in `build_spells(idx)` is a dict:

| Key | Meaning |
|-----|---------|
| `id` | spell ID |
| `client` | `True` → client `Spell.dbc` + MPQ; `False` → SQL only (hidden server spell) |
| `template` | a real spell ID to clone for a valid baseline (client spells) |
| `name` | `Name_Lang_enUS` |
| `desc` / `aura_desc` | optional client tooltip strings; support tokens like `$o1`, `$s1`, `$d` |
| `skill_line` | spellbook tab (a `SkillLineAbility` row; e.g. Arcane = 237) |
| `script` | C++ script class → a `spell_script_names` row |
| `bonus` | spellpower coefficients → a `spell_bonus_data` row |
| `proc` | optional dict → a `spell_proc` row (required for proc auras) |
| `overrides` | `spell_dbc` column name → value (applied to both the DBC and the SQL) |

`overrides` keys are literal `spell_dbc` column names (e.g. `CastingTimeIndex`,
`EffectAura_1`, `SpellVisualID_1`); the DBC field index is resolved from the core's
`spell_dbc` table definition, so column names stay readable.

`idx` is the runtime resolver the builder passes in — e.g. `idx["cast"][3000]` is
the `SpellCastTimes` index holding 3000 ms, `idx["icon"]["spell_fire_..."]` is an
icon ID. Nothing is hardcoded, so the output is correct for whatever client it runs
against.

## Building the patch

From a checkout of `sod-client` (needs Python 3 + `pympq`, client fully closed):

```bash
python build_patch.py --server "<azerothcore root>" --client "<WoW client root>"
```

That regenerates `data/sql/db-world/base/sod_mage_spell_dbc.sql` (committed) and
writes the consolidated client patch. The
[SoD installer](https://github.com/mod-sod/sod-installer) runs it for you.

## Custom item icons (handled elsewhere)

Spells aside, the module's custom items declare their client `Item.dbc` rows in
`tools/client_items.json` — one object each with `id`, `class`, `subclass`,
`material`, `display`, `invtype`, `sheath`. `sod-client` aggregates these into the
one shared item patch. The items' **server** rows (name, stats, vendor/loot,
`ScriptName`) are hand-written in the rune-unlock SQL; keep `client_items.json` in
sync with their `class`/`subclass`/`Material`/`displayid`.

See [Adding a spell](adding-a-spell.md) for the full workflow.
