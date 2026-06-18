# mod-sod-mage

An [AzerothCore](https://www.azerothcore.org/) module that adds custom Mage spells
inspired by **Season of Discovery** (SoD) Classic, for the WotLK **3.3.5a** client
(build 12340).

## Goal

Recreate SoD-style Mage abilities on a 3.3.5a server. Because the 3.3.5a client
can't be taught brand-new spells at runtime, each ability is delivered as **two
halves that stay in sync**: server-side data + scripts, and a small **client patch
(MPQ)** that lets the client render the spell. A generator tool produces both from
one definition.

## Current state — early / proof of concept

Part of the SoD "Chronomancy" healing kit is implemented and working in-game:

- **Regeneration** — a channeled 3s heal-over-time that applies **Temporal Beacon**.
- **Mass Regeneration** — the AoE version: a channeled 3s heal-over-time on the
  target and their party, applying a (shorter) Temporal Beacon to each.
- **Temporal Beacon** — a 30s buff that converts a share of the Mage's **Arcane**
  damage into healing on the beacon target (reduced on self and for multi-target
  spells), plus a small passive heal-over-time.

Custom icons, visuals (the Regeneration spells reuse Drain Mana's beam; Temporal
Beacon reuses Lightning Shield's orbiting effect), and tooltips are in place.
Healing values match SoD where implemented (e.g. Regeneration's 165%-of-healing-
power scaling); no broader balance pass yet. More of the kit (Chronostatic
Preservation, Rewind Time, Temporal Anomaly) is documented and planned.

## Install

You need both halves. Server first, then the client patch.

### Server

1. Place this module at `modules/mod-sod-mage/` in your AzerothCore source tree.
2. Build the worldserver with modules enabled (`-DMODULES=static`) — modules are
   compiled into `worldserver`, not built separately.
3. Apply the base SQL to your **`acore_world`** database:
   ```bash
   mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_spell_dbc.sql
   mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_runes.sql
   mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_mass_regeneration.sql
   mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_regeneration_unlock.sql
   ```
   (Idempotent — safe to re-run. `sod_mage_spell_dbc.sql` seeds `spell_dbc`,
   `spell_script_names`, `spell_proc`. `sod_mage_runes.sql` registers the spells
   as engravable runes and is a **no-op unless** the optional rune engine is
   installed. `sod_mage_mass_regeneration.sql` and `sod_mage_regeneration_unlock.sql`
   add the two SoD rune-unlock chains (items/loot work standalone; only the rune
   mappings are engine-guarded). The Mass Regeneration chain also needs the shared
   [`mod-sod-world`](../mod-sod-world) module, which provides the Awakened Lich
   encounter — see below.)
4. Restart the worldserver (these tables load at startup).

### Client patch (required — otherwise the spells won't render)

The spells use IDs the stock client doesn't have, so the client needs a patch MPQ.
Generate it against your own client (it reuses stock icons/visuals — no custom art):

```bash
pip install pympq                       # StormLib binding for MPQ read/write
# Fully close the WoW client first (a running client locks its MPQs):
python modules/mod-sod-mage/tools/build_sod_mage_patch.py --client "<path to your WoW 3.3.5a client>"
```

This writes `patch-enus-z.mpq` (spell DBCs) into the client's `Data/enus` folder
and (re)writes the server spell SQL. It assumes an **enUS** client; adjust for
other locales.

The module's **custom items** (Comprehension Charm, the Spell Notes) also need a
client `Item.dbc` row, or they show a red "?" icon in bags. That row ships in the
**shared item patch built by [`mod-sod-world`](../mod-sod-world)** — one
consolidated `Item.dbc` for all SoD modules (WoW replaces whole DBCs per patch, so
the rows can't be split across MPQs). Run that module's
`tools/build_sod_world_patch.py` too; this module contributes its items via
`tools/client_items.json`. Launch the client after generating both patches.

### Use it

No trainer entry yet — learn the spells directly:

```
.learn 401417    # Regeneration
.learn 412510    # Mass Regeneration
```

### Optional: engravable runes (`mod-rune-engraving`)

This module's spells can also be acquired as **engravable runes** if the reusable
[`mod-rune-engraving`](../mod-rune-engraving) engine is installed alongside it.
`sod_mage_runes.sql` writes this module's runes into the engine's catalog,
guarded so it's a harmless no-op when the engine is absent — `mod-sod-mage`
installs and works fine on its own (spells via `.learn`).

With the engine present, the two runes demonstrate both of the engine's
**earned-rune** paths:

- **Regeneration** (Chest slot) is **item-unlocked** via the faithful SoD chain
  (`sod_mage_regeneration_unlock.sql`): buy a **Comprehension Charm** from any
  reagent vendor, kill **Defias Pillager / Defias Renegade Mage / Dalaran
  Apprentice** for **Spell Notes: TENGI RONEERA**, use them with the charm to make
  **Spell Notes: Regeneration**, then use those to unlock the rune. Without the
  engine the items/combine still exist but the final unlock no-ops.
- **Mass Regeneration** (Legs slot) is **drop-gated**
  (`sod_mage_mass_regeneration.sql`), faithful to SoD: kill the **Awakened Lich**
  in Raven Hill for **Spell Notes: Mass Regeneration**, then use them to unlock the
  rune. The Lich encounter (Dusty Coffer → Decrepit Phylactery → summon) is shared
  content provided by [`mod-sod-world`](../mod-sod-world).

This module owns the rune-id band **7000000–7000999** in the engine's catalog.
Its items and the Lich drop use the **real SoD ids** (Comprehension Charm
`211779`, the Spell Notes `208754`/`208753`/`211514`); the Awakened Lich
(`212261`) is owned by [`mod-sod-world`](../mod-sod-world).

## Documentation

Developer docs (architecture, the generator, adding spells, deploy/verify,
gotchas) live in [`docs/`](docs/Home.md) and are mirrored to the project wiki.
Start at [docs/Home.md](docs/Home.md).

## Conventions

No core edits — everything stays under `modules/mod-sod-mage/`.

## License

GPL v2 (inherited from AzerothCore). See [LICENSE](LICENSE).
