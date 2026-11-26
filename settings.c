#include <X11/X.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

static const char defaults[] =
#include "settings-defaults.h"
;

struct settings settings;

static void
invalid_setting(const char *path, const char *group, const char *key)
{
	if (!path)
		g_error("Invalid or missing built-in s2 setting [%s] %s", group, key);
	fprintf(stderr, "s2: %s: ignoring invalid [%s] %s\n", path, group, key);
}

static char *
read_value(GKeyFile *file, const char *path, const char *group, const char *key)
{
	char *value = g_key_file_get_value(file, group, key, NULL);
	if (!path && !value)
		invalid_setting(path, group, key);
	return value ? g_strstrip(value) : NULL;
}

static int
parse_number(const char *value, int min, int max, int *out)
{
	char *end;
	long n = strtol(value, &end, 10);
	if (end == value || *end || n < min || n > max)
		return 0;
	*out = n;
	return 1;
}

static int
parse_color(const char *value, int *out)
{
	if (*value == '#')
		value++;
	if (strlen(value) != 6 || strspn(value, "0123456789abcdefABCDEF") != 6)
		return 0;
	*out = strtol(value, NULL, 16);
	return 1;
}

static void
read_text(GKeyFile *file, const char *path, const char *group, const char *key,
          char *out, size_t size)
{
	g_autofree char *value = read_value(file, path, group, key);
	if (!value)
		return;
	if (!*value || strlen(value) >= size || !g_utf8_validate(value, -1, NULL)) {
		invalid_setting(path, group, key);
		return;
	}
	strcpy(out, value);
}

static void
read_color(GKeyFile *file, const char *path, const char *group, const char *key,
           unsigned int *out)
{
	g_autofree char *value = read_value(file, path, group, key);
	int color;
	if (!value)
		return;
	if (parse_color(value, &color))
		*out = color;
	else
		invalid_setting(path, group, key);
}

static int
read_choice(GKeyFile *file, const char *path, const char *group, const char *key,
            const char *const choices[], int fallback)
{
	g_autofree char *value = read_value(file, path, group, key);
	if (!value)
		return fallback;
	for (int i = 0; choices[i]; i++)
		if (g_ascii_strcasecmp(value, choices[i]) == 0)
			return i;
	invalid_setting(path, group, key);
	return fallback;
}

/* Validate the entire list before replacing the current values. */
static void
read_list(GKeyFile *file, const char *path, const char *group, const char *key,
          int out[], int capacity, int *count, int min, int max)
{
	g_autofree char *value = read_value(file, path, group, key);
	if (!value)
		return;
	g_auto(GStrv) items = g_strsplit(value, ",", capacity + 1);
	int length = g_strv_length(items);
	int parsed[capacity];
	int colors = strcmp(group, "palette") == 0;
	if (length > 0 && length <= capacity) {
		int i;
		for (i = 0; i < length; i++) {
			char *item = g_strstrip(items[i]);
			int valid = colors
			    ? parse_color(item, &parsed[i])
			    : parse_number(item, min, max, &parsed[i]);
			if (!valid)
				break;
			/* Numeric lists need unique values for cycling and size selection. */
			if (!colors) {
				int j = 0;
				while (j < i && parsed[j] != parsed[i])
					j++;
				if (j < i)
					break;
			}
		}
		if (i == length) {
			memcpy(out, parsed, length * sizeof(*out));
			*count = length;
			return;
		}
	}
	invalid_setting(path, group, key);
}

static void
read_theme(GKeyFile *file, const char *path, int index)
{
	char group[16];
	struct ui_theme *theme = &settings.themes[index];
	struct { const char *name; unsigned int *out; } colors[] = {
		{"workspace", &theme->workspace}, {"dots", &theme->dots},
		{"panel", &theme->panel}, {"text", &theme->text},
		{"accent", &theme->accent}, {"selected", &theme->selected},
		{"hover", &theme->hover}, {"border", &theme->border},
	};
	snprintf(group, sizeof(group), "theme.%d", index + 1);
	read_text(file, path, group, "name", theme->name, sizeof(theme->name));
	for (size_t i = 0; i < G_N_ELEMENTS(colors); i++)
		read_color(file, path, group, colors[i].name, colors[i].out);
}

