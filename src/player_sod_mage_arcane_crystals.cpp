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

#include "spell_sod_mage.h"
#include "GameObject.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include <algorithm>

// The Zoram Strand (Ashenvale) discovery that earns the Arcane Blast rune. Casting any
// player Arcane Explosion rank within range of the next crystal IN ORDER (Southern ->
// Middle -> Northern) builds a stack of Arcane Charge (900005). The current stack count
// is the progress, so it also selects the only crystal that advances -- a wrong or
// out-of-order crystal simply does nothing. At 3 stacks the buff is consumed and the
// player receives Spell Notes: Arcane Blast (211691), which unlocks the Hands rune.
class player_sod_mage_arcane_crystals : public PlayerScript
{
public:
    player_sod_mage_arcane_crystals() : PlayerScript("player_sod_mage_arcane_crystals") {}

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell || !SodMageEnabled())
            return;

        SpellInfo const* info = spell->GetSpellInfo();
        if (!info)
            return;

        // Only player Arcane Explosion ranks drive the discovery.
        if (std::find(std::begin(SPELL_MAGE_ARCANE_EXPLOSION_RANKS),
                      std::end(SPELL_MAGE_ARCANE_EXPLOSION_RANKS),
                      info->Id) == std::end(SPELL_MAGE_ARCANE_EXPLOSION_RANKS))
            return;

        Aura* charge = player->GetAura(SPELL_SOD_MAGE_ARCANE_CHARGE);
        uint8 const stacks = charge ? charge->GetStackAmount() : 0;
        if (stacks >= 3)
            return;

        // Already earned the notes -- don't re-run the discovery.
        if (player->HasItemCount(ITEM_SOD_MAGE_ARCANE_BLAST_NOTES, 1, true))
            return;

        // The next crystal in sequence must be nearby (strict order from the stack count).
        uint32 const expected = GO_SOD_MAGE_ARCANE_CRYSTALS[stacks];
        if (!player->FindNearestGameObject(expected, SodMageArcaneCrystalsRange()))
            return;

        player->CastSpell(player, SPELL_SOD_MAGE_ARCANE_CHARGE, true); // adds one stack

        Aura* updated = player->GetAura(SPELL_SOD_MAGE_ARCANE_CHARGE);
        if (updated && updated->GetStackAmount() >= 3)
        {
            player->RemoveAurasDueToSpell(SPELL_SOD_MAGE_ARCANE_CHARGE);
            player->AddItem(ITEM_SOD_MAGE_ARCANE_BLAST_NOTES, 1);
        }
    }
};

void AddSC_player_sod_mage_arcane_crystals()
{
    new player_sod_mage_arcane_crystals();
}
