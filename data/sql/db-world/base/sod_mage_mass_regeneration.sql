-- mod-sod-mage: the Season of Discovery acquisition of the Mass Regeneration
-- rune (rune_id 7000002, Legs). Faithful to SoD: the rune is learned from
-- "Spell Notes: Mass Regeneration" (item 211514) dropped by the Awakened Lich
-- in Raven Hill. The Lich encounter (Dusty Coffer -> Decrepit Phylactery ->
-- summon the Lich) is shared world content owned by mod-sod-world; this file
-- only adds the MAGE-specific pieces:
--   * the notes item (211514) + its bag-icon manifest entry (tools/client_items.json),
--   * the notes as a Mage-only drop off the Lich (creature 212261),
--   * the engine unlock mapping (notes -> rune 7000002).
--
-- DEPENDENCY: the Awakened Lich (creature 212261) is provided by mod-sod-world.
-- Without it the loot row is inert (the creature never spawns) but harmless.
--
-- COUPLING: the item, loot and condition live in always-present world tables, so
-- they are inserted unconditionally. Only the engine-owned `rune_template` and
-- `rune_item_unlock` rows are GUARDED (information_schema check -> dynamic SQL),
-- so without mod-rune-engraving they are a clean no-op.
--
-- IDs: rune_id 7000002 is the engine key (no SoD analogue); item 211514 is the
-- real SoD item id (wago.tools wow_classic_era). Spell 412510 = Mass Regeneration.
-- Idempotent and safe to re-run.

-- =====================================================================
-- Retire the old demo quest path (quest 700100 "Echoes of Renewal" + its NPC
-- Magister Tessaril 700100). Mass Regeneration is now drop-gated, so the quest
-- and quest-giver are removed. Scoped DELETEs make existing DBs converge.
-- =====================================================================
DELETE FROM `creature_queststarter`   WHERE `id` = 700100 AND `quest` = 700100;
DELETE FROM `creature_questender`     WHERE `id` = 700100 AND `quest` = 700100;
DELETE FROM `quest_template_addon`    WHERE `ID` = 700100;
DELETE FROM `quest_template`          WHERE `ID` = 700100;
DELETE FROM `creature`                WHERE `id1` = 700100;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 700100;
DELETE FROM `creature_template`       WHERE `entry` = 700100;

-- =====================================================================
-- Spell Notes: Mass Regeneration (item 211514). Mage-only, BoP, Unique,
-- Uncommon, ilvl 25 (SoD values). icon inv_scroll_03 (displayid 1102, like the
-- other notes). ScriptName 'item_rune_unlock' is the mod-rune-engraving engine
-- script: using the notes unlocks the mapped rune and consumes them. Spell 55884
-- is a harmless existing use-spell, present only so the client offers "Use"; the
-- engine script suppresses it. NOTE: 211514 must also be in tools/client_items.json
-- so the bag icon resolves (built into mod-sod-world's consolidated patch MPQ).
-- =====================================================================
DELETE FROM `item_template` WHERE `entry` = 211514;

INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (211514, 15, 0, 'Spell Notes: Mass Regeneration', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 25, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Drop the notes off the shared Awakened Lich (creature 212261, mod-sod-world).
-- The Lich's lootid is 212261, so this row rolls when it dies. Guaranteed for a
-- Mage (the condition below restricts the roll to Mages -- SoD class loot).
-- =====================================================================
DELETE FROM `creature_loot_template` WHERE `Item` = 211514 AND `Entry` = 212261;

INSERT INTO `creature_loot_template`
    (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (212261, 211514, 0, 100, 0, 1, 0, 1, 1, 'mod-sod-mage Mass Regeneration rune notes');

DELETE FROM `conditions`
    WHERE `SourceTypeOrReferenceId` = 1 AND `SourceGroup` = 212261 AND `SourceEntry` = 211514;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `Comment`)
VALUES
    (1, 212261, 211514, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Mass Regeneration notes drop for Mages only');

-- =====================================================================
-- Rune 7000002 (GUARDED) -- engine-owned `rune_template`. Re-created here (its
-- previous home, the demo-quest SQL, is removed). No-op without the engine.
-- =====================================================================
SET @rune_tbl := (SELECT COUNT(*) FROM information_schema.tables
                  WHERE table_schema = DATABASE() AND table_name = 'rune_template');

SET @sql := IF(@rune_tbl > 0,
'INSERT INTO `rune_template`
    (`rune_id`, `spell_id`, `class_mask`, `slot_mask`, `name`, `icon`, `description`, `source`, `enabled`)
 VALUES
    (7000002, 412510, 128, 256, ''Mass Regeneration'', ''spell_arcane_arcane03'',
     ''A channeled heal-over-time that applies Temporal Beacon to nearby allies.'',
     ''mod-sod-mage'', 1)
 ON DUPLICATE KEY UPDATE
    `spell_id`    = VALUES(`spell_id`),
    `class_mask`  = VALUES(`class_mask`),
    `slot_mask`   = VALUES(`slot_mask`),
    `name`        = VALUES(`name`),
    `icon`        = VALUES(`icon`),
    `description` = VALUES(`description`),
    `source`      = VALUES(`source`),
    `enabled`     = VALUES(`enabled`)',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- =====================================================================
-- Remove the stale quest-unlock mapping (7000002 was quest-gated; it is now
-- item-gated). GUARDED -- engine-owned `rune_quest_unlock`. No-op without engine.
-- =====================================================================
SET @quest_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                          WHERE table_schema = DATABASE() AND table_name = 'rune_quest_unlock');

SET @sql := IF(@quest_unlock_tbl > 0,
'DELETE FROM `rune_quest_unlock` WHERE `rune_id` = 7000002 AND `quest_id` = 700100',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the notes (211514) unlocks Mass
-- Regeneration (7000002). This flips the rune to "gated" -- hidden at the
-- engraver until the character uses the notes. No-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (211514, 7000002)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
