# Deploy & verify

A change touches up to three things — the **server binary** (C++), the **database**
(`spell_dbc` etc.), and the **client MPQ** (visuals/tooltips). Know which your
change involves so you only do the work needed.

| Change type | Rebuild worldserver? | Apply SQL + restart? | Repack MPQ + relaunch client? |
|-------------|:--:|:--:|:--:|
| C++ script logic (spell or item scripts) | ✅ | — | — |
| `spell_dbc` / `spell_script_names` / `spell_proc` | — | ✅ | — |
| Client visual / tooltip / icon / cast time | — | (optional¹) | ✅ |
| Rune catalog / item chain (`item_template`, loot, vendor, `rune_*`) | — | ✅ | (item icons²) |

¹ Cast time lives in both halves; visuals/tooltips/icons are client-only — the
server ignores `SpellVisualID`, `Description`, etc., so re-applying SQL for those
is only for keeping the DB row byte-identical to the file.

² A custom item's name/stats/tooltip come from the server, but its **bag inventory
icon** is resolved client-side from `Item.dbc`. New/changed custom items need the
shared `mod-sod-world` item patch regenerated (it carries the consolidated
`Item.dbc`), or they show a red "?" in bags. See [Gotchas](gotchas.md).

## 1. Regenerate artifacts

```bash
python tools/build_sod_mage_patch.py --client "<client root>"   # client fully closed
```

Rewrites `data/sql/db-world/base/sod_mage_spell_dbc.sql` and repacks the client
patch MPQ. See [Spell generator](spell-generator.md).

## 2. Apply the SQL (idempotent)

```bash
mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_spell_dbc.sql
```

The module ships more base SQL than just the spells — apply all of
`data/sql/db-world/base/` to `acore_world`:

- `sod_mage_spell_dbc.sql` — the spells.
- `sod_mage_module_string.sql` — player-facing strings.
- `sod_mage_runes.sql` — the rune catalog.
- `sod_mage_regeneration_unlock.sql` / `sod_mage_living_flame_unlock.sql` — the
  item-chain rune unlocks.
- `sod_mage_mass_regeneration.sql` — the Awakened Lich drop unlock.

The rune files are **engine-guarded** — a clean no-op without `mod-rune-engraving`.
The Mass Regeneration drop also needs the shared `mod-sod-world` module (the Lich
encounter). See the [README](../README.md) for the full list.

For a Dockerized server, run it through the DB container instead (adjust names/creds
to your setup), e.g.:

```bash
docker exec -i ac-database mysql -uroot -p<pw> acore_world < .../sod_mage_spell_dbc.sql
```

Every file does `DELETE … WHERE …` (own ids) then `INSERT`, so re-running is safe.

> Note: `docker compose restart` does **not** re-run the `db-import` service — that
> only runs on `up`. So a plain restart never applies changed SQL; apply it
> yourself (or `up`).

## 3. Restart the worldserver

`spell_dbc` and `spell_script_names` load **at startup** (not hot-reloadable), so a
restart is required for DB changes to take effect. Rebuild first **only** if you
changed C++ (e.g. `docker compose up -d --build`; the build compiles modules in via
`-DMODULES=static`).

## 4. Relaunch the client

The client loads the repacked patch MPQ on launch. Tooltip text can be cached in
the client's `Cache`/`WDB` folder — if old wording lingers, clear that folder.

## Verifying it actually loaded

- **Scripts bound?** After restart, the server's `Errors.log` should have **no**
  `Script named 'spell_sod_mage_* is not assigned in the database` lines. Those
  errors prove the module is compiled in but a `spell_script_names` row is missing.
- **Spell present server-side?**
  `SELECT ID, CastingTimeIndex FROM spell_dbc WHERE ID IN (401417,412510,400735,401405,900001);`
- **Module compiled in?** The worldserver startup log lists `Using modules
  configuration:` → `> mod_sod_mage.conf`.
- **In-game:** `.learn <id>`, cast, watch the combat log and buff bar.

See [Gotchas](gotchas.md) for what each failure mode looks like.
