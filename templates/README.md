# Contribution templates — mod-sod-mage

Copy-paste starting points for adding content to this module. Each file is a
bare-minimum skeleton with `<PLACEHOLDER>` tokens and `TODO` notes — copy it into
its real location, fill the blanks, and delete the banner comment.

These files are **never built or applied** from `templates/`. The C++ build globs
only `src/`, and the database updater only applies `.sql` under `data/sql/db-*/` —
so nothing here is sibling to those paths. They exist purely as references.

## What's here

| Folder / file | For adding | Copy into |
|---|---|---|
| `spell/` | a new SoD-style spell (script + spell spec) | `src/` + `tools/sod_spells.py` |
| `rune/` | a rune + its unlock chain | `data/sql/db-world/base/` + `src/` |
| `item/` | a custom item (notes, charm, reagent…) | `data/sql/db-world/base/` + `tools/client_items.json` |
| `module_string.sql` | localized player-facing text | `data/sql/db-world/base/` |
| `conf_snippet.conf` | a new config tunable | `conf/mod_sod_mage.conf.dist` |

Each folder has its own README with the step-by-step recipe.

## ID bands

Reuse the **real SoD id** whenever the thing exists in SoD (so it stays greppable
to wago.tools / Wowhead). Only invent an id when there's no SoD analogue — then pick
from a documented custom band:

| Kind | Band |
|---|---|
| Runes (engine catalog) | `7000000–7000999` |
| Server-only helper spells | `900000–900099` |

## Sources of truth

- **[wago.tools](https://wago.tools)** — authoritative SoD values and tooltips from
  the modern Classic Era client's DB2s, as CSV
  (`/db2/<Table>/csv?build=<classic-era build>`). Full recipe and the conversion
  traps: [docs/pulling-sod-data.md](../docs/pulling-sod-data.md).
- **[Wowhead Classic](https://www.wowhead.com/classic)** — human-readable
  cross-reference for ids, icons, and drop sources.
- **[AzerothCore wiki](https://www.azerothcore.org/wiki)** — table schemas and the
  [hooks list](https://www.azerothcore.org/wiki/hooks-script).
- This module's [docs/](../docs/Home.md) — the full recipes these templates condense.
