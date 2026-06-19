-- TEMPLATE — append to the rune's unlock SQL (e.g. a new
-- data/sql/db-world/base/sod_mage_<name>_unlock.sql), fill the placeholders. Not
-- applied from templates/. Idempotent.
-- Model: the rune_item_unlock block in
-- data/sql/db-world/base/sod_mage_regeneration_unlock.sql.
--
-- Maps <ITEM_ID> -> <RUNE_ID>: using the item discovers (unlocks) the rune. This is
-- what flips the rune to "gated" -- it's hidden at the engraver until unlocked. The
-- item must carry item_template.ScriptName = 'item_rune_unlock' (the engine's generic
-- script). GUARDED: no-op without mod-rune-engraving.

SET @item_unlock_tbl := (SELECT COUNT(*) FROM information_schema.tables
                         WHERE table_schema = DATABASE() AND table_name = 'rune_item_unlock');

SET @sql := IF(@item_unlock_tbl > 0,
'INSERT INTO `rune_item_unlock` (`item_id`, `rune_id`)
 VALUES (<ITEM_ID>, <RUNE_ID>)
 ON DUPLICATE KEY UPDATE `rune_id` = VALUES(`rune_id`)',
'DO 0');

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- <ITEM_ID> : the item whose use unlocks the rune (its bag icon, loot, and
--             ScriptName = 'item_rune_unlock' are defined in the item SQL).
-- <RUNE_ID> : the rune to unlock (the rune_template row above).
