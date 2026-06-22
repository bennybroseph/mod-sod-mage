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

// Unit tests for the Temporal Beacon conversion math (the percentage chain that
// turns the Mage's Arcane damage into healing). These run in the core's
// unit_tests target (registered via mod-sod-mage.cmake) and touch no game state.
// The percentage values are chosen so CalculatePct's intermediate (base * pct) is
// an exact multiple of 100, keeping the float results stable.

#include "spell_sod_mage.h"        // beacon-duration constants
#include "spell_sod_mage_rules.h"  // the logic under test
#include "gtest/gtest.h"

using namespace SodMageRules;

// --- BeaconHeal: conversion%, then multi-target reduction ---
TEST(SodMageBeaconHeal, ConversionOnly)
{
    EXPECT_EQ(BeaconHeal(100, 70, /*multi*/ false, 20), 70);
    EXPECT_EQ(BeaconHeal(200, 70, false, 20), 140);
}

TEST(SodMageBeaconHeal, MultiTargetReduces)
{
    // 100 -> 70 (conversion) -> 14 (20% multi-target)
    EXPECT_EQ(BeaconHeal(100, 70, /*multi*/ true, 20), 14);
}

TEST(SodMageBeaconHeal, EdgeValues)
{
    EXPECT_EQ(BeaconHeal(0, 70, false, 20), 0);     // no damage
    EXPECT_EQ(BeaconHeal(100, 0, false, 20), 0);    // 0% conversion
    EXPECT_EQ(BeaconHeal(100, 100, false, 20), 100); // full conversion
    EXPECT_EQ(BeaconHeal(100, 70, true, 0), 0);     // multi-target reduced to 0%
}

TEST(SodMageBeaconHeal, TruncatesTowardZero)
{
    // CalculatePct floors: 10 * 25 / 100 = 2.5 -> 2
    EXPECT_EQ(BeaconHeal(10, 25, false, 20), 2);
}

// --- BeaconSelfHeal: the caster's self-heal reduction ---
TEST(SodMageBeaconSelfHeal, Reduces)
{
    EXPECT_EQ(BeaconSelfHeal(70, 50), 35);
    EXPECT_EQ(BeaconSelfHeal(70, 100), 70); // unreduced
    EXPECT_EQ(BeaconSelfHeal(70, 0), 0);    // fully suppressed
}

// --- Composition: matches HandleProc (conversion -> multi -> self) ---
TEST(SodMageBeaconScenario, DefaultConfig)
{
    // Shipped defaults: ConversionPct 70, MultiTargetPct 20, SelfPct 50.
    int32 single = BeaconHeal(100, 70, false, 20);
    int32 multi  = BeaconHeal(100, 70, true, 20);
    EXPECT_EQ(single, 70);                       // single-target, other player
    EXPECT_EQ(multi, 14);                        // multi-target, other player
    EXPECT_EQ(BeaconSelfHeal(single, 50), 35);   // single-target, self
    EXPECT_EQ(BeaconSelfHeal(multi, 50), 7);     // multi-target, self
}

// --- Beacon duration contract (the SoD relationship) ---
TEST(SodMageBeaconContract, MassRegenBeaconIsHalfOfRegen)
{
    EXPECT_EQ(SOD_MAGE_BEACON_MS_REGEN, 30000);
    EXPECT_EQ(SOD_MAGE_BEACON_MS_MASS_REGEN, 15000);
    EXPECT_EQ(SOD_MAGE_BEACON_MS_REGEN, 2 * SOD_MAGE_BEACON_MS_MASS_REGEN);
}

// --- Level-curve base values (so the spells work with no gear). Expected values
// computed from the wago/wowhead tooltip polynomials; chosen levels land well away
// from integer boundaries so float truncation is stable across compilers. ---
TEST(SodMageLevelCurves, LivingFlameTickDamage)
{
    EXPECT_EQ(LivingFlameTickDamage(1), 13);
    EXPECT_EQ(LivingFlameTickDamage(60), 173);
}

TEST(SodMageLevelCurves, RegenTickHeal)
{
    // Per tick (3 ticks over 3 sec); ~370/tick -> ~1110 total at level 60.
    EXPECT_EQ(RegenTickHeal(1), 21);
    EXPECT_EQ(RegenTickHeal(60), 370);
}

