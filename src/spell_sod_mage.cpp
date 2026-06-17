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
#include "ObjectAccessor.h"
#include "SpellDefines.h"
#include "SpellInfo.h"
#include <unordered_map>

// Maps a Mage caster to the set of units currently carrying their Temporal
// Beacon. Kept in sync by the beacon AuraScript (apply/remove). Caster and
// beacon targets are always within 100yd, i.e. on the same map, so access is
// confined to a single map's update.
namespace
{
    std::unordered_map<ObjectGuid, GuidUnorderedSet> sTemporalBeaconTargets;

    // Applies Temporal Beacon to `target` for `durationMs` and ensures the caster
    // carries the hidden conversion aura. The duration is overridden per caller
    // (Regeneration 30s, Mass Regeneration 15s) instead of being baked into the
    // beacon spell. Registry upkeep happens in the beacon AuraScript's apply hook.
    void ApplyTemporalBeacon(Unit* caster, Unit* target, int32 durationMs)
    {
        caster->CastCustomSpell(SPELL_SOD_MAGE_TEMPORAL_BEACON, SPELLVALUE_AURA_DURATION,
            durationMs, target, true);

        if (!caster->HasAura(SPELL_SOD_MAGE_TEMPORAL_CONVERSION))
            caster->CastSpell(caster, SPELL_SOD_MAGE_TEMPORAL_CONVERSION, true);
    }
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

        ApplyTemporalBeacon(caster, target, SOD_MAGE_BEACON_MS_REGEN);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_regeneration::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
    }
};

// 412510 Mass Regeneration: the AoE sibling of Regeneration. The spell's own
// party-area targeting applies the periodic heal to each party member, so this
// hook fires once per target — stamping each with a shorter (15s) Temporal Beacon.
class spell_sod_mage_mass_regeneration : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_mass_regeneration);

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

        ApplyTemporalBeacon(caster, target, SOD_MAGE_BEACON_MS_MASS_REGEN);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_mass_regeneration::HandleApply,
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

// 900001 Temporal Beacon (conversion): hidden passive on the Mage. Converts a
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

        // Any Arcane damage the caster deals feeds the beacon — Arcane spells and
        // an Arcane wand alike. We don't require a SpellInfo, so wand ranged
        // auto-attacks (which carry no cast spell) still qualify; the proc flags
        // and this school check do the filtering.
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

        // Multi-target Arcane spells contribute much less per beacon.
        SpellInfo const* procSpell = eventInfo.GetSpellInfo();
        bool multiTarget = procSpell && procSpell->IsTargetingArea();
        uint32 multiPct =
            sConfigMgr->GetOption<uint32>("SodMage.TemporalBeacon.MultiTargetPct", 20);

        int32 heal = SodMageRules::BeaconHeal(
            static_cast<int32>(damageInfo->GetDamage()), conversionPct,
            multiTarget, multiPct);

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

            int32 amount = (beaconTarget == caster) ? SodMageRules::BeaconSelfHeal(heal, selfPct) : heal;
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
    RegisterSpellScript(spell_sod_mage_mass_regeneration);
    RegisterSpellScript(spell_sod_mage_temporal_beacon);
    RegisterSpellScript(spell_sod_mage_temporal_conversion);
}
