-- mod-sod-mage: the Season of Discovery item chain that unlocks the Regeneration
-- rune (rune_id 7000001), reproducing the original puzzle:
--   1. Buy a Comprehension Charm (700200) from any reagent vendor.
--   2. Kill Defias Renegade Mage (450) / Defias Pillager (589) /
--      Dalaran Apprentice (1867) for Spell Notes: TENGI RONEERA (700201)
--      -- an anagram of REGENERATION.
--   3. Use the scrambled notes while holding the charm -> Spell Notes:
--      Regeneration (700202). The combine is handled by the mod-sod-mage
--      ItemScript 'item_sod_mage_decode_notes', which consumes one charm and one
--      scrambled note (the charm stacks to 5 / is conjured in 5s, like SoD).
--   4. Use the deciphered notes -> unlocks the rune. Handled by the engine's
--      generic 'item_rune_unlock' ItemScript via the rune_item_unlock mapping.
--
-- COUPLING: items, loot, vendor entries and the mage-only loot condition live in
-- standard world tables (always present) -> inserted unconditionally. Only the
-- engine-owned `rune_item_unlock` mapping is GUARDED (see sod_mage_runes.sql), so
-- without the engine the rune rows no-op and the deciphered notes (whose
-- ScriptName resolves to the engine) simply do nothing.
--
-- mod-sod-mage item id band: 700200-700299 (charm 700200, scrambled 700201,
-- deciphered 700202). Spell 55884 is a harmless existing "whistle" use-spell,
-- present only so the client offers "Use"; both ItemScripts suppress it.
-- Idempotent and safe to re-run.

-- =====================================================================
-- Items (unconditional). Reused displayids carry the real SoD icons: 31029 charm
-- (Spell_Holy_MindSooth), 1102 both notes (inv_scroll_03). NOTE: a custom item's
-- name/tooltip come from the server, but the bag inventory ICON is resolved
-- client-side via Item.dbc (itemId -> DisplayInfoID). These ids must also be
-- added to the client's Item.dbc (tools/build_sod_mage_patch.py ships them in the
-- patch MPQ) or the items show a "?" icon in bags.
-- =====================================================================
DELETE FROM `item_template` WHERE `entry` IN (700200, 700201, 700202);

INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    -- Comprehension Charm: the decoder tool. Values match SoD item 211779 --
    -- Mage-only, BoP, Reagent subclass, ilvl 1, req level 2, stacks to 5, buy
    -- price 7s. No use-spell/script: the decode script just checks it's in bags.
    -- displayid 31029 carries the real SoD icon (Spell_Holy_MindSooth).
    (700200, 15, 1, 'Comprehension Charm', 31029, 1, 0,
     1, 700, 175, 0,
     128, -1, 1, 2,
     0, 5, 1, 1, 0,
     0, 0, '', 'Aids the translation of scrolls and spell notes.'),

    -- Spell Notes: TENGI RONEERA (scrambled). Values match SoD item 208754 --
    -- Uncommon, BoP, Unique, ilvl 10, Mage-only, icon inv_scroll_03 (displayid
    -- 1102). Decoded by the mod-sod-mage combine script with a Comprehension Charm.
    (700201, 15, 0, 'Spell Notes: TENGI RONEERA', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 0,
     1, 1, 1, 1, 0,
     55884, 0, 'item_sod_mage_decode_notes',
     'Decipher these with a Comprehension Charm to learn a new Engraving spell.'),

    -- Spell Notes: Regeneration (deciphered). Values match SoD item 208753 --
    -- Uncommon, BoP, Unique, ilvl 10, Mage-only, same inv_scroll_03 icon. Using
    -- it unlocks the rune via the engine's item_rune_unlock script.
    (700202, 15, 0, 'Spell Notes: Regeneration', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 0,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Loot (unconditional): the scrambled notes drop from the three SoD sources.
-- lootid == entry for all three. ~20% chance, single drop.
-- =====================================================================
DELETE FROM `creature_loot_template` WHERE `Item` = 700201 AND `Entry` IN (450, 589, 1867);

INSERT INTO `creature_loot_template`
    (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (450,  700201, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Regeneration rune notes'),
    (589,  700201, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Regeneration rune notes'),
    (1867, 700201, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Regeneration rune notes');

-- =====================================================================
-- Loot condition (unconditional): only Mages roll the notes (SoD class loot).
-- CONDITION_SOURCE_TYPE_CREATURE_LOOT_TEMPLATE = 1; CONDITION_CLASS = 15,
-- ConditionValue1 = classmask (Mage = 128).
-- =====================================================================
DELETE FROM `conditions`
    WHERE `SourceTypeOrReferenceId` = 1 AND `SourceEntry` = 700201 AND `SourceGroup` IN (450, 589, 1867);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `Comment`)
VALUES
    (1, 450,  700201, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Regeneration notes drop for Mages only'),
    (1, 589,  700201, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Regeneration notes drop for Mages only'),
    (1, 1867, 700201, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Regeneration notes drop for Mages only');

-- =====================================================================
-- Vendor (unconditional): the Comprehension Charm is sold by EVERY reagent
-- vendor (npcflag bit UNIT_NPC_FLAG_VENDOR_REAGENT = 2048). Set-based + idempotent.
-- =====================================================================
DELETE FROM `npc_vendor` WHERE `item` = 700200;

INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
SELECT `entry`, 0, 700200, 0, 0, 0
FROM `creature_template`
WHERE (`npcflag` & 2048) <> 0;

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the deciphered notes (700202) unlocks
-- the Regeneration rune (7000001). This is what flips the rune to "gated".
-- Engine-owned `rune_item_unlock`; no-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (700202, 7000001)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
