# s2

![s2 cover](cover-og.png)

`s2` (simple screenshot / suckless screenshots) is a keyboard-first screenshot annotation and redaction tool.

## Build

```sh
make
```

## Installation

### Dependencies (Arch Linux)

```sh
sudo pacman -S libx11 libxft freetype2 libpng pango cairo harfbuzz xclip maim slop
```

### Dependencies (generic Linux)

Install equivalents for:

- X11 development headers/libs (`libX11`)
- Xft + FreeType (`libXft`, `freetype2`)
- text shaping/rendering stack (`pango`, `cairo`, `harfbuzz`)
- PNG library (`libpng`)
- clipboard helper (`xclip`)
- optional capture workflow tools (`maim`, `slop`)

### Build

```sh
make
```

### Install to system

```sh
sudo make clean install
```

Install destination defaults to `/usr/local/bin/s2` and can be changed in `config.mk`.

## Suckless-style config flow

- `config.h` is generated from `config.def.h` during build.
- Edit `config.h` for local customization.
- Save defaults can be customized in `config.h` with `default_save_directory`.
- Tool defaults can be customized in `config.h` with `default_palette_index`, `default_thickness_index`, and `default_text_scale`.
- UI defaults can be customized in `config.h` with `default_tool_index` and `selection_bbox_color`.
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

## Quick Start

```sh
maim -s | s2 -i -
```

- `Ctrl+C`: copy and exit
- `Ctrl+S`: save timestamped PNG and exit (prints saved absolute path)
- `q` / `Esc`: quit/cancel (non-zero exit)

## Current controls

- `Ctrl+C`: copy rendered image to clipboard and exit success
- `Ctrl+Y`: copy current image to clipboard
- `Ctrl+V`: paste clipboard text only (text MIME); starts/appends text input
- `Ctrl+S`: save to timestamped file in cwd (`YYYY-MM-DDTHHMM.png`)
- `Enter`: save and exit
- `q` or `Esc`: quit/cancel (returns non-zero)
- `s`: selection tool
- `a`: arrow tool
- `l`: line tool
- `r`: rectangle tool
- `o`: circle tool
- `t`: text tool
- `h`: highlight tool
- `b`: blur tool
- `p`: pen tool (freehand)
- `n`: number tool (auto-incrementing markers)
- `x`: pixelate tool
- `c`: color picker tool (sample from image)
- `h/j/k/l` or arrow keys: move keyboard cursor by 1px
- `H/J/K/L`: move keyboard cursor by 10px
- `Space` or left click: apply tool action
- `[` / `]`: thickness, text size, or pixelate block size
- `[` / `]` on highlight tool changes `hl-strength` (1..100)
- `Ctrl+Z`: undo last action
- `Ctrl+Shift+Z`: redo
- `1..9`: select color from palette
- `#`: enter hex color mode (type 6 hex chars, Enter to apply)
- `f`: toggle fill mode (filled circles + text background with inverse contrast)
- `X`: cancel pending anchor or active pen/text input
- `?`: toggle keybindings help panel
- `Backspace`/`Delete`: delete selected object

Text mode note:

- Pressing `Esc` while typing exits text input first; pressing `Esc` again then quits.

Tool flow:

- Arrow/Circle/Pixelate/Blur keyboard flow: first `Space` sets anchor, second `Space` commits.
- Arrow/Circle/Pixelate/Blur mouse flow: press sets anchor, release commits at current cursor.
- Text: click or `Space` enters text input mode; type with live preview; `Enter` or click commits text.
- Text live-preview: while typing, current text is previewed on image before commit.
- Picker: `Space` samples color under cursor into active color.
- Selection: click object to select (shows configurable bounding box), drag moves only the bbox preview until release, then commits object move.
- Pen: click-hold and drag to draw freehand using current thickness.
- Number: each click places a filled numbered circle (`1`, `2`, `3`, ...) using current color; number size follows text-size.

CLI additions:

- `-C <class>` sets X11 `WM_CLASS` (instance/class) for window rules in WMs like dwm.
- `-D <absdir>` sets absolute save directory for Ctrl+S timestamp files (overrides config default).
- `--normal-window` uses X11 normal window type instead of the default dialog window type.

UI behavior:

- While dragging/selecting anchored tools, a rectangle guide is shown.
- Status is shown in a dedicated bottom bar (not drawn on top of the image).
- Window/status colors follow a best-effort light/dark preference detection from environment/X resources.
- Window resize scales image and all objects to fit window canvas.
- Editor enforces a minimum usable window size so very small captures remain operable.

`config.def.h` now includes `font_name` for text rendering defaults.

Text fill styling in `config.def.h`:

- `text_fill_padding`: background padding around filled text
- `text_fill_corner_radius`: rounded corner radius for filled text background
- `window_padding`: outer margin around canvas/status so bars stay visible at large sizes
- `max_text_scale`: max text size reachable with `]` in text tool

Keyboard movement:

- Cursor movement now uses arrow keys only (`Left`/`Right`/`Up`/`Down`)
