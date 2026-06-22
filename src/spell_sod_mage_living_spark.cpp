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
#include "SpellAuras.h"
#include "Unit.h"
#include <algorithm>

// 900007 Living Spark: the Living Bomb stand-in. A silent fuse (AURA_DUMMY, no periodic
// damage, SPELL_ATTR1_NO_THREAT so planting it neither threatens nor pulls the target);
// when it expires or is dispelled it detonates for the SoD Living Bomb explosion damage
// (900008) -- only THEN does the mob aggro. The explosion level is capped to the SoD
// Living Bomb level so the stand-in never out-scales the real spell. Mirrors the core's
// spell_mage_living_bomb (explode on EXPIRE/ENEMY_SPELL removal).
class spell_sod_mage_living_spark : public AuraScript
{
    PrepareAuraScript(spell_sod_mage_living_spark);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // Only the fuse running out or being dispelled detonates -- not death/cancel.
        AuraRemoveMode const removeMode = GetTargetApplication()->GetRemoveMode();
        if (removeMode != AURA_REMOVE_BY_EXPIRE && removeMode != AURA_REMOVE_BY_ENEMY_SPELL)
            return;

        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target)
            return;

        // Cast the AoE at the target's position (a DESTINATION cast). The explosion's
        // TARGET_UNIT_DEST_AREA_ENEMY needs a destination; casting it on a unit would
        // leave the dst unset and crash on AoE resolution -- so we pass coordinates,
        // exactly like Living Flame's trail (spell_sod_mage_living_flame). The explosion
        // computes its own (capped) damage in spell_sod_mage_living_spark_boom.
        caster->CastSpell(target->GetPositionX(), target->GetPositionY(),
            target->GetPositionZ(), SPELL_SOD_MAGE_LIVING_SPARK_BOOM, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(
            spell_sod_mage_living_spark::HandleRemove,
            EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 900008 Living Spark explosion: sets its damage to the SoD curve at a level capped to
// the SoD Living Bomb level (so the stand-in never out-scales the real spell). Gear
// spell power adds on top via spell_bonus_data. Mirrors Living Flame's damage script.
class spell_sod_mage_living_spark_boom : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_living_spark_boom);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        uint8 const level = std::min<uint8>(caster->GetLevel(),
            static_cast<uint8>(SodMageLivingBombCapLevel()));
        SetEffectValue(SodMageRules::LivingBombExplosion(level));
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(
            spell_sod_mage_living_spark_boom::HandleDamage,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddSC_sod_mage_living_spark()
{
    RegisterSpellScript(spell_sod_mage_living_spark);
    RegisterSpellScript(spell_sod_mage_living_spark_boom);
}
