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
#include "SpellInfo.h"
#include "Unit.h"
#include <algorithm>

// 900004 Nether Vortex: the hidden passive the rune driver (900003) applies once the
// Mage knows the real Arcane Blast. When their Arcane Blast deals damage, it applies
// Slow (31589) to the struck target if that target isn't already slowed. A Cataclysm
// talent ported to keep the Hands rune relevant after the stand-in (Arcane Burst) is
// retired. Mirrors the Temporal Beacon conversion proc.
class spell_sod_mage_nether_vortex : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_nether_vortex);

    bool Load() override
    {
        return SodMageEnabled();
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* spell = eventInfo.GetSpellInfo();
        if (!spell)
            return false;

        // Only the real Arcane Blast (any rank) triggers Nether Vortex.
        return std::find(std::begin(SPELL_MAGE_ARCANE_BLAST_RANKS),
                         std::end(SPELL_MAGE_ARCANE_BLAST_RANKS),
                         spell->Id) != std::end(SPELL_MAGE_ARCANE_BLAST_RANKS);
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget(); // the aura sits on the Mage
        Unit* target = eventInfo.GetActionTarget();
        if (!caster || !target || !target->IsAlive())
            return;

        // Leave an existing Slow alone; only apply when the target isn't slowed.
        if (target->HasAura(SPELL_MAGE_SLOW))
            return;

        caster->CastSpell(target, SPELL_MAGE_SLOW, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_sod_mage_nether_vortex::CheckProc);
        OnEffectProc += AuraEffectProcFn(
            spell_sod_mage_nether_vortex::HandleProc,
            EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_sod_mage_nether_vortex()
{
    RegisterSpellScript(spell_sod_mage_nether_vortex);
}
