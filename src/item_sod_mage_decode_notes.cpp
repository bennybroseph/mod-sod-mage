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

// The "combine" step of the SoD Regeneration chain: use the scrambled
// Spell Notes: TENGI RONEERA (700201) while holding a Comprehension Charm
// (700200) to produce the deciphered Spell Notes: Regeneration (700202). Both
// the charm and the scrambled note are consumed (SoD charms stack to 5 / are
// conjured in 5s -- one per decode). Pure inventory logic -- no rune-engine
// call -- so mod-sod-mage stays usable without the engine installed.
enum SodMageDecodeItems
{
    ITEM_SOD_MAGE_CHARM            = 700200,
    ITEM_SOD_MAGE_NOTES_SCRAMBLED  = 700201,
    ITEM_SOD_MAGE_NOTES_DECIPHERED = 700202,
};

class item_sod_mage_decode_notes : public ItemScript
{
public:
    item_sod_mage_decode_notes() : ItemScript("item_sod_mage_decode_notes") {}

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

        // Consume one charm and one scrambled note, hand back the deciphered
        // version.
        player->DestroyItemCount(ITEM_SOD_MAGE_CHARM, 1, true);
        player->DestroyItemCount(ITEM_SOD_MAGE_NOTES_SCRAMBLED, 1, true);
        player->AddItem(ITEM_SOD_MAGE_NOTES_DECIPHERED, 1);

        handler.SendSysMessage(
            "|cFF00FF00[Spell Notes]|r The charm unscrambles the arcane shorthand "
            "into |cFFFFD700Spell Notes: Regeneration|r.");
        return true;
    }
};

void AddSC_item_sod_mage_decode_notes()
{
    new item_sod_mage_decode_notes();
}
