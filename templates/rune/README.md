# Template — rune + unlock chain

A spell becomes an **engravable rune** when the optional
[`mod-rune-engraving`](../../../mod-rune-engraving) engine is installed. The coupling
is **data-only**: you add rows to the engine's tables, guarded so they're a clean
no-op when the engine is absent.

The full contract (slot/class masks, the guarded-SQL rationale) lives in the engine's
[integrating-content.md](../../../mod-rune-engraving/docs/integrating-content.md).
These files are the mage-flavored copy-paste version.

## Files

- **`rune_template.sql`** — registers the spell as a rune in the engine catalog.
- **`rune_item_unlock.sql`** — gates the rune behind *using an item* (the SoD notes
  pattern). Skip it to leave the rune available by class.
- **`decode_item_script.cpp`** — optional: the "decode scrambled notes with a
  Comprehension Charm" combine step. Only needed if your acquisition chain has a
  decode step. Model: [src/item_sod_mage_decode_notes.cpp](../../src/item_sod_mage_decode_notes.cpp).

## How it fits together

1. The spell already works (its `spell_dbc` + script — see `../spell/`).
2. `rune_template.sql` lists it as a rune (Mage classmask `128`, a `slot_mask`).
3. To make it **earned** rather than free: ship the unlock item (see `../item/`),
   bind that item to `item_rune_unlock` (the engine's generic script), and map it in
   `rune_item_unlock.sql`. Using the item discovers the rune.

We own the rune-id band **7000000–7000999**. Items and drops use the **real SoD id**.

`slot_mask` is `1 << RuneSlot` (Chest = 16, Legs = 256); `class_mask` is the
AzerothCore classmask (Mage = 128). The `icon` must equal the spell's displayed icon
(the texture name in `tools/sod_spells.py`) so the rune panel matches.
