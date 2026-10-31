/* Built-in fallback for older config.h files. Override in config.def.h/config.h. */
#ifndef S2_THEME_PRESETS
/* Theme mapping: modifier + 1..9 selects the matching row, like palette[].
 * modifier + C (or modifier-click the color control) opens the theme picker.
 * Columns: name, workspace, dots, panel, text, accent, selected, hover, border.
 * Mod1Mask = Alt; use Mod4Mask for Super if preferred. */
#define S2_THEME_MODIFIER Mod1Mask
#define S2_DEFAULT_THEME 0
#define S2_THEME_PRESETS { \
 {"Dark",0x0c0d0f,0x30323a,0x181a20,0xaab1c2,0xb3caff,0x303d58,0x272c38,0x3a3e4a}, \
 {"Light",0xf4f5f7,0xd0d5df,0xffffff,0x394150,0x2459b8,0xdce8ff,0xe9edf4,0xc4cbd7}, \
 {"Tokyo Night",0x16161e,0x343b58,0x1a1b26,0xa9b1d6,0x7aa2f7,0x283457,0x24283b,0x414868}, \
 {"Gruvbox",0x1d2021,0x504945,0x282828,0xebdbb2,0xfabd2f,0x504945,0x3c3836,0x665c54}, \
 {"Dracula",0x21222c,0x44475a,0x282a36,0xf8f8f2,0xbd93f9,0x4b3d65,0x383a49,0x6272a4}, \
 {"Nord",0x242933,0x434c5e,0x2e3440,0xd8dee9,0x88c0d0,0x434c5e,0x3b4252,0x4c566a}, \
 {"Catppuccin",0x11111b,0x313244,0x1e1e2e,0xcdd6f4,0xcba6f7,0x45405c,0x313244,0x585b70}, \
 {"Solarized",0x00212b,0x164450,0x002b36,0x93a1a1,0x2aa198,0x164450,0x073642,0x586e75}, \
 {"Rose Pine",0x191724,0x393552,0x1f1d2e,0xe0def4,0xebbcba,0x524052,0x26233a,0x6e6a86} }

#endif
#ifndef S2_THEME_MODIFIER
#define S2_THEME_MODIFIER Mod1Mask
#endif
#ifndef S2_DEFAULT_THEME
#define S2_DEFAULT_THEME 0
#endif
struct ui_theme {
 const char *name;
 unsigned int workspace, dots, panel, text, accent, selected, hover, border;
};
static const struct ui_theme themes[] = S2_THEME_PRESETS;
#define THEME_COUNT (sizeof themes / sizeof *themes)
