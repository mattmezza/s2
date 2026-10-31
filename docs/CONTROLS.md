# Controls and behavior

The README contains the main shortcut tables. This guide covers the interactions that depend on context.

## Window lifecycle

The default window advertises X11 NORMAL type, supports resizing/tiling, and honors WM_DELETE_WINDOW. `--modal` advertises DIALOG and MODAL hints, supplies a centered initial position, and lets the WM decide placement. `--normal-window` remains a compatibility alias for the default. `-C` overrides WM_CLASS without changing these modes.

Esc in a regular window cancels pending input and interactions, closes transient UI, switches to Select, and clears selection. Esc in a modal window first dismisses transient UI/text entry; otherwise it cancels and quits. About dismisses on Esc without quitting in either mode. `q` and WM Quit cancel with status 0. A deliberate forced X11 kill/server disconnect remains an external termination.

Enter commits text or applies a custom hex color; it never saves an image. Ctrl+S and Save commit pending input and save/exit. An explicit `-o` path is honored; otherwise a local-time timestamp with seconds and six microsecond digits is used. Timestamp creation is exclusive, with numeric collision suffixes. `-D` overrides the timestamp directory and must be absolute. An explicit output path retains overwrite semantics.

Ctrl+C commits pending input and copies/exits; Ctrl+Y copies without exiting. `-c` requests copying when finishing successfully; cancellation skips output. Real load/save/copy errors return nonzero.

## Selection and appearance

New annotations are selected automatically. Clicking an object selects it; dragging empty canvas creates a marquee, including objects that intersect it. Dragging a selected member moves the group. Shift constrains movement to one axis. Delete removes selected objects.

Square handles resize; Shift preserves aspect ratio. Text and pins always resize proportionally. The round handle rotates; Shift snaps to 45°. Comma/period rotate by 15°. `<`/`>` rotate by 1° except on selected arrows, where they toggle start/end decorations.

The midpoint diamond curves lines/arrows; Shift-drag straightens them. Arrow end styles cycle with `e` and reverse with `E`. The configured cycle uses IDs 0=open, 1=filled triangle, 2=outlined triangle, 3=diamond, 4=dot, 5=bar.

Brackets and the dimension picker edit selected objects directly: stroke width, text/pin size, marker width, highlight strength, blur radius, or pixelate block size. Rectangle/circle outline steps are 3 px; text/pins 8 px; marker 8 px; highlight 5 percentage points; blur 3 px. Unsupported fill-color operations are ignored.

Plain palette numbers apply foreground/stroke color to colorable selections. Shift+numbers apply independent fill colors to rectangles, circles and text, enabling fill. The color picker exposes matching Stroke/Text and Fill tabs, custom hex input, and image sampling. Shift-click opens its Fill target. Text reserves c/C for corner radius, so use the color control for sampling while text is selected.

`f` controls fill; Shift+F independently controls rectangle/circle outlines. `g` controls gradients on rectangles, circles, text backgrounds, number pins, highlights, and filled arrow ends. Each updates compatible selected actions and the current default for future actions. The outline and fill toolbar icons show these states. Turning both rectangle/circle fill and outline off intentionally produces an invisible object that remains selectable. Chrome gradients are separate from annotation gradients.

## Text entry

The editor is single-line. Enter commits and selects; t on selected text reopens it in place. Typing, UTF-8 navigation, Home/End, Shift selection, Ctrl+A, Backspace/Delete and paste are supported. Mouse clicking finishes entry rather than positioning a caret. Esc restores an existing edited object or cancels a new one. Committing empty replacement text deletes that object.

Selected-text shortcuts override tool shortcuts: b/B weight, i italic, u underline, s strikethrough, p/P padding, c/C corner radius, and brackets font size. Palette and picker controls preserve the other typography properties.

## Footer and dialogs

Theme, zoom and Fit controls stay accessible in narrow windows; information moves to a second footer row. The question-mark button opens help. The info button opens a themed, responsive About card with version, attribution and license. Its repository link invokes xdg-open with the literal HTTPS URL. Tab/Shift+Tab navigate the repository and return button; Enter/Space activate the focused control. Close, backdrop click and Esc dismiss it. Dialogs block annotation and export shortcuts behind them.
