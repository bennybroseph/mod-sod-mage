-- mod-sod-mage: the Living Bomb rune's only coupling to a STOCK spell.
--
-- The rune (7000008 -> driver 900006) grants the Living Spark stand-in until the Mage
-- learns the real WotLK Living Bomb. Once they do, this redirect makes a rune-holder's
-- Living Bomb fire a Scorch-tagged copy (per rank, 900009-900014) instead of the stock
-- effect, so the copy "benefits from all talents and effects that trigger from or modify
-- Scorch" -- the synergy stays gated to rune-holders without globally editing the stock spell.
--
-- The redirect SpellScript (spell_sod_mage_living_bomb_redirect, src/) is a no-op for
-- non-rune mages (the stock Living Bomb behaves normally). spell_script_names PK is
-- (spell_id, ScriptName), so REPLACE adds our binding alongside the core's
-- spell_mage_living_bomb without disturbing it. Idempotent and safe to re-run. Living
-- Bomb ranks: 44457 (R1) / 55359 (R2) / 55360 (R3).

REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (44457, 'spell_sod_mage_living_bomb_redirect'),
    (55359, 'spell_sod_mage_living_bomb_redirect'),
    (55360, 'spell_sod_mage_living_bomb_redirect');

-- =====================================================================
-- The Scorch-tagged copy keeps SpellIconID 3000 so it reads as a real Living Bomb. That
-- makes the core's Master of Elements treat it as a Living Bomb explosion and call
-- GetSpellWithRank(44457, GetSpellRank(copy)); without a rank chain GetSpellRank returns 0
-- and the lookup walks off the chain and crashes. So the copy mirrors the real spell's
-- THREE ranks: copy DoT 900009/900010/900011 (R1/R2/R3) and copy explosion 900012/900013/
-- 900014 (R1/R2/R3), index-matched to 44457/55359/55360 in spell_sod_mage.h. GetSpellRank
-- then returns 1-3 and GetSpellWithRank(44457, rank) resolves to the matching real DoT.
-- The copies are cast-only (never learned), so chain membership is harmless aside from this
-- rank mapping. Idempotent: clear only our chains, then insert. spell_ranks is a core table.
-- =====================================================================
DELETE FROM `spell_ranks` WHERE `first_spell_id` IN (900009, 900012);
INSERT INTO `spell_ranks` (`first_spell_id`, `spell_id`, `rank`) VALUES
    (900009, 900009, 1), (900009, 900010, 2), (900009, 900011, 3),
    (900012, 900012, 1), (900012, 900013, 2), (900012, 900014, 3);
