-- TEMPLATE — copy into data/sql/db-world/base/sod_mage_<name>.sql, fill the
-- placeholders, keep what you need. Not applied from templates/. Idempotent: scoped
-- DELETE before INSERT. Model: data/sql/db-world/base/sod_mage_regeneration_unlock.sql.

-- =====================================================================
-- The item. class/subclass/Material/displayid MUST match the client_items.json
-- entry. Reuse a real SoD id when one exists.
-- =====================================================================
DELETE FROM `item_template` WHERE `entry` = <ITEM_ID>;

INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (<ITEM_ID>, 15, 0, '<Item Name>', <DISPLAY_ID>, 2, 0,
     1, 0, 0, 0,
     128, -1, <ILVL>, <REQ_LEVEL>,
     1, 1, 1, 1, 0,
     55884, 0, '<SCRIPT_NAME_OR_EMPTY>', '<Tooltip description.>');
--   AllowableClass 128 = Mage. Quality 2 = Uncommon. bonding 1 = BoP.
--   spellid_1 55884 + spelltrigger_1 0 = a harmless ON_USE so the client offers
--     "Use"; the ItemScript suppresses it. Drop both if the item has no "Use".
--   ScriptName: 'item_rune_unlock' (engine unlock), your own ItemScript, or '' .

-- =====================================================================
-- OPTIONAL companions — uncomment the ones your item needs.
-- =====================================================================

-- Drop it off creatures (~20% here). Scope the DELETE to YOUR item + entries.
-- DELETE FROM `creature_loot_template` WHERE `Item` = <ITEM_ID> AND `Entry` IN (<NPC_A>, <NPC_B>);
-- INSERT INTO `creature_loot_template`
--     (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
-- VALUES
--     (<NPC_A>, <ITEM_ID>, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage <name>'),
--     (<NPC_B>, <ITEM_ID>, 0, 20, 0, 1, 0, 1, 1, 'mod-sod-mage <name>');

-- Class-gate the drop (SoD class loot): only Mages roll it.
-- CONDITION_SOURCE_TYPE_CREATURE_LOOT_TEMPLATE = 1; CONDITION_CLASS = 15; value1 = classmask.
-- DELETE FROM `conditions`
--     WHERE `SourceTypeOrReferenceId` = 1 AND `SourceEntry` = <ITEM_ID> AND `SourceGroup` IN (<NPC_A>, <NPC_B>);
-- INSERT INTO `conditions`
--     (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
--      `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
--      `NegativeCondition`, `Comment`)
-- VALUES
--     (1, <NPC_A>, <ITEM_ID>, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: <name> for Mages only'),
--     (1, <NPC_B>, <ITEM_ID>, 0, 0, 15, 0, 128, 0, 0, 0, 'mod-sod-mage: <name> for Mages only');

-- Sell it at every reagent vendor (npcflag bit UNIT_NPC_FLAG_VENDOR_REAGENT = 2048).
-- DELETE FROM `npc_vendor` WHERE `item` = <ITEM_ID>;
-- INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
-- SELECT `entry`, 0, <ITEM_ID>, 0, 0, 0
-- FROM `creature_template` WHERE (`npcflag` & 2048) <> 0;
