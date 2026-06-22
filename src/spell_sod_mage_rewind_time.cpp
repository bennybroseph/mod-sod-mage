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
#include "SpellAuras.h"
#include "Timer.h"
#include "Unit.h"
#include <deque>
#include <limits>
#include <unordered_map>

// Rewind Time (401462, the ability the SoD rune-grant 401734 teaches): instantly
// heals a beaconed ally for ALL damage they took over the last few seconds. The heal
// is a real SPELL_EFFECT_HEAL whose value is injected here, so it flows through
// SpellHealingBonusDone/Taken (any +healing% and healing crit apply) -- the script
// only decides the amount. Two halves live in this file:
//   * a rolling per-unit damage log, fed by every damage event but kept only for
//     units carrying a Temporal Beacon (unit_sod_mage_rewind_time);
//   * the spell itself, which sums that log over the window and injects it as the
//     heal (spell_sod_mage_rewind_time).
namespace
{
    struct DamageEntry
    {
        uint32 atMs;
        uint32 amount;
    };

    // Damage taken, keyed by victim GUID. Only beaconed units are ever inserted, so
    // the map stays bounded to the handful of units a Mage is beaconing. Accessed
    // from map update only (a beacon target is always near its caster), matching the
    // unlocked-global-map convention of sTemporalBeaconTargets in spell_sod_mage.cpp.
    std::unordered_map<ObjectGuid, std::deque<DamageEntry>> sBeaconDamageLog;

    // Drop entries older than the window. `now`/`atMs` are getMSTime() stamps, so we
    // compare with getMSTimeDiff to stay correct across the 32-bit ms wraparound.
    void PruneOld(std::deque<DamageEntry>& log, uint32 now, uint32 windowMs)
    {
        while (!log.empty() && getMSTimeDiff(log.front().atMs, now) > windowMs)
            log.pop_front();
    }
}

// Called from the Temporal Beacon AuraScript when a unit's last beacon ends, so the
// log doesn't retain a (slowly accumulating) entry for every unit ever beaconed.
void SodMageForgetRewindLog(ObjectGuid const& guid)
{
    sBeaconDamageLog.erase(guid);
}

// Records damage taken by units that carry a Temporal Beacon. Cheap checks first so
// the common (non-beaconed) case bails before any config read or map access.
class unit_sod_mage_rewind_time : public UnitScript
{
public:
    unit_sod_mage_rewind_time() : UnitScript("unit_sod_mage_rewind_time") {}

    void OnDamage(Unit* /*attacker*/, Unit* victim, uint32& damage) override
    {
        if (!damage || !victim)
            return;

        if (!victim->HasAura(SPELL_SOD_MAGE_TEMPORAL_BEACON))
            return;

        if (!SodMageEnabled())
            return;

        uint32 const now = getMSTime();
        std::deque<DamageEntry>& log = sBeaconDamageLog[victim->GetGUID()];
        log.push_back({ now, damage });
        PruneOld(log, now, SodMageRewindTimeWindowMs());
    }
};

// 401462 Rewind Time: heal a beaconed ally for the damage they took in the window.
class spell_sod_mage_rewind_time : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_rewind_time);

    bool Load() override
    {
        return SodMageEnabled();
    }

    // Must target a unit carrying *our* Temporal Beacon -- otherwise block the cast so
    // a mis-click doesn't burn the cooldown.
    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        Unit* target = GetExplTargetUnit();
        if (!caster || !target)
            return SPELL_FAILED_BAD_TARGETS;

        if (!target->HasAura(SPELL_SOD_MAGE_TEMPORAL_BEACON, caster->GetGUID()))
            return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    // Inject the heal amount before EffectHeal runs (launch phase), so the engine then
    // applies healing modifiers and crit on top. Value = damage taken in the window.
    void HandleHeal(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        uint32 const windowMs = SodMageRewindTimeWindowMs();

        // "Ineffective on targets that did not have a Temporal Beacon 5 sec ago":
        // require the beacon to have been up for the full window, else heal nothing
        // (the cast still resolves and consumes the cooldown).
        Aura* beacon = target->GetAura(SPELL_SOD_MAGE_TEMPORAL_BEACON, caster->GetGUID());
        if (!beacon || uint32(beacon->GetMaxDuration() - beacon->GetDuration()) < windowMs)
        {
            SetEffectValue(0);
            return;
        }

        uint32 const now = getMSTime();
        auto it = sBeaconDamageLog.find(target->GetGUID());
        if (it == sBeaconDamageLog.end())
        {
            SetEffectValue(0);
            return;
        }

        PruneOld(it->second, now, windowMs);

        uint64 sum = 0;
        for (DamageEntry const& entry : it->second)
            sum += entry.amount;

        SetEffectValue(int32(std::min<uint64>(sum,
            static_cast<uint64>(std::numeric_limits<int32>::max()))));
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sod_mage_rewind_time::CheckCast);
        OnEffectLaunchTarget += SpellEffectFn(
            spell_sod_mage_rewind_time::HandleHeal,
            EFFECT_0, SPELL_EFFECT_HEAL);
    }
};

void AddSC_sod_mage_rewind_time()
{
    new unit_sod_mage_rewind_time();
    RegisterSpellScript(spell_sod_mage_rewind_time);
}
