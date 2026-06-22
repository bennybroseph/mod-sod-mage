-- mod-sod-mage: the Season of Discovery acquisition for the Arcane Blast rune
-- (rune_id 7000006 -> spell 900003, the Hands-slot driver), via the Zoram Strand
-- crystal discovery in Ashenvale, using the REAL SoD item id (wago.tools wow_classic_era):
--   * Cast Arcane Explosion on three purple crystals IN ORDER -- Southern (701100),
--     Middle (701101), Northern (701102) -- to build the Arcane Charge buff (spell
--     900005). At 3 stacks the player_sod_mage_arcane_crystals PlayerScript consumes it
--     and grants Spell Notes: Arcane Blast (211691).
--   * Using the notes unlocks the rune via the engine's generic 'item_rune_unlock'
--     ItemScript, driven by the guarded rune_item_unlock mapping (211691 -> 7000006).
--
-- The crystal proximity + stack logic is all in C++ (player_sod_mage_arcane_crystals);
-- this file just places the gameobjects, defines the notes item, and (GUARDED) maps the
-- notes to the rune. The Arcane Charge buff itself is a generated spell (sod_spells.py
-- 900005 -> sod_mage_spell_dbc.sql).
--
-- Bag icon: 211691 -> tools/client_items.json (display 1102 = inv_scroll_03, reused).
-- Spell 55884 is a harmless use-spell so the client offers "Use"; item_rune_unlock
-- suppresses it. Idempotent and safe to re-run.

-- =====================================================================
-- Item (unconditional). Uncommon, BoP, Unique, ilvl 25, Mage-only. displayid 1102
-- carries the real inv_scroll_03 icon. Not sold (puzzle-granted) -> BuyPrice/SellPrice 0.
-- =====================================================================
REPLACE INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (211691, 15, 0, 'Spell Notes: Arcane Blast', 1102, 2, 0,
     1, 0, 0, 0,
     128, -1, 25, 1,
     1, 1, 1, 1, 0,
     55884, 0, 'item_rune_unlock',
     'Teaches you a new Engraving ability.');

-- =====================================================================
-- The three discovery crystals. type 5 = GAMEOBJECT_TYPE_GENERIC (scenery you cast
-- Arcane Explosion near; no click interaction). displayId 5011 = a purple amethyst
-- crystal cluster (GameObjectDisplayInfo model Maraudon AmethystCrystal01.mdx; stock,
-- present in the server DBC). Distinct entries so the C++ knows each crystal's order:
-- 701100 Southern, 701101 Middle, 701102 Northern (mod-sod-mage GO band 701100-701199).
-- =====================================================================
REPLACE INTO `gameobject_template`
    (`entry`, `type`, `displayId`, `name`, `size`)
VALUES
    (701100, 5, 5011, 'Arcane Crystal', 2.0),
    (701101, 5, 5011, 'Arcane Crystal', 2.0),
    (701102, 5, 5011, 'Arcane Crystal', 2.0);

-- Spawn seeds in The Zoram Strand, Ashenvale -- map 1 (Kalimdor). Final captured coords
-- (in-game .gps at each SoD /way spot): Southern 701100 (/way 13.05, 24.87), Middle 701101
-- (/way 14.05, 19.82), Northern 701102 (/way 13.51, 15.74). INSERT IGNORE so re-applying
-- never overwrites a later in-game move. zoneId/areaId 0 = auto-detect. guid band 8830000+.
INSERT IGNORE INTO `gameobject`
    (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`,
     `position_x`, `position_y`, `position_z`, `orientation`,
     `rotation0`, `rotation1`, `rotation2`, `rotation3`,
     `spawntimesecs`, `animprogress`, `state`)
VALUES
    (8830001, 701100, 1, 0, 0, 1, 1,
     3717.0215, 947.4662, -4.8100915, 0.23702684, 0, 0, 0.118238, 0.992986, 300, 0, 1),
    (8830002, 701101, 1, 0, 0, 1, 1,
     3910.9976, 889.83105, -4.9130816, 3.4178524, 0, 0, 0.990474, -0.137691, 300, 0, 1),
    (8830003, 701102, 1, 0, 0, 1, 1,
     4067.8003, 920.9232, -0.29570368, 0.33907986, 0, 0, 0.168728, 0.985659, 300, 0, 1);

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the notes (211691) unlocks the Arcane Blast
-- rune (7000006). Engine-owned `rune_item_unlock`; no-op without the engine.
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (211691, 7000006)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
