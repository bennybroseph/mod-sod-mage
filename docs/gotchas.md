# Gotchas & troubleshooting

The hard-won lessons, in one place. Most "it doesn't work" cases are one of these.

## Scripts: a `RegisterSpellScript` does nothing without a DB binding

A registered `SpellScript`/`AuraScript` is inert until a **`spell_script_names`**
row maps its class name → spell ID. Symptom: the spell casts and its DBC effects
fire, but custom script behavior (procs, applying other auras) never happens, and
the server logs:

```
Script named 'spell_sod_mage_regeneration' is not assigned in the database.
```

That error also *confirms the module is compiled in* — it only appears for scripts
present in the binary. The generator emits these rows; make sure your spec has a
`script` key.

## Channeled spells use an *instant* cast-time index

The channel length comes from the **duration**, not the cast time. A channeled
spell needs:

- `AttributesEx` channeled bit (`SPELL_ATTR1_IS_CHANNELED = 0x4`)
- an **instant** `CastingTimeIndex` (the index whose base is 0 ms)
- the channel length in `DurationIndex`
- `InterruptFlags = 15`, `ChannelInterruptFlags = 31756` (so movement interrupts)

Symptom of getting it wrong (non-instant cast index): a cast bar **then** a channel,
and movement not interrupting. The server's `spell_dbc` cast time is authoritative
for the cast bar, so fix it in **both** the SQL and the client MPQ (the generator
does both).

## Proc auras need a `spell_proc` row

`Aura::GetProcEffectMask` returns 0 for any aura without a `spell_proc` entry — so
it never procs. Add a `proc` dict to the spec. `Chance`/`ProcFlags` fall back to the
`spell_dbc` values if left 0. Set `AttributesMask = 2` (`PROC_ATTR_TRIGGERED_CAN_PROC`)
if it should proc off triggered spells (e.g. Arcane Missiles' per-missile ticks).

## DB changes need a worldserver **restart**, not just a `compose restart`

`spell_dbc` and `spell_script_names` load at startup and are not hot-reloadable.
And `docker compose restart` does **not** run the `db-import` service (that's only
on `up`), so changed SQL is never auto-applied by a restart — apply it yourself.

## C++ changes need a rebuild; data changes don't

Script logic → rebuild the worldserver (`-DMODULES=static` bakes modules into the
binary). `spell_dbc`/visual/tooltip/sound changes → no rebuild.

## MPQ patch priority is **not** alphabetical

The client loads `patch.mpq`, then `patch-2`…`patch-9`, then `patch-a`…`patch-z`;
later suffixes override earlier. So `patch-enus-s.mpq` overrides `patch-enus.mpq`
even though it sorts earlier as a string (`-` < `.`). The "effective" `Spell.dbc`
is the one in the highest-suffix archive that contains it — which is what the
generator extracts and clones from. Our patch ships as `patch-enus-z.mpq` to load
last and win.

## A running client locks its MPQs

You cannot read or overwrite the client DBCs/MPQ while the client is running
(StormLib error 32 / sharing violation) — and "closed window" isn't always "process
exited". If extraction/repack fails with a lock, confirm `Wow.exe` is actually gone.
The generator skips its own output patch when picking the source DBC, so re-runs
don't compound.

## Visuals: you can't recolor a beam, only reuse one

A beam's color is baked into its particle model/texture, not a DBC field. There is
**no purple smooth beam** in stock 3.3.5a — the only caster→target *tethers* are the
drain family (green/blue) and Mind Flay (purple, but tendril-shaped). The elemental
"beam-state" models (Nature/Fire/Frost/Lightning) are vertical **skybeams**, not
tethers. A true recolor would require shipping custom `.mdx`/`.blp` art in the MPQ.
Pick an existing `SpellVisualID` that already looks right.

## Sounds

Stock sound files already have `SoundEntries` IDs, so you don't ship audio — you
either reuse a visual that carries the sound (its `SpellVisualKit.SoundID`) or play
one from script via `WorldObject::PlayDistanceSound(soundId)`. Caveat: reusing a
visual brings *its* sound too, so layering a script sound on top can double up; for
a clean swap you'd give the spell its own cloned visual/kit with the desired
`SoundID`.

## Tooltips don't update? Clear the client cache

Tooltip strings live in the client `Spell.dbc` (`Description_Lang_enUS`,
`AuraDescription_Lang_enUS`). If new wording doesn't show after relaunch, delete the
client's `Cache`/`WDB` folder to force a refresh. Tokens like `$o1` (total periodic),
`$s1` (per-tick), `$d` (duration) are computed by the client from its own DBC.

## Custom item shows a "?" icon in bags (but the vendor icon is fine)

A custom item's name/stats/tooltip come from the server item-query, but its **bag
inventory icon** is resolved client-side from the client's own `Item.dbc`
(itemId → `DisplayInfoID` → `ItemDisplayInfo` icon). A custom item id absent from
`Item.dbc` shows the red "?" icon in bags. The **vendor** frame is unaffected — the
vendor packet carries the displayid directly — which is the tell: vendor OK, bag
"?". Clearing the `Cache`/`WDB` folder does **not** fix it (the icon never came from
that cache). The fix is a client `Item.dbc` row per custom item, shipped in the
patch MPQ — the [generator](spell-generator.md)'s `ITEMS` list does this; regenerate
and reinstall the patch. Reuse an existing `DisplayInfoID` (find one by its
`InventoryIcon` in `ItemDisplayInfo.dbc`) so no new art is needed.
