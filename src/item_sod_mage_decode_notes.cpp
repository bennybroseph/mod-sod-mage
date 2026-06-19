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

#include "Chat.h"
#include "Item.h"
#include "ItemScript.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "StringFormat.h"

#include <unordered_map>

// The "combine" step of the SoD note chains: use a scrambled (anagram-named)
// Spell Notes item while holding a Comprehension Charm (211779) to produce the
// deciphered notes. Both the charm and the scrambled note are consumed (SoD
// charms stack to 5 / are conjured in 5s -- one per decode). Pure inventory
// logic -- no rune-engine call -- so mod-sod-mage stays usable without the
// engine installed.
//
// Data-driven so one ItemScript serves every scrambled note: each entry maps a
// scrambled item id -> its deciphered item id + the display name for the
// success message. All ids are the real SoD ids (wago.tools).
enum SodMageDecodeItems
{
    ITEM_SOD_MAGE_CHARM = 211779,
};

struct SodMageNotePair
{
    uint32 deciphered;
    char const* name;
};

namespace
{
    std::unordered_map<uint32, SodMageNotePair> const sSodMageNotePairs =
    {
        // scrambled        deciphered  deciphered display name
        { 208754, { 208753, "Spell Notes: Regeneration" } },  // TENGI RONEERA
        { 203752, { 203746, "Spell Notes: Living Flame" } },  // MILEGIN VALF
    };
}

class item_sod_mage_decode_notes : public ItemScript
{
public:
    item_sod_mage_decode_notes() : ItemScript("item_sod_mage_decode_notes") {}

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return false;

        ChatHandler handler(player->GetSession());

        auto const it = sSodMageNotePairs.find(item->GetEntry());
        if (it == sSodMageNotePairs.end())
            return true; // unknown note -- nothing to do, suppress use-spell

        SodMageNotePair const& pair = it->second;

        if (!player->HasItemCount(ITEM_SOD_MAGE_CHARM, 1))
        {
            handler.SendSysMessage(
                "|cFFFF0000[Spell Notes]|r The shorthand is gibberish. You need a "
                "|cFFFFD700Comprehension Charm|r to decipher it.");
            return true; // handled — suppress the benign use-spell
        }

        // Consume one charm and one scrambled note, hand back the deciphered
        // version.
        player->DestroyItemCount(ITEM_SOD_MAGE_CHARM, 1, true);
        player->DestroyItemCount(item->GetEntry(), 1, true);
        player->AddItem(pair.deciphered, 1);

        handler.SendSysMessage(Acore::StringFormat(
            "|cFF00FF00[Spell Notes]|r The charm unscrambles the arcane shorthand "
            "into |cFFFFD700{}|r.", pair.name).c_str());
        return true;
    }
};

void AddSC_item_sod_mage_decode_notes()
{
    new item_sod_mage_decode_notes();
}
