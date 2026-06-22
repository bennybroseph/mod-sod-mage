-- mod-sod-mage: the Season of Discovery ALLIANCE acquisition of the Living Bomb
-- rune (rune_id 7000008 -> spell 900006, Hands slot). Faithful to SoD: the rune is
-- learned from "Spell Notes: Living Bomb" (item 208799, real SoD id), which drops
-- from Stonesplinter Seer (creature 1166) in Dun Morogh -- the Alliance source.
-- This file adds only the acquisition pieces:
--   * the notes item (208799) + its bag-icon manifest entry (tools/client_items.json),
--   * the notes as a Mage-only drop off Stonesplinter Seer (creature 1166),
--   * the engine unlock mapping (notes -> rune 7000008).
-- The rune_template row for 7000008 lives in sod_mage_runes.sql (the catalog home for
-- every rune); this file does NOT re-create it.
--
-- COUPLING: the item, loot and condition live in always-present world tables, so they
-- are inserted unconditionally. Only the engine-owned `rune_item_unlock` row is GUARDED
-- (information_schema check -> dynamic SQL), so without mod-rune-engraving it is a clean
-- no-op. Stonesplinter Seer (1166) is a STOCK AzerothCore creature -- no dependency.
--
-- Drop rate: the loot `Chance` below is just a seed; the WorldScript world_sod_mage_drops
-- overwrites it from SodMage.Drops.LivingBombNotesChance on startup and `.reload config`
-- (forced to 0 when SodMage.Enable = 0), like the other trash-drop notes.
--
-- IDs: rune_id 7000008 is the engine key; item 208799 is the real SoD item id
-- (wago.tools wow_classic_era). Spell 900006 = the Living Bomb rune driver. The notes'
-- ilvl 10 / Phase 1 mirror wago. Idempotent and safe to re-run.

-- =====================================================================
-- Spell Notes: Living Bomb (item 208799). Mage-only, BoP, Unique, Uncommon, ilvl 10
-- (SoD values). icon inv_scroll_03 (displayid 1102, shared with the other notes).
-- ScriptName 'item_rune_unlock' is the mod-rune-engraving engine script: using the
-- notes unlocks the mapped rune and consumes them. Spell 55884 is a harmless existing
-- use-spell, present only so the client offers "Use"; the engine script suppresses it.
-- A drop (not vendor-sold), so BuyPrice/SellPrice 0. NOTE: 208799 must also be in
-- tools/client_items.json so the bag icon resolves (built into the sod-client patch).
-- Upsert via REPLACE -- committed SQL never DELETEs (see the module CLAUDE.md).
-- =====================================================================
REPLACE INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (208799, 15, 0, 'Spell Notes: Living Bomb', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Drop the notes off Stonesplinter Seer (creature 1166, stock; lootid 1166). Its loot
-- table already exists, so this row is additive. The condition below restricts the roll
-- to Mages (SoD class loot). Chance 20 is a seed -- tuned by world_sod_mage_drops.
-- =====================================================================
REPLACE INTO `creature_loot_template`
    (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (1166, 208799, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Bomb rune notes');

REPLACE INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `Comment`)
VALUES
    (1, 1166, 208799, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Bomb notes drop for Mages only');

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the notes (208799) unlocks the Living Bomb
-- rune (7000008). Engine-owned `rune_item_unlock`; no-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (208799, 7000008)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
