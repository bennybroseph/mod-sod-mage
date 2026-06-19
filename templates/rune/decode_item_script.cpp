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

// TEMPLATE — only needed if your acquisition chain has a "decode" step (scrambled
// notes + Comprehension Charm -> deciphered notes). The deciphered notes then carry
// ScriptName = 'item_rune_unlock' (the engine script) to do the actual unlock.
// The existing src/item_sod_mage_decode_notes.cpp ALREADY handles every mage note
// pair via its sSodMageNotePairs table -- prefer adding a row there to writing a new
// script. This file is the standalone shape for reference.
//
// Copy into src/, then add AddSC_item_sod_mage_<name>() to src/sod_mage_loader.cpp.

#include "Chat.h"
#include "Item.h"
#include "ItemScript.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"

enum SodMageDecode<Name>
{
    ITEM_SOD_MAGE_CHARM            = 211779,      // Comprehension Charm (real SoD id)
    ITEM_SOD_MAGE_NOTES_SCRAMBLED  = <SCRAMBLED>, // the anagram-named drop
    ITEM_SOD_MAGE_NOTES_DECIPHERED = <DECIPHERED>,// the readable result
};

class item_sod_mage_<name> : public ItemScript
{
public:
    item_sod_mage_<name>() : ItemScript("item_sod_mage_<name>") {}

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return false;

        ChatHandler handler(player->GetSession());

        if (!player->HasItemCount(ITEM_SOD_MAGE_CHARM, 1))
        {
            handler.SendSysMessage(
                "|cFFFF0000[Spell Notes]|r The shorthand is gibberish. You need a "
                "|cFFFFD700Comprehension Charm|r to decipher it.");
            return true; // handled — suppress the benign use-spell
        }

        // Consume one charm and one scrambled note, hand back the deciphered note.
        player->DestroyItemCount(ITEM_SOD_MAGE_CHARM, 1, true);
        player->DestroyItemCount(ITEM_SOD_MAGE_NOTES_SCRAMBLED, 1, true);
        player->AddItem(ITEM_SOD_MAGE_NOTES_DECIPHERED, 1);

        handler.SendSysMessage(
            "|cFF00FF00[Spell Notes]|r The charm unscrambles the arcane shorthand.");
        return true;
    }
};

void AddSC_item_sod_mage_<name>()
{
    new item_sod_mage_<name>();
}
