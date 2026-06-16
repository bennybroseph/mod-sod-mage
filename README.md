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
   ```
   (Idempotent — safe to re-run. Seeds `spell_dbc`, `spell_script_names`, `spell_proc`.)
4. Restart the worldserver (these tables load at startup).

### Client patch (required — otherwise the spells won't render)

The spells use IDs the stock client doesn't have, so the client needs a patch MPQ.
Generate it against your own client (it reuses stock icons/visuals — no custom art):

```bash
pip install pympq                       # StormLib binding for MPQ read/write
# Fully close the WoW client first (a running client locks its MPQs):
python modules/mod-sod-mage/tools/build_sod_mage_patch.py --client "<path to your WoW 3.3.5a client>"
```

This writes a patch into the client's `Data/` folder and (re)writes the server SQL.
It currently assumes an **enUS** client and ships the patch as `patch-enus-z.mpq`;
adjust for other locales. Launch the client after generating.

### Use it

No trainer entry yet — learn the spells directly:

```
.learn 401417    # Regeneration
.learn 412510    # Mass Regeneration
```

## Documentation

Developer docs (architecture, the generator, adding spells, deploy/verify,
gotchas) live in [`docs/`](docs/Home.md) and are mirrored to the project wiki.
Start at [docs/Home.md](docs/Home.md).

## Conventions

No core edits — everything stays under `modules/mod-sod-mage/`.

## License

GPL v2 (inherited from AzerothCore). See [LICENSE](LICENSE).
