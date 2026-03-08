# s2

`s2` (simple screenshot / suckless screenshots) is a keyboard-first screenshot annotation and redaction tool.

## Status

Early scaffold. See `SPEC.md` for the v1 contract and roadmap.

## Build

```sh
make
```

Dependencies (Arch):

```sh
sudo pacman -S libx11 libxft freetype2 libpng xclip maim slop
```

## Install

```sh
sudo make clean install
```

## Suckless-style config flow

- `config.h` is generated from `config.def.h` during build.
- Edit `config.h` for local customization.
- Save defaults can be customized in `config.h` with `default_save_directory`.
- Tool defaults can be customized in `config.h` with `default_palette_index`, `default_thickness_index`, and `default_text_scale`.
- Before reinstalling after upstream changes, remove local `config.h` so defaults can refresh:

```sh
rm -f config.h
sudo make clean install
```

You can also run:

```sh
make distclean
```

## Planned usage

```sh
maim -s | s2 -i -
```

## Current controls

- `Ctrl+C`: copy rendered image to clipboard and exit success
- `Ctrl+Y`: copy current image to clipboard
- `Ctrl+S`: save to timestamped file in cwd (`YYYY-MM-DDTHHMM.png`)
- `Enter`: save and exit
- `q` or `Esc`: quit
- `s`: selection tool
- `a`: arrow tool
- `l`: line tool
- `r`: rectangle tool
- `o`: circle tool
- `t`: text tool
- `h`: highlight tool
- `b`: blur tool
- `p`: pixelate tool
- `c`: color picker tool (sample from image)
- `h/j/k/l` or arrow keys: move keyboard cursor by 1px
- `H/J/K/L`: move keyboard cursor by 10px
- `Space` or left click: apply tool action
- `[` / `]`: thickness, text size, or pixelate block size
- `[` / `]` on highlight tool changes `hl-strength` (1..100)
- `Ctrl+Z`: undo last action
- `Ctrl+Shift+Z`: redo
- `Ctrl+V` style clipboard use depends on target app supporting image MIME from X11 selections
- `1..9`: select color from palette
- `#`: enter hex color mode (type 6 hex chars, Enter to apply)
- `f`: toggle fill mode (filled circles + text background with inverse contrast)
- `x`: cancel pending anchor (arrow/circle/pixelate/blur)
- `Backspace`/`Delete`: delete selected object

Tool flow:

- Arrow/Circle/Pixelate/Blur keyboard flow: first `Space` sets anchor, second `Space` commits.
- Arrow/Circle/Pixelate/Blur mouse flow: press sets anchor, release commits at current cursor.
- Text: `Space` enters text input mode, type, `Enter` commits text.
- Text live-preview: while typing, current text is previewed on image before commit.
- Picker: `Space` samples color under cursor into active color.
- Selection: click object to select, drag to move, `Backspace`/`Delete` to remove.

CLI additions:

- `-C <class>` sets X11 `WM_CLASS` (instance/class) for window rules in WMs like dwm.
- `-D <absdir>` sets absolute save directory for Ctrl+S timestamp files (overrides config default).

UI behavior:

- While dragging/selecting anchored tools, a rectangle guide is shown.
- Status is shown in a dedicated bottom bar (not drawn on top of the image).
- Window/status colors follow a best-effort light/dark preference detection from environment/X resources.
- Window resize scales image and all objects to fit window canvas.

`config.def.h` now includes `font_name` for text rendering defaults.
