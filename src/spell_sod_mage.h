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
    // Enlightenment (Chest rune). 412324 is the passive driver the rune teaches;
    // it polls mana % and toggles two mutually-exclusive sub-auras. SoD's
    // "Gain the Enlightenment ability" entry (415729) points at 412324 — we teach
    // 412324 directly, so 415729 has no server-side row.
    SPELL_SOD_MAGE_ENLIGHTENMENT        = 412324, // passive driver (periodic dummy)
    SPELL_SOD_MAGE_ENLIGHTENMENT_DAMAGE = 412326, // high-mana: +10% spell damage
    SPELL_SOD_MAGE_ENLIGHTENMENT_REGEN  = 412325, // low-mana: regen through the FSR
    SPELL_SOD_MAGE_ARCANE_SURGE         = 425124, // instant nuke: consume all mana, scale dmg
    SPELL_SOD_MAGE_ARCANE_BURST         = 400574, // rune Arcane Blast stand-in (capped)
    SPELL_SOD_MAGE_ARCANE_BLAST_RUNE    = 900003, // hidden driver: Burst <-> Nether Vortex
    SPELL_SOD_MAGE_NETHER_VORTEX        = 900004, // proc aura: Slow on real Arcane Blast
    SPELL_SOD_MAGE_ARCANE_CHARGE        = 900005, // Zoram Strand discovery progress buff
    // Rewind Time (Wrist rune). 401462 is the active ability the SoD rune-grant
    // (401734) teaches: an instant heal on a beaconed ally for the damage they took
    // over the last few seconds. See spell_sod_mage_rewind_time.cpp.
    SPELL_SOD_MAGE_REWIND_TIME          = 401462, // beacon-gated "rewind" heal
    // Living Bomb (Hands rune). 900006 is the hidden driver the rune teaches: it grants
    // the Living Spark stand-in (900007 cast -> 900008 explosion) until the player learns
    // the real WotLK Living Bomb, then a cast-redirect fires a Scorch-tagged copy. The
    // copy is PER-RANK (900009-900014, below) so Master of Elements' rank lookup resolves;
    // see spell_sod_mage_living_bomb_* / _living_spark.
    SPELL_SOD_MAGE_LIVING_BOMB_RUNE      = 900006, // hidden driver: grants Living Spark
    SPELL_SOD_MAGE_LIVING_SPARK          = 900007, // stand-in: no-DoT bomb, no aggro until boom
    SPELL_SOD_MAGE_LIVING_SPARK_BOOM     = 900008, // Living Spark explosion (plain Fire)
};

// The Scorch-tagged Living Bomb copy, mirrored across the real spell's three ranks so the
// core's Master of Elements (Mage + icon 3000 + no mana -> GetSpellWithRank(44457, rank))
// resolves the matching real-Living-Bomb rank instead of crashing on an out-of-range one.
// Index i lines up with SPELL_MAGE_LIVING_BOMB_RANKS[i]: cast real LB Rx -> COPY_DOTS[x]
// -> (on expire) COPY_BOOMS[x]. The two id lists are wired into spell_ranks (R1/R2/R3) in
// data/sql/db-world/base/sod_mage_living_bomb.sql. All ranks deal the same level-curve
// damage; the ranks exist only for the core's rank mapping.
constexpr uint32 SOD_MAGE_LIVING_BOMB_COPY_DOTS[]  = { 900009, 900010, 900011 };
constexpr uint32 SOD_MAGE_LIVING_BOMB_COPY_BOOMS[] = { 900012, 900013, 900014 };

// Core spells the Arcane Blast rune keys on (not ours). The real Arcane Blast ranks
// (trainable from level 64) decide the rune's mode; Slow is what Nether Vortex applies.
constexpr uint32 SPELL_MAGE_ARCANE_BLAST_RANKS[] = { 30451, 42894, 42896, 42897 };
constexpr uint32 SPELL_MAGE_SLOW = 31589;

