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
#include "Player.h"

// 900006 Living Bomb (rune driver): the hidden passive the Hands rune teaches. A 1s
// periodic dummy that polls whether the Mage knows the real WotLK Living Bomb and
// switches the rune's mode accordingly (mirrors the Arcane Blast driver):
//   * no real Living Bomb -> grant the Living Spark stand-in (900007).
//   * knows real Living Bomb -> drop Living Spark; the cast-redirect bound to the real
//     Living Bomb (spell_sod_mage_living_bomb_redirect) then fires the Scorch-tagged copy.
// Living Spark is granted as a *temporary* spell (like the rune engine's grants) so it
// never persists on its own; this driver re-grants it each login via HandleApply.
namespace
{
    bool KnowsRealLivingBomb(Player* player)
    {
        for (uint32 rankId : SPELL_MAGE_LIVING_BOMB_RANKS)
            if (player->HasSpell(rankId))
                return true;
        return false;
    }
}

class spell_sod_mage_living_bomb_rune : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_living_bomb_rune);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void Evaluate(Player* player)
    {
        if (!player)
            return;

        if (KnowsRealLivingBomb(player))
        {
            // Real Living Bomb known: drop the stand-in. The cast-redirect augments the
            // real spell with the Scorch-tagged copy, so nothing to grant here.
            if (player->HasActiveSpell(SPELL_SOD_MAGE_LIVING_SPARK))
                player->removeSpell(SPELL_SOD_MAGE_LIVING_SPARK, SPEC_MASK_ALL, /*onlyTemporary*/ true);
        }
        else
        {
            // No real Living Bomb yet: grant the Living Spark stand-in.
            if (!player->HasActiveSpell(SPELL_SOD_MAGE_LIVING_SPARK))
                player->addSpell(SPELL_SOD_MAGE_LIVING_SPARK, SPEC_MASK_ALL,
                    /*updateActive*/ true, /*temporary*/ true);
        }
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            Evaluate(caster->ToPlayer());
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            Evaluate(caster->ToPlayer());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (player && player->HasActiveSpell(SPELL_SOD_MAGE_LIVING_SPARK))
            player->removeSpell(SPELL_SOD_MAGE_LIVING_SPARK, SPEC_MASK_ALL, /*onlyTemporary*/ true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(
            spell_sod_mage_living_bomb_rune::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_living_bomb_rune::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_living_bomb_rune::HandleRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_sod_mage_living_bomb_rune()
{
    RegisterSpellScript(spell_sod_mage_living_bomb_rune);
}
