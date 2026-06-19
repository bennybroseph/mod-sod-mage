# Template — new spell

A SoD-style spell is **two halves kept in sync** by the generator: server data +
C++ script, and a client DBC patch. The full recipe is in
[docs/adding-a-spell.md](../../docs/adding-a-spell.md); this folder is the
copy-paste skeleton.

## Files

- **`generator_spec.py`** — one spell entry. Paste it into `build_spells(idx)` in
  [tools/sod_spells.py](../../tools/sod_spells.py). This is the single source of
  truth (built by the shared `sod-client` pipeline) for both the client `Spell.dbc`
  row and the server `spell_dbc` / `spell_script_names` / `spell_proc` SQL.
- **`spell_script.cpp`** — the `SpellScript` / `AuraScript`. Copy into `src/`, then
  add its `AddSC_*()` to [src/sod_mage_loader.cpp](../../src/sod_mage_loader.cpp).

## Steps

1. **Source the values** from wago.tools (see
   [docs/pulling-sod-data.md](../../docs/pulling-sod-data.md)) — watch the two traps:
   aura/effect *type* enums differ from 3.3.5a, and base points are stored value − 1.
2. **Pick the visual/icon** — reuse an existing `SpellVisualID` and `SpellIconID`;
   you can't recolor a beam without custom art.
3. Add the `generator_spec.py` entry, write the `spell_script.cpp`, register it in
   the loader.
4. **Regenerate** (from a `sod-client` checkout:
   `python build_patch.py --server "<ac root>" --client "<client>"`, client
   closed), apply the SQL, restart the worldserver, relaunch the client.
   See [docs/deploy-and-verify.md](../../docs/deploy-and-verify.md).

## Gotchas (the ones that bite)

- A `RegisterSpellScript` does nothing without the `spell_script_names` row — the
  generator emits it only if your spec has a `script` key.
- A channeled spell needs an **instant** `CastingTimeIndex` (length comes from the
  duration), the channeled `AttributesEx` bit, and the channel interrupt flags.
- A proc aura needs a `spell_proc` row, or it never procs.

See [docs/gotchas.md](../../docs/gotchas.md) for the full list.