// The real WotLK Living Bomb ranks the Living Bomb rune keys on. While the player knows
// none, the driver grants Living Spark; once they do, the cast-redirect (bound to these
// ids) swaps their Living Bomb for the Scorch-tagged copy.
constexpr uint32 SPELL_MAGE_LIVING_BOMB_RANKS[] = { 44457, 55359, 55360 };

// Zoram Strand crystal discovery (Arcane Blast rune acquisition). Casting any player
// Arcane Explosion rank near the next crystal in order builds Arcane Charge; at 3 stacks
// the player gets Spell Notes: Arcane Blast (211691), which unlocks the Hands rune.
constexpr uint32 SPELL_MAGE_ARCANE_EXPLOSION_RANKS[] =
    { 1449, 8437, 8438, 8439, 10201, 10202, 27080, 27082, 42920, 42921 };
constexpr uint32 ITEM_SOD_MAGE_ARCANE_BLAST_NOTES = 211691;
// The three crystals in sequence: index 0 = Southern (charged at 0 stacks), 1 = Middle,
// 2 = Northern. Custom GO band 701100-701199 (sod-world owns 701000-701099).
constexpr uint32 GO_SOD_MAGE_ARCANE_CRYSTALS[] = { 701100, 701101, 701102 };

// How close (yards) to a crystal you must Arcane Explosion to charge it.
inline float SodMageArcaneCrystalsRange()
{
    return sConfigMgr->GetOption<float>("SodMage.ArcaneCrystals.Range", 15.0f);
}

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

// Arcane Burst stops scaling at the level the real Arcane Blast becomes trainable (64),
// so the rune stand-in never out-scales the real spell. Admin-tunable.
inline uint32 SodMageArcaneBlastCapLevel()
{
    return sConfigMgr->GetOption<uint32>("SodMage.ArcaneBlast.ScalingCapLevel", 64);
}

// Living Spark (the Living Bomb stand-in) stops scaling at the SoD Living Bomb level
// (60) so it never out-scales the real spell; admin-tunable. The fuse is how long a
// planted Spark waits before it explodes (SoD Living Bomb is 12s).
inline uint32 SodMageLivingBombCapLevel()
{
    return sConfigMgr->GetOption<uint32>("SodMage.LivingBomb.ScalingCapLevel", 60);
}

inline uint32 SodMageLivingBombFuseMs()
{
    return sConfigMgr->GetOption<uint32>("SodMage.LivingBomb.FuseSeconds", 12) * 1000;
}

// Enlightenment tunables. The high/low mana thresholds gate the two sub-auras;
// PollMs throttles how often the passive re-evaluates state (on top of its 1s
// periodic-dummy tick) — 5000 keeps SoD's cadence while staying admin-tunable.
inline uint32 SodMageEnlightenmentHighPct()
{
    return sConfigMgr->GetOption<uint32>("SodMage.Enlightenment.HighManaPct", 70);
}

inline uint32 SodMageEnlightenmentLowPct()
{
    return sConfigMgr->GetOption<uint32>("SodMage.Enlightenment.LowManaPct", 30);
}

inline uint32 SodMageEnlightenmentPollMs()
{
    return sConfigMgr->GetOption<uint32>("SodMage.Enlightenment.PollMs", 5000);
}

// Rewind Time lookback (ms). Both the damage window summed by the heal and the
// minimum beacon age required for it to take effect ("had a Temporal Beacon 5 sec
// ago"). SoD is 5s; admin-tunable. Config is in whole seconds.
inline uint32 SodMageRewindTimeWindowMs()
{
    return sConfigMgr->GetOption<uint32>("SodMage.RewindTime.WindowSeconds", 5) * 1000;
}

// Drops a unit's Rewind Time damage log (defined in spell_sod_mage_rewind_time.cpp).
// Called when a unit's last Temporal Beacon falls off, so the log stays bounded to
// currently-beaconed units.
void SodMageForgetRewindLog(ObjectGuid const& guid);

#endif // MODULE_SOD_MAGE_H
