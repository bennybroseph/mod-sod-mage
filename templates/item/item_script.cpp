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

// TEMPLATE — only needed if "Use" should DO something. Set the item's
// item_template.ScriptName to this script's name. Copy into src/, then add
// AddSC_item_sod_mage_<name>() to src/sod_mage_loader.cpp. Not built from templates/.
// Model: src/item_sod_mage_decode_notes.cpp.
//
// To merely UNLOCK a rune, don't write a script — use the engine's generic
// 'item_rune_unlock' (see ../rune/). Use this only for custom inventory logic.

#include "Chat.h"
#include "Item.h"
#include "ItemScript.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"

class item_sod_mage_<name> : public ItemScript
{
public:
    item_sod_mage_<name>() : ItemScript("item_sod_mage_<name>") {}

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return false;

        // TODO: your inventory logic. Use player->HasItemCount / DestroyItemCount /
        // AddItem; message via ChatHandler(player->GetSession()).

        // Return true to mark the use HANDLED — this suppresses the item's benign
        // ON_USE spell (present only so the client offers "Use").
        return true;
    }
};

void AddSC_item_sod_mage_<name>()
{
    new item_sod_mage_<name>();
}
