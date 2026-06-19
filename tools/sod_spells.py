#!/usr/bin/env python3
"""mod-sod-mage spell spec — the declarative source for this module's custom
spells. Consumed by sod-client (build_patch.py) to build the consolidated client
patch and this module's server data/sql/db-world/base/sod_mage_spell_dbc.sql.

build_spells(idx) returns the spell list (idx is the runtime resolver the
builder supplies: icon-name/cast/duration/range -> DBC index/id). Shared WoW
enum constants come from sod_dbc; only Mage-specific ids/curves live here.
"""

from sod_dbc import *  # noqa: F401,F403  (shared WoW enum constants)

# Mage skill lines — control the spellbook tab the client files a spell under.
SKILL_ARCANE = 237
SKILL_FIRE = 8

# Custom SpellVisual: Living Flame's cast = Fire Blast's visual (143) with the
# on-target impact kit (field 3) dropped, so the caster plays the instant
# fire-cast gesture but no fire bursts on the enemy. Declared in SPELL_VISUALS
# (built by sod-client); referenced by 401556's SpellVisualID_1 override below.
SPELLVIS_FIRE_BLAST = 143
SPELLVIS_LIVING_FLAME_CAST = 700556
SPELLVIS_FIELD_IMPACT_KIT = 3


def living_flame_tick(level):
    return 13.828124 + 0.018012 * level + 0.044141 * level * level


def regen_tick(level):
    power = 38.258376 + 0.904195 * level + 0.161311 * level * level
    return power * 55.0 / 100.0


def tooltip_fit(curve, lo=60, hi=80):
    """Linear fit of `curve` exact at `lo` and `hi`. Returns client DBC fields:
    int EffectBasePoints_1 / BaseLevel / MaxLevel and float EffectRealPointsPerLevel_1."""
    base = round(curve(lo))
    rpl = (curve(hi) - curve(lo)) / (hi - lo)
    ints = {"EffectBasePoints_1": base, "BaseLevel": lo, "MaxLevel": hi}
    floats = {"EffectRealPointsPerLevel_1": rpl}
    return ints, floats


