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

#include "spell_sod_mage.h"
#include "spell_sod_mage_rules.h"
#include "Random.h"
#include "Unit.h"
#include <algorithm>

// 400574 Arcane Burst: the rune Arcane Blast stand-in. A 2.5s Arcane nuke whose base
// damage is the SoD curve x[4.53..5.27] (rolled here); gear spell power adds on top via
// spell_bonus_data (direct 0.714). The level fed to the curve is CAPPED at the real
// Arcane Blast learn level (SodMage.ArcaneBlast.ScalingCapLevel, default 64), so the
// stand-in never out-scales the real spell. The rune driver (900003) removes it once the
// player trains the real Arcane Blast.
class spell_sod_mage_arcane_burst : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_arcane_burst);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        uint8 const level = std::min<uint8>(caster->GetLevel(),
            static_cast<uint8>(SodMageArcaneBlastCapLevel()));
        SetEffectValue(urand(SodMageRules::ArcaneBurstMinDamage(level),
                             SodMageRules::ArcaneBurstMaxDamage(level)));
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(
            spell_sod_mage_arcane_burst::HandleDamage,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddSC_sod_mage_arcane_burst()
{
    RegisterSpellScript(spell_sod_mage_arcane_burst);
}
