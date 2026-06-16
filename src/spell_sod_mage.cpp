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

// Template SpellScript. Copy this for each SoD ability, rename, and wire the
// real spell ID(s) in SodMageSpells (spell_sod_mage.h) plus a spell_dbc row.
// Delete this example once a real spell takes its place.
class spell_sod_mage_example : public SpellScript
{
    PrepareSpellScript(spell_sod_mage_example);

    // Skip all behavior when the module is disabled.
    bool Load() override
    {
        return SodMageEnabled();
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        // Mechanics go here, e.g. adjust damage, apply an aura, trigger a
        // proc. GetCaster()/GetHitUnit() are available during effect hooks.
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(
            spell_sod_mage_example::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_sod_mage_spell_scripts()
{
    RegisterSpellScript(spell_sod_mage_example);
}
