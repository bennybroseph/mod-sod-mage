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

// 900003 Arcane Blast (rune driver): the hidden passive the Hands rune teaches. A 1s
// periodic dummy that polls whether the Mage knows the real Arcane Blast and switches
// the rune's effect accordingly (mirrors Enlightenment's driver):
//   * no real Arcane Blast -> grant the capped stand-in Arcane Burst (400574), drop NV.
//   * knows real Arcane Blast -> remove Arcane Burst, apply Nether Vortex (900004).
// Arcane Burst is granted as a *temporary* spell (mirroring the rune engine's GrantSpell)
// so it never persists on its own; this driver re-grants it each login via HandleApply.
namespace
{
    bool KnowsRealArcaneBlast(Player* player)
    {
        for (uint32 rankId : SPELL_MAGE_ARCANE_BLAST_RANKS)
            if (player->HasSpell(rankId))
                return true;
        return false;
    }
}

class spell_sod_mage_arcane_blast_rune : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_arcane_blast_rune);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void Evaluate(Player* player)
    {
        if (!player)
            return;

        if (KnowsRealArcaneBlast(player))
        {
            // Real Arcane Blast known: drop the stand-in, augment with Nether Vortex.
            if (player->HasActiveSpell(SPELL_SOD_MAGE_ARCANE_BURST))
                player->removeSpell(SPELL_SOD_MAGE_ARCANE_BURST, SPEC_MASK_ALL, /*onlyTemporary*/ true);
            if (!player->HasAura(SPELL_SOD_MAGE_NETHER_VORTEX))
                player->CastSpell(player, SPELL_SOD_MAGE_NETHER_VORTEX, true);
        }
        else
        {
            // No real Arcane Blast yet: grant the capped stand-in, no Nether Vortex.
            player->RemoveAurasDueToSpell(SPELL_SOD_MAGE_NETHER_VORTEX);
            if (!player->HasActiveSpell(SPELL_SOD_MAGE_ARCANE_BURST))
                player->addSpell(SPELL_SOD_MAGE_ARCANE_BURST, SPEC_MASK_ALL,
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
        if (!player)
            return;

        player->RemoveAurasDueToSpell(SPELL_SOD_MAGE_NETHER_VORTEX);
        if (player->HasActiveSpell(SPELL_SOD_MAGE_ARCANE_BURST))
            player->removeSpell(SPELL_SOD_MAGE_ARCANE_BURST, SPEC_MASK_ALL, /*onlyTemporary*/ true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(
            spell_sod_mage_arcane_blast_rune::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_arcane_blast_rune::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_arcane_blast_rune::HandleRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_sod_mage_arcane_blast_rune()
{
    RegisterSpellScript(spell_sod_mage_arcane_blast_rune);
}