TEST(SodMageLevelCurves, ArcaneSurgeDamage)
{
    // Living Flame's curve x[2.26 .. 2.64] (the base before mana scaling).
    EXPECT_EQ(ArcaneSurgeMinDamage(1), 31);
    EXPECT_EQ(ArcaneSurgeMaxDamage(1), 36);
    EXPECT_EQ(ArcaneSurgeMinDamage(64), 442);
    EXPECT_EQ(ArcaneSurgeMaxDamage(64), 516);
    EXPECT_LT(ArcaneSurgeMinDamage(64), ArcaneSurgeMaxDamage(64));
    EXPECT_GT(ArcaneSurgeMinDamage(64), ArcaneSurgeMinDamage(1)); // scales with level
}

TEST(SodMageLevelCurves, ArcaneBurstDamage)
{
    // Living Flame's curve x[4.53 .. 5.27]. The SpellScript caps the level it feeds
    // in at the real Arcane Blast learn level (64); the curve itself is unbounded, so
    // 64 is the value used at and above the cap.
    EXPECT_EQ(ArcaneBurstMinDamage(21), 152);
    EXPECT_EQ(ArcaneBurstMaxDamage(21), 177);
    EXPECT_EQ(ArcaneBurstMinDamage(64), 886);
    EXPECT_EQ(ArcaneBurstMaxDamage(64), 1031);
    EXPECT_LT(ArcaneBurstMinDamage(64), ArcaneBurstMaxDamage(64));
    EXPECT_GT(ArcaneBurstMinDamage(64), ArcaneBurstMinDamage(21)); // scales with level
}

TEST(SodMageLevelCurves, ArcaneBurstHitsHarderThanSurge)
{
    // Arcane Blast (Burst, x4.53-5.27) is a bigger base nuke than Arcane Surge
    // (x2.26-2.64) at the same level (before Surge's mana multiplier).
    EXPECT_GT(ArcaneBurstMinDamage(64), ArcaneSurgeMinDamage(64));
    EXPECT_GT(ArcaneBurstMaxDamage(64), ArcaneSurgeMaxDamage(64));
}

// --- Enlightenment mana-state gating (the two binary thresholds) ---
TEST(SodMageEnlightenment, ShippedDefaults)
{
    // Defaults: high 70, low 30. Strict inequalities, so 70 and 30 are *not* in.
    EXPECT_EQ(EnlightenmentStateFor(100, 70, 30), EnlightenmentState::HighMana);
    EXPECT_EQ(EnlightenmentStateFor(71, 70, 30), EnlightenmentState::HighMana);
    EXPECT_EQ(EnlightenmentStateFor(70, 70, 30), EnlightenmentState::None);
    EXPECT_EQ(EnlightenmentStateFor(50, 70, 30), EnlightenmentState::None);
    EXPECT_EQ(EnlightenmentStateFor(30, 70, 30), EnlightenmentState::None);
    EXPECT_EQ(EnlightenmentStateFor(29, 70, 30), EnlightenmentState::LowMana);
    EXPECT_EQ(EnlightenmentStateFor(0, 70, 30), EnlightenmentState::LowMana);
}

TEST(SodMageEnlightenment, CustomThresholds)
{
    // Admin-tuned band stays mutually exclusive at the edges.
    EXPECT_EQ(EnlightenmentStateFor(91, 90, 10), EnlightenmentState::HighMana);
    EXPECT_EQ(EnlightenmentStateFor(90, 90, 10), EnlightenmentState::None);
    EXPECT_EQ(EnlightenmentStateFor(11, 90, 10), EnlightenmentState::None);
    EXPECT_EQ(EnlightenmentStateFor(9, 90, 10), EnlightenmentState::LowMana);
}

// --- Distance-scaled critter seeding (15% near the tower -> 5% far) ---
TEST(SodMageSeedChance, ShippedDefaults)
{
    // inner 50, outer 600, max 15, min 5.
    EXPECT_EQ(SeedChanceForDistance(0.0f, 50, 600, 15, 5), 15u);   // on the tower
    EXPECT_EQ(SeedChanceForDistance(50.0f, 50, 600, 15, 5), 15u);  // at inner edge
    EXPECT_EQ(SeedChanceForDistance(600.0f, 50, 600, 15, 5), 5u);  // at outer edge
    EXPECT_EQ(SeedChanceForDistance(2000.0f, 50, 600, 15, 5), 5u); // far beyond
    // Midpoint of the 50..600 band -> halfway between 15 and 5 = 10.
    EXPECT_EQ(SeedChanceForDistance(325.0f, 50, 600, 15, 5), 10u);
}

TEST(SodMageSeedChance, DegenerateRadii)
{
    // outer <= inner: no gradient, always the max.
    EXPECT_EQ(SeedChanceForDistance(100.0f, 200, 200, 15, 5), 15u);
    EXPECT_EQ(SeedChanceForDistance(100.0f, 300, 200, 15, 5), 15u);
}
