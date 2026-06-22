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
--   tools/sod_spells.py) so the rune panel matches the learned spell.
--
-- Mapping for rune 7000003 (Living Flame -> spell 401556):
--   class_mask 128 = Mage
--   slot_mask  256 = Legs           (1 << RUNE_SLOT_LEGS   = 1 << 8)
--   icon must equal spell 401556's displayed icon (the `icon_living_flame`
--   texture in tools/sod_spells.py).
--
-- Mapping for rune 7000004 (Enlightenment -> spell 412324):
--   class_mask 128 = Mage
--   slot_mask   16 = Chest          (1 << RUNE_SLOT_CHEST  = 1 << 4)
--   412324 is the passive driver the rune teaches (SoD's "Gain the ability"
--   entry 415729 points at it); icon must equal its displayed icon (the
--   `icon_mindmastery` texture in tools/sod_spells.py).
--
-- Mapping for rune 7000005 (Arcane Surge -> spell 425124):
--   class_mask 128 = Mage
--   slot_mask  256 = Legs           (1 << RUNE_SLOT_LEGS   = 1 << 8) -- a second
--     Legs option alongside Living Flame (7000003); you engrave one. Unlocked by
--     Spell Notes: Arcane Surge (211386), sold by the supply officers at Friendly --
--     see sod_mage_arcane_surge_unlock.sql. icon must equal 425124's displayed icon
--     (`icon_arcane_surge` = spell_arcane_arcanetorrent in tools/sod_spells.py).
--
-- Mapping for rune 7000006 (Arcane Blast -> spell 900003, the hidden driver):
--   class_mask 128 = Mage
--   slot_mask   64 = Hands          (1 << RUNE_SLOT_HANDS  = 1 << 6)
--   The rune teaches the hidden driver 900003 (DO_NOT_DISPLAY), which grants the
--   capped stand-in Arcane Burst (400574) until the Mage trains the real Arcane Blast
--   at 64, then swaps to Nether Vortex (900004). spell_id is the DRIVER, not 400574.
--   icon `spell_arcane_blast` matches the granted ability's theme.
--
-- Mapping for rune 7000007 (Rewind Time -> spell 401462):
--   class_mask 128 = Mage
--   slot_mask   32 = Wrist          (1 << RUNE_SLOT_WRIST  = 1 << 5)
--   401462 is the active ability SoD's rune-grant 401734 teaches; icon must equal
--   its displayed icon (`spell_holy_borrowedtime`, the `icon_rewind` texture in
--   tools/sod_spells.py).

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
     ''mod-sod-mage'', 1),
    (7000004, 412324, 128, 16, ''Enlightenment'', ''spell_arcane_mindmastery'',
     ''Deal 10% more damage while above 70% mana; below 30% mana, 10% of your mana regeneration continues while casting.'',
     ''mod-sod-mage'', 1),
    (7000005, 425124, 128, 256, ''Arcane Surge'', ''spell_arcane_arcanetorrent'',
     ''Unleash all of your remaining mana to deal Arcane damage scaled by the mana spent, then sharply boost your mana regeneration for 8 sec.'',
     ''mod-sod-mage'', 1),
    (7000006, 900003, 128, 64, ''Arcane Blast'', ''spell_arcane_blast'',
     ''Grants Arcane Blast (as Arcane Burst) until you can train the real spell at level 64; afterward grants Nether Vortex, causing your Arcane Blast to apply Slow.'',
     ''mod-sod-mage'', 1),
    (7000007, 401462, 128, 32, ''Rewind Time'', ''spell_holy_borrowedtime'',
     ''Instantly heals an ally carrying your Temporal Beacon for all damage they took over the last 5 sec. Ineffective if the beacon was applied less than 5 sec ago.'',
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
