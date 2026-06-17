# Architecture

## The hard constraint: the 3.3.5a client

The client only knows the spells, icons, animations, and tooltips baked into its
local DBC files (inside its MPQ archives). The server **cannot** push a brand-new
spell definition to the client at runtime. Anything the player sees — the spell
in their book, its icon, cast bar, beam, tooltip — must already exist client-side.

This forces every "new" SoD spell to be **two artifacts that must agree**:

| Half | What it is | Where it lives |
|------|------------|----------------|
| **Server** | `spell_dbc` rows (so the core treats the ID as a real spell) + `SpellScript`/`AuraScript` C++ for custom behavior | `acore_world` DB + the module compiled into `worldserver` |
| **Client** | matching `Spell.dbc` rows (so the client renders the spell) | a patch MPQ dropped into the client's `Data/` folder |

If the two disagree on an ID or a value, you get subtle bugs (e.g. a cast bar the
client shows but the server treats differently). The [generator](spell-generator.md)
exists precisely to derive both halves from one spec.

### Why the server can "know" an ID the client patch defines

AzerothCore loads `Spell.dbc` from its data dir and then overlays the
`acore_world.spell_dbc` table on top (`LoadDBC(..., "spell_dbc")`). The overlay
**adds new IDs**, not just edits to existing ones — the in-memory index grows to
fit. That's why a row in `spell_dbc` for ID `401417` makes the server treat it as
a real spell even though stock `Spell.dbc` has no such ID. (Empirically confirmed:
the spell casts server-side with only the SQL applied.)

The client has no such overlay — hence the MPQ patch.

## Custom items: a smaller version of the same split

The module also ships custom **items** (the rune-acquisition chain — a charm and
spell notes). Items are *mostly* server-authoritative: a custom `item_template`
row gives the client its name, stats, and tooltip via the item-query response, so
**no** client patch is needed for those. The one exception is the **bag inventory
icon**, which the client resolves from its own `Item.dbc` (itemId → `DisplayInfoID`
→ icon). A custom item id absent from `Item.dbc` shows a red "?" in bags (the
vendor frame is fine — the vendor packet carries the displayid). So the same patch
MPQ also carries an `Item.dbc` with a row per custom item; reuse an existing
`DisplayInfoID` (no new art). See [Gotchas](gotchas.md) and the
[generator](spell-generator.md)'s `ITEMS` list.

## Spell ID allocation

The module reserves the band **900000–900099** for its own server-side helper
IDs. It also reuses the **real SoD IDs** for player-facing spells so a future
"real" SoD client patch would line up.

| ID | Name | Side | Role |
|----|------|------|------|
| `401417` | Regeneration | client + server | player-cast channeled HoT; applies the beacon |
| `412510` | Mass Regeneration | client + server | AoE party channeled HoT; applies a 15s beacon to each |
| `400735` | Temporal Beacon | client + server | the 30s buff + passive HoT marker |
| `401405` | Chronomantic Healing | client + server | instant triggered heal (the converted healing) |
| `900001` | Temporal Beacon (conversion) | **server only** | hidden passive proc aura on the Mage |

`900001` is server-only: it's a hidden passive (no icon/tooltip), so the client
never needs to know it. See [Spells](spells.md) for the mechanics.

## How a cast flows (Regeneration → Temporal Beacon)

1. Player casts **Regeneration** (`401417`), a 3s channel. Its periodic-heal aura
   applies on the target.
2. The Regeneration `AuraScript` (on apply) casts **Temporal Beacon** (`400735`)
   on the target and ensures the Mage carries the hidden **conversion aura**
   (`900001`).
3. While a beacon is up, any **Arcane** spell damage the Mage deals procs `900001`,
   which heals each beacon target via **Chronomantic Healing** (`401405`).
4. A small file-static registry maps `caster GUID → beacon target GUIDs` so the
   proc knows where to route healing (modeled on the core's Beacon of Light).

## No core edits

Everything lives under `modules/mod-sod-mage/`. Behavior is driven through script
hooks, DB rows, and the client MPQ — never by patching AzerothCore itself.

See also: [Adding a spell](adding-a-spell.md) · [Gotchas](gotchas.md)
