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

    // Arcane Surge (425124) base Arcane damage: Living Flame's quadratic curve
    // scaled by the SoD tooltip's [x2.26 .. x2.64] spread. The script rolls a value
    // in [min, max]; gear spell power then adds on top via spell_bonus_data (direct
    // 0.429), and the result is scaled by the caster's mana fraction (below).
    inline int32 ArcaneSurgeMinDamage(uint8 level)
    {
        float l = float(level);
        return int32((13.828124f + 0.018012f * l + 0.044141f * l * l) * 2.26f);
    }

    inline int32 ArcaneSurgeMaxDamage(uint8 level)
    {
        float l = float(level);
        return int32((13.828124f + 0.018012f * l + 0.044141f * l * l) * 2.64f);
    }

    // Arcane Burst (400574) base Arcane damage: the same quadratic curve scaled by the
    // SoD Arcane Blast tooltip's [x4.53 .. x5.27] spread. The script rolls a value in
    // [min, max] using a level CAPPED at the real Arcane Blast learn level (64), so the
    // stand-in never out-scales the real spell; gear spell power adds on top (0.714).
    inline int32 ArcaneBurstMinDamage(uint8 level)
    {
        float l = float(level);
        return int32((13.828124f + 0.018012f * l + 0.044141f * l * l) * 4.53f);
    }

    inline int32 ArcaneBurstMaxDamage(uint8 level)
    {
        float l = float(level);
        return int32((13.828124f + 0.018012f * l + 0.044141f * l * l) * 5.27f);
    }

    // Living Bomb (the rune) shares the same base curve. The SoD tooltip:
    //   DoT  per tick = c(L) * 85/100  (4 ticks over 12 sec)
    //   explosion     = c(L) * 171/100 (after 12 sec or on dispel, 10yd AoE)
    // where c(L) = 13.828124 + 0.018012*L + 0.044141*L^2. Set in script (works
    // with no gear); gear spell power adds on top via spell_bonus_data. Living
    // Spark (the stand-in) reuses the explosion at a level capped to the SoD level.
    inline int32 LivingBombDotTick(uint8 level)
    {
        float l = float(level);
        return int32((13.828124f + 0.018012f * l + 0.044141f * l * l) * 0.85f);
    }

    inline int32 LivingBombExplosion(uint8 level)
    {
        float l = float(level);
        return int32((13.828124f + 0.018012f * l + 0.044141f * l * l) * 1.71f);
    }

    // Arcane Surge's damage multiplier from remaining mana: 1.0 at empty, up to
    // 1 + maxBonusPct/100 at full (SoD: +300% at full mana). Guards max == 0.
    inline float ArcaneSurgeManaMultiplier(uint32 curMana, uint32 maxMana, uint32 maxBonusPct)
    {
        if (maxMana == 0)
            return 1.0f;
        float frac = float(curMana) / float(maxMana);
        if (frac > 1.0f)
            frac = 1.0f;
        return 1.0f + (float(maxBonusPct) / 100.0f) * frac;
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

    // Enlightenment's mana-gated state. SoD: "+X% damage while you have more than
    // highPct mana; below lowPct mana, part of your mana regen continues while
    // casting." The two states are mutually exclusive (highPct > lowPct), and the
    // inequalities are strict ("more than" / "below"), so the lowPct..highPct band
    // (inclusive) yields None. Resolved by spell_sod_mage_enlightenment each poll.
    enum class EnlightenmentState
    {
        None,
        HighMana,   // > highPct -> +spell damage sub-aura
        LowMana,    // < lowPct  -> regen-through-FSR sub-aura
    };

    inline EnlightenmentState EnlightenmentStateFor(int32 manaPct,
                                                    int32 highPct, int32 lowPct)
    {
        if (manaPct > highPct)
            return EnlightenmentState::HighMana;
        if (manaPct < lowPct)
            return EnlightenmentState::LowMana;
        return EnlightenmentState::None;
    }

    // Distance-scaled spawn chance for the disguised critters: `maxPct` within
    // `inner` of the focal point (the Tower of Azora), falling off linearly to
    // `minPct` at/beyond `outer`. Used by sod_mage_critter_seeder so apprentices
    // cluster near Azora's tower and thin out across the rest of Elwynn.
    inline uint32 SeedChanceForDistance(float dist, float inner, float outer,
                                        uint32 maxPct, uint32 minPct)
    {
        if (dist <= inner || outer <= inner)
            return maxPct;
        if (dist >= outer)
            return minPct;
        float t = (dist - inner) / (outer - inner);            // 0 (near) .. 1 (far)
        return uint32(float(maxPct) - t * (float(maxPct) - float(minPct)) + 0.5f);
    }
}

#endif // MODULE_SOD_MAGE_RULES_H
