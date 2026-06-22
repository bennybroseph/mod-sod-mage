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
#include "Player.h"
#include "SpellAuras.h"
#include "Unit.h"

// The Scorch-tagged Living Bomb the Living Bomb rune substitutes for the stock spell once
// the Mage knows the real Living Bomb. The synergy ("benefits from all talents/effects that
// trigger from or modify Scorch") is gated to rune-holders by REPLACING their cast, not by
// editing the global spell: a redirect on the stock Living Bomb fires this copy, which
// carries Scorch's SpellFamilyFlags so every Scorch talent SpellMod and Scorch-keyed proc
// applies to it. Non-rune mages keep the stock Living Bomb untouched.
//
// The copy exists PER RANK (R1/R2/R3, index-matched to SPELL_MAGE_LIVING_BOMB_RANKS) and is
// wired into spell_ranks (sod_mage_living_bomb.sql). That keeps icon 3000 -- so it reads as
// a real Living Bomb -- without crashing Master of Elements, whose Living-Bomb-explosion
// case does GetSpellWithRank(44457, GetSpellRank(thisSpell)); a valid rank 1-3 resolves to
// the matching real-Living-Bomb DoT instead of walking off the chain.
namespace
{
    // Index of `spellId` in a 3-rank id array (-1 if absent). The copy ranks, the copy
    // explosions, and the real Living Bomb ranks all share the same index ordering.
    int LivingBombRankIndex(uint32 spellId, uint32 const (&ranks)[3])
    {
        for (int i = 0; i < 3; ++i)
            if (ranks[i] == spellId)
                return i;
        return -1;
    }
}

// Copy DoT (per rank): c(L)*0.85 per tick (4 ticks/12s); explodes on expire/dispel.
class spell_sod_mage_living_bomb_copy : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_living_bomb_copy);

    bool Load() override
    {
        return SodMageEnabled();
    }

    // Add the SoD level curve to the per-tick damage. `amount` already holds the
    // spell-power bonus the engine folded in (CalculateAmount runs first), so we ADD.
    void CalcTick(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        if (Unit* caster = GetCaster())
            amount += SodMageRules::LivingBombDotTick(caster->GetLevel());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        AuraRemoveMode const removeMode = GetTargetApplication()->GetRemoveMode();
        if (removeMode != AURA_REMOVE_BY_EXPIRE && removeMode != AURA_REMOVE_BY_ENEMY_SPELL)
            return;

        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        // Fire the same-rank explosion. Destination cast at the target's position: the
        // explosion's DEST_AREA target needs a dst, or AoE resolution crashes on a unit cast.
        int const idx = LivingBombRankIndex(GetSpellInfo()->Id, SOD_MAGE_LIVING_BOMB_COPY_DOTS);
        if (idx < 0)
            return;

        caster->CastSpell(target->GetPositionX(), target->GetPositionY(),
            target->GetPositionZ(), SOD_MAGE_LIVING_BOMB_COPY_BOOMS[idx], true);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(
            spell_sod_mage_living_bomb_copy::CalcTick,
            EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_living_bomb_copy::HandleRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

// Copy explosion (per rank): the copy's AoE damage (uncapped -- the real spell). Scorch-
// tagged in the DBC; gear spell power adds via spell_bonus_data. Same curve for every rank.
class spell_sod_mage_living_bomb_copy_boom : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_living_bomb_copy_boom);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            SetEffectValue(SodMageRules::LivingBombExplosion(caster->GetLevel()));
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(
            spell_sod_mage_living_bomb_copy_boom::HandleDamage,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// Redirect bound to the stock Living Bomb (44457 + ranks). For a rune-holder it suppresses
// the stock aura and fires the same-rank Scorch-tagged copy instead; others are unaffected.
class spell_sod_mage_living_bomb_redirect : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_living_bomb_redirect);

    bool Load() override
    {
        return SodMageEnabled();
    }

    bool IsRuneHolder()
    {
        Unit* caster = GetCaster();
        return caster && caster->HasAura(SPELL_SOD_MAGE_LIVING_BOMB_RUNE);
    }

    void SuppressStock(SpellEffIndex /*effIndex*/)
    {
        if (IsRuneHolder())
            PreventHitAura();
    }

    void FireCopy()
    {
        if (!IsRuneHolder())
            return;

        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        // Fire the copy DoT of the same rank as the Living Bomb the player cast, so Master
        // of Elements maps it back to that real rank.
        int const idx = LivingBombRankIndex(GetSpellInfo()->Id, SPELL_MAGE_LIVING_BOMB_RANKS);
        if (idx >= 0)
            caster->CastSpell(target, SOD_MAGE_LIVING_BOMB_COPY_DOTS[idx], true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(
            spell_sod_mage_living_bomb_redirect::SuppressStock,
            EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        AfterHit += SpellHitFn(spell_sod_mage_living_bomb_redirect::FireCopy);
    }
};

void AddSC_sod_mage_living_bomb_copy()
{
    RegisterSpellScript(spell_sod_mage_living_bomb_copy);
    RegisterSpellScript(spell_sod_mage_living_bomb_copy_boom);
    RegisterSpellScript(spell_sod_mage_living_bomb_redirect);
}
