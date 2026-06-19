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
#include "spell_sod_mage_rules.h"
#include "Chat.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "ItemScript.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Random.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include <cmath>

// The Alliance acquisition of the Enlightenment rune (Servant of Azora event).
//
//   1. Odd critters carrying Wild Polymorph (23603) wander eastern Elwynn near
//      Ridgepoint Tower. They are custom creatures (npc_sod_mage_wild_critter,
//      700210, "Polymorphed Apprentice") shown with a random non-Elwynn critter
//      model -- a Beast so Polymorph can land.
//   2. Casting Polymorph reveals the apprentice: the SAME creature clears its auras
//      (cancelling the sheep morph), snaps to a human model, and poofs -- so it
//      reads as the Polymorph transforming it (no despawn/summon, no death fade).
//   3. The apprentice thanks the player, plays the Blink visual in place, and fades
//      out, summoning a paper gameobject (700220) on the ground.
//   4. Clicking the paper grants the next Azora Apprentice Notes page the player
//      lacks (203756 / 203960 / 203961 / 203962).
//   5. Using any page while holding all four assembles Spell Notes: Enlightenment
//      (203749), whose use unlocks the rune via the engine's item_rune_unlock.
//
// All ids are real SoD item ids (wago.tools); the creatures/GO are custom (no SoD
// creature id is sourceable -- creatures are server-side, absent from client DB2s).

enum SodMageAzora
{
    NPC_SOD_MAGE_POLY_CRITTER          = 700210, // disguised critter -> apprentice
    GO_SOD_MAGE_AZORA_PAGE             = 700220, // the paper dropped on reveal

    SPELL_SOD_MAGE_WILD_POLYMORPH      = 23603,  // the debuff (applied permanent)
    SPELL_SOD_MAGE_POOF                = 12244,  // "Poof" -- the dust-cloud on reveal
    // Bijou Vanish reuses Blink's visual (263) with a dummy effect: the Blink
    // animation with NO movement (real Blink/1953 would teleport the apprentice).
    SPELL_SOD_MAGE_BLINK_VISUAL        = 15993,

    ITEM_SOD_MAGE_NOTES_ENLIGHTENMENT  = 203749, // assembled Spell Notes

    ZONE_ELWYNN_FOREST                 = 12,     // where the disguised critters live
    // On reveal the creature switches from the critter faction (188) to a normal
    // "friendly" NPC faction so the client renders its overhead speech bubble (and
    // it's non-attackable) instead of treating it as an ambient critter.
    FACTION_SOD_MAGE_FRIENDLY          = 35,
};

// The four Azora Apprentice Notes pages, in order (real SoD ids).
static constexpr uint32 SOD_MAGE_AZORA_PAGES[] = { 203756, 203960, 203961, 203962 };

// The disguise pool: random non-Elwynn critter displays the critter wears. Set in
// script (a creature is capped at 4 DBC models, and Wild Polymorph (23603) would
// otherwise force its own transform). Ram, Gazelle, Fire Beetle, Parrot, Cat.
static constexpr uint32 SOD_MAGE_CRITTER_MODELS[] = { 10000, 1547, 8971, 8816, 5555 };

// The revealed form: Servant of Azora's (1949) human apprentice models. Set via
// SetDisplayId on reveal (no creature_template_model row needed).
static constexpr uint32 SOD_MAGE_APPRENTICE_MODELS[] = { 5015, 5016, 5017, 5018 };

// Pick a random display from a fixed model pool.
template <size_t N>
static uint32 PickModel(uint32 const (&models)[N])
{
    return models[urand(0, uint32(N) - 1)];
}

// creature_text group: the apprentice's "I'm back to my old self!" line.
constexpr uint8 SAY_SOD_MAGE_APPRENTICE = 0;

// How long an unclicked paper lingers before self-cleaning (seconds).
constexpr uint32 SOD_MAGE_AZORA_PAGE_DESPAWN_SECS = 300;