static void
apply_settings(GKeyFile *file, const char *path)
{
	static const char *const tools[] = {
		[TOOL_SELECT] = "select", [TOOL_ARROW] = "arrow", [TOOL_LINE] = "line",
		[TOOL_PEN] = "pen", [TOOL_NUMBER] = "number", [TOOL_RECT] = "rect",
		[TOOL_CIRCLE] = "circle", [TOOL_TEXT] = "text", [TOOL_HIGHLIGHT] = "highlight",
		[TOOL_PIXELATE] = "pixelate", [TOOL_BLUR] = "blur", [TOOL_PICKER] = "picker",
		[TOOL_MARKER] = "marker", NULL,
	};
	static const char *const booleans[] = {"false", "true", NULL};
	static const char *const modifiers[] = {"alt", "super", NULL};
	static const struct {
		const char *group, *key;
		int *out, min, max;
	} numbers[] = {
		{"window", "padding", &settings.window_padding, 0, 128},
		{"palette", "default", &settings.palette_default, 1, G_N_ELEMENTS(settings.palette)},
		{"stroke", "default", &settings.stroke_default, 1, G_N_ELEMENTS(settings.stroke_widths)},
		{"arrow", "edge_style", &settings.arrow_edge_style, 0, 5},
		{"effects", "highlight_strength", &settings.highlight_strength, 1, 100},
		{"effects", "pixelate_block", &settings.pixelate_block, 2, 64},
		{"effects", "blur_radius", &settings.blur_radius, 1, 16},
		{"effects", "marker_height", &settings.marker_height, 4, 400},
		{"effects", "marker_strength", &settings.marker_strength, 1, 100},
		{"text", "scale", &settings.text_scale, 1, 64},
		{"text", "max_scale", &settings.max_text_scale, 1, 64},
		{"text", "weight", &settings.text_weight, 100, 900},
		{"text", "fill_padding", &settings.text_fill_padding, 0, 32},
		{"text", "fill_corner_radius", &settings.text_fill_corner_radius, 0, 32},
		{"theme", "default", &settings.theme_default, 1, THEME_COUNT},
	};
	for (size_t i = 0; i < G_N_ELEMENTS(numbers); i++) {
		const char *group = numbers[i].group, *key = numbers[i].key;
		g_autofree char *value = read_value(file, path, group, key);
		if (value && !parse_number(value, numbers[i].min, numbers[i].max, numbers[i].out))
			invalid_setting(path, group, key);
	}

	read_text(file, path, "window", "class", settings.window_class, sizeof(settings.window_class));
	read_text(file, path, "window", "save_directory", settings.save_directory, sizeof(settings.save_directory));
	read_text(file, path, "text", "font", settings.font, sizeof(settings.font));
	settings.tool = read_choice(file, path, "annotation", "tool", tools, settings.tool);
	settings.fill = read_choice(file, path, "annotation", "fill", booleans, settings.fill);
	settings.gradient_fill = read_choice(file, path, "annotation", "gradient_fill", booleans, settings.gradient_fill);
	settings.shape_outline = read_choice(file, path, "annotation", "shape_outline", booleans, settings.shape_outline);
	int modifier = read_choice(file, path, "theme", "modifier", modifiers, settings.theme_modifier == Mod4Mask);
	settings.theme_modifier = modifier == 1 ? Mod4Mask : Mod1Mask;

	read_list(file, path, "palette", "colors", settings.palette,
	          G_N_ELEMENTS(settings.palette), &settings.palette_count, 0, 0);
	read_list(file, path, "stroke", "widths", settings.stroke_widths,
	          G_N_ELEMENTS(settings.stroke_widths), &settings.stroke_count, 1, 100);
	read_list(file, path, "arrow", "edge_cycle", settings.arrow_edge_cycle,
	          G_N_ELEMENTS(settings.arrow_edge_cycle), &settings.arrow_edge_count, 0, 5);

	g_autofree char *selection = read_value(file, path, "theme", "selection_bbox_color");
	if (selection && g_ascii_strcasecmp(selection, "theme") == 0)
		settings.selection_bbox_color = SELECTION_THEME_ACCENT;
	else if (selection && !parse_color(selection, &settings.selection_bbox_color))
		invalid_setting(path, "theme", "selection_bbox_color");
	for (int i = 0; i < THEME_COUNT; i++)
		read_theme(file, path, i);
}

void
settings_load_defaults(void)
{
	g_autoptr(GKeyFile) file = g_key_file_new();
	g_autoptr(GError) error = NULL;
	if (!g_key_file_load_from_data(file, defaults, sizeof(defaults) - 1, G_KEY_FILE_NONE, &error))
		g_error("Invalid built-in s2 configuration: %s", error->message);
	apply_settings(file, NULL);
}

void
settings_load(void)
{
	settings_load_defaults();
	const char *xdg = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");
	g_autofree char *path = NULL;
	if (xdg && xdg[0] == '/')
		path = g_build_filename(xdg, "s2", "config.ini", NULL);
	else if (home && home[0] == '/')
		path = g_build_filename(home, ".config", "s2", "config.ini", NULL);
	if (!path)
		return;

	g_autoptr(GKeyFile) file = g_key_file_new();
	g_autoptr(GError) error = NULL;
	if (g_key_file_load_from_file(file, path, G_KEY_FILE_NONE, &error))
		apply_settings(file, path);
	else if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
		fprintf(stderr, "s2: %s: %s\n", path, error->message);
}

int
settings_print_defaults(void)
{
	return fputs(defaults, stdout) == EOF || fflush(stdout) == EOF ? 1 : 0;
}
