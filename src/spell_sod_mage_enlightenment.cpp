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
#include "Unit.h"

using SodMageRules::EnlightenmentState;
using SodMageRules::EnlightenmentStateFor;

// 412324 Enlightenment: the passive the Chest rune teaches. Its only effect is a
// periodic dummy (1s DBC tick) that polls the Mage's mana and toggles two
// mutually-exclusive sub-auras: 412326 (+10% spell damage) above the high
// threshold, 412325 (mana regen continues through the Five Second Rule) below the
// low threshold. The sub-auras are pure DBC — 412326 is MOD_DAMAGE_PERCENT_DONE,
// 412325 is MOD_MANA_REGEN_INTERRUPT, the same aura retail Meditation uses, so the
// engine's mana-regen math handles the FSR exemption with no custom regen code.
class spell_sod_mage_enlightenment : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_enlightenment);

    bool Load() override
    {
        return SodMageEnabled();
    }

    // Ensure `keep` is applied and `drop` is gone, without recasting `keep` when it
    // is already up (which would needlessly refresh the buff each poll).
    static void ApplyExclusive(Unit* caster, uint32 keep, uint32 drop)
    {
        caster->RemoveAurasDueToSpell(drop);
        if (!caster->HasAura(keep))
            caster->CastSpell(caster, keep, true);
    }

    void Evaluate(Unit* caster)
    {
        if (!caster)
            return;

        uint32 maxMana = caster->GetMaxPower(POWER_MANA);
        if (!maxMana)
            return;

        int32 manaPct = int32(caster->GetPower(POWER_MANA) * 100 / maxMana);

        switch (EnlightenmentStateFor(manaPct,
                    int32(SodMageEnlightenmentHighPct()),
                    int32(SodMageEnlightenmentLowPct())))
        {
            case EnlightenmentState::HighMana:
                ApplyExclusive(caster, SPELL_SOD_MAGE_ENLIGHTENMENT_DAMAGE,
                    SPELL_SOD_MAGE_ENLIGHTENMENT_REGEN);
                break;
            case EnlightenmentState::LowMana:
                ApplyExclusive(caster, SPELL_SOD_MAGE_ENLIGHTENMENT_REGEN,
                    SPELL_SOD_MAGE_ENLIGHTENMENT_DAMAGE);
                break;
            case EnlightenmentState::None:
                caster->RemoveAurasDueToSpell(SPELL_SOD_MAGE_ENLIGHTENMENT_DAMAGE);
                caster->RemoveAurasDueToSpell(SPELL_SOD_MAGE_ENLIGHTENMENT_REGEN);
                break;
        }
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        // The DBC ticks every second; only re-evaluate once the configured poll
        // window has elapsed, so admins can widen the cadence back toward SoD's 5s.
        // GetAmplitude() is the effect's periodic interval in ms (EffectAuraPeriod).
        _sinceEval += aurEff->GetAmplitude();
        if (_sinceEval < SodMageEnlightenmentPollMs())
            return;
        _sinceEval = 0;

        Evaluate(caster);
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Evaluate immediately on learn/login instead of waiting a full poll.
        _sinceEval = 0;
        Evaluate(GetCaster());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
        {
            caster->RemoveAurasDueToSpell(SPELL_SOD_MAGE_ENLIGHTENMENT_DAMAGE);
            caster->RemoveAurasDueToSpell(SPELL_SOD_MAGE_ENLIGHTENMENT_REGEN);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(
            spell_sod_mage_enlightenment::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        AfterEffectApply += AuraEffectApplyFn(
            spell_sod_mage_enlightenment::HandleApply,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_enlightenment::HandleRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }

private:
    uint32 _sinceEval = 0;
};

void AddSC_sod_mage_enlightenment()
{
    RegisterSpellScript(spell_sod_mage_enlightenment);
}
