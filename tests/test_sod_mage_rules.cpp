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
