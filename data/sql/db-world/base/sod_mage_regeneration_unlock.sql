-- mod-sod-mage: the Season of Discovery item chain that unlocks the Regeneration
-- rune (rune_id 7000001), reproducing the original puzzle with the REAL SoD item
-- ids (wago.tools wow_classic_era):
--   1. Buy a Comprehension Charm (211779) from any reagent vendor.
--   2. Kill Defias Renegade Mage (450) / Defias Pillager (589) /
--      Dalaran Apprentice (1867) for Spell Notes: TENGI RONEERA (208754)
--      -- an anagram of REGENERATION.
--   3. Use the scrambled notes while holding the charm -> Spell Notes:
--      Regeneration (208753). The combine is handled by the mod-sod-mage
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
-- Bag icons: 211779/208754/208753 are added to tools/client_items.json so their
-- Item.dbc rows ship in mod-sod-world's consolidated client patch (else they show
-- a "?" icon in bags). Spell 55884 is a harmless existing use-spell, present only
-- so the client offers "Use"; both ItemScripts suppress it.
-- Idempotent and safe to re-run.

-- =====================================================================
-- Clean up the previous custom ids (700200-700202), retired in favor of the real
-- SoD ids. Scoped to our own rows so existing DBs converge on re-apply.
-- =====================================================================
DELETE FROM `item_template`          WHERE `entry` IN (700200, 700201, 700202);
DELETE FROM `creature_loot_template` WHERE `Item` = 700201 AND `Entry` IN (450, 589, 1867);
DELETE FROM `conditions`
    WHERE `SourceTypeOrReferenceId` = 1 AND `SourceEntry` = 700201 AND `SourceGroup` IN (450, 589, 1867);
DELETE FROM `npc_vendor` WHERE `item` = 700200;

-- =====================================================================
-- Items (unconditional). Reused displayids carry the real SoD icons: 31029 charm
-- (spell_holy_mindsooth), 1102 both notes (inv_scroll_03). Values per wago.tools:
-- charm Common/req 1/stacks 5; notes Uncommon/Unique/ilvl 10.
-- =====================================================================
DELETE FROM `item_template` WHERE `entry` IN (211779, 208754, 208753);

INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    -- Comprehension Charm (211779): the decoder tool. Mage-only, BoP, Reagent
    -- subclass, ilvl/req 1, stacks to 5. No use-spell/script: the decode script
    -- just checks it's in bags. displayid 31029 carries the real SoD icon.
    (211779, 15, 1, 'Comprehension Charm', 31029, 1, 0,
     1, 700, 175, 0,
     128, -1, 1, 1,
     0, 5, 1, 1, 0,
     0, 0, '', 'Aids the translation of scrolls and spell notes.'),

    -- Spell Notes: TENGI RONEERA (208754, scrambled). Uncommon, BoP, Unique,
    -- ilvl 10, Mage-only, icon inv_scroll_03 (displayid 1102). Decoded by the
    -- mod-sod-mage combine script with a Comprehension Charm.
    (208754, 15, 0, 'Spell Notes: TENGI RONEERA', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 0,
     1, 1, 1, 1, 0,
     55884, 0, 'item_sod_mage_decode_notes',
     'Decipher these with a Comprehension Charm to learn a new Engraving spell.'),

    -- Spell Notes: Regeneration (208753, deciphered). Uncommon, BoP, Unique,
    -- ilvl 10, req 1, Mage-only, same inv_scroll_03 icon. Using it unlocks the
    -- rune via the engine's item_rune_unlock script.
    (208753, 15, 0, 'Spell Notes: Regeneration', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Loot (unconditional): the scrambled notes drop from the three SoD sources.
-- ~20% chance, single drop.
-- =====================================================================
DELETE FROM `creature_loot_template` WHERE `Item` = 208754 AND `Entry` IN (450, 589, 1867);

INSERT INTO `creature_loot_template`
    (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (450,  208754, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Regeneration rune notes'),
    (589,  208754, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Regeneration rune notes'),
    (1867, 208754, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Regeneration rune notes');

-- =====================================================================
-- Loot condition (unconditional): only Mages roll the notes (SoD class loot).
-- CONDITION_SOURCE_TYPE_CREATURE_LOOT_TEMPLATE = 1; CONDITION_CLASS = 15,
-- ConditionValue1 = classmask (Mage = 128).
-- =====================================================================
DELETE FROM `conditions`
    WHERE `SourceTypeOrReferenceId` = 1 AND `SourceEntry` = 208754 AND `SourceGroup` IN (450, 589, 1867);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `Comment`)
VALUES
    (1, 450,  208754, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Regeneration notes drop for Mages only'),
    (1, 589,  208754, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Regeneration notes drop for Mages only'),
    (1, 1867, 208754, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Regeneration notes drop for Mages only');

-- =====================================================================
-- Vendor (unconditional): the Comprehension Charm is sold by EVERY reagent
-- vendor (npcflag bit UNIT_NPC_FLAG_VENDOR_REAGENT = 2048). Set-based + idempotent.
-- =====================================================================
DELETE FROM `npc_vendor` WHERE `item` = 211779;

INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
SELECT `entry`, 0, 211779, 0, 0, 0
FROM `creature_template`
WHERE (`npcflag` & 2048) <> 0;

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the deciphered notes (208753) unlocks
-- the Regeneration rune (7000001). This is what flips the rune to "gated".
-- Engine-owned `rune_item_unlock`; no-op without the engine. Also drops the old
-- 700202 mapping so existing DBs converge.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'DELETE FROM `rune_item_unlock` WHERE `item_id` = 700202', 'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (208753, 7000001)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