// 700210: the disguised critter AND, after Polymorph, the revealed apprentice --
// one creature, two states. Keeping it a single object means the reveal is a
// seamless in-place transform (Polymorph -> apprentice) with no despawn/summon
// death-fade. Any Polymorph rank/variant counts (matched by mechanic, not id).
struct npc_sod_mage_wild_critter : public ScriptedAI
{
    npc_sod_mage_wild_critter(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _disguised = false; // re-roll the disguise on (re)spawn
        _revealed = false;
        _phase = 0;
        _timer = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_disguised)
        {
            _disguised = true;

            // Wild Polymorph as a PERMANENT debuff (its DBC duration would expire
            // and revert the disguise). It also transforms the model, so we override
            // it right after with a named critter display -- a permanent aura never
            // restores, so our model sticks.
            if (Aura* poly = me->AddAura(SPELL_SOD_MAGE_WILD_POLYMORPH, me))
            {
                poly->SetMaxDuration(-1);
                poly->SetDuration(-1);
            }
            me->SetDisplayId(PickModel(SOD_MAGE_CRITTER_MODELS));
            me->SetFullHealth(); // converted critters have a tiny max HP -- full bar
            return;
        }

        if (!_revealed) // disguised and waiting to be Polymorphed
            return;

        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }

        switch (_phase)
        {
            case 0: // greet -- delayed a beat after the transform so the client has
                    // settled the (re-faction'd) NPC and renders the overhead bubble
                Talk(SAY_SOD_MAGE_APPRENTICE);
                _timer = 2200;
                _phase = 1;
                break;
            case 1: // the Blink animation, in place (no movement)
                DoCastSelf(SPELL_SOD_MAGE_BLINK_VISUAL, true);
                _timer = 1200;
                _phase = 2;
                break;
            case 2: // drop the paper where we stand, then disappear
                // GO_SUMMON_TIMED_DESPAWN: the paper lives on its own timer and is
                // NOT owner-tracked, so it survives the despawn on the next line.
                me->SummonGameObject(GO_SOD_MAGE_AZORA_PAGE,
                    me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(),
                    me->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f,
                    SOD_MAGE_AZORA_PAGE_DESPAWN_SECS, true, GO_SUMMON_TIMED_DESPAWN);
                me->DespawnOrUnsummon();
                _phase = 3;
                break;
            default: // done; awaiting despawn
                break;
        }
    }

    void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
    {
        if (_revealed || !_disguised || !SodMageEnabled() || !spell)
            return;

        bool isPolymorph = spell->Mechanic == MECHANIC_POLYMORPH
            || (spell->GetAllEffectsMechanicMask() & (1ULL << MECHANIC_POLYMORPH)) != 0;
        if (!isPolymorph)
            return;

        _revealed = true;

        // Transform in place: clear the Polymorph + Wild Polymorph auras so no sheep
        // morph plays, switch from the critter faction to a friendly NPC (so the
        // client shows its overhead bubble), snap to a human model, stop, and poof --
        // it reads as the Polymorph turning the critter into the apprentice. The
        // greeting is said a beat later (phase 0) once the client has settled.
        me->RemoveAllAuras();
        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToAll(true);
        me->SetFaction(FACTION_SOD_MAGE_FRIENDLY);
        me->SetDisplayId(PickModel(SOD_MAGE_APPRENTICE_MODELS));
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MoveIdle();
        me->CastSpell(me, SPELL_SOD_MAGE_POOF, true);

        _phase = 0;
        _timer = 500; // brief beat, then greet (phase 0) -> blink -> drop
    }

private:
    bool _disguised = false;
    bool _revealed = false;
    uint8 _phase = 0;
    uint32 _timer = 0;
};

// 700220: the paper on the ground. On click, hand a Mage the lowest page they're
// still missing, then despawn. Non-Mages get nothing (the chain is Mage-only).
class go_sod_mage_azora_page : public GameObjectScript
{
public:
    go_sod_mage_azora_page() : GameObjectScript("go_sod_mage_azora_page") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!player || !go)
            return true;

        if (!SodMageEnabled() || !player->IsClass(CLASS_MAGE, CLASS_CONTEXT_ABILITY))
            return true; // not for this player -- leave the paper untouched

        ChatHandler handler(player->GetSession());

        uint32 next = 0;
        for (uint32 page : SOD_MAGE_AZORA_PAGES)
            if (!player->HasItemCount(page, 1, true))
            {
                next = page;
                break;
            }

        if (!next)
        {
            handler.SendSysMessage(
                "|cFF00FF00[Azora's Notes]|r You have already copied every page.");
            return true;
        }

        player->AddItem(next, 1);
        go->Delete(); // remove now (DespawnOrUnsummon won't delete a summoned GO)
        return true;
    }
};

