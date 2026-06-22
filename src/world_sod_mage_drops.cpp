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

#include "Configuration/Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "LootMgr.h"
#include "ScriptMgr.h"

// Bridges the SodMage.Drops.* config keys onto the rune-note drop chances seeded
// in the *_unlock / mass_regeneration SQL. The creature_loot_template rows stay in
// SQL (greppable); this just tunes their `Chance` from the .conf so an admin can
// retune note drop rates without editing SQL. Each note item id appears only in our
// loot rows, so a single UPDATE keyed by Item covers all its sources. When
// SodMage.Enable is off every chance is forced to 0.
//
// Applied at startup (OnStartup runs after scripts + the loot store are loaded) and
// on a live `.reload config`. OnAfterConfigLoad(false) is NOT used for the startup
// pass: at startup config loads before module scripts register.

namespace
{
    // (note item id, .conf key, default %). The item id keys the UPDATE -- each is
    // unique to mod-sod-mage's loot rows. Mirrors the creature_loot_template seeds.
    struct NoteDrop
    {
        uint32      Item;
        char const* Key;
        float       Default;
    };

    // Defaults: Mass Regeneration 100% (off the Awakened Lich elite); Living Flame,
    // Regeneration, and Living Bomb 20% (off leveling-zone trash).
    NoteDrop const NOTE_DROPS[] =
    {
        { 211514, "SodMage.Drops.MassRegenNotesChance", 100.0f },
        { 203752, "SodMage.Drops.LivingFlameNotesChance", 20.0f },
        { 208754, "SodMage.Drops.RegenNotesChance", 20.0f },
        { 208799, "SodMage.Drops.LivingBombNotesChance", 20.0f },
    };
}

class world_sod_mage_drops : public WorldScript
{
public:
    world_sod_mage_drops() : WorldScript("world_sod_mage_drops") {}

    void OnStartup() override
    {
        Apply();
    }

    void OnAfterConfigLoad(bool reload) override
    {
        // Only a live `.reload config`; the startup pass is OnStartup (see header).
        if (reload)
            Apply();
    }

private:
    static void Apply()
    {
        bool const enabled = sConfigMgr->GetOption<bool>("SodMage.Enable", true);

        for (NoteDrop const& drop : NOTE_DROPS)
        {
            float const chance = enabled
                ? sConfigMgr->GetOption<float>(drop.Key, drop.Default) : 0.0f;

            WorldDatabase.DirectExecute(
                "UPDATE `creature_loot_template` SET `Chance` = {} WHERE `Item` = {}",
                chance, drop.Item);
        }

        // Re-read the creature loot store so the updated chances take effect.
        LoadLootTemplates_Creature();

        LOG_INFO("server.loading", "mod-sod-mage: rune-note drop chances applied.");
    }
};

void AddSC_world_sod_mage_drops()
{
    new world_sod_mage_drops();
}