def build_spells(idx):
    """idx: resolver giving runtime DBC index/icon values."""
    cast_3s = idx["cast"][3000]
    cast_instant = idx["cast"][0]
    dur_3s = idx["dur"][3000]
    dur_30s = idx["dur"][30000]
    dur_perm = idx["dur"][-1]
    range_40 = idx["range"][40.0]
    range_35 = idx["range"][35.0]
    range_100 = idx["range"][100.0]
    range_self = idx["range"][0.0]
    # These texture names are each spell's displayed icon. If a spell is also a
    # rune, keep `rune_template.icon` in data/sql/db-world/base/sod_mage_*.sql
    # equal to the name here (Regeneration -> sod_mage_runes.sql; Mass
    # Regeneration -> sod_mage_mass_regeneration.sql) so the rune panel icon
    # matches the spell it unlocks.
    icon_beacon = idx["icon"]["spell_nature_timestop"]
    icon_regen = idx["icon"]["inv_enchant_essencemysticalsmall"]
    icon_regen_large = idx["icon"]["inv_enchant_essencemysticallarge"]
    icon_living_flame = idx["icon"]["spell_fire_masterofelements"]
    # Enlightenment shares one icon across the passive and both sub-buffs (and the
    # rune panel in sod_mage_runes.sql) — keep all four in sync.
    icon_mindmastery = idx["icon"]["spell_arcane_mindmastery"]

    # Client-only tooltip scaling: a linear fit of the SoD curve over the full
    # server level range 1..80 (exact at levels 1 and 80, scaling the whole way).
    # The actual values stay the exact quadratic curve (computed in script); this
    # only feeds the client tooltip, which can't render the quadratic — so it reads
    # high mid-range. `lo`/`hi` tune the fit anchors.
    regen_tip_int, regen_tip_flt = tooltip_fit(regen_tick, lo=1, hi=80)
    flame_tip_int, flame_tip_flt = tooltip_fit(living_flame_tick, lo=1, hi=80)

    return [
        {  # 401417 Regeneration: channeled 3s single-target HoT. A channel uses
           # an instant CastingTimeIndex; its length comes from the duration.
            "id": 401417, "client": True, "template": 139,  # clone Renew
            "skill_line": SKILL_ARCANE,  # spellbook tab: Arcane
            "name": "Regeneration", "script": "spell_sod_mage_regeneration",
            # Dynamic tooltip: $o1 = the periodic heal total, which the client
            # computes per the player's level from the client-only linear fit below
            # (the server applies the exact SoD curve in script). $d = 3s duration.
            "desc": "Heals the target for $o1 health over $d sec and applies "
                    "Temporal Beacon for 30 sec.",
            "client_overrides": dict(regen_tip_int),
            "client_overrides_float": dict(regen_tip_flt),
            # SoD: heals 165% of healing power over 3 ticks, no flat base. We model
            # that with a 0.55/tick spellpower coefficient (spell_bonus_data) and a
            # zero base. SpellLevel 0 avoids AzerothCore's low-level coefficient penalty.
            "bonus": {"direct": 0.0, "dot": 0.55, "ap": 0.0, "ap_dot": 0.0},
            "overrides": {
                "Attributes": 0, "AttributesEx": SPELL_ATTR1_IS_CHANNELED,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_3s,
                "InterruptFlags": 15, "ChannelInterruptFlags": 31756,
                "SpellVisualID_1": 12657,  # Drain Mana blue caster->target beam
                "RangeIndex": range_40, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 28, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_regen, "EquippedItemClass": -1, "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_HEAL,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 0,  # pure SP coeff
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 412510 Mass Regeneration: AoE party channeled HoT (sibling of
           # Regeneration). Targets the chosen ally's party in radius
           # (A = ally, B = the target's party area), like Prayer of Healing.
            "id": 412510, "client": True, "template": 139,  # clone Renew
            "skill_line": SKILL_ARCANE,  # spellbook tab: Arcane
            "name": "Mass Regeneration", "script": "spell_sod_mage_mass_regeneration",
            "desc": "Heals the target and their party members for $o1 health over "
                    "$d sec and applies Temporal Beacon to each for 15 sec.",
            "client_overrides": dict(regen_tip_int),
            "client_overrides_float": dict(regen_tip_flt),
            "bonus": {"direct": 0.0, "dot": 0.55, "ap": 0.0, "ap_dot": 0.0},
            "overrides": {
                "Attributes": 0, "AttributesEx": SPELL_ATTR1_IS_CHANNELED,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_3s,
                "InterruptFlags": 15, "ChannelInterruptFlags": 31756,
                "SpellVisualID_1": 12657,  # Drain Mana beam (Regeneration family)
                "RangeIndex": range_40, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 45, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_regen_large, "EquippedItemClass": -1, "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_HEAL,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 0,
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "ImplicitTargetB_1": TARGET_UNIT_LASTTARGET_AREA_PARTY,
                "EffectRadiusIndex_1": 10,  # party spread radius (as Prayer of Healing)
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 400735 Temporal Beacon: 30s buff + ~8/sec passive HoT.
            "id": 400735, "client": True, "template": 774,  # clone Rejuvenation
            "name": "Temporal Beacon", "script": "spell_sod_mage_temporal_beacon",
            "desc": "Records the subject's position in time and space. Damage done "
                    "by caster will result in this subject receiving chronomantic healing.",
            "aura_desc": "Records the subject's position in time and space. Damage done "
                         "by caster will result in this subject receiving chronomantic healing.",
            "overrides": {
                "Attributes": 0, "AttributesEx": 0, "DispelType": DISPEL_MAGIC,
                "SpellVisualID_1": 37,  # Lightning Shield orbiting visual + sounds
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_30s,
                "RangeIndex": range_100, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_beacon, "EquippedItemClass": -1,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_HEAL,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 7,  # 8/sec
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 401405 Chronomantic Healing: instant triggered heal (amount at cast).
            "id": 401405, "client": True, "template": 2050,  # clone Lesser Heal
            "name": "Chronomantic Healing",
            "overrides": {
                "Attributes": 0, "AttributesEx": 0,
                "CastingTimeIndex": cast_instant, "DurationIndex": 0,
                "RangeIndex": range_100, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_beacon, "EquippedItemClass": -1,
                "Effect_1": EFFECT_HEAL, "EffectAura_1": 0,
                "EffectBasePoints_1": 0,
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 900001 Temporal Beacon (conversion): server-only hidden proc aura.
            "id": 900001, "client": False, "template": None,
            "name": "Temporal Beacon",
            "script": "spell_sod_mage_temporal_conversion",
            "overrides": {
                "Attributes": SPELL_ATTR0_PASSIVE | SPELL_ATTR0_DO_NOT_DISPLAY,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_perm,
                "RangeIndex": range_self, "SchoolMask": SCHOOL_MASK_ARCANE,
                "EquippedItemClass": -1,
                "ProcTypeMask": PROC_DONE_MAGIC_NEG | PROC_DONE_NONE_NEG | PROC_DONE_RANGED_AUTO,
                "ProcChance": 100,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_DUMMY,
                "ImplicitTargetA_1": TARGET_UNIT_CASTER,
                "EffectBasePoints_1": 0,
            },
            # spell_proc row is REQUIRED: an aura with no spell_proc entry never
            # procs (Aura::GetProcEffectMask returns 0). AttributesMask=2
            # (TRIGGERED_CAN_PROC) lets Arcane Missiles' triggered ticks (and the
            # triggered wand shot) feed the beacon. Chance/ProcFlags fall back to
            # the spell_dbc values when left 0.
            #
            # SchoolMask is left 0 (no engine-level school filter) on purpose: the
            # proc engine's school check uses ProcEventInfo::GetSchoolMask(), which
            # returns the *spell's* base school when a proc spell is present — and a
            # wand's "Shoot" (5019) is base-school Physical even when the wand deals
            # Arcane. A SchoolMask=Arcane here would reject the arcane wand. Instead
            # CheckProc filters on the *damage's* school (the real Arcane), which is
            # correct for both spells and wands.
            "proc": {
                "SchoolMask": 0, "SpellFamilyName": 0,
                "SpellFamilyMask0": 0, "SpellFamilyMask1": 0,
                "SpellFamilyMask2": 0,
                "ProcFlags": PROC_DONE_MAGIC_NEG | PROC_DONE_NONE_NEG | PROC_DONE_RANGED_AUTO,
                "SpellTypeMask": 1,    # PROC_SPELL_TYPE_DAMAGE
                "SpellPhaseMask": 2,   # PROC_SPELL_PHASE_HIT
                "HitMask": 0, "AttributesMask": 2, "DisableEffectsMask": 0,
                "ProcsPerMinute": 0, "Chance": 0, "Cooldown": 0, "Charges": 0,
            },
        },
        {  # 401556 Living Flame: summons an invisible creeping flame (a script
           # creature, npc_sod_mage_living_flame) that homes on the target. Each
           # second the creature makes the MAGE recast 401558 at its position, so
           # the Spellfire (Fire|Arcane) AoE is the caster's — feeding Chronomantic
           # Healing and any Fire/Arcane proc. SoD drives the flame with an
           # AreaTrigger (no 3.3.5a analogue), hence the chasing-creature adaptation.
           # Cloned from Fire Blast (2136) for an instant enemy-target fire cast.
           # SoD cooldown 30s (wago RecoveryTime) + 1.5s GCD set explicitly so the
           # server row (built from overrides only) carries them.
            "id": 401556, "client": True, "template": 2136,  # clone Fire Blast
            "skill_line": SKILL_FIRE,  # spellbook tab: Fire
            "name": "Living Flame", "script": "spell_sod_mage_living_flame",
            # $401558s1 cross-references the trail spell's per-second damage, which
            # the client computes per the player's level from 401558's client-only
            # linear fit below (server applies the exact SoD curve in script).
            "desc": "Summons a spellfire flame that moves toward the target, "
                    "leaving a trail of spellfire. This trail deals $401558s1 "
                    "Spellfire damage every second to nearby enemies. Lasts 10 sec.",
            "overrides": {
                # NO_THREAT so the cast itself doesn't engage; combat begins only
                # when the trail (401558) deals damage. SpellVisualID points at our
                # custom cast visual (Fire Blast's caster gesture with the on-target
                # impact removed) so the Mage animates the cast but no fire renders
                # on the target — the only fire is the ground patch the mover leaves.
                "Attributes": 0, "AttributesEx": SPELL_ATTR1_NO_THREAT,
                "SpellVisualID_1": SPELLVIS_LIVING_FLAME_CAST,
                "CastingTimeIndex": cast_instant, "DurationIndex": 0,
                "RecoveryTime": 30000, "CategoryRecoveryTime": 0,
                "StartRecoveryCategory": 133, "StartRecoveryTime": 1500,
                "RangeIndex": range_35, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 11, "SchoolMask": SCHOOL_MASK_SPELLFIRE,
                "SpellIconID": icon_living_flame, "EquippedItemClass": -1,
                "SpellLevel": 0,
                "Effect_1": EFFECT_DUMMY, "EffectAura_1": 0,
                "EffectBasePoints_1": 0,
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ENEMY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 401558 Living Flame (damage): the per-second Spellfire AoE the Mage
           # recasts at the creeping creature's position. Cloned from Flamestrike
           # (2120) so it inherits a ground-fire visual — the repeated patches read
           # as the flame's trail. SchoolMask 68 (Fire|Arcane); DefenseType 1
           # (magic) so it lands as magic spell damage and feeds the beacon proc.
           # Per-tick damage is set in script from a level curve (works without
           # spellpower); the spell_script binds spell_sod_mage_living_flame_damage.
           # gear spellpower adds on top via spell_bonus_data direct 1.0 (matches
           # the SoD tooltip's x m1/100 = x1.0). EffectBasePoints 0 (script supplies).
            "id": 401558, "client": True, "template": 2120,  # clone Flamestrike
            "name": "Living Flame",
            "script": "spell_sod_mage_living_flame_damage",
            "bonus": {"direct": 1.0, "dot": 0.0, "ap": 0.0, "ap_dot": 0.0},
            # Client-only level fit so $401558s1 (referenced by 401556's tooltip)
            # scales with the player's level; server damage is the script's curve.
            "client_overrides": dict(flame_tip_int),
            "client_overrides_float": dict(flame_tip_flt),
            "overrides": {
                "Attributes": 0, "AttributesEx": 0,
                "CastingTimeIndex": cast_instant, "DurationIndex": 0,
                "RangeIndex": range_100, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_SPELLFIRE,
                "DefenseType": 1,  # SPELL_DAMAGE_CLASS_MAGIC
                "SpellIconID": icon_living_flame, "EquippedItemClass": -1,
                "SpellLevel": 0,
                "Effect_1": EFFECT_SCHOOL_DAMAGE, "EffectAura_1": 0,
                "EffectBasePoints_1": 0,  # set in script from a level curve
                "ImplicitTargetA_1": TARGET_UNIT_DEST_AREA_ENEMY,
                "EffectRadiusIndex_1": 15,  # 3 yd (SpellRadius index 15) -- narrow
                                            # trail; SoD's "nearby enemies" AoE.
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 412324 Enlightenment: the passive the Chest rune teaches. SoD's
           # "Gain the Enlightenment ability" entry (415729) points at this id, so
           # we teach 412324 directly and never ship a 415729 row. Its lone effect
           # is a 1s periodic dummy that spell_sod_mage_enlightenment polls to toggle
           # the two sub-auras below: above 70% mana -> 412326 (+10% spell damage),
           # below 30% -> 412325 (regen through the Five Second Rule). Values are
           # static (no level scaling), so the tooltip text is literal. Cloned from
           # Rejuvenation for a valid periodic self-aura record.
            "id": 412324, "client": True, "template": 774,  # clone Rejuvenation
            "skill_line": SKILL_ARCANE,  # spellbook tab: Arcane
            "name": "Enlightenment", "script": "spell_sod_mage_enlightenment",
            "desc": "You deal 10% more damage while you have more than 70% mana. "
                    "While below 30% mana, 10% of your mana regeneration "
                    "continues while casting.",
            "aura_desc": "You deal 10% more damage while you have more than 70% "
                         "mana. While below 30% mana, 10% of your mana "
                         "regeneration continues while casting.",
            "overrides": {
                "Attributes": SPELL_ATTR0_PASSIVE, "AttributesEx": 0,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_perm,
                "RangeIndex": range_self, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_mindmastery, "EquippedItemClass": -1,
                "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_DUMMY,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 0,  # script-driven
                "ImplicitTargetA_1": TARGET_UNIT_CASTER,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 412326 Enlightenment (high-mana sub-buff): +10% spell damage. Pure DBC,
           # no script — the driver (412324) applies/removes it. MiscValue = magic
           # school mask (all six schools). Permanent duration; the driver strips it
           # when leaving the high state. Base points are the LITERAL percent (10),
           # not value-1: the server spell_dbc row's EffectDieSides defaults to 0, so
           # the engine adds nothing (SpellInfo::CalcValue) — we pin DieSides 0 so the
           # client agrees and the percent is exact (the usual -1 convention only
           # matches a cloned template's DieSides=1 on the client tooltip).
            "id": 412326, "client": True, "template": 774,  # clone Rejuvenation
            "name": "Enlightenment",
            "aura_desc": "Increases all spell damage done by 10%.",
            "overrides": {
                "Attributes": 0, "AttributesEx": 0,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_perm,
                "RangeIndex": range_self, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_mindmastery, "EquippedItemClass": -1,
                "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA,
                "EffectAura_1": AURA_MOD_DAMAGE_PERCENT_DONE,
                "EffectAuraPeriod_1": 0, "EffectBasePoints_1": 10,  # +10% (exact)
                "EffectDieSides_1": 0,
                "EffectMiscValue_1": SCHOOL_MASK_MAGIC,
                "ImplicitTargetA_1": TARGET_UNIT_CASTER,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 412325 Enlightenment (low-mana sub-buff): MOD_MANA_REGEN_INTERRUPT, so
           # 10% of spirit-based mana regen keeps flowing during the Five Second Rule
           # (the same aura retail Meditation uses; the engine recomputes regen on
           # apply/remove, no custom code). Pure DBC; driver-applied. Permanent; the
           # driver strips it when leaving the low state. Base points are the LITERAL
           # percent (10) with DieSides pinned 0 for an exact value (see 412326).
            "id": 412325, "client": True, "template": 774,  # clone Rejuvenation
            "name": "Enlightenment",
            "aura_desc": "Allows 10% of your Mana regeneration to continue "
                         "while casting.",
            "overrides": {
                "Attributes": 0, "AttributesEx": 0,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_perm,
                "RangeIndex": range_self, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_mindmastery, "EquippedItemClass": -1,
                "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA,
                "EffectAura_1": AURA_MOD_MANA_REGEN_INTERRUPT,
                "EffectAuraPeriod_1": 0, "EffectBasePoints_1": 10,  # 10% (exact)
                "EffectDieSides_1": 0,
                "ImplicitTargetA_1": TARGET_UNIT_CASTER,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
    ]

# Custom SpellVisual rows for sod-client to add (clone_from, zero listed fields).
SPELL_VISUALS = [
    {"id": SPELLVIS_LIVING_FLAME_CAST, "clone_from": SPELLVIS_FIRE_BLAST,
     "zero_fields": [SPELLVIS_FIELD_IMPACT_KIT]},
]

# IDs we used to ship but no longer do — cleaned from every table we own so a
# re-apply of the base SQL removes the orphan server rows.
RETIRED_IDS = [900002]  # old conversion-aura id; conversion moved to 900001
