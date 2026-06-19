-- mod-sod-mage: the Season of Discovery item chain that unlocks the Living Flame
-- rune (rune_id 7000003, Legs), reproducing the original SoD puzzle with the REAL
-- SoD item ids (wago.tools wow_classic_era):
--   1. Buy a Comprehension Charm (211779) from any reagent vendor (already sold by
--      sod_mage_regeneration_unlock.sql -- shared across all note chains).
--   2. Kill one of the SoD sources below for Spell Notes: MILEGIN VALF (203752)
--      -- an anagram of LIVING FLAME.
--   3. Use the scrambled notes while holding the charm -> Spell Notes: Living
--      Flame (203746). The combine is handled by the mod-sod-mage ItemScript
--      'item_sod_mage_decode_notes', which consumes one charm and one scrambled
--      note (the script's note table carries the 203752 -> 203746 pair).
--   4. Use the deciphered notes -> unlocks the rune. Handled by the engine's
--      generic 'item_rune_unlock' ItemScript via the rune_item_unlock mapping.
--
-- COUPLING: items, loot and the mage-only loot condition live in standard world
-- tables (always present) -> inserted unconditionally. Only the engine-owned
-- `rune_item_unlock` mapping is GUARDED (information_schema check -> dynamic SQL),
-- so without the engine the rune row no-ops and the deciphered notes (whose
-- ScriptName resolves to the engine) simply do nothing. The rune_template row for
-- 7000003 lives in sod_mage_runes.sql.
--
-- Bag icons: 203752/203746 are added to tools/client_items.json so their Item.dbc
-- rows ship in mod-sod-world's consolidated client patch (else they show a "?"
-- icon in bags). Both reuse displayid 1102 (inv_scroll_03), like the other notes.
-- Spell 55884 is a harmless existing use-spell, present only so the client offers
-- "Use"; both ItemScripts suppress it.
-- Idempotent and safe to re-run.

-- =====================================================================
-- Items (unconditional). Values per wago.tools: both Uncommon, BoP, Unique,
-- ilvl 10, Mage-only, icon inv_scroll_03 (displayid 1102).
-- =====================================================================
REPLACE INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    -- Spell Notes: MILEGIN VALF (203752, scrambled). Decoded by the mod-sod-mage
    -- combine script with a Comprehension Charm.
    (203752, 15, 0, 'Spell Notes: MILEGIN VALF', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 0,
     1, 1, 1, 1, 0,
     55884, 0, 'item_sod_mage_decode_notes',
     'Decipher these with a Comprehension Charm to learn a new Engraving spell.'),

    -- Spell Notes: Living Flame (203746, deciphered). Using it unlocks the rune
    -- via the engine's item_rune_unlock script.
    (203746, 15, 0, 'Spell Notes: Living Flame', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 10, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- Loot (unconditional): the scrambled notes drop from the SoD sources.
-- ~20% chance, single drop (matches the Regeneration notes' rate).
-- =====================================================================
REPLACE INTO `creature_loot_template`
    (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (476,  203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Kobold Geomancer
    (1124, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Frostmane Shadowcaster
    (1397, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Frostmane Seer
    (1535, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Scarlet Warrior
    (1536, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Scarlet Missionary
    (1537, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Scarlet Zealot
    (3195, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Burning Blade Thug
    (3196, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Burning Blade Neophyte
    (3197, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Burning Blade Fanatic
    (3198, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes'),  -- Burning Blade Apprentice
    (3199, 203752, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage Living Flame rune notes');  -- Burning Blade Cultist

-- =====================================================================
-- Loot condition (unconditional): only Mages roll the notes (SoD class loot).
-- CONDITION_SOURCE_TYPE_CREATURE_LOOT_TEMPLATE = 1; CONDITION_CLASS = 15,
-- ConditionValue1 = classmask (Mage = 128).
-- =====================================================================
REPLACE INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `Comment`)
VALUES
    (1, 476,  203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 1124, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 1397, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 1535, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 1536, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 1537, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 3195, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 3196, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 3197, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 3198, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only'),
    (1, 3199, 203752, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: Living Flame notes drop for Mages only');

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the deciphered notes (203746) unlocks
-- the Living Flame rune (7000003). This is what flips the rune to "gated".
-- Engine-owned `rune_item_unlock`; no-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (203746, 7000003)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
