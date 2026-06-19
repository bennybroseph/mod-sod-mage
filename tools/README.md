# mod-sod-mage tools

This module contributes **data only** to the client patch — no build tooling lives
here. The shared [`sod-client`](https://github.com/mod-sod/sod-client) pipeline
consumes these files and produces the one consolidated client patch (and this
module's server SQL).

## What's here

- **`sod_spells.py`** — the spell spec (`build_spells(idx)`): every custom spell's
  client `Spell.dbc` row and server `spell_dbc` data, declared once. Player-facing
  spells (`401417` Regeneration, `412510` Mass Regeneration, `400735` Temporal
  Beacon, `401405` Chronomantic Healing, `401556`/`401558` Living Flame) get a
  client row; the hidden conversion proc (`900001`) is server-only.
  See [../docs/spell-generator.md](../docs/spell-generator.md).
- **`client_items.json`** — this module's custom `Item.dbc` rows (bag icons). Keep
  it in sync with the `item_template` rows in the rune-unlock SQL.

## Building

The patch is built from a `sod-client` checkout, not from here:

```bash
python build_patch.py --server "<azerothcore root>" --client "<WoW client root>"
```

It regenerates `../data/sql/db-world/base/sod_mage_spell_dbc.sql` (committed) and
writes the client MPQ. Requires Python 3 + `pympq`, with the WoW client closed. The
[SoD installer](https://github.com/mod-sod/sod-installer) runs it for you.

> Built and validated against the real client DBCs — expect a couple of iterations
> confirming cast time / range / duration in-game.
