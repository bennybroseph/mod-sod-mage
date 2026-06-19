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

// TEMPLATE — copy into src/spell_sod_mage_<name>.cpp, fill the placeholders, then
// add AddSC_sod_mage_<name>() to src/sod_mage_loader.cpp. Not built from templates/.
// Model: src/spell_sod_mage.cpp. See templates/spell/README.md.

#include "spell_sod_mage.h"   // SodMageEnabled(), SPELL_SOD_MAGE_* id constants
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

// The spell id this script binds to. Match the `id` in your SPELLS entry and the
// `spell_script_names.ScriptName` the generator emits for it. Prefer the real SoD
// id; a server-only helper goes in the 900000-900099 band.
enum SodMage<Name>Ids
{
    SPELL_SOD_MAGE_<NAME> = <SPELL_ID>,
};

// --- Direct-effect example (instant damage/heal on hit) --------------------
// Bound via RegisterSpellScript. Delete if your spell is aura-only.
class spell_sod_mage_<name> : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_<name>);

    bool Load() override
    {
        return SodMageEnabled(); // module master switch — keep every script gated
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        // TODO: your effect. e.g. caster->CastSpell(target, SPELL_..., true);
    }

    void Register() override
    {
        // TODO: match the effect index/type your spell_dbc row defines.
        OnEffectHitTarget += SpellEffectFn(
            spell_sod_mage_<name>::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// --- Proc-aura example (a buff that reacts to the caster's actions) --------
// Needs a spell_proc row (add a `proc` dict to the SPELLS entry). Delete if unused.
class aura_sod_mage_<name> : public AuraScript
{
    PrepareAuraScript(aura_sod_mage_<name>);

    bool Load() override
    {
        return SodMageEnabled();
    }

    bool CheckProc(ProcEventInfo& /*eventInfo*/)
    {
        // TODO: return true only for the events this should proc on.
        return true;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        // TODO: your proc effect.
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_sod_mage_<name>::CheckProc);
        OnEffectProc += AuraEffectProcFn(
            aura_sod_mage_<name>::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Exposed to the loader. Keep the name unique; declare + call it in
// src/sod_mage_loader.cpp.
void AddSC_sod_mage_<name>()
{
    RegisterSpellScript(spell_sod_mage_<name>);
    RegisterSpellScript(aura_sod_mage_<name>);
}
