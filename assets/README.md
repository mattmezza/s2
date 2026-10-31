# UI assets

Geist upright and italic variable fonts (100–900), plus the original Regular and Medium source assets, are bundled from [vercel/geist-font](https://github.com/vercel/geist-font/tree/main/fonts/Geist).
The SIL Open Font License is included in `fonts/OFL.txt`. The app registers these
fonts privately with Fontconfig; no system font installation is required.
`scripts/embed_fonts.py` embeds both variable fonts and their license in the executable. At runtime they are registered through anonymous in-memory files; no font or icon files need to accompany the binary. Use `s2 --licenses` to read the embedded license.

The icon atlas was generated with Runware (`runware:101@1`) using the user's
`RUNWARE_API_KEY`. `icons/generation.json` records its prompt, task ID and cost;
credentials are never saved. `scripts/prepare_icons.py` extracts the usable
white glyphs into transparent PNGs and embedded 48px alpha masks. Straight lines,
arrows, rectangles, circles, text and number glyphs are drawn geometrically for
precise tool semantics. App startup makes no network/API requests.
