# Testing

The module ships Google Test unit tests that run inside AzerothCore's existing
`unit_tests` target — no core edits, and they only build when you ask for them.

## What's covered

These spells are mechanics-heavy `AuraScript`s glued to the game runtime (`Unit`,
`SpellInfo`, `DamageInfo`, casting, config, the beacon registry), which isn't
constructable in the unit harness. The one piece of genuinely pure, bug-prone
logic is the **Temporal Beacon conversion math** — the percentage chain that turns
the Mage's Arcane damage into healing — so that's what's extracted and tested.

`namespace SodMageRules` (`src/spell_sod_mage_rules.h`) holds it:

- `BeaconHeal(damage, conversionPct, multiTarget, multiTargetPct)` — `conversionPct`
  of the damage, then reduced to `multiTargetPct` for a multi-target (AoE) Arcane
  spell.
- `BeaconSelfHeal(heal, selfPct)` — the reduction applied when the caster heals
  itself through its own beacon.

`spell_sod_mage_temporal_conversion::HandleProc` delegates to these, so the tests
exercise the production code. The tests cover the conversion/multi-target/self
chain, ordering, edge values (0 damage, 0%/100% conversion), `CalculatePct`'s
floor-toward-zero, the shipped-default scenario (70/20/50), and the beacon-duration
contract (Mass Regeneration's beacon is half of Regeneration's).

### What's *not* unit-tested (and why)

Everything else: the proc firing (`CheckProc`'s Arcane filter), the caster→beacon
registry, applying Temporal Beacon, and casting Chronomantic Healing all need a
live `Unit`/`SpellInfo`/database. Those stay covered by in-game verification
([Deploy & verify](deploy-and-verify.md)).

## How it's wired

- `mod-sod-mage.cmake` (included inline by `modules/CMakeLists.txt`) appends the
  test source and the `src/` include dir to the global `ACORE_MODULE_TEST_SOURCES`
  / `ACORE_MODULE_TEST_INCLUDES` properties — only when `BUILD_TESTING` is on.
- The core's `src/test/CMakeLists.txt` adds those sources to `unit_tests` and links
  the `modules` library, so the tests compile and link against the module.
- Test sources live in `tests/` (a sibling of `src/`), so they are **not** compiled
  into the `modules` library itself — only into `unit_tests`.

## Building and running

If you run the server via the AzerothCore Docker stack, the easiest path is the
`ac-dev-server` container (compose profile `dev`), which builds in dedicated Docker
volumes — isolated from your live `ac-worldserver`/`ac-database` and leaving no
build files in your project folder:

```bash
# from the repo root
docker compose --profile dev run --rm ac-dev-server \
  bash modules/mod-sod-mage/tests/run-in-docker.sh        # all cores
docker compose --profile dev run --rm ac-dev-server \
  bash modules/mod-sod-mage/tests/run-in-docker.sh 4      # -j4, leaves host headroom
```

Or build directly with a local toolchain:

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DSCRIPTS=static -DMODULES=static
cmake --build build --target unit_tests -j"$(nproc)"
./build/src/test/unit_tests --gtest_filter='SodMage*'
```

Notes:

- Building `unit_tests` compiles the full `game` + `modules` libraries (it links
  them), so the **first** run is a heavy compile; ccache + the persisted build
  volume make re-runs fast.
- `--gtest_filter='SodMage*'` runs just this module's cases; drop it to run the
  whole suite. When configured you'll see `mod-sod-mage: registered unit tests`.
