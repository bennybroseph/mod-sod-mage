-- mod-sod-mage: the Season of Discovery ALLIANCE acquisition of the Enlightenment
-- rune (rune_id 7000004, Chest). Faithful to SoD's "Servant of Azora" event:
--   1. Odd critters carrying Wild Polymorph (spell 23603) wander all over Elwynn
--      (custom creature 700210 "Polymorphed Apprentice", a Beast so Polymorph
--      lands, shown with a random non-Elwynn critter model). They are seeded at
--      runtime by the AllCreatureScript `sod_mage_critter_seeder` -- a share of real
--      Elwynn critters are converted in place as they spawn (no DB spawn edits).
--   2. Casting Polymorph reveals the apprentice: the SAME creature (700210) clears
--      its auras, snaps to a human model in place, says its line, plays Blink, and
--      drops a paper (gameobject 700220) -- a seamless transform, no second NPC.
--   3. Clicking the paper grants the next Azora Apprentice Notes page the player
--      lacks (203756 / 203960 / 203961 / 203962 -- the real SoD page ids).
--   4. Using any page while holding all four assembles Spell Notes: Enlightenment
--      (203749); using THAT unlocks the rune via the engine's item_rune_unlock.
--
-- The C++ for steps 1-2 and the paper/combine is bound by ScriptName (see
-- src/sod_mage_azora_event.cpp). All ITEM ids are real SoD ids (wago.tools); the
-- creatures and GO are custom (no SoD creature id is sourceable -- creatures are
-- server-side, absent from client DB2s). Item icons mirror the source: pages use
-- INV_Misc_Note_06 (custom displayid 99010, tools/client_displays.json); Spell
-- Notes uses inv_scroll_03 (displayid 1102, like the sibling notes).
--
-- COUPLING: items, creatures, GO, spawns and text live in always-present world
-- tables -> inserted unconditionally. Only the engine-owned `rune_item_unlock`
-- mapping is GUARDED, so without mod-rune-engraving it is a clean no-op (and the
-- assembled notes, whose ScriptName resolves to the engine, simply do nothing).
--
-- IDs: creature 700210 (the disguised-critter/apprentice), GO 700220 (in the
-- module's 700200-700299 band). This file never DELETEs (committed SQL must not --
-- see the module CLAUDE.md); it upserts via REPLACE / ON DUPLICATE KEY UPDATE.
-- Retired test ids (the old apprentice 700211, fixed spawns) are cleaned by hand on
-- the test DB, never via committed SQL. Idempotent and safe to re-run.

