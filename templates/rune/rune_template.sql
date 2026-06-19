-- TEMPLATE — append to data/sql/db-world/base/sod_mage_runes.sql (or a new base
-- file), fill the placeholders. Not applied from templates/. Idempotent.
-- Model: data/sql/db-world/base/sod_mage_runes.sql.
--
-- GUARDED: rune_template only exists when mod-rune-engraving is installed. A plain
-- INSERT would error on the missing table, so we build it as dynamic SQL and only
-- PREPARE it when the table is present; otherwise it runs `DO 0` (a clean no-op).

SET @rune_tbl := (SELECT COUNT(*) FROM information_schema.tables
                  WHERE table_schema = DATABASE() AND table_name = 'rune_template');

-- The INSERT is a single-quoted multi-line literal (escape inner quotes as '')
-- so it parses under any sql_mode, including NO_BACKSLASH_ESCAPES.
SET @sql := IF(@rune_tbl > 0,
'INSERT INTO `rune_template`
    (`rune_id`, `spell_id`, `class_mask`, `slot_mask`, `name`, `icon`, `description`, `source`, `enabled`)
 VALUES
    (<RUNE_ID>, <SPELL_ID>, 128, <SLOT_MASK>, ''<Rune Name>'', ''<icon_texture_name>'',
     ''<Short description shown in the rune panel.>'', ''mod-sod-mage'', 1)
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

-- <RUNE_ID>   : pick from the mod-sod-mage band 7000000-7000999.
-- <SPELL_ID>  : the spell the rune grants (already in spell_dbc).
-- <SLOT_MASK> : 1 << RuneSlot. Chest = 16, Legs = 256, Hands = 64, ...
-- class_mask 128 = Mage. icon must match the spell's displayed icon texture.
