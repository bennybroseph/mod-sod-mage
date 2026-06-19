# Contributing to mod-sod-mage

Want to add a spell, rune, or item? Start with a template.

1. Open **[`sod-class-templates`](https://github.com/mod-sod/sod-class-templates)** and find the content type you're
   adding (spell, rune, item, and a couple of helpers).
2. Copy the skeleton into its real location and fill the `<PLACEHOLDER>` blanks.
3. Follow the full recipe in **[`docs/`](docs/Home.md)** — architecture, the spell
   generator, deploy & verify, and the gotchas that bite.

Source accurate SoD values from the places listed in
[`sod-class-templates`](https://github.com/mod-sod/sod-class-templates#sources-of-truth) (wago.tools, Wowhead
Classic, the AzerothCore wiki).

Keep custom content greppable: prefix C++ classes and SQL variables with
`sod_mage_` / `SOD_MAGE`, config keys with `SodMage.`, and reuse real SoD ids where
they exist (else use the documented id bands). No core edits — everything stays under
`modules/mod-sod-mage/`.
