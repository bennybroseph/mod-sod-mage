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

// Spell IDs owned by this module. The 4xxxxx pair are the real Season of
// Discovery IDs (matched by the client MPQ patch); the 9000xx pair are custom
// server-side helpers in this module's reserved 900000-900099 band. Every ID
// here has a row in spell_dbc (base SQL); 900002 is server-only.
enum SodMageSpells
{
    SPELL_SOD_MAGE_REGENERATION         = 401417, // channeled HoT, applies beacon
    SPELL_SOD_MAGE_TEMPORAL_BEACON      = 400735, // target marker + passive HoT
    SPELL_SOD_MAGE_TEMPORAL_BEACON_HEAL = 900001, // triggered chronomantic heal
    SPELL_SOD_MAGE_TEMPORAL_CONVERSION  = 900002, // hidden caster-side proc aura
};

// Reads the module master enable flag.
inline bool SodMageEnabled()
{
    return sConfigMgr->GetOption<bool>("SodMage.Enable", true);
}

#endif // MODULE_SOD_MAGE_H
