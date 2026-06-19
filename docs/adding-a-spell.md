# Adding a spell

The end-to-end recipe for a new SoD-style spell. Read [Architecture](architecture.md)
first if you haven't — a spell is two halves (server + client) kept in sync by the
[generator](spell-generator.md).

## 1. Decide the visual/audio strategy first

The client can only render what it already has. Before writing anything, decide:

- **Spell ID** — reuse the real SoD ID if there is one; otherwise pick from the
  reserved `900000–900099` band.
- **Icon** — pick an existing `SpellIcon` (the generator resolves it by texture
  name). Use the **same texture name in every place the spell's icon appears**, or
  they drift out of sync:
  - the spell's `SpellIconID` in `tools/sod_spells.py` (what the learned spell
    shows in the spellbook/action bar), and
  - if the spell is acquirable as a **rune**, the `rune_template.icon` in the rune
    SQL (`data/sql/db-world/base/sod_mage_*.sql`) — the rune panel renders this, so
    a mismatch makes the rune-list icon differ from the spell it unlocks.
- **Visual** — reuse an existing `SpellVisualID`. There is no way to recolor a
  beam without custom art; pick a visual that already looks right. (Regeneration
  reuses Drain Mana's beam `12657`; Temporal Beacon reuses Lightning Shield `37`.)
- **Template to clone** — a real spell with the right *shape* (channel, HoT,
  instant heal…) so the cloned record's index columns are valid.

Record these choices in the spec comment.

## 2. Add the spec

In `tools/sod_spells.py`, add an entry to `build_spells(idx)` (see
[Spell spec](spell-generator.md) for every key). Set `overrides` using literal
`spell_dbc` column names.

### Non-obvious requirements (these *will* bite you)

- **Bind the script** — a `RegisterSpellScript(...)` class does **nothing** until a
  `spell_script_names` row maps its class name to the spell ID. Give the spec a
  `script` key. Without it the server logs `Script named 'X' is not assigned in the
  database` and the spell silently runs only its DBC effects.
- **Channeled spells use an *instant* `CastingTimeIndex`** — the channel length
  comes from the *duration*, not the cast time. Set `AttributesEx` channeled bit,
  an instant cast index, the channel `DurationIndex`, and
  `ChannelInterruptFlags = 31756` (+ `InterruptFlags = 15`). A non-instant cast
  index produces a cast bar *then* a channel.
- **Proc auras need a `spell_proc` row** — an aura with no `spell_proc` entry never
  procs (`Aura::GetProcEffectMask` returns 0). Add a `proc` dict; use
  `AttributesMask = 2` (TRIGGERED_CAN_PROC) if it should proc off triggered spells
  (e.g. Arcane Missiles' ticks).
- **`EquippedItemClass = -1`** so the spell doesn't require an equipped item.

See [Gotchas](gotchas.md) for the full list.

## 3. Write the C++ script

Add the `SpellScript`/`AuraScript` in `src/spell_sod_mage.cpp`, register it in
`AddSC_sod_mage_spell_scripts()`, and define IDs as constants in
`src/spell_sod_mage.h`. Model hook usage on the core's
`src/server/scripts/Spells/spell_mage.cpp`. Gate behavior behind
`SodMageEnabled()` so the module is a no-op when disabled.

C++ changes require a **worldserver rebuild**; pure DBC/SQL/visual changes do not.

## 4. Regenerate and deploy

```bash
# from a sod-client checkout; client fully closed
python build_patch.py --server "<azerothcore root>" --client "<client root>"
```

Then follow [Deploy & verify](deploy-and-verify.md): apply the SQL, restart the
worldserver (and rebuild it if you changed C++), and relaunch the client.

## 5. Verify in-game

`.learn <id>`, cast it, and confirm: it casts; icon/cost/cooldown behave; the
effect/proc fires; the combat log is sane; and (after restart) the server log has
no `not assigned` errors for your script. Note any tooltip-vs-behavior mismatch
from reused DBC data.
