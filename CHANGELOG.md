# Changelog

## Unreleased

- Redesign the native editor with CUI, embedded Geist fonts/icons, centered responsive bars, themes, and a themed About dialog with a repository link.
- Make regular tileable windows the default; add `--modal` for floating dialog hints and mode-aware Escape behavior. Support graceful WM Quit with successful cancellation status.
- Improve cursor/drawing responsiveness through X-server capture caching, cached chrome/text, bounded motion coalescing, cropped previews, and exact sliding-sum blur.
- Add shape/text rotation, aspect-preserving resize, constrained drawing, curved connectors, configurable arrow ends, reverse cycling and endpoint toggles.
- Select new annotations automatically; expose selected-object dimensions, stroke/fill colors, gradient fills and independent rectangle/circle outlines.
- Improve numbered pins and text shaping, typography, styling, caret/selection editing, UTF-8 handling and embedded font distribution.
- Add cursor-centered zoom, Ctrl-drag panning and reliable keyboard zoom/fit.
- Use seconds/microseconds and exclusive creation for timestamped saves. Save only with Ctrl+S/Save, honoring explicit `-o` paths; Enter only finishes input.
- Add man page, current screenshots, configuration/controls documentation, and isolated native/regression/performance checks.
