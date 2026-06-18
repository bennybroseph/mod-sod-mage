/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MODULE_SOD_MAGE_RULES_H
#define MODULE_SOD_MAGE_RULES_H

#include "Define.h"
#include "Util.h" // CalculatePct

// Pure Temporal Beacon conversion math, factored out of the AuraScript so it can
// be unit-tested without a Unit / SpellInfo / DamageInfo. These mirror the steps
// in spell_sod_mage_temporal_conversion::HandleProc exactly (same CalculatePct
// calls, same order), so testing them tests the production behavior.
namespace SodMageRules
{
    // Healing routed to one (non-self) beacon target from `damage` Arcane damage:
    // `conversionPct` of the damage, then reduced to `multiTargetPct` when the
    // damage came from a multi-target (AoE) Arcane spell.
    inline int32 BeaconHeal(int32 damage, uint32 conversionPct,
                            bool multiTarget, uint32 multiTargetPct)
    {
        int32 heal = CalculatePct(damage, conversionPct);
        if (multiTarget)
            heal = CalculatePct(heal, multiTargetPct);
        return heal;
    }

    // The caster healing itself through its own beacon keeps only `selfPct` of the
    // per-target heal.
    inline int32 BeaconSelfHeal(int32 heal, uint32 selfPct)
    {
        return CalculatePct(heal, selfPct);
    }

    // SoD scales several rune spells off a per-level "spell power" curve (the
    // tooltip's $<power>), so they work with no gear; gear spell/healing power
    // then adds on top via spell_bonus_data. The curves below are lifted from
    // wago/wowhead's computed tooltips. Callers set the spell's base value (per
    // tick) from these; the gear coefficient is applied by the heal/damage bonus.

    // Living Flame (401558) per-second Spellfire damage. Tooltip:
    //   (13.828124 + 0.018012*L + 0.044141*L^2) * m1/100, m1 = 100 -> x1.0.
    inline int32 LivingFlameTickDamage(uint8 level)
    {
        float l = float(level);
        return int32(13.828124f + 0.018012f * l + 0.044141f * l * l);
    }

    // Regeneration (401417) / Mass Regeneration (412510) per-tick heal (3 ticks
    // over 3 sec). Tooltip total: $<power> * m1/100 * 3, m1 = 55 -> 0.55/tick of
    //   (38.258376 + 0.904195*L + 0.161311*L^2).
    inline int32 RegenTickHeal(uint8 level)
    {
        float l = float(level);
        float power = 38.258376f + 0.904195f * l + 0.161311f * l * l;
        return int32(power * 55.0f / 100.0f);
    }
}

#endif // MODULE_SOD_MAGE_RULES_H
