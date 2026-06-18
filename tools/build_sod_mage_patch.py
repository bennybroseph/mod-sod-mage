#!/usr/bin/env python3
"""Generate the mod-sod-mage client patch MPQ and server spell_dbc SQL.

Single source of truth: SPELLS below. For each new spell we clone a real
template record from the client's effective Spell.dbc (so every index column
is valid), apply our overrides, and emit BOTH:

  * a patched Spell.dbc packed into a client patch MPQ (real SoD IDs only),
  * a patched SkillLineAbility.dbc in the same MPQ so spells flagged with a
    `skill_line` land under the right spellbook tab (client-side grouping), and
  * a spell_dbc INSERT (named columns) for the server.

Custom-item client rows (Item.dbc / ItemDisplayInfo.dbc) are NOT built here: WoW
replaces whole DBCs per MPQ, so every SoD module's item rows must share one file.
mod-sod-world owns that consolidated patch; this module contributes its items via
tools/client_items.json (keep it in sync with the item_template rows in
sod_mage_regeneration_unlock.sql / sod_mage_mass_regeneration.sql). This tool
patches only the spell DBCs.

Index/icon values are resolved at runtime from the client's own DBCs, so the
output is correct for whatever client it is pointed at. Requires `pympq`
(StormLib binding) for MPQ read/write.

Usage:
    python build_sod_mage_patch.py [--client DIR] [--dry-run]

See tools/README.md for the pipeline overview.
"""

import argparse
import os
import re
import struct
import sys

# ---------------------------------------------------------------------------
# Paths (defaults match this machine; override with --client / --repo).
# ---------------------------------------------------------------------------
HERE = os.path.dirname(os.path.abspath(__file__))
MODULE_ROOT = os.path.dirname(HERE)
REPO_ROOT = os.path.abspath(os.path.join(MODULE_ROOT, os.pardir, os.pardir))

DEFAULT_CLIENT = r"E:\Games\World of Warcraft 3.3.5a HD"
SQL_OUT = os.path.join(MODULE_ROOT, "data", "sql", "db-world", "base",
                       "sod_mage_spell_dbc.sql")
TABLE_DEF = os.path.join(REPO_ROOT, "data", "sql", "base", "db_world",
                         "spell_dbc.sql")
PATCH_MPQ_NAME = "patch-enus-z.mpq"  # highest-priority locale patch letter

INNER = "DBFilesClient\\Spell.dbc"
SKILL_INNER = "DBFilesClient\\SkillLineAbility.dbc"

# ---------------------------------------------------------------------------
# Enum values (from core SharedDefines.h / SpellAuraDefines.h).
# ---------------------------------------------------------------------------
SPELL_ATTR0_PASSIVE = 0x00000040
SPELL_ATTR0_DO_NOT_DISPLAY = 0x00000080
SPELL_ATTR1_IS_CHANNELED = 0x00000004
EFFECT_APPLY_AURA = 6
EFFECT_HEAL = 10
AURA_DUMMY = 4
AURA_PERIODIC_HEAL = 8
TARGET_UNIT_CASTER = 1
TARGET_UNIT_TARGET_ALLY = 21
TARGET_UNIT_LASTTARGET_AREA_PARTY = 37  # the target's party within radius
SCHOOL_MASK_ARCANE = 1 << 6  # 64
DISPEL_MAGIC = 1
SKILL_ARCANE = 237  # mage Arcane skill line; controls the spellbook tab (client-side)
NAME_MASK = 16712190  # standard "enUS available" locale mask
# Proc on the caster dealing harmful magic/none-class spell damage, plus ranged
# auto-attacks so an Arcane wand (a ranged auto-attack of Arcane school) also feeds
# the beacon. The Arcane SchoolMask filter keeps non-Arcane wands out.
PROC_DONE_MAGIC_NEG = 0x00010000
PROC_DONE_NONE_NEG = 0x00001000
PROC_DONE_RANGED_AUTO = 0x00000040


