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

// Spell IDs owned by this module. The 4xxxxx IDs are the real Season of
// Discovery IDs (matched by the client MPQ patch). 900002 is a custom server-side
// helper in the reserved 900000-900099 band — it has no SoD equivalent because
// SoD bakes the conversion into Temporal Beacon itself, whereas in 3.3.5a the
// proc must live on the caster. Every ID here has a spell_dbc row; 900002 is
// server-only.
enum SodMageSpells
{
    SPELL_SOD_MAGE_REGENERATION         = 401417, // channeled HoT, applies beacon
    SPELL_SOD_MAGE_MASS_REGENERATION    = 412510, // AoE party channeled HoT
    SPELL_SOD_MAGE_TEMPORAL_BEACON      = 400735, // target marker + passive HoT
    SPELL_SOD_MAGE_TEMPORAL_BEACON_HEAL = 401405, // triggered chronomantic heal (SoD)
    SPELL_SOD_MAGE_TEMPORAL_CONVERSION  = 900001, // hidden caster-side proc aura
    SPELL_SOD_MAGE_LIVING_FLAME         = 401556, // summons the creeping flame
    SPELL_SOD_MAGE_LIVING_FLAME_DAMAGE  = 401558, // per-second Spellfire AoE tick
};

// Living Flame's mover is a custom creature in the module's reserved creature
// band (700200-700299), shown with the Fire Elemental model (a walking flame).
// The 700100 demo creature was retired (see sod_mage_mass_regeneration.sql).
// Defined in sod_mage_living_flame.sql.
constexpr uint32 NPC_SOD_MAGE_LIVING_FLAME = 700200;

// Living Flame lasts 10s (SoD) and ticks its AoE once per second.
constexpr uint32 SOD_MAGE_LIVING_FLAME_DURATION_MS = 10000;
constexpr uint32 SOD_MAGE_LIVING_FLAME_TICK_MS     = 1000;

// Temporal Beacon durations (ms). SoD applies different lengths depending on the
// caster spell — Regeneration 30s, Mass Regeneration 15s — so we override the
// duration at apply time (SPELLVALUE_AURA_DURATION); 400735's own duration is just
// a fallback. See ApplyTemporalBeacon in spell_sod_mage.cpp.
constexpr int32 SOD_MAGE_BEACON_MS_REGEN      = 30000;
constexpr int32 SOD_MAGE_BEACON_MS_MASS_REGEN = 15000;

// Reads the module master enable flag.
inline bool SodMageEnabled()
{
    return sConfigMgr->GetOption<bool>("SodMage.Enable", true);
}

#endif // MODULE_SOD_MAGE_H
