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
#include <unordered_map>

// Maps a Mage caster to the set of units currently carrying their Temporal
// Beacon. Kept in sync by the beacon AuraScript (apply/remove). Caster and
// beacon targets are always within 100yd, i.e. on the same map, so access is
// confined to a single map's update.
namespace
{
    std::unordered_map<ObjectGuid, GuidUnorderedSet> sTemporalBeaconTargets;
}

// 401417 Regeneration: a channeled heal-over-time that also stamps Temporal
// Beacon on the healed target and arms the caster's conversion aura.
class spell_sod_mage_regeneration : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_regeneration);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        caster->CastSpell(target, SPELL_SOD_MAGE_TEMPORAL_BEACON, true);

        if (!caster->HasAura(SPELL_SOD_MAGE_TEMPORAL_CONVERSION))
            caster->CastSpell(caster, SPELL_SOD_MAGE_TEMPORAL_CONVERSION, true);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_regeneration::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 400735 Temporal Beacon: the passive heal-over-time comes from the spell's
// own periodic-heal effect; this script only tracks beacon membership so the
// caster's conversion proc knows where to route healing.
class spell_sod_mage_temporal_beacon : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_temporal_beacon);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        sTemporalBeaconTargets[caster->GetGUID()].insert(target->GetGUID());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        auto it = sTemporalBeaconTargets.find(GetCasterGUID());
        if (it == sTemporalBeaconTargets.end())
            return;

        it->second.erase(target->GetGUID());
        if (!it->second.empty())
            return;

        // Last beacon gone: drop the registry entry and disarm the caster.
        sTemporalBeaconTargets.erase(it);
        if (Unit* caster = GetCaster())
            caster->RemoveAurasDueToSpell(SPELL_SOD_MAGE_TEMPORAL_CONVERSION);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_temporal_beacon::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_temporal_beacon::HandleRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 900002 Temporal Beacon (conversion): hidden passive on the Mage. Converts a
// share of the caster's Arcane spell damage into chronomantic healing on each
// of their beacon targets, reduced on self and for multi-target Arcane spells.
class spell_sod_mage_temporal_conversion : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_temporal_conversion);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SOD_MAGE_TEMPORAL_BEACON_HEAL });
    }

    bool Load() override
    {
        return SodMageEnabled();
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return false;

        // Only the caster's Arcane spell damage feeds the beacon.
        if (!eventInfo.GetSpellInfo())
            return false;

        return (damageInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_ARCANE) != 0;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget(); // the aura sits on the Mage
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!caster || !damageInfo)
            return;

        auto it = sTemporalBeaconTargets.find(caster->GetGUID());
        if (it == sTemporalBeaconTargets.end() || it->second.empty())
            return;

        uint32 conversionPct =
            sConfigMgr->GetOption<uint32>("SodMage.TemporalBeacon.ConversionPct", 70);
        int32 heal = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), conversionPct);

        // Multi-target Arcane spells contribute much less per beacon.
        if (SpellInfo const* procSpell = eventInfo.GetSpellInfo())
            if (procSpell->IsTargetingArea())
            {
                uint32 multiPct =
                    sConfigMgr->GetOption<uint32>("SodMage.TemporalBeacon.MultiTargetPct", 20);
                heal = CalculatePct(heal, multiPct);
            }

        if (heal <= 0)
            return;

        uint32 selfPct =
            sConfigMgr->GetOption<uint32>("SodMage.TemporalBeacon.SelfPct", 50);

        // Copy the GUID set: casting the heal can remove auras and mutate it.
        GuidUnorderedSet const targets = it->second;
        for (ObjectGuid const& guid : targets)
        {
            Unit* beaconTarget = ObjectAccessor::GetUnit(*caster, guid);
            if (!beaconTarget || !beaconTarget->IsAlive())
                continue;

            // Confirm the beacon is still ours before healing through it.
            if (!beaconTarget->HasAura(SPELL_SOD_MAGE_TEMPORAL_BEACON, caster->GetGUID()))
                continue;

            int32 amount = (beaconTarget == caster) ? CalculatePct(heal, selfPct) : heal;
            if (amount <= 0)
                continue;

            caster->CastCustomSpell(beaconTarget, SPELL_SOD_MAGE_TEMPORAL_BEACON_HEAL,
                &amount, nullptr, nullptr, true, nullptr, aurEff);
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_sod_mage_temporal_conversion::CheckProc);
        OnEffectProc += AuraEffectProcFn(
            spell_sod_mage_temporal_conversion::HandleProc,
            EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_sod_mage_spell_scripts()
{
    RegisterSpellScript(spell_sod_mage_regeneration);
    RegisterSpellScript(spell_sod_mage_temporal_beacon);
    RegisterSpellScript(spell_sod_mage_temporal_conversion);
}