# ---------------------------------------------------------------------------
# WDBC reader/writer (3.3.5a fixed-width records + trailing string block).
# ---------------------------------------------------------------------------
class WDBC:
    def __init__(self, raw):
        magic, self.nrec, self.nfield, self.recsize, self.strsize = \
            struct.unpack("<4siiii", raw[:20])
        if magic != b"WDBC":
            raise ValueError("not a WDBC file")
        body = 20 + self.nrec * self.recsize
        self.records = [bytearray(raw[20 + i * self.recsize:
                                      20 + (i + 1) * self.recsize])
                        for i in range(self.nrec)]
        self.strings = bytearray(raw[body:body + self.strsize])

    @classmethod
    def load(cls, path):
        with open(path, "rb") as fh:
            return cls(fh.read())

    def get_int(self, rec, field):
        return struct.unpack_from("<i", rec, field * 4)[0]

    def set_int(self, rec, field, value):
        struct.pack_into("<i", rec, field * 4, int(value))

    def add_string(self, text):
        """Append a string, return its offset within the string block."""
        offset = len(self.strings)
        self.strings += text.encode("utf-8") + b"\x00"
        return offset

    def find(self, spell_id):
        for rec in self.records:
            if self.get_int(rec, 0) == spell_id:
                return rec
        raise KeyError(spell_id)

    def serialize(self):
        out = bytearray()
        out += struct.pack("<4siiii", b"WDBC", len(self.records),
                           self.nfield, self.recsize, len(self.strings))
        for rec in self.records:
            out += rec
        out += self.strings
        return bytes(out)


def load_aux(path):
    """Parse a simple int-only aux DBC into a list of field tuples."""
    with open(path, "rb") as fh:
        raw = fh.read()
    _, nrec, nfield, recsize, _ = struct.unpack("<4siiii", raw[:20])
    rows = []
    for i in range(nrec):
        off = 20 + i * recsize
        rows.append(struct.unpack_from("<%di" % nfield, raw, off))
    return rows, raw


# ---------------------------------------------------------------------------
# Column-name -> field-index map, parsed from the core spell_dbc table def.
# ---------------------------------------------------------------------------
def load_columns(table_sql_path):
    cols = []
    with open(table_sql_path, encoding="utf-8") as fh:
        in_table = False
        for line in fh:
            if "CREATE TABLE" in line and "spell_dbc" in line:
                in_table = True
                continue
            if in_table:
                if line.lstrip().startswith("PRIMARY KEY") or \
                        line.lstrip().startswith(")"):
                    break
                m = re.match(r"\s*`([A-Za-z0-9_]+)`", line)
                if m:
                    cols.append(m.group(1))
    if len(cols) < 200:
        raise RuntimeError("failed to parse spell_dbc columns (%d)" % len(cols))
    return cols  # index in list == DBC field index