-- =====================================================================
-- Items: the 4 pages + the assembled Spell Notes. Mage-only, BoP, Uncommon.
-- Pages carry the combine ScriptName; Spell Notes carries the engine unlock.
-- Spell 55884 is a harmless existing use-spell so the client offers "Use"; both
-- ItemScripts suppress it. (Source class for 203749 is 9/Recipe; we use 15 to
-- match the working item_rune_unlock notes.) Upsert via REPLACE -- committed SQL
-- never DELETEs (see the module CLAUDE.md SQL rules).
-- =====================================================================
REPLACE INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`,
     `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`,
     `maxcount`, `stackable`, `bonding`, `Material`, `sheath`,
     `spellid_1`, `spelltrigger_1`, `ScriptName`, `description`)
VALUES
    (203756, 15, 0, 'Azora Apprentice Notes: Page 1', 99010, 2, 0,
     1, 0, 0, 0, 128, -1, 10, 0, 1, 1, 1, 2, 0,
     55884, 0, 'item_sod_mage_azora_pages', ''),
    (203960, 15, 0, 'Azora Apprentice Notes: Page 2', 99010, 2, 0,
     1, 0, 0, 0, 128, -1, 10, 0, 1, 1, 1, 2, 0,
     55884, 0, 'item_sod_mage_azora_pages', ''),
    (203961, 15, 0, 'Azora Apprentice Notes: Page 3', 99010, 2, 0,
     1, 0, 0, 0, 128, -1, 10, 0, 1, 1, 1, 2, 0,
     55884, 0, 'item_sod_mage_azora_pages', ''),
    (203962, 15, 0, 'Azora Apprentice Notes: Page 4', 99010, 2, 0,
     1, 0, 0, 0, 128, -1, 10, 0, 1, 1, 1, 2, 0,
     55884, 0, 'item_sod_mage_azora_pages', ''),
    (203749, 15, 0, 'Spell Notes: Enlightenment', 1102, 2, 0,
     1, 0, 0, 0, 128, -1, 10, 1, 1, 1, 1, 2, 0,
     55884, 0, 'item_rune_unlock', 'Teaches you a new Engraving ability.');

-- =====================================================================
-- Creature 700210: the disguised "Polymorphed Apprentice" critter. Beast (so
-- Polymorph is a valid cast), critter faction 188 (neutral/attackable/passive),
-- tiny hp, random wander (MovementType 1). Wild Polymorph (23603) is applied via
-- script, applied permanently. Models are picked at random per spawn. Upsert via
-- REPLACE -- committed SQL never DELETEs (see the module CLAUDE.md SQL rules).
-- =====================================================================
REPLACE INTO `creature_template`
    (`entry`, `name`, `subname`,
     `minlevel`, `maxlevel`, `faction`, `npcflag`,
     `speed_walk`, `speed_run`, `rank`,
     `unit_class`, `unit_flags`, `unit_flags2`, `type`, `lootid`,
     `AIName`, `MovementType`, `HoverHeight`,
     `HealthModifier`, `ManaModifier`, `ArmorModifier`, `RegenHealth`,
     `flags_extra`, `ScriptName`)
VALUES
    -- One creature, two states (disguised critter, then the revealed apprentice
    -- after Polymorph -- the reveal is an in-place transform done in script).
    -- Beast so Polymorph lands; neutral critter faction; wanders; no XP/loot.
    (700210, 'Polymorphed Apprentice', '',
     10, 10, 188, 0,
     1.0, 0.85714, 0,
     1, 0, 2048, 1, 0,
     '', 1, 1.0,
     0.01, 1, 1, 1,
     2, 'npc_sod_mage_wild_critter');

-- One base model (a creature is capped at 4 DBC models, Idx <= 3). The disguise is
-- set per-spawn in script -- which rolls one of the 5 named critter displays and
-- overrides Wild Polymorph's transform, then snaps to a human model on reveal -- so
-- this row is just the pre-script fallback.
REPLACE INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES
    (700210, 0, 10000, 1.0, 1.0);  -- Ram (script re-rolls the disguise/reveal model)

-- Wild Polymorph (23603) is applied in script (npc_sod_mage_wild_critter), not via
-- creature_template_addon: it must be PERMANENT (its DBC duration would expire and
-- revert the disguise), and the script overrides its transform with a named model.

-- =====================================================================
-- The apprentice's say line (creature_text group 0). Type 12 = MONSTER_SAY.
-- =====================================================================
REPLACE INTO `creature_text`
    (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`,
     `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`)
VALUES
    (700210, 0, 0, 'I''m back to my old self! Thank you, hero!', 12, 0, 100,
     0, 0, 0, 0, 0, 'mod-sod-mage Polymorphed Apprentice');

-- =====================================================================
-- No fixed spawns. The disguised critters are seeded at runtime across all of
-- Elwynn by the AllCreatureScript `sod_mage_critter_seeder`: a share of real Elwynn
-- critters are converted in place (UpdateEntry) as they spawn, so they appear
-- everywhere critters do -- without editing any existing spawn rows.
-- =====================================================================

-- =====================================================================
-- GameObject 700220: the paper dropped on reveal. Type 10 (GOOBER) so a click
-- runs the GameObjectScript (GameObject::Use dispatches OnGossipHello before the
-- type switch). Display 210 = a paper-page-on-ground (reused from Highvale Notes).
-- The script grants the next page and despawns the GO; Data fields stay 0 (no
-- quest/spell), so default goober behavior never runs. Upsert via REPLACE.
-- =====================================================================
REPLACE INTO `gameobject_template`
    (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `size`,
     `ScriptName`)
VALUES
    (700220, 10, 210, 'Azora Apprentice Notes', '', '', 1.0,
     'go_sod_mage_azora_page');

-- =====================================================================
-- Item-unlock mapping (GUARDED): using the assembled Spell Notes (203749) unlocks
-- Enlightenment (7000004). This flips the rune from "free by class" to gated --
-- hidden at the engraver until the character uses the notes. Engine-owned
-- `rune_item_unlock`; no-op without mod-rune-engraving. No `rune_template` change
-- (7000004 already exists in sod_mage_runes.sql with enabled = 1).
-- =====================================================================
SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (203749, 7000004)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
