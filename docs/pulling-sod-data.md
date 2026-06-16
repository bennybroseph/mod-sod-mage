# Pulling SoD reference data

The authoritative SoD spell values and tooltip strings live in the modern Classic
Era client's DB2 tables, browsable as CSV on [wago.tools](https://wago.tools) — no
local client or custom parser needed. Use this to source accurate numbers/text
before implementing or tuning a spell. See [Adding a spell](adding-a-spell.md) for
where these values plug in, and the [healer kit reference](healer-kit-reference.md)
for the kit pulled this way.

## Recipe

1. **Pick the build** — latest `wow_classic_era` from
   `https://wago.tools/api/builds` (e.g. `1.15.8.67156`).
2. **Download tables as CSV** — `https://wago.tools/db2/<Table>/csv?build=<version>`
   (send a `User-Agent` header). The tables you need:

   | Table | Useful columns | What it gives |
   |-------|----------------|---------------|
   | `SpellName` | `ID`, `Name_lang` | grep the name → spell ID |
   | `Spell` | `Description_lang`, `AuraDescription_lang` | tooltip + buff text |
   | `SpellEffect` (filter `SpellID`) | `Effect`, `EffectAura`, `EffectBasePoints`, `ImplicitTarget_0` | the values the tooltip `$m1`/`$sN` tokens resolve to |
   | `SpellMisc` (filter `SpellID`) | `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask` | **index values match 3.3.5a directly** |
   | `SpellPower` (filter `SpellID`) | `PowerCostPct`, `ManaCost` | mana cost |
   | `SpellCastTimes`/`SpellDuration`/`SpellRange`/`SpellRadius` | `Base`/`Duration`/`RangeMax_0`/`Radius` | resolve the index columns above to ms/yards |

3. **Read the tooltip tokens** — `$mN`/`$sN` = effect-N base points, `$wN` =
   effect-N value, `$aN` = radius, `$d` = duration, `$<healpower>` = healing power,
   `$<spellid>sN` = another spell's effect-N (e.g. `$400735s3` = Temporal Beacon's
   3rd effect, which is its 30s duration value).

Each "ability" usually has several IDs — the cast, ranks, an aura/trigger, and a
`…332` "Gain the ability" rune-learn entry. Read the `Description` to pick the real
player-cast spell.

## Two conversions that bite — do NOT copy raw

- **Effect/Aura *type* enums differ** between the modern client and 3.3.5a. A SoD
  heal-over-time shows `EffectAura = 227`, which is **not** 3.3.5a's
  `SPELL_AURA_PERIODIC_HEAL = 8`. Map types to the 3.3.5a enum by *meaning*, never
  by number. Only numeric tuning values (base points, percentages, durations,
  coefficients) and the index columns port directly.
- **Base-points offset** — modern `EffectBasePoints` is the *actual* value; 3.3.5a
  stores value − 1 (the game adds 1). So a SoD value of `8` becomes `7` in our
  `spell_dbc`/DBC, `55` becomes `54`, and so on.

## Spellpower scaling

SoD's `$<healpower>*$mN/100` tooltips mean pure spellpower scaling. On 3.3.5a we
reproduce that server-side with a `spell_bonus_data` row whose per-tick coefficient
= `$mN/100` (e.g. `0.55`), a **zero** base, and `SpellLevel = 0` to avoid the
low-level coefficient penalty. The 3.3.5a client can't display a server-side
coefficient, so tooltips state the value statically. See
[Regeneration](spells.md) for a worked example.
