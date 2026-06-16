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

#ifndef MODULE_SOD_MAGE_H
#define MODULE_SOD_MAGE_H

#include "Config.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"

#define MODULE_STRING "mod-sod-mage"

// Custom spell IDs owned by this module.
// Keep these in a reserved high band to avoid clashing with Blizzlike data
// and other modules. Mirror every ID here with a row in spell_dbc (base SQL).
enum SodMageSpells
{
    // SPELL_SOD_MAGE_LIVING_FLAME = 900000,
};

// Reads the module master enable flag.
inline bool SodMageEnabled()
{
    return sConfigMgr->GetOption<bool>("SodMage.Enable", true);
}

#endif // MODULE_SOD_MAGE_H
