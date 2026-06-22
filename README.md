# mod-sod-mage

An [AzerothCore](https://www.azerothcore.org/) module that adds custom Mage spells
inspired by **Season of Discovery** (SoD) Classic, for the WotLK **3.3.5a** client
(build 12340).

> **Just want to play?** The [**SoD installer**](https://github.com/mod-sod/sod-installer)
> sets this module up for you in one command — it clones everything, builds the
> client patches, and installs the addon. The manual steps below are for building
> from source.

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
- **Arcane Surge** — an instant Arcane nuke that **consumes all your current mana**,
  dealing more damage the more mana you had (up to +300% at full), then an 8s buff
  that triples mana regen and lets it flow through the Five Second Rule. The mana
  scaling and consume-all aren't shown in the client tooltip (script-side behavior).

Custom icons, visuals, and tooltips are in place (the Regeneration spells reuse
Drain Mana's beam; Temporal Beacon reuses Lightning Shield's orbiting effect).

Healing values match SoD where implemented (e.g. Regeneration's
165%-of-healing-power scaling); there's no broader balance pass yet.

More of the kit — Chronostatic Preservation, Rewind Time, Temporal Anomaly — is
documented and planned.

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
   mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_living_flame_unlock.sql
   mysql -u <user> -p acore_world < modules/mod-sod-mage/data/sql/db-world/base/sod_mage_enlightenment_unlock.sql
   ```
   All idempotent — safe to re-run:
   - `sod_mage_spell_dbc.sql` — seeds `spell_dbc`, `spell_script_names`, `spell_proc`.
   - `sod_mage_runes.sql` — registers the spells as engravable runes; a **no-op
     unless** the optional rune engine is installed.
   - `sod_mage_regeneration_unlock.sql` / `sod_mage_living_flame_unlock.sql` /
     `sod_mage_mass_regeneration.sql` / `sod_mage_enlightenment_unlock.sql` — the
     SoD rune-unlock chains. Items, creatures and loot work standalone; only the
     rune mappings are engine-guarded. Mass Regeneration also needs
     [`mod-sod-world`](../mod-sod-world) for the Awakened Lich (see below).
4. Restart the worldserver (these tables load at startup).

### Client patch (required — otherwise the spells won't render)

The spells use IDs the stock client doesn't have, so the client needs a patch MPQ.
It's built by the shared [`sod-client`](https://github.com/mod-sod/sod-client)
pipeline, which consolidates every SoD module's spells and items into **one** patch
(it reuses stock icons/visuals — no custom art):

```bash
pip install pympq                       # StormLib binding for MPQ read/write
# Fully close the WoW client first (a running client locks its MPQs); from a
# sod-client checkout:
python build_patch.py --server "<azerothcore root>" --client "<path to your WoW 3.3.5a client>"
```

This writes the consolidated `patch-z.mpq` into the client's `Data/` folders and
(re)writes this module's server spell SQL. The
[SoD installer](https://github.com/mod-sod/sod-installer) runs it for you.

The module's **custom items** (Comprehension Charm, the Spell Notes) also need a
client `Item.dbc` row, or they show a red "?" icon in bags. Those rows ship in the
**same consolidated patch** built by `sod-client` above — one `Item.dbc` for all
SoD modules (WoW replaces whole DBCs per patch, so the rows can't be split across
MPQs). This module just contributes its items via `tools/client_items.json`; the
single `build_patch.py` run handles both spells and items. Launch the client after
generating the patch.

### Use it

No trainer entry yet — learn the spells directly:

```
.learn 401417    # Regeneration
.learn 412510    # Mass Regeneration
.learn 401556    # Living Flame
.learn 412324    # Enlightenment
.learn 425124    # Arcane Surge
```

### Optional: engravable runes (`mod-rune-engraving`)

This module's spells can also be acquired as **engravable runes** if the reusable
[`mod-rune-engraving`](../mod-rune-engraving) engine is installed alongside it.
`sod_mage_runes.sql` writes this module's runes into the engine's catalog,
guarded so it's a harmless no-op when the engine is absent — `mod-sod-mage`
installs and works fine on its own (spells via `.learn`).

With the engine present, the runes demonstrate both of the engine's
**earned-rune** paths — all faithful to SoD:

- **Regeneration** (Chest) — *item-unlocked.* With a **Comprehension Charm** (sold
  by reagent vendors), kill Defias Pillager / Renegade Mage / Dalaran Apprentice
  for **Spell Notes: TENGI RONEERA**, decode them with the charm, then use the
  result to unlock the rune.
- **Living Flame** (Legs) — *item-unlocked,* same chain. With a charm, kill a
  low-level caster mob (Kobold Geomancer, Frostmane Shadowcaster/Seer, Scarlet
  Warrior/Missionary/Zealot, the Burning Blade pack) for **Spell Notes: MILEGIN
  VALF**, decode, then use to unlock.
- **Mass Regeneration** (Legs) — *drop-gated.* Kill the **Awakened Lich** in Raven
  Hill for **Spell Notes: Mass Regeneration**, then use them to unlock. The Lich
  encounter is shared content from [`mod-sod-world`](../mod-sod-world).
- **Enlightenment** (Chest) — *event-gated* (Alliance). Out-of-place critters
  (*Polymorphed Apprentice*) wander **all over Elwynn Forest**; **Polymorph** one to
  free Azora's apprentice. It poofs in, thanks you, blinks out, and drops a paper —
  click it to collect one of four **Azora Apprentice Notes** pages. Read all four
  together (use any page) to form **Spell Notes: Enlightenment**, then use it to
  unlock the rune. (Horde version is a later pass.)
- **Arcane Surge** (Legs) — *reputation-vendor-gated.* Reach **Friendly** with your
  city's supply faction ([`mod-sod-world`](../mod-sod-world)'s supply officers), buy
  **Spell Notes: Arcane Surge** from them, and use it to unlock the rune. (Mage-only
  and bind-on-pickup, so the notes only show on the vendor for Mages.)
- **Arcane Blast** (Hands) — *discovery-unlocked.* In **Zoram Strand, Ashenvale**, cast
  **Arcane Explosion** on three purple crystals **in order** (Southern → Middle →
  Northern) to build a stacking **Arcane Charge** buff; at three stacks you receive
  **Spell Notes: Arcane Blast**, which unlocks the rune. Engraving it is **dual-mode**:
  until you can train the real Arcane Blast at level 64 it grants a capped stand-in,
  **Arcane Burst** (a 2.5s Arcane nuke that feeds the Chronomancy healing kit but stops
  scaling at 64 so it never out-paces the real spell); once you know the real Arcane
  Blast it drops Arcane Burst and instead grants **Nether Vortex**, causing your Arcane
  Blast to apply **Slow** to a target it hits that isn't already slowed.
- **Rewind Time** (Wrist) — *acquisition TBD.* An instant ability (30 sec cooldown)
  that heals an ally carrying your **Temporal Beacon** for **all the damage they took
  over the last 5 sec** — ineffective unless the beacon has been on them that long. It's
  coded as a real heal, so anything that modifies healing (+healing%, healing crits)
  applies. The unlock chain isn't built yet — for now learn it with `.learn 401462`, or
  force-unlock the rune with the engine's `.rune` GM command.

Without the engine, the items and combines still work — only the final unlock
no-ops.

The discovery-note **drop rates** are config-tunable (`SodMage.Drops.*NotesChance`):
Mass Regeneration notes default 100% (off the Awakened Lich elite), Living Flame and
Regeneration notes default 20% (off leveling-zone trash). Changes apply on
`.reload config`.

This module owns the rune-id band **7000000–7000999** in the engine's catalog.
Its items and drops use the **real SoD ids** (Comprehension Charm `211779`, the
Spell Notes `208754`/`208753`/`211514`/`203752`/`203746`/`203749`, the Azora
Apprentice Notes pages `203756`/`203960`/`203961`/`203962`); the Awakened Lich
(`212261`) is owned by [`mod-sod-world`](../mod-sod-world). The Enlightenment
event's critter/apprentice and paper are custom (creature `700210`, GO `700220`) —
no SoD creature id is sourceable.

## Documentation

Developer docs (architecture, the generator, adding spells, deploy/verify,
gotchas) live in [`docs/`](docs/Home.md) and are mirrored to the project wiki.
Start at [docs/Home.md](docs/Home.md).

## Contributing

Adding a spell, rune, or item? Start from a skeleton in
[`sod-class-templates`](https://github.com/mod-sod/sod-class-templates) — copy it into place and fill the blanks. See
[CONTRIBUTING.md](CONTRIBUTING.md).

## Conventions

No core edits — everything stays under `modules/mod-sod-mage/`.

## License

GPL v2 (inherited from AzerothCore). See [LICENSE](LICENSE).
