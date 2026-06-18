-- mod-sod-mage: the Living Flame mover creature (entry 700200).
--
-- Living Flame (spell 401556) summons this creature, which homes on the target
-- and once per second has its summoner (the Mage) cast the Spellfire AoE 401558
-- at its position (see src/spell_sod_mage_living_flame.cpp). The creature deals
-- no damage itself and is never attackable: it is an invisible, immune trigger
-- whose only jobs are to move and to be a moving cast origin.
--
-- IDs: mod-sod-mage owns the creature band 700200-700299 (the 700100 demo NPC
-- was retired in sod_mage_mass_regeneration.sql). display 1405 = Fire Elemental
-- (a visible walking flame -- reads as the creeping "Living Flame"; a core model
-- that renders reliably). The C++ AI is bound by `ScriptName`. Idempotent.

DELETE FROM `creature_template_model` WHERE `CreatureID` = 700200;
DELETE FROM `creature_template`       WHERE `entry` = 700200;

INSERT INTO `creature_template`
    (`entry`, `name`, `subname`,
     `minlevel`, `maxlevel`, `faction`, `npcflag`,
     `speed_walk`, `speed_run`, `rank`,
     `unit_class`, `unit_flags`, `unit_flags2`, `type`, `lootid`,
     `AIName`, `MovementType`,
     `HealthModifier`, `ManaModifier`, `ArmorModifier`, `RegenHealth`,
     `flags_extra`, `ScriptName`)
VALUES
    (700200, 'Living Flame', '',
     1, 1, 35, 0,
     1.0, 1.0, 0,
     -- unit_flags: NON_ATTACKABLE|IMMUNE_TO_PC|IMMUNE_TO_NPC|NOT_SELECTABLE
     1, 33555202, 0, 4, 0,
     '', 0,
     1, 1, 1, 1,
     -- flags_extra 0: a visible mover, not a hidden trigger.
     0, 'npc_sod_mage_living_flame');

INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES
    (700200, 0, 1405, 1.0, 1.0);
