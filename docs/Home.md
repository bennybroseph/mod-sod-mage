# mod-sod-mage

`mod-sod-mage` is an [AzerothCore](https://www.azerothcore.org/) module that adds
custom Mage spells inspired by **Season of Discovery** (SoD) Classic, targeting
the WotLK **3.3.5a** client (build 12340).

These are developer docs: how the module is built, why it works the way it does,
and how to extend it. For a high-level summary and install steps, see the
[README](https://github.com/bennybroseph/mod-sod-mage) in the repo root.

> **Just want to play?** You don't need these docs — the
> [**SoD installer**](https://github.com/bennybroseph/sod-installer) sets everything
> up in one command. These pages are for developers building or extending the module.

## Start here

- **[Architecture](architecture.md)** — the hard 3.3.5a-client constraint and the
  "two halves" design (server-side `spell_dbc` + a client-side MPQ patch) that
  every spell relies on.
- **[Adding a spell](adding-a-spell.md)** — the end-to-end recipe for a new spell,
  with the non-obvious requirements that will bite you (script binding, channel
  cast time, proc setup).
- **[Deploy & verify](deploy-and-verify.md)** — applying SQL, restarting the
  worldserver, repacking the client MPQ, and confirming it actually loaded.

## Reference

- **[Spell generator](spell-generator.md)** — `tools/build_sod_mage_patch.py`: the
  single source of truth that produces both the client MPQ and the server SQL.
- **[Spells](spells.md)** — the implemented spells (Regeneration, Temporal Beacon
  and friends): IDs, mechanics, visuals, tooltips, config.
- **[SoD healer kit reference](healer-kit-reference.md)** — exact SoD values for the
  Chronomancy healing spells (implemented and planned), for future work.
- **[Pulling SoD data](pulling-sod-data.md)** — how to source authoritative SoD
  spell values/tooltips from wago.tools, and the porting traps to watch.
- **[Gotchas & troubleshooting](gotchas.md)** — the hard-won lessons, in one place.

## The one thing to internalize

The 3.3.5a client can only render spells, icons, animations, and tooltips that
already exist in its DBC files. The server **cannot teach the client a brand-new
spell**. So every "new" spell is really *two* artifacts that must agree on IDs and
values: a **server** side (`spell_dbc` rows + C++ scripts) and a **client** side
(an MPQ patch with matching `Spell.dbc` rows). The generator keeps them in sync.
See [Architecture](architecture.md).
