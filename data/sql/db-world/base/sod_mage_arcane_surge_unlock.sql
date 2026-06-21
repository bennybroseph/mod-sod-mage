-- mod-sod-mage: the Season of Discovery acquisition for the Arcane Surge rune
-- (rune_id 7000005 -> spell 425124, Legs slot), using the REAL SoD item id
-- (wago.tools wow_classic_era):
--   * Spell Notes: Arcane Surge (211386) -- Mage-only, BoP, Unique. Its SoD "Use"
--     engraves the Arcane Surge rune on your pants (Legs). Sold by the six supply
--     officers (mod-sod-world) at Friendly with the player's supply faction.
--   * Using it unlocks the rune via the engine's generic 'item_rune_unlock'
--     ItemScript, driven by the guarded rune_item_unlock mapping (211386 -> 7000005).
--
-- COUPLING (both GUARDED so each is a harmless no-op when its provider module is
-- absent -- same information_schema PREPARE/EXECUTE pattern as sod_mage_runes.sql,
-- since a plain INSERT errors on the missing table before any WHERE runs):
--   * rune_item_unlock  -- owned by mod-rune-engraving (the engine).
--   * sod_world_supply_vendor -- owned by mod-sod-world (the supply-officer vendor;
--     one row = item + minimum reputation RANK; mod-sod-world's startup WorldScript
--     builds it into the per-rank tier catalogs). 4 = Friendly.
-- The item itself lives in a standard world table -> inserted unconditionally. Being
-- BoP + Mage-only, the core's vendor list auto-hides it from non-mages, so the rep
-- tier and the class filter stack.
--
-- Bag icon: 211386 is added to tools/client_items.json so its Item.dbc row ships in
-- the consolidated sod-client patch (else it shows a "?" in bags). displayid 1102 is
-- the shared Spell Notes scroll (inv_scroll_03), reused -- no new display. Spell 55884
-- is a harmless existing use-spell, present only so the client offers "Use"; the
-- engine's item_rune_unlock script suppresses it.
-- Idempotent and safe to re-run.

-- =====================================================================
-- Item (unconditional). Uncommon, BoP, Unique, ilvl 25, Mage-only. displayid 1102
-- carries the real inv_scroll_03 icon. (wago lists class 9 / Material 2; we use
-- class 15 / Material 1 to match this module's other Spell Notes.) BuyPrice 20000
-- (2g) is the supply-officer price; SellPrice 0.
-- =====================================================================
REPLACE INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (211386, 15, 0, 'Spell Notes: Arcane Surge', 1102, 2, 0,
     1, 20000, 0, 0,
     128, -1, 25, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the notes (211386) unlocks the Arcane Surge
-- rune (7000005). Engine-owned `rune_item_unlock`; no-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (211386, 7000005)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- =====================================================================
-- Supply-vendor placement (GUARDED): the six supply officers sell the notes once
-- the buyer is Friendly (rank 4) with their supply faction. mod-sod-world-owned
-- `sod_world_supply_vendor` (item + minimum rank); no-op without that module.
-- =====================================================================
SET @supply_tbl := (SELECT COUNT(*) FROM information_schema.tables
                    WHERE table_schema = DATABASE() AND table_name = 'sod_world_supply_vendor');

SET @sql := IF(@supply_tbl > 0,
'INSERT INTO `sod_world_supply_vendor` (`item`, `RequiredRank`)
 VALUES (211386, 4)
 ON DUPLICATE KEY UPDATE `RequiredRank` = VALUES(`RequiredRank`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
