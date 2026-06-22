-- mod-sod-mage: the Season of Discovery acquisition for the Rewind Time rune
-- (rune_id 7000007 -> spell 401462, Wrist slot), using the REAL SoD item id
-- (wago.tools wow_classic_era):
--   * Spell Notes: Rewind Time (210654) -- Mage-only, BoP, Unique. Its SoD "Use"
--     engraves the Rewind Time rune on your bracers (Wrist). Sold by Grizzby, the
--     Ratchet goblin vendor (mod-sod-world creature 211653).
--   * Using it unlocks the rune via the engine's generic 'item_rune_unlock'
--     ItemScript, driven by the guarded rune_item_unlock mapping (210654 -> 7000007).
--
-- COUPLING:
--   * rune_item_unlock -- owned by mod-rune-engraving (the engine); GUARDED so it's a
--     clean no-op when the engine is absent (information_schema PREPARE/EXECUTE, since
--     a plain INSERT errors on the missing table before any WHERE runs).
--   * npc_vendor (211653 -> 210654) -- Grizzby's stock. Grizzby himself (creature +
--     spawn) is owned by mod-sod-world (sod_world_grizzby.sql); this file supplies what
--     he sells, the same direction as the shared Lich loot (a class module references
--     sod-world's creature id). npc_vendor is a core table validated at world load, so
--     a plain INSERT IGNORE is order-independent; with mod-sod-world absent the row is
--     just an orphaned (ignored) vendor entry.
-- The item itself lives in a standard world table -> inserted unconditionally. Being
-- BoP + Mage-only, the core's vendor list auto-hides it from non-mages.
--
-- Bag icon: 210654 is added to tools/client_items.json so its Item.dbc row ships in
-- the consolidated sod-client patch (else it shows a "?" in bags). displayid 1102 is
-- the shared Spell Notes scroll (inv_scroll_03), reused. Spell 55884 is a harmless
-- existing use-spell, present only so the client offers "Use"; the engine's
-- item_rune_unlock script suppresses it. Idempotent and safe to re-run.

-- =====================================================================
-- Item (unconditional). Uncommon, BoP, Unique, ilvl 25, Mage-only. displayid 1102
-- carries the real inv_scroll_03 icon. BuyPrice 20000 (2g) matches this module's
-- other vendor-sold Spell Notes (Arcane Surge); SellPrice 0.
-- =====================================================================
REPLACE INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (210654, 15, 0, 'Spell Notes: Rewind Time', 1102, 2, 0,
     1, 20000, 0, 0,
     128, -1, 25, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Grizzby's stock: he sells the notes (Ratchet vendor 211653, mod-sod-world). Plain
-- INSERT IGNORE -- npc_vendor is validated at world load, so this is order-independent
-- and a harmless ignored row if Grizzby (mod-sod-world) isn't installed. maxcount 0 =
-- unlimited; ExtendedCost 0 = bought with coin (item_template.BuyPrice).
-- =====================================================================
INSERT IGNORE INTO `npc_vendor`
    (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
VALUES
    (211653, 0, 210654, 0, 0, 0);

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the notes (210654) unlocks the Rewind Time
-- rune (7000007). Engine-owned `rune_item_unlock`; no-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (210654, 7000007)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
