# SoD Mage healer kit — reference

The Season of Discovery Mage "Chronomancy" healing kit, for planning future
implementation. Values pulled from **wago.tools**, `wow_classic_era` build
**1.15.8.67156** (`Spell`, `SpellEffect`, `SpellMisc`, `SpellPower`,
`SpellCastTimes/Duration/Range/Radius`). All IDs below are confirmed **not** present
in WotLK 3.3.5a `Spell.dbc`. See [Gotchas](gotchas.md) for how to pull this data and
[Architecture](architecture.md) for how we deliver a spell.

> **Porting caveats (see CLAUDE.md → "Pulling SoD reference data"):** modern
> effect/aura *type* enums differ from 3.3.5a — map by meaning, not number. Modern
> `EffectBasePoints` is the actual value; 3.3.5a stores value − 1. Spellpower
> scaling (the `$<healpower>*$mN/100` tooltips) is done server-side via
> `spell_bonus_data` (per-tick coefficient = `$mN/100`), with `SpellLevel = 0` to
> avoid the level penalty — as done for [Regeneration](spells.md).

## Status

| Spell | ID | Status |
|-------|----|--------|
| Temporal Beacon | `400735` | ✅ implemented |
| Regeneration | `401417` | ✅ implemented |
| Chronomantic Healing | `401405` | ✅ implemented (real SoD id) |
| Mass Regeneration | `412510` | ✅ implemented |
| **Chronostatic Preservation** | `436516`/`436517`/`443369` | ⬜ to do |
| **Rewind Time** | `401462` | ⬜ to do |
| **Temporal Anomaly** | `428885`/`428888`/`428895` | ⬜ to do |

Each ability also has a `…332` "Gain the ability" learn entry (e.g. `415467`,
`425187`, `401734`, `429305`) — these are the rune grants, not the cast; ignore for
our `.learn`-based setup.

---

## Mass Regeneration — `412510`  *(✅ implemented)*

The AoE sibling of Regeneration.

- **Cast:** instant → **3s channel** (CastTime idx 1, Duration idx 27), **40 yd**
  (Range idx 5), **Arcane** (school 64), **45% base mana**.
- **Effect:** periodic heal, **1s ticks** (3 ticks), base coefficient `$m1 = 55`,
  target = party (132).
- **Heal:** `healpower × 55/100 × 3` = **165% of healing power over 3s** to every
  party member near the target, and applies **Temporal Beacon to each — but only
  for 15 sec** (`$400735m3/2`, half of single-target Regeneration's 30s `$400735s3`).
- **Impl notes:** Regeneration's logic with an AoE party target; apply the beacon
  (and arm the conversion aura) per target, but with a **15s** beacon (vs 30s for
  Regeneration) — so the per-target beacon duration must be overridable, not baked
  into `400735`. `spell_bonus_data dot_bonus = 0.55`, base 0, `SpellLevel 0`.
  Otherwise reuses everything we built for Regeneration.

## Chronostatic Preservation — `436516` → `443369` → `436517`

A charge-and-release burst heal. Multi-school (**Arcane+Fire+Frost**, mask 84).

- **Store (`436516`):** **2s cast** (CastTime idx 5), applies a **20s** hold buff
  (`443369`, Duration idx 18), **24% base mana**, unlimited range to self.
- **Held buff (`443369`):** "chronomantic energy ready"; if not spent in 20s it
  combusts/expires.
- **Unleash (`436517`):** **instant**, **40 yd**, direct heal
  `healpower × 219/100` (min) to `…×$M1/100` (max) — i.e. **~219%+ of healing
  power**. School 84.
- **Impl notes:** two-stage. Cast `436516` → apply the 20s buff → a second
  (consume) cast heals and removes the buff; on expiry, "combust". Needs the
  multi-school flag and a high direct coefficient (`spell_bonus_data direct ≈ 2.19`).

## Rewind Time — `401462`

A retroactive heal — the trickiest to implement.

- **Cast:** instant, **60 yd** (Range idx 14), **Arcane**, negligible/no mana.
- **Effect:** script effect (type 77), `$s1 = 5`.
- **Mechanic:** instantly heals your current **Temporal Beacon** target for **all
  damage they took over the last 5 seconds**. Ineffective on targets that didn't
  have a beacon 5s ago.
- **Impl notes:** no DBC analog — pure script. Requires a rolling per-beacon-target
  **damage-taken log** (last 5s), summed and healed on cast. Build on the existing
  beacon registry. Highest effort of the set.

## Temporal Anomaly — `428885` (+ `428888` pulse, `428895` shield)

A summoned, slow-moving shield orb.

- **Summon (`428885`):** instant, **40 yd**, **Arcane**, **11% base mana**;
  summons an orb (effect 28 = Summon, creature misc `214063`) lasting **16s**
  (Duration idx 387).
- **Pulse (`428888`):** **every 2s** (periodic, 2000ms) while the orb lives.
- **Shield (`428895`):** **school absorb** (aura 69), `$m1 = 100` →
  **100% of spellpower absorbed**, **15s** duration (Duration idx 8), applied to
  party within **8 yd** (Radius idx 14 = 8) of the orb; **stacks** while members
  stay near it.
- **Impl notes:** most complex — needs a summoned **mobile orb** (custom
  creature + slow forward movement/AI) that periodically AoE-applies a **stacking
  absorb** to nearby party. Note this is a *shield*, not a heal. Consider a custom
  creature script + `SPELL_AURA_SCHOOL_ABSORB` aura.

---

## Suggested implementation order

1. ~~**Mass Regeneration**~~ — ✅ done (reused the Regeneration/beacon pipeline;
   applies a 15s beacon via `SPELLVALUE_AURA_DURATION` in `ApplyTemporalBeacon`).
2. **Chronostatic Preservation** — moderate; two-stage buff + big instant heal.
3. **Temporal Anomaly** — high; summoned moving orb + stacking party absorb.
4. **Rewind Time** — high; bespoke damage-history tracking.
