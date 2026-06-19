# TEMPLATE — paste this dict into the SPELLS list in tools/build_sod_mage_patch.py,
# fill the placeholders, then regenerate. Not imported or run from templates/.
# Model: the SPELLS entries in tools/build_sod_mage_patch.py.
#
# `overrides` keys are literal spell_dbc column names (CastingTimeIndex, EffectAura_1,
# SpellVisualID_1, ...). Index columns (cast/dur/range/radius/icon) come from the
# runtime resolver `idx[...]`, so they're correct for whatever client you build against.

{
    "id": <SPELL_ID>,            # real SoD id if one exists; else 900000-900099
    "client": True,             # True -> client Spell.dbc/MPQ; False -> SQL only
    "template": <TEMPLATE_ID>,  # a real spell to clone for a valid baseline shape
    "skill_line": SKILL_ARCANE, # spellbook tab (SKILL_ARCANE/FIRE/FROST)
    "name": "<Spell Name>",
    "script": "spell_sod_mage_<name>",  # MUST match the C++ class -> emits
                                        # spell_script_names; omit if no script
    # Tooltip. Tokens: $o1 total periodic, $s1 per-tick, $d duration,
    # $<otherSpellId>s1 another spell's effect. The client computes these.
    "desc": "<Tooltip text with $tokens.>",

    # Optional: server-side spellpower scaling (per-tick/direct coefficients).
    # Drop this key if the spell has no SP scaling.
    "bonus": {"direct": 0.0, "dot": 0.0, "ap": 0.0, "ap_dot": 0.0},

    "overrides": {
        # --- timing / cost / school ---
        "Attributes": 0, "AttributesEx": 0,
        "CastingTimeIndex": cast_instant,   # idx["cast"][ms]; channels use instant
        "DurationIndex": <DURATION_IDX>,    # idx["dur"][ms] (-1 = permanent)
        "RangeIndex": <RANGE_IDX>,          # idx["range"][yards]
        "PowerType": 0, "ManaCost": 0, "ManaCostPct": <PCT>,
        "SchoolMask": SCHOOL_MASK_ARCANE,
        # --- visuals (client-only; reuse existing) ---
        "SpellVisualID_1": <VISUAL_ID>,
        "SpellIconID": <ICON>,              # idx["icon"]["<texture_name>"]
        "EquippedItemClass": -1,            # don't require an equipped item
        # --- effect 1 (clear effects 2/3 unless you use them) ---
        "Effect_1": <EFFECT>, "EffectAura_1": <AURA>,
        "EffectBasePoints_1": 0,            # remember: stored value - 1
        "ImplicitTargetA_1": <TARGET>,
        "EffectRadiusIndex_1": 0,
        "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
        "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
    },

    # Optional: required for a proc aura, ignored otherwise.
    # "proc": {"ProcFlags": <FLAGS>, "Chance": <PCT>, "AttributesMask": 0},
},
