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

// Each script source exposes one AddSC_* function; declare them here.
void AddSC_sod_mage_spell_scripts();
void AddSC_sod_mage_living_flame();
void AddSC_sod_mage_arcane_surge();
void AddSC_sod_mage_arcane_burst();
void AddSC_sod_mage_arcane_blast_rune();
void AddSC_sod_mage_nether_vortex();
void AddSC_player_sod_mage_arcane_crystals();
void AddSC_sod_mage_rewind_time();
void AddSC_sod_mage_enlightenment();
void AddSC_sod_mage_azora_event();
void AddSC_item_sod_mage_decode_notes();
void AddSC_world_sod_mage_drops();

// Entry point invoked by the module loader. The name must be
// Add<folder-name-with-underscores>Scripts — the build generates the call.
void Addmod_sod_mageScripts()
{
    AddSC_sod_mage_spell_scripts();
    AddSC_sod_mage_living_flame();
    AddSC_sod_mage_arcane_surge();
    AddSC_sod_mage_arcane_burst();
    AddSC_sod_mage_arcane_blast_rune();
    AddSC_sod_mage_nether_vortex();
    AddSC_player_sod_mage_arcane_crystals();
    AddSC_sod_mage_rewind_time();
    AddSC_sod_mage_enlightenment();
    AddSC_sod_mage_azora_event();
    AddSC_item_sod_mage_decode_notes();
    AddSC_world_sod_mage_drops();
}
