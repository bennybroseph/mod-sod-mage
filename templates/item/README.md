# Template — custom item

A custom item is *mostly* server-authoritative: an `item_template` row gives the
client its name, stats, and tooltip. The **one** thing the server can't supply is the
**bag inventory icon** — the client resolves that from its own `Item.dbc`, so every
custom item also needs a manifest entry.

## Files

- **`item_template.sql`** — the item row, plus the companion rows it usually needs
  (loot drop, class-gate condition, vendor entry) shown commented. Model:
  [sod_mage_regeneration_unlock.sql](../../data/sql/db-world/base/sod_mage_regeneration_unlock.sql).
- **`client_items.json`** — one manifest object. Add it to
  [tools/client_items.json](../../tools/client_items.json) so its `Item.dbc` row ships
  in mod-sod-world's consolidated client patch (else the item shows a red "?" in bags).
- **`item_script.cpp`** — optional `ItemScript`, only if "Use" should *do* something.

## Notes

- **Bag icon:** reuse an existing `displayid` whose icon matches (find one by its
  `InventoryIcon` in `ItemDisplayInfo.dbc`). `inv_scroll_03` = displayid `1102` (used
  by the SoD notes). No new art needed.
- **Manifest must mirror the SQL:** keep `class` / `subclass` / `material` / `display`
  identical in both files.
- **Class-gate a drop** with a `conditions` row (Mage = classmask `128`) so only the
  right class rolls it — SoD class loot.
- After editing the manifest, regenerate mod-sod-world's patch
  (`build_sod_world_patch.py`, client closed). See
  [docs/architecture.md](../../docs/architecture.md) and
  [docs/gotchas.md](../../docs/gotchas.md).
