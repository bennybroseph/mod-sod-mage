-- mod-sod-mage: the Living Flame mover creature (entry 700200).
--
-- Living Flame (spell 401556) summons this creature, which homes on the target
-- and once per second has its summoner (the Mage) cast the Spellfire AoE 401558
-- at its position (see src/spell_sod_mage_living_flame.cpp). The creature deals
-- no damage itself and is never attackable: it is an invisible, immune trigger
-- whose only jobs are to move and to be a moving cast origin.
--
-- IDs: mod-sod-mage owns the creature band 700200-700299 (the 700100 demo NPC
-- was retired in sod_mage_mass_regeneration.sql). display 17522 = the Vazruden
-- Fire Trap ground flame (Hellfire Ramparts) -- a self-contained, reliably
-- rendering ground-fire patch, so the creeping mover reads as fire crawling along
-- the ground (not a walking elemental; Al'ar's 16946 is emitter-only and renders
-- nothing on a plain creature). The C++ AI is bound by `ScriptName`. Idempotent.
--
-- Presentation: DisplayScale 0.25 (small creeping patch) and HoverHeight -1.0 to
-- sink the model down onto the ground (it renders raised at its native anchor).

DELETE FROM `creature_template_model` WHERE `CreatureID` = 700200;
DELETE FROM `creature_template`       WHERE `entry` = 700200;

INSERT INTO `creature_template`
    (`entry`, `name`, `subname`,
     `minlevel`, `maxlevel`, `faction`, `npcflag`,
     `speed_walk`, `speed_run`, `rank`,
     `unit_class`, `unit_flags`, `unit_flags2`, `type`, `lootid`,
     `AIName`, `MovementType`, `HoverHeight`,
     `HealthModifier`, `ManaModifier`, `ArmorModifier`, `RegenHealth`,
     `flags_extra`, `ScriptName`)
VALUES
    (700200, 'Living Flame', '',
     1, 1, 35, 0,
     1.0, 1.0, 0,
     -- unit_flags: NON_ATTACKABLE|IMMUNE_TO_PC|IMMUNE_TO_NPC|NOT_SELECTABLE
     1, 33555202, 0, 4, 0,
     -- HoverHeight -1.0: sink the flame onto the ground (default 1 floats it up).
     '', 0, -1.0,
     1, 1, 1, 1,
     -- flags_extra 0: a visible mover, not a hidden trigger.
     0, 'npc_sod_mage_living_flame');

INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES
    -- DisplayScale 0.25: small creeping patch (17522 is large at native size).
    (700200, 0, 17522, 0.25, 1.0);
