-- mod-sod-mage: a quest-gated rune, demonstrating the optional
-- mod-rune-engraving engine's "earned rune" path end to end.
--
-- A mage completes a small custom quest ("Echoes of Renewal") from a quest-giver
-- NPC; on completion the engine unlocks the Mass Regeneration rune, which can
-- then be engraved in the Legs slot. The existing Regeneration rune (7000001,
-- Chest) stays freely available by class, so both acquisition paths are visible
-- side by side.
--
-- COUPLING: the quest, the quest-giver NPC, its spawn, and the queststarter/
-- questender links live in standard world tables that ALWAYS exist, so they are
-- inserted unconditionally -- without the engine they form a legitimate, minor
-- standalone mage quest. Only the two engine-owned tables (`rune_template`,
-- `rune_quest_unlock`) are written through the GUARDED dynamic-SQL pattern (see
-- sod_mage_runes.sql), so without the engine the rune rows are a clean no-op.
--
-- ID bands owned by mod-sod-mage:
--   rune_id            7000000-7000999  (this rune: 7000002)
--   creature entry     700100-700199    (quest giver: 700100)
--   quest id           700100-700199    (quest: 700100)
--   creature spawn guid 8810000+        (this spawn: 8810001)
--
-- Mapping for rune 7000002 (Mass Regeneration -> spell 412510):
--   class_mask 128 = Mage   (1 << (CLASS_MAGE - 1) = 1 << 7)
--   slot_mask  256 = Legs   (1 << RUNE_SLOT_LEGS   = 1 << 8)
-- Idempotent and safe to re-run.

-- =====================================================================
-- Quest-giver NPC 700100 (unconditional). Gossip + questgiver, FRIENDLY
-- faction (35) so any race can use it, non-attackable civilian. No
-- ScriptName: default questgiver gossip handles accept/turn-in.
-- DisplayID 3167 (Stormwind City Guard) is a verified neutral placeholder.
-- =====================================================================
DELETE FROM `creature`                WHERE `id1` = 700100;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 700100;
DELETE FROM `creature_template`       WHERE `entry` = 700100;

INSERT INTO `creature_template` (
    `entry`,
    `name`, `subname`,
    `minlevel`, `maxlevel`,
    `faction`, `npcflag`,
    `speed_walk`, `speed_run`,
    `unit_class`, `unit_flags`, `unit_flags2`,
    `type`, `flags_extra`
) VALUES (
    700100,
    'Magister Tessaril',
    'Runic Studies',
    80, 80,
    35,    -- FRIENDLY (usable by any race)
    3,     -- GOSSIP | QUESTGIVER
    1.0, 1.14286,
    1,     -- warrior (simplest stats)
    2,     -- NON_ATTACKABLE
    0,
    7,     -- humanoid
    2      -- CIVILIAN (no aggro radius)
);

INSERT INTO `creature_template_model` (
    `CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`
) VALUES (
    700100, 0, 3167, 1.0, 1.0
);

-- Spawn in Stormwind, Trade District, beside the Rune Engraver (8800001).
-- guid 8810001 sits in mod-sod-mage's high spawn band. Horde test characters
-- can place their own copy with `.npc add 700100`.
INSERT INTO `creature` (
    `guid`, `id1`, `map`, `spawnMask`, `phaseMask`,
    `position_x`, `position_y`, `position_z`, `orientation`,
    `spawntimesecs`
) VALUES
    (8810001, 700100, 0, 1, 1, -8822.0, 651.0, 94.57176, 4.7048225, 180);

-- =====================================================================
-- Quest 700100 "Echoes of Renewal" (unconditional). Mage-only, low level,
-- no objective -> accept and immediately turn in at the same NPC, which
-- fires OnPlayerCompleteQuest and unlocks the rune (when the engine is
-- present). A small XP/money reward keeps it a legitimate quest on its own.
-- =====================================================================
DELETE FROM `quest_template_addon` WHERE `ID` = 700100;
DELETE FROM `quest_template`       WHERE `ID` = 700100;

INSERT INTO `quest_template` (
    `ID`, `QuestType`, `QuestLevel`, `MinLevel`, `QuestSortID`, `QuestInfoID`,
    `RewardXPDifficulty`, `RewardMoney`, `Flags`,
    `LogTitle`, `LogDescription`, `QuestDescription`, `QuestCompletionLog`
) VALUES (
    700100, 2, 5, 1, 0, 0,
    1, 250, 0,
    'Echoes of Renewal',
    'Speak with Magister Tessaril about the lost art of runic regeneration.',
    'The Season of Discovery stirred old magics back to life. I have been '
    'piecing together the runework of the magi who first channeled renewal '
    'into the weave itself. Study what I have recovered, $N, and the knowledge '
    'of Mass Regeneration will be yours to engrave.',
    'You have grasped the principles of runic renewal. Seek a Rune Engraver to '
    'inscribe Mass Regeneration upon your legs.'
);

INSERT INTO `quest_template_addon` (
    `ID`, `AllowableClasses`, `MaxLevel`, `SpecialFlags`
) VALUES (
    700100, 128, 0, 0
);

DELETE FROM `creature_queststarter` WHERE `id` = 700100 AND `quest` = 700100;
DELETE FROM `creature_questender`   WHERE `id` = 700100 AND `quest` = 700100;

INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES (700100, 700100);
INSERT INTO `creature_questender`   (`id`, `quest`) VALUES (700100, 700100);

-- =====================================================================
-- Rune 7000002 (GUARDED) -- engine-owned `rune_template`. No-op without
-- the engine. The INSERT lives in a plain single-quoted multi-line literal
-- (inner quotes escaped as '') so it parses under any sql_mode.
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
-- Mapping 7000002 -> quest 700100 (GUARDED) -- engine-owned
-- `rune_quest_unlock`. A rune referenced here is gated: hidden at the
-- engraver until the character completes the quest. No-op without the engine.
-- =====================================================================
SET @unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                    WHERE table_schema = DATABASE() AND table_name = 'rune_quest_unlock');

SET @sql := IF(@unlock_tbl > 0,
'INSERT INTO `rune_quest_unlock` (`rune_id`, `quest_id`)
 VALUES (7000002, 700100)
 ON DUPLICATE KEY UPDATE `quest_id` = VALUES(`quest_id`)',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
