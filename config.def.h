/* See LICENSE file for copyright and license details. */

/* runtime behavior */
static const char *default_window_class = "s2";
static const int default_fill_mode = 1;
/* g toggles filled-shape gradients; Shift+F toggles rect/circle outlines.
 * Both affect the selection and the defaults for subsequent shapes. */
#define S2_DEFAULT_GRADIENT_FILL 1
#define S2_DEFAULT_SHAPE_OUTLINE 1
static const char *default_save_directory = "~/screenshots";
static const int window_padding = 16;

/* tool palette (hex RGB, no leading # in code) */
static const unsigned int palette[] = {
	0xff0000, /* red */
	0x00ff00, /* green */
	0x5689ef, /* blue */
	0xffff00, /* yellow */
	0xff8800, /* orange */
	0xffffff, /* white */
	0x000000, /* black */
	0x00ffff, /* cyan */
	0xff00ff, /* magenta */
};

/* default tool values */
static const int default_palette_index = 2;
static const int default_thickness_index = 4;
static const int default_text_scale = 4;
static const int default_highlight_strength = 25;
static const int default_pixelate_block = 8;
static const int default_blur_radius = 5;
static const int default_marker_height = 24;   /* text-highlight band height (image px) */
static const int default_marker_strength = 40;  /* text-highlight alpha/strength (1..100) */

/* UI defaults */
/* tool index mapping: 0=select, 1=arrow, 2=line, 3=pen, 4=number, 5=rect, 6=circle, 7=text, 8=highlight, 9=marker, 10=pixelate, 11=blur, 12=picker */
static const int default_tool_index = 1;
static const unsigned int selection_bbox_color = 0x00ffffu;

/* Arrow ends: 0=open, 1=filled triangle, 2=outlined triangle,
 * 3=diamond, 4=dot, 5=bar. e cycles selected arrows in this order.
 * < toggles the start end; > toggles the finish end. */
#define S2_ARROW_EDGE_CYCLE {0,1,2,3,4,5}
#define S2_DEFAULT_ARROW_EDGE_STYLE 0

/* stroke/effect presets */
static const int thickness_presets[] = { 1, 2, 3, 5, 8 };

/* text rendering */
#define S2_DEFAULT_TEXT_WEIGHT 500 /* Geist Medium; 100..900 */
static const char *font_name = "Geist, Noto Color Emoji:size=16";
static const int text_fill_padding = 3;
static const int text_fill_corner_radius = 2;
static const int max_text_scale = 16;
//static const char *font_name = "DejaVu Serif:size=16";
/* if you don't care about emojis you can use a cool monospace defaul
 * static const char *font_name = "monospace:size=16";
 */

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
