#ifndef S2_SETTINGS_H
#define S2_SETTINGS_H

#include <limits.h>

enum tool {
	TOOL_SELECT,
	TOOL_ARROW,
	TOOL_LINE,
	TOOL_PEN,
	TOOL_NUMBER,
	TOOL_RECT,
	TOOL_CIRCLE,
	TOOL_TEXT,
	TOOL_HIGHLIGHT,
	TOOL_PIXELATE,
	TOOL_BLUR,
	TOOL_PICKER,
	TOOL_MARKER,
};

enum { THEME_COUNT = 9, SELECTION_THEME_ACCENT = 0x1000000 };

struct ui_theme {
	char name[32];
	unsigned int workspace, dots, panel, text, accent, selected, hover, border;
};

struct settings {
	char window_class[128];
	char save_directory[PATH_MAX];
	int window_padding;
	enum tool tool;
	int fill, gradient_fill, shape_outline;
	int palette[9], palette_count, palette_default;
	int stroke_widths[8], stroke_count, stroke_default;
	int arrow_edge_cycle[6], arrow_edge_count, arrow_edge_style;
	int highlight_strength, pixelate_block, blur_radius;
	int marker_height, marker_strength;
	char font[160];
	int text_scale, max_text_scale, text_weight;
	int text_fill_padding, text_fill_corner_radius;
	int theme_default, selection_bbox_color;
	unsigned int theme_modifier;
	struct ui_theme themes[THEME_COUNT];
};

extern struct settings settings;

void settings_load_defaults(void);
void settings_load(void);
int settings_print_defaults(void);

#endif
