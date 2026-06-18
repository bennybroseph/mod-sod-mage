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
#include "Creature.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"

// Living Flame (401556). SoD summons a spellfire flame that creeps toward the
// target, leaving a trail that deals Spellfire (Fire|Arcane) damage to nearby
// enemies. SoD implements the mover with an AreaTrigger, which 3.3.5a lacks, so
// we summon a creature (npc_sod_mage_living_flame, a slow Fire Elemental) that
// homes on the target. The creature itself deals no damage: once per second it
// makes the MAGE recast the Spellfire AoE (401558) at its position, so the
// damage is attributed to the caster — that is what lets it feed Chronomantic
// Healing (the Temporal Beacon conversion procs on the Mage's own damage) and
// any other Fire- or Arcane-gated proc.

// 401556 cast: summon the creeping flame aimed at the player's target.
class spell_sod_mage_living_flame : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_living_flame);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        // Spawn at the caster; the creature creeps toward the target from there.
        if (Creature* flame = caster->SummonCreature(NPC_SOD_MAGE_LIVING_FLAME,
                caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(),
                caster->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN,
                SOD_MAGE_LIVING_FLAME_DURATION_MS))
            if (flame->AI())
                flame->AI()->SetGUID(target->GetGUID());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(
            spell_sod_mage_living_flame::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// The creeping flame creature: passive, invisible, homes on the stored target
// and once per second has its summoner (the Mage) cast the Spellfire AoE here.
struct npc_sod_mage_living_flame : public ScriptedAI
{
    npc_sod_mage_living_flame(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToAll(true);

        // "Slowly creeps": a fraction of normal run speed (config-tunable).
        float pct = sConfigMgr->GetOption<float>("SodMage.LivingFlame.SpeedPct", 40.0f) / 100.0f;
        if (pct <= 0.0f)
            pct = 0.4f;
        me->SetSpeed(MOVE_RUN, pct, true);
        me->SetSpeed(MOVE_WALK, pct, true);
    }

    void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
    {
        _enemyGuid = guid;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!SodMageEnabled())
            return;

        // Home continuously: send the flame toward the target the moment we have
        // one, then re-aim only when the target drifts from our last destination.
        // A standing mob gets a single smooth glide (no per-second re-path), and
        // movement starts on the first tick instead of waiting for the damage one.
        // MovePoint's forceDestination moves it even when off the navmesh.
        if (Unit* enemy = ObjectAccessor::GetUnit(*me, _enemyGuid))
            if (enemy->IsAlive() && (!_hasDest || _dest.GetExactDist(enemy) > 2.0f))
            {
                _dest = enemy->GetPosition();
                _hasDest = true;
                me->GetMotionMaster()->MovePoint(0, _dest);
            }

        _tickTimer += diff;
        if (_tickTimer < SOD_MAGE_LIVING_FLAME_TICK_MS)
            return;
        _tickTimer = 0;

        TempSummon* summon = me->ToTempSummon();
        Unit* owner = summon ? ObjectAccessor::GetUnit(*me, summon->GetSummonerGUID()) : nullptr;
        if (!owner)
            return;

        // The Mage (not the creature) casts the AoE at our position, so the
        // Spellfire damage is the caster's and feeds the beacon proc.
        owner->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(),
            SPELL_SOD_MAGE_LIVING_FLAME_DAMAGE, true);
    }

private:
    ObjectGuid _enemyGuid;
    Position _dest;
    bool _hasDest = false;
    uint32 _tickTimer = 0;
};

// 401558 per-tick damage. SoD scales the trail's damage off a level curve (so it
// works with no spellpower); gear spellpower then adds on top via spell_bonus_data
// (direct 1.0). Curve in SodMageRules::LivingFlameTickDamage.
class spell_sod_mage_living_flame_damage : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_living_flame_damage);

    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            SetEffectValue(SodMageRules::LivingFlameTickDamage(caster->GetLevel()));
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(
            spell_sod_mage_living_flame_damage::HandleDamage,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddSC_sod_mage_living_flame()
{
    RegisterSpellScript(spell_sod_mage_living_flame);
    RegisterSpellScript(spell_sod_mage_living_flame_damage);
    RegisterCreatureAI(npc_sod_mage_living_flame);
}