# ---------------------------------------------------------------------------
# Spell specification (single source of truth).
# `overrides` keys are spell_dbc column names. `client` controls whether the
# spell goes into the client DBC/MPQ (server-only spells are SQL only).
# ---------------------------------------------------------------------------
def build_spells(idx):
    """idx: resolver giving runtime DBC index/icon values."""
    cast_3s = idx["cast"][3000]
    cast_instant = idx["cast"][0]
    dur_3s = idx["dur"][3000]
    dur_30s = idx["dur"][30000]
    dur_perm = idx["dur"][-1]
    range_40 = idx["range"][40.0]
    range_100 = idx["range"][100.0]
    range_self = idx["range"][0.0]
    # These texture names are each spell's displayed icon. If a spell is also a
    # rune, keep `rune_template.icon` in data/sql/db-world/base/sod_mage_*.sql
    # equal to the name here (Regeneration -> sod_mage_runes.sql; Mass
    # Regeneration -> sod_mage_mass_regeneration.sql) so the rune panel icon
    # matches the spell it unlocks.
    icon_beacon = idx["icon"]["spell_nature_timestop"]
    icon_regen = idx["icon"]["inv_enchant_essencemysticalsmall"]
    icon_regen_large = idx["icon"]["inv_enchant_essencemysticallarge"]

    return [
        {  # 401417 Regeneration: channeled 3s single-target HoT. A channel uses
           # an instant CastingTimeIndex; its length comes from the duration.
            "id": 401417, "client": True, "template": 139,  # clone Renew
            "skill_line": SKILL_ARCANE,  # spellbook tab: Arcane
            "name": "Regeneration", "script": "spell_sod_mage_regeneration",
            # Static wording: the 3.3.5a client can't resolve a server-side
            # coefficient (no $<healpower> engine, no client coeff override), so a
            # dynamic token would under-report. State the SoD value plainly instead.
            "desc": "Heals the target for an amount equal to 165% of your healing "
                    "power over 3 sec and applies Temporal Beacon for 30 sec.",
            # SoD: heals 165% of healing power over 3 ticks, no flat base. We model
            # that with a 0.55/tick spellpower coefficient (spell_bonus_data) and a
            # zero base. SpellLevel 0 avoids AzerothCore's low-level coefficient penalty.
            "bonus": {"direct": 0.0, "dot": 0.55, "ap": 0.0, "ap_dot": 0.0},
            "overrides": {
                "Attributes": 0, "AttributesEx": SPELL_ATTR1_IS_CHANNELED,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_3s,
                "InterruptFlags": 15, "ChannelInterruptFlags": 31756,
                "SpellVisualID_1": 12657,  # Drain Mana blue caster->target beam
                "RangeIndex": range_40, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 28, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_regen, "EquippedItemClass": -1, "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_HEAL,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 0,  # pure SP coeff
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 412510 Mass Regeneration: AoE party channeled HoT (sibling of
           # Regeneration). Targets the chosen ally's party in radius
           # (A = ally, B = the target's party area), like Prayer of Healing.
            "id": 412510, "client": True, "template": 139,  # clone Renew
            "skill_line": SKILL_ARCANE,  # spellbook tab: Arcane
            "name": "Mass Regeneration", "script": "spell_sod_mage_mass_regeneration",
            "desc": "Heals the target and their party members for an amount equal to "
                    "165% of your healing power over 3 sec and applies Temporal Beacon "
                    "to each for 15 sec.",
            "bonus": {"direct": 0.0, "dot": 0.55, "ap": 0.0, "ap_dot": 0.0},
            "overrides": {
                "Attributes": 0, "AttributesEx": SPELL_ATTR1_IS_CHANNELED,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_3s,
                "InterruptFlags": 15, "ChannelInterruptFlags": 31756,
                "SpellVisualID_1": 12657,  # Drain Mana beam (Regeneration family)
                "RangeIndex": range_40, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 45, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_regen_large, "EquippedItemClass": -1, "SpellLevel": 0,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_HEAL,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 0,
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "ImplicitTargetB_1": TARGET_UNIT_LASTTARGET_AREA_PARTY,
                "EffectRadiusIndex_1": 10,  # party spread radius (as Prayer of Healing)
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 400735 Temporal Beacon: 30s buff + ~8/sec passive HoT.
            "id": 400735, "client": True, "template": 774,  # clone Rejuvenation
            "name": "Temporal Beacon", "script": "spell_sod_mage_temporal_beacon",
            "desc": "Records the subject's position in time and space. Damage done "
                    "by caster will result in this subject receiving chronomantic healing.",
            "aura_desc": "Records the subject's position in time and space. Damage done "
                         "by caster will result in this subject receiving chronomantic healing.",
            "overrides": {
                "Attributes": 0, "AttributesEx": 0, "DispelType": DISPEL_MAGIC,
                "SpellVisualID_1": 37,  # Lightning Shield orbiting visual + sounds
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_30s,
                "RangeIndex": range_100, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_beacon, "EquippedItemClass": -1,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_PERIODIC_HEAL,
                "EffectAuraPeriod_1": 1000, "EffectBasePoints_1": 7,  # 8/sec
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 401405 Chronomantic Healing: instant triggered heal (amount at cast).
            "id": 401405, "client": True, "template": 2050,  # clone Lesser Heal
            "name": "Chronomantic Healing",
            "overrides": {
                "Attributes": 0, "AttributesEx": 0,
                "CastingTimeIndex": cast_instant, "DurationIndex": 0,
                "RangeIndex": range_100, "PowerType": 0, "ManaCost": 0,
                "ManaCostPct": 0, "SchoolMask": SCHOOL_MASK_ARCANE,
                "SpellIconID": icon_beacon, "EquippedItemClass": -1,
                "Effect_1": EFFECT_HEAL, "EffectAura_1": 0,
                "EffectBasePoints_1": 0,
                "ImplicitTargetA_1": TARGET_UNIT_TARGET_ALLY,
                "EffectRadiusIndex_1": 0,
                "Effect_2": 0, "EffectAura_2": 0, "ImplicitTargetA_2": 0,
                "Effect_3": 0, "EffectAura_3": 0, "ImplicitTargetA_3": 0,
            },
        },
        {  # 900001 Temporal Beacon (conversion): server-only hidden proc aura.
            "id": 900001, "client": False, "template": None,
            "name": "Temporal Beacon",
            "script": "spell_sod_mage_temporal_conversion",
            "overrides": {
                "Attributes": SPELL_ATTR0_PASSIVE | SPELL_ATTR0_DO_NOT_DISPLAY,
                "CastingTimeIndex": cast_instant, "DurationIndex": dur_perm,
                "RangeIndex": range_self, "SchoolMask": SCHOOL_MASK_ARCANE,
                "EquippedItemClass": -1,
                "ProcTypeMask": PROC_DONE_MAGIC_NEG | PROC_DONE_NONE_NEG | PROC_DONE_RANGED_AUTO,
                "ProcChance": 100,
                "Effect_1": EFFECT_APPLY_AURA, "EffectAura_1": AURA_DUMMY,
                "ImplicitTargetA_1": TARGET_UNIT_CASTER,
                "EffectBasePoints_1": 0,
            },
            # spell_proc row is REQUIRED: an aura with no spell_proc entry never
            # procs (Aura::GetProcEffectMask returns 0). AttributesMask=2
            # (TRIGGERED_CAN_PROC) lets Arcane Missiles' triggered ticks (and the
            # triggered wand shot) feed the beacon. Chance/ProcFlags fall back to
            # the spell_dbc values when left 0.
            #
            # SchoolMask is left 0 (no engine-level school filter) on purpose: the
            # proc engine's school check uses ProcEventInfo::GetSchoolMask(), which
            # returns the *spell's* base school when a proc spell is present — and a
            # wand's "Shoot" (5019) is base-school Physical even when the wand deals
            # Arcane. A SchoolMask=Arcane here would reject the arcane wand. Instead
            # CheckProc filters on the *damage's* school (the real Arcane), which is
            # correct for both spells and wands.
            "proc": {
                "SchoolMask": 0, "SpellFamilyName": 0,
                "SpellFamilyMask0": 0, "SpellFamilyMask1": 0,
                "SpellFamilyMask2": 0,
                "ProcFlags": PROC_DONE_MAGIC_NEG | PROC_DONE_NONE_NEG | PROC_DONE_RANGED_AUTO,
                "SpellTypeMask": 1,    # PROC_SPELL_TYPE_DAMAGE
                "SpellPhaseMask": 2,   # PROC_SPELL_PHASE_HIT
                "HitMask": 0, "AttributesMask": 2, "DisableEffectsMask": 0,
                "ProcsPerMinute": 0, "Chance": 0, "Cooldown": 0, "Charges": 0,
            },
        },
    ]


SPELL_PROC_COLUMNS = [
    "SpellId", "SchoolMask", "SpellFamilyName", "SpellFamilyMask0",
    "SpellFamilyMask1", "SpellFamilyMask2", "ProcFlags", "SpellTypeMask",
    "SpellPhaseMask", "HitMask", "AttributesMask", "DisableEffectsMask",
    "ProcsPerMinute", "Chance", "Cooldown", "Charges",
]


# ---------------------------------------------------------------------------
# Index/icon resolvers (read the client's own aux DBCs).
# ---------------------------------------------------------------------------
def resolve_indexes(aux_dir):
    idx = {"cast": {}, "dur": {}, "range": {}, "icon": {}}

    rows, _ = load_aux(os.path.join(aux_dir, "SpellCastTimes.dbc"))
    for r in rows:  # id, base, perlevel, min
        idx["cast"].setdefault(r[1], r[0])

    rows, _ = load_aux(os.path.join(aux_dir, "SpellDuration.dbc"))
    for r in rows:  # id, duration, perlevel, max
        idx["dur"].setdefault(r[1], r[0])

    with open(os.path.join(aux_dir, "SpellRange.dbc"), "rb") as fh:
        raw = fh.read()
    _, nrec, nfield, recsize, _ = struct.unpack("<4siiii", raw[:20])
    for i in range(nrec):
        rid = struct.unpack_from("<i", raw, 20 + i * recsize)[0]
        maxr = struct.unpack_from("<f", raw, 20 + i * recsize + 12)[0]  # RangeMax[0]
        idx["range"].setdefault(round(maxr, 3), rid)

    with open(os.path.join(aux_dir, "SpellIcon.dbc"), "rb") as fh:
        raw = fh.read()
    _, nrec, nfield, recsize, _ = struct.unpack("<4siiii", raw[:20])
    body = 20 + nrec * recsize
    sb = raw[body:]
    for i in range(nrec):
        iid, off = struct.unpack_from("<ii", raw, 20 + i * recsize)
        end = sb.find(b"\x00", off)
        name = sb[off:end].decode("latin-1").replace("/", "\\").split("\\")[-1].lower()
        idx["icon"].setdefault(name, iid)
    return idx


# ---------------------------------------------------------------------------
# MPQ extraction / packing (pympq / StormLib).
# ---------------------------------------------------------------------------
def mpq_priority(path):
    """Approximate WoW's archive load priority (higher wins).

    Patch suffixes load after the plain archive in the order 2..9 then a..z,
    so `patch-enus-s.mpq` overrides `patch-enus.mpq` even though it sorts
    earlier alphabetically. Non-patch base archives rank lowest.
    """
    name = os.path.basename(path).lower()
    m = re.match(r"patch(?:-[a-z]{2,4})?(?:-([0-9a-z]+))?\.mpq$", name)
    if not m:
        return (0, 0)  # base archive (common/locale/base/expansion/lichking)
    suffix = m.group(1) or ""
    if suffix == "":
        rank = 0
    elif suffix.isdigit():
        rank = int(suffix)
    else:
        rank = 10 + (ord(suffix[0]) - ord("a"))
    return (1, rank)


def extract_client_dbcs(client_dir, dest):
    import pympq
    os.makedirs(dest, exist_ok=True)
    locale = os.path.join(client_dir, "data", "enus")
    base = os.path.join(client_dir, "data")
    search = []
    for d in (base, locale):
        if os.path.isdir(d):
            search += [os.path.join(d, f) for f in os.listdir(d)
                       if f.lower().endswith(".mpq")
                       and f.lower() != PATCH_MPQ_NAME.lower()]  # not our own output

    def extract(name):
        inner = "DBFilesClient\\" + name
        found = None
        best = (-1, -1)
        for p in search:  # pick the highest-priority archive that has the file
            try:
                m = pympq.open_archive(p, None)
            except Exception:
                continue
            try:
                if m.has_file(inner) and mpq_priority(p) > best:
                    best = mpq_priority(p)
                    found = p
            finally:
                m.close()
        if not found:
            raise RuntimeError("not found in any archive: " + name)
        m = pympq.open_archive(found, None)
        try:
            m.extract_file(inner, os.path.join(dest, name))
        finally:
            m.close()
        return found

    used = {}
    for dbc in ("Spell.dbc", "SpellCastTimes.dbc", "SpellDuration.dbc",
                "SpellRange.dbc", "SpellRadius.dbc", "SpellIcon.dbc",
                "SkillLineAbility.dbc"):
        used[dbc] = extract(dbc)
    return used


def build_skill_line_ability(workdir, spells):
    """Append SkillLineAbility rows so categorized spells land in the right
    spellbook tab (the client groups known spells into tabs by their skill
    line). Returns the patched file path, or None if no spell opts in.

    Rows are zeroed except ID / SkillLine / Spell: no race or class restriction,
    no skill-rank requirement, and AcquireMethod 0 (not trainer/auto-learned) —
    purely a display-categorization entry. The full (base + appended) file is
    packed into the patch MPQ, overriding the client's copy.
    """
    targets = [s for s in spells if s.get("client") and s.get("skill_line")]
    if not targets:
        return None

    sla = WDBC.load(os.path.join(workdir, "SkillLineAbility.dbc"))
    next_id = max(sla.get_int(r, 0) for r in sla.records) + 1
    for s in targets:
        rec = bytearray(sla.recsize)
        sla.set_int(rec, 0, next_id)          # ID
        sla.set_int(rec, 1, s["skill_line"])  # SkillLine (237 = Arcane)
        sla.set_int(rec, 2, s["id"])          # Spell
        sla.records.append(rec)
        print("[*] SkillLineAbility row added: spell %d -> skill line %d (id %d)"
              % (s["id"], s["skill_line"], next_id))
        next_id += 1

    out = os.path.join(workdir, "SkillLineAbility.dbc.patched")
    with open(out, "wb") as fh:
        fh.write(sla.serialize())
    return out


def pack_mpq(files, out_mpq):
    """files: list of (src_path, inner_mpq_path) to add to a fresh patch MPQ."""
    import pympq
    if os.path.exists(out_mpq):
        os.remove(out_mpq)
    m = pympq.create_archive(
        out_mpq,
        [pympq.MPQ_CREATE_ARCHIVE_V1, pympq.MPQ_CREATE_LISTFILE,
         pympq.MPQ_CREATE_ATTRIBUTES],
        8)
    try:
        for src, inner in files:
            m.add_file(src, inner,
                       [pympq.MPQ_FILE_COMPRESS, pympq.MPQ_FILE_REPLACEEXISTING],
                       [pympq.MPQ_COMPRESSION_ZLIB])
    finally:
        m.close()


# ---------------------------------------------------------------------------
# SQL emission (minimal named-column INSERT; unset columns take table default).
# ---------------------------------------------------------------------------
# IDs we used to ship but no longer do — cleaned out of every table we touch so
# re-applying the base SQL removes the orphan server rows (the MPQ is rebuilt
# fresh, so it never carries them).
RETIRED_IDS = [900002]  # old conversion-aura id; conversion moved to 900001


def emit_sql(spells, cols):
    ids = ",".join(str(s["id"]) for s in spells)
    out = [
        "-- mod-sod-mage: server-side spell_dbc rows for Regeneration /",
        "-- Temporal Beacon. GENERATED by tools/build_sod_mage_patch.py -",
        "-- do not edit by hand. The 4xxxxx IDs are mirrored by the client MPQ.",
        "",
        "DELETE FROM `spell_dbc` WHERE `ID` IN (%s);" % ids,
    ]
    if RETIRED_IDS:
        r = ",".join(str(i) for i in RETIRED_IDS)
        out += ["-- retired ids (removed across every table we own)",
                "DELETE FROM `spell_dbc` WHERE `ID` IN (%s);" % r,
                "DELETE FROM `spell_script_names` WHERE `spell_id` IN (%s);" % r,
                "DELETE FROM `spell_proc` WHERE `SpellId` IN (%s);" % r]
    out.append("")
    for s in spells:
        row = dict(s["overrides"])
        row["ID"] = s["id"]
        row["Name_Lang_enUS"] = s["name"]
        row["Name_Lang_Mask"] = NAME_MASK
        colnames = []
        values = []
        for col in cols:  # stable column order
            if col in row:
                colnames.append("`%s`" % col)
                v = row[col]
                values.append("'%s'" % v.replace("'", "''") if isinstance(v, str)
                              else str(v))
        out.append("INSERT INTO `spell_dbc` (%s) VALUES (%s);"
                   % (", ".join(colnames), ", ".join(values)))

    scripted = [s for s in spells if s.get("script")]
    if scripted:
        sids = ",".join(str(s["id"]) for s in scripted)
        out += ["",
                "DELETE FROM `spell_script_names` WHERE `spell_id` IN (%s);" % sids]
        vals = ",\n".join("(%d,'%s')" % (s["id"], s["script"]) for s in scripted)
        out.append("INSERT INTO `spell_script_names` "
                   "(`spell_id`, `ScriptName`) VALUES\n%s;" % vals)

    bonuses = [s for s in spells if s.get("bonus")]
    if bonuses:
        bids = ",".join(str(s["id"]) for s in bonuses)
        out += ["", "DELETE FROM `spell_bonus_data` WHERE `entry` IN (%s);" % bids]
        for s in bonuses:
            b = s["bonus"]
            out.append("INSERT INTO `spell_bonus_data` "
                       "(`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) "
                       "VALUES (%d, %s, %s, %s, %s, 'mod-sod-mage %s');"
                       % (s["id"], b["direct"], b["dot"], b["ap"], b["ap_dot"], s["name"]))

    procs = [s for s in spells if s.get("proc")]
    if procs:
        pids = ",".join(str(s["id"]) for s in procs)
        out += ["", "DELETE FROM `spell_proc` WHERE `SpellId` IN (%s);" % pids]
        for s in procs:
            row = dict(s["proc"])
            row["SpellId"] = s["id"]
            vals = ", ".join(str(row[c]) for c in SPELL_PROC_COLUMNS)
            cn = ", ".join("`%s`" % c for c in SPELL_PROC_COLUMNS)
            out.append("INSERT INTO `spell_proc` (%s) VALUES (%s);" % (cn, vals))
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--client", default=DEFAULT_CLIENT,
                    help="WoW client root (contains Data/ or data/)")
    ap.add_argument("--workdir", default=os.path.join(HERE, "_work"),
                    help="scratch dir for extracted DBCs / patched output")
    ap.add_argument("--dry-run", action="store_true",
                    help="emit SQL + patched DBC but do not write the MPQ")
    args = ap.parse_args()

    print("[*] extracting client DBCs from", args.client)
    used = extract_client_dbcs(args.client, args.workdir)
    for k, v in used.items():
        print("    %-22s <- %s" % (k, os.path.basename(v)))

    idx = resolve_indexes(args.workdir)
    print("[*] resolved indexes: cast3s=%s dur3s=%s dur30s=%s durPerm=%s "
          "r40=%s r100=%s rSelf=%s icons=%s"
          % (idx["cast"].get(3000), idx["dur"].get(3000), idx["dur"].get(30000),
             idx["dur"].get(-1), idx["range"].get(40.0), idx["range"].get(100.0),
             idx["range"].get(0.0),
             {k: idx["icon"].get(k) for k in
              ("spell_nature_timestop", "inv_enchant_essencemysticalsmall")}))

    spells = build_spells(idx)
    cols = load_columns(TABLE_DEF)

    # --- build patched client Spell.dbc ---
    spell = WDBC.load(os.path.join(args.workdir, "Spell.dbc"))
    field_of = {c: i for i, c in enumerate(cols)}
    for s in spells:
        if not s["client"]:
            continue
        if s["id"] in [spell.get_int(r, 0) for r in spell.records]:
            raise RuntimeError("ID already exists in Spell.dbc: %d" % s["id"])
        base = bytearray(spell.find(s["template"]))
        for col, val in s["overrides"].items():
            spell.set_int(base, field_of[col], val)
        spell.set_int(base, field_of["ID"], s["id"])
        spell.set_int(base, field_of["Name_Lang_enUS"], spell.add_string(s["name"]))
        spell.set_int(base, field_of["Name_Lang_Mask"], NAME_MASK)
        # Optional client-only tooltip strings ($o1 = total periodic heal, $d = duration).
        if s.get("desc"):
            spell.set_int(base, field_of["Description_Lang_enUS"], spell.add_string(s["desc"]))
            spell.set_int(base, field_of["Description_Lang_Mask"], NAME_MASK)
        if s.get("aura_desc"):
            spell.set_int(base, field_of["AuraDescription_Lang_enUS"], spell.add_string(s["aura_desc"]))
            spell.set_int(base, field_of["AuraDescription_Lang_Mask"], NAME_MASK)
        spell.records.append(base)
        print("[*] client row added: %d (%s) from template %d"
              % (s["id"], s["name"], s["template"]))

    patched = os.path.join(args.workdir, "Spell.dbc.patched")
    with open(patched, "wb") as fh:
        fh.write(spell.serialize())
    print("[*] wrote patched Spell.dbc (%d records)" % len(spell.records))

    # --- build patched client SkillLineAbility.dbc (spellbook tab grouping) ---
    sla_patched = build_skill_line_ability(args.workdir, spells)

    # --- emit SQL ---
    os.makedirs(os.path.dirname(SQL_OUT), exist_ok=True)
    with open(SQL_OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(emit_sql(spells, cols))
    print("[*] wrote SQL ->", SQL_OUT)

    # --- pack MPQ ---
    if args.dry_run:
        print("[*] dry-run: skipping MPQ pack")
        return
    out_mpq = os.path.join(args.client, "data", "enus", PATCH_MPQ_NAME)
    files = [(patched, INNER)]
    if sla_patched:
        files.append((sla_patched, SKILL_INNER))
    pack_mpq(files, out_mpq)
    print("[*] wrote client patch -> %s (%d DBC file(s))" % (out_mpq, len(files)))


if __name__ == "__main__":
    sys.exit(main())
