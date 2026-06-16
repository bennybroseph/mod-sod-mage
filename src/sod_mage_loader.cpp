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

// Entry point invoked by the module loader. The name must be
// Add<folder-name-with-underscores>Scripts — the build generates the call.
void Addmod_sod_mageScripts()
{
    AddSC_sod_mage_spell_scripts();
}
