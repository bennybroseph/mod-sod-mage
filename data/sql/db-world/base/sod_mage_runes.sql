-- mod-sod-mage: rune catalog entries for the optional mod-rune-engraving engine.
--
-- These rows make this module's spells acquirable as engravable runes WHEN the
-- engine is installed. They are GUARDED: if `rune_template` does not exist (the
-- engine isn't installed), the statement is a harmless no-op and the spells
-- remain available via GM `.learn` only. Safe and idempotent to re-run.
--
-- A plain `INSERT INTO rune_template ...` cannot be guarded with a WHERE clause:
-- MySQL errors on the missing target table before the WHERE runs. So we build
-- the INSERT as dynamic SQL and only PREPARE it when the table is present.
--
-- mod-sod-mage owns the rune_id band 7000000-7000999.
--
-- Mapping for rune 7000001 (Regeneration -> spell 401417):
--   class_mask 128 = Mage           (1 << (CLASS_MAGE - 1) = 1 << 7)
--   slot_mask   16 = Chest          (1 << RUNE_SLOT_CHEST  = 1 << 4)
--   icon must equal spell 401417's displayed icon (the `icon_regen` texture in
--   tools/build_sod_mage_patch.py) so the rune panel matches the learned spell.
--
-- Mapping for rune 7000003 (Living Flame -> spell 401556):
--   class_mask 128 = Mage
--   slot_mask  256 = Legs           (1 << RUNE_SLOT_LEGS   = 1 << 8)
--   icon must equal spell 401556's displayed icon (the `icon_living_flame`
--   texture in tools/build_sod_mage_patch.py).

SET @rune_tbl := (SELECT COUNT(*) FROM information_schema.tables
                  WHERE table_schema = DATABASE() AND table_name = 'rune_template');

-- The INSERT lives inside a plain multi-line string literal (real newlines, no
-- backslash continuations) so it parses identically under any sql_mode,
-- including NO_BACKSLASH_ESCAPES.
SET @sql := IF(@rune_tbl > 0,
'INSERT INTO `rune_template`
    (`rune_id`, `spell_id`, `class_mask`, `slot_mask`, `name`, `icon`, `description`, `source`, `enabled`)
 VALUES
    (7000001, 401417, 128, 16, ''Regeneration'', ''inv_enchant_essencemysticalsmall'',
     ''A channeled heal-over-time that applies Temporal Beacon to the target.'',
     ''mod-sod-mage'', 1),
    (7000003, 401556, 128, 256, ''Living Flame'', ''spell_fire_masterofelements'',
     ''Summons a spellfire flame that creeps toward the target, dealing Fire and Arcane damage to nearby enemies.'',
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
