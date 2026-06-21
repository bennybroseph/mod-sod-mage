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

// 425124 Arcane Surge: an instant Arcane nuke. Base damage is Living Flame's level
// curve x [2.26 .. 2.64] (rolled here); gear spell power adds on top via
// spell_bonus_data; the total is then scaled by the caster's remaining mana, up to
// +300% at full mana. Finally ALL current mana is consumed. The 8s self-buff
// (+300% mana regen, regen through the Five Second Rule) is the spell's own
// self-targeted aura effects in the DBC, so only the damage scaling and the mana
// drain live here.
class spell_sod_mage_arcane_surge : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_arcane_surge);

    bool Load() override
    {
        return SodMageEnabled();
    }

    // Base damage before spell power (the engine folds gear SP in afterward).
    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        uint8 const level = caster->GetLevel();
        SetEffectValue(urand(SodMageRules::ArcaneSurgeMinDamage(level),
                             SodMageRules::ArcaneSurgeMaxDamage(level)));
    }

    // Scale the full (base + spell power) hit by remaining mana. Mana is still full
    // here -- the spell costs 0 and is drained in AfterCast.
    void HandleScale()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        uint32 const bonusPct =
            sConfigMgr->GetOption<uint32>("SodMage.ArcaneSurge.BonusManaPct", 300);
        float const mult = SodMageRules::ArcaneSurgeManaMultiplier(
            caster->GetPower(POWER_MANA), caster->GetMaxPower(POWER_MANA), bonusPct);

        SetHitDamage(int32(GetHitDamage() * mult));
    }

    // "Unleash all of your remaining mana": drain it once the cast resolves.
    void HandleConsumeMana()
    {
        if (Unit* caster = GetCaster())
            caster->SetPower(POWER_MANA, 0);
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(
            spell_sod_mage_arcane_surge::HandleDamage,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        OnHit += SpellHitFn(spell_sod_mage_arcane_surge::HandleScale);
        AfterCast += SpellCastFn(spell_sod_mage_arcane_surge::HandleConsumeMana);
    }
};

void AddSC_sod_mage_arcane_surge()
{
    RegisterSpellScript(spell_sod_mage_arcane_surge);
}
