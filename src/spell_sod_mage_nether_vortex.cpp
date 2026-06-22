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
#include "ObjectAccessor.h"
#include "SpellInfo.h"
#include "Unit.h"
#include <algorithm>
#include <unordered_map>

// 900004 Nether Vortex: the hidden passive the rune driver (900003) applies once the
// Mage knows the real Arcane Blast. Each Arcane Blast hit (re)applies Slow (31589) to
// the struck target, but only ONE target stays slowed at a time -- hitting a new target
// moves the Slow off the old one (per the rune tooltip: "...if no target is currently
// affected by Slow"). A Cataclysm talent ported to keep the Hands rune relevant after
// the stand-in (Arcane Burst) is retired. Mirrors the Temporal Beacon conversion proc.
namespace
{
    // The single target each Mage currently has Slowed via Nether Vortex (caster GUID
    // -> Slow target GUID). Touched only from the Mage's own map update (the proc and
    // the aura-remove hook), matching the unlocked-global-map convention elsewhere in
    // this module (single-threaded map update, MapUpdate.Threads = 1).
    std::unordered_map<ObjectGuid, ObjectGuid> sNetherVortexSlowTarget;
}

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

        // Move our Slow off whatever target previously held it (if it's a different
        // unit), so only one target is ever slowed -- then (re)apply to the struck
        // target, refreshing it on a repeat hit like a re-cast.
        auto it = sNetherVortexSlowTarget.find(caster->GetGUID());
        if (it != sNetherVortexSlowTarget.end() && it->second != target->GetGUID())
            if (Unit* prev = ObjectAccessor::GetUnit(*caster, it->second))
                prev->RemoveAura(SPELL_MAGE_SLOW, caster->GetGUID());

        caster->CastSpell(target, SPELL_MAGE_SLOW, true);
        sNetherVortexSlowTarget[caster->GetGUID()] = target->GetGUID();
    }

    // Forget the tracked target when the rune is un-engraved (aura removed).
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetTarget())
            sNetherVortexSlowTarget.erase(caster->GetGUID());
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_sod_mage_nether_vortex::CheckProc);
        OnEffectProc += AuraEffectProcFn(
            spell_sod_mage_nether_vortex::HandleProc,
            EFFECT_0, SPELL_AURA_DUMMY);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_nether_vortex::HandleRemove,
            EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_sod_mage_nether_vortex()
{
    RegisterSpellScript(spell_sod_mage_nether_vortex);
}