// The four pages share this combine script: using any one while holding all four
// consumes them and yields Spell Notes: Enlightenment. Mirrors the decode-notes
// inventory pattern (no rune-engine call here -- the engine unlock is on 203749).
class item_sod_mage_azora_pages : public ItemScript
{
public:
    item_sod_mage_azora_pages() : ItemScript("item_sod_mage_azora_pages") { }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/) override
    {
        if (!player)
            return false;

        ChatHandler handler(player->GetSession());

        for (uint32 page : SOD_MAGE_AZORA_PAGES)
            if (!player->HasItemCount(page, 1, true))
            {
                handler.SendSysMessage(
                    "|cFFFFD700[Azora's Notes]|r The notes are incomplete. Collect "
                    "all four pages, then read them together.");
                return true; // handled -- suppress the benign use-spell
            }

        for (uint32 page : SOD_MAGE_AZORA_PAGES)
            player->DestroyItemCount(page, 1, true);

        player->AddItem(ITEM_SOD_MAGE_NOTES_ENLIGHTENMENT, 1);
        handler.SendSysMessage(
            "|cFF00FF00[Azora's Notes]|r The pages align into "
            "|cFFFFD700Spell Notes: Enlightenment|r.");
        return true;
    }
};

// Scatters the disguised critters across Elwynn without touching DB spawns: when a
// real Elwynn critter is added to the world, a configurable share are converted
// in place to our critter (UpdateEntry adopts our template + AI). The original
// entry is untouched in the DB, so on respawn it returns as a normal critter and
// the roll happens again -- net critter population is unchanged.
class sod_mage_critter_seeder : public AllCreatureScript
{
public:
    sod_mage_critter_seeder() : AllCreatureScript("sod_mage_critter_seeder") { }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!creature || !SodMageEnabled())
            return;

        // Cheap rejects first; GetZoneId() (a terrain lookup) is checked last.
        if (creature->GetEntry() == NPC_SOD_MAGE_POLY_CRITTER || creature->IsSummon())
            return;
        if (creature->GetMapId() != 0 || !creature->IsCritter())
            return;
        if (creature->GetZoneId() != ZONE_ELWYNN_FOREST)
            return;

        // Chance scales with distance from the Tower of Azora: dense near the
        // tower (apprentices belong to Azora), thinning out across Elwynn.
        float cx = sConfigMgr->GetOption<float>(
            "SodMage.Enlightenment.CritterCenterX", -9528.949f);
        float cy = sConfigMgr->GetOption<float>(
            "SodMage.Enlightenment.CritterCenterY", -691.179f);
        float inner = sConfigMgr->GetOption<float>(
            "SodMage.Enlightenment.CritterInnerRadius", 50.0f);
        float outer = sConfigMgr->GetOption<float>(
            "SodMage.Enlightenment.CritterOuterRadius", 600.0f);
        uint32 maxPct = sConfigMgr->GetOption<uint32>(
            "SodMage.Enlightenment.CritterChanceMaxPct", 15);
        uint32 minPct = sConfigMgr->GetOption<uint32>(
            "SodMage.Enlightenment.CritterChanceMinPct", 5);

        float dx = creature->GetPositionX() - cx;
        float dy = creature->GetPositionY() - cy;
        float dist = std::sqrt(dx * dx + dy * dy);

        uint32 chance =
            SodMageRules::SeedChanceForDistance(dist, inner, outer, maxPct, minPct);
        if (!roll_chance_i(int32(chance)))
            return;

        // Adopt our template + AI in place; npc_sod_mage_wild_critter does the rest.
        creature->UpdateEntry(NPC_SOD_MAGE_POLY_CRITTER, true);
        // Our template's max HP is tiny -- top it off so the swap doesn't show the
        // bar draining from the original critter's health to a sliver.
        creature->SetFullHealth();
    }
};

void AddSC_sod_mage_azora_event()
{
    RegisterCreatureAI(npc_sod_mage_wild_critter);
    new go_sod_mage_azora_page();
    new item_sod_mage_azora_pages();
    new sod_mage_critter_seeder();
}
