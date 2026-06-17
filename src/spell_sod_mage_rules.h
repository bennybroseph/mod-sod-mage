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
}

#endif // MODULE_SOD_MAGE_RULES_H
