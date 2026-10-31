#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <fontconfig/fontconfig.h>
#include <stdint.h>
#include "cui/cui.h"
#include <cairo/cairo-ft.h>
#include <pango/pangocairo.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include "assets/fonts/embedded.h"

#include "s2.h"

#include "config.h"
#include "themes.h"
#include "assets/icons/rotate-cursor.h"
#include "assets/icons/pan-cursors.h"

#ifndef S2_DEFAULT_TEXT_WEIGHT
#define S2_DEFAULT_TEXT_WEIGHT 500
#endif

#ifndef default_tool_index
#define default_tool_index 1
#endif

#ifndef selection_bbox_color
#define selection_bbox_color 0x00ffffu
#endif

#ifndef default_pixelate_block
#define default_pixelate_block 8
#endif

#ifndef default_marker_height
#define default_marker_height 24
#endif

#ifndef default_marker_strength
#define default_marker_strength 40
#endif

#ifndef window_padding
#define window_padding 16
#endif

#ifndef max_text_scale
#define max_text_scale 16
#endif

#ifndef S2_DEFAULT_GRADIENT_FILL
#define S2_DEFAULT_GRADIENT_FILL 1
#endif
#ifndef S2_DEFAULT_SHAPE_OUTLINE
#define S2_DEFAULT_SHAPE_OUTLINE 1
#endif

/* Arrow end styles: open, filled, outline, diamond, dot, bar. */
#ifndef S2_ARROW_EDGE_CYCLE
#define S2_ARROW_EDGE_CYCLE {0,1,2,3,4,5}
#endif
#ifndef S2_DEFAULT_ARROW_EDGE_STYLE
#define S2_DEFAULT_ARROW_EDGE_STYLE 0
#endif
static const int arrow_edge_cycle[]=S2_ARROW_EDGE_CYCLE;

enum tool {
	TOOL_SELECT = 0,
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

enum action_type {
	ACTION_ARROW = 0,
	ACTION_LINE,
	ACTION_PEN,
	ACTION_NUMBER,
	ACTION_RECT,
	ACTION_CIRCLE,
	ACTION_TEXT,
	ACTION_HIGHLIGHT,
	ACTION_MARKER,
	ACTION_PIXELATE,
	ACTION_BLUR,
};

struct action {
	enum action_type type;
	int x0;
	int y0;
	int x1;
	int y1;
	int p0;
	int rotation;
 double bend; /* quadratic control offset as a fraction of endpoint length */
 int no_gradient, no_outline;
 int edge_style, edges; /* start=1, end=2 */
 int font_weight, italic, underline, strikethrough, outline_thickness;
 int text_box_custom,text_padding,text_radius,has_fill_color;
 unsigned int fill_color;
	unsigned int color;
	char *text;
	int *pen_points;
	int pen_len;
};

struct action_stack {
	struct action *items;
	size_t len;
	size_t cursor;
	size_t cap;
};

struct editor_state {
	Display *dpy;
	int screen;
	Window win;
 Atom wm_protocols,wm_delete_window;
	Pixmap backbuf;
 Pixmap canvasbuf,source_pixmap;
 cairo_surface_t *source_surface;
 int source_dirty,source_preview;
 Pixmap chromebuf;
 char chrome_key[768];
 int chrome_cursor_x,chrome_cursor_y,motion_only;
 double chrome_time,idle_time;
 int damage_x,damage_y,damage_w,damage_h;
	Cursor cursor;
	Cursor cursor_text;
 Cursor cursor_hand;
 Cursor cursor_grab,cursor_pan;
 int control_down,panning,pan_pointer_x,pan_pointer_y,pan_view_x,pan_view_y;
 Cursor cursor_arrow;
 Cursor cursor_rotate, cursor_resize[4];
 int pointer_x, pointer_y;
 Cursor current_cursor;
	GC gc;
	XImage *ximg;
	int status_h;
	int status_pad;
	int win_w;
	int win_h;
	float scale;
 float zoom;
	int canvas_x;
	int canvas_y;
	int canvas_w;
	int canvas_h;
	int view_x;
	int view_y;
	Colormap cmap;
	unsigned long window_bg;
	unsigned long status_bg;
	unsigned long status_fg;
	XIM xim;
	XIC xic;
	XftDraw *xftdraw;
	XftFont *xftfont_status;
	XftFont *xftfont_tool[9];
	struct image *img;
	struct image base;
	struct image rendered;
	struct image preview;
	struct action_stack actions;
 const struct action *text_style;
	int dirty;
	int raster_dirty;
 int profile; unsigned long frame_count; double frame_ms, frame_max_ms;
	int running;
	int should_save;
	int save_timestamp;
	int should_copy;
	int cancelled;
	enum tool tool;
	unsigned int color;
	int fill_mode;
 int gradient_mode, outline_mode;
	int thickness_idx;
	int text_scale;
	int pixelate_block;
	int blur_radius;
	int highlight_strength;
	int marker_height;
	int cursor_x;
	int cursor_y;
	int anchor_active;
	int anchor_x;
	int anchor_y;
	int text_mode;
	char text_buf[256];
	int text_len;
 int text_cursor,text_anchor,text_edit_index,show_text_selection;
 struct action text_edit_style;
	int color_mode;
 int color_target_fill, picker_active;
	char color_buf[7];
	int color_len;
	int mouse_b1_down;
	int mouse_anchor_set_on_press;
	int mouse_drag_moved;
	int selected_idx;
	int *sel;
	int sel_len;
	int sel_cap;
	int marquee_active;
	int marquee_x;
	int marquee_y;
	int drag_active;
	int drag_origin_x;
	int drag_origin_y;
	int drag_dx;
	int drag_dy;
	int rotating;
 int resizing, resize_corner, transform_preview, curving;
 struct action resize_original;
	int pen_active;
	int pen_is_marker;
	int pen_color;
	int pen_thickness;
	int *pen_points;
	int pen_len;
	int pen_cap;
	int text_x;
	int text_y;
	int toolbar_hover;
 int chrome_hover;
 int popup;
 int theme;
	int show_help;
 int show_about,about_focus;
	int number_next;
	int suppress_escape_once;
	const struct app_config *cfg;
};

#define TEXT_RENDER_PAD 4
#define TEXT_SCALE_PX 8

static int action_supports_gradient(const struct action *a);
static unsigned int action_fill_color(const struct action *a);
static int action_supports_fill(const struct action *a);
static int push_action(struct action_stack *st, const struct action *in);
static int text_bg_padding_for_scale(int scale);
static int text_bg_radius_for_scale(int scale);
static void text_box_metrics(const struct editor_state *ed,
			     int x,
			     int y,
			     const char *text,
			     int scale,
			     int with_fill,
			     int *bx,
			     int *by,
			     int *bw,
			     int *bh);
static void update_cursor(struct editor_state *ed);
static void set_tool(struct editor_state *ed, enum tool tool);
static int tool_uses_anchor(enum tool tool);
static void apply_action(struct editor_state *ed, struct image *img, const struct action *a);
static void apply_action_rotated(struct editor_state *ed, struct image *img, const struct action *a);
static int action_is_rotatable(enum action_type t);
static void action_center(const struct editor_state *ed, const struct action *a, double *cx, double *cy);
static void rotate_selected_by(struct editor_state *ed, int delta);
static void sel_clear(struct editor_state *ed);
static void sel_set_single(struct editor_state *ed,int index);
static int delete_selected(struct editor_state *ed);
static void action_move_by(struct action *a,int dx,int dy);
static int resize_handle_hit(struct editor_state *ed, int x, int y);
static int action_is_resizable(enum action_type t);

static void
build_font_pattern(char *out, size_t outlen, int px)
{
	char base[160];
	const char *src;
	char *size_pos;
	char *end;

	if (!out || outlen == 0) {
		return;
	}
	src = (font_name && *font_name) ? font_name : "monospace:size=16";
	snprintf(base, sizeof(base), "%s", src);
	size_pos = strstr(base, ":size=");
	if (size_pos) {
		*size_pos = '\0';
	}
	end = base + strlen(base);
	while (end > base && (end[-1] == ' ' || end[-1] == '\t')) {
		end--;
		*end = '\0';
	}
	if (base[0] == '\0') {
		snprintf(base, sizeof(base), "monospace");
	}
	snprintf(out, outlen, "%s:size=%d", base, px);
}

static void
build_font_pattern_primary(char *out, size_t outlen, int px)
{
	char tmp[192];
	char *comma;
	char *end;

	if (!out || outlen == 0) {
		return;
	}
	build_font_pattern(tmp, sizeof(tmp), px);
	comma = strchr(tmp, ',');
	if (comma) {
		*comma = '\0';
		end = comma;
		while (end > tmp && (end[-1] == ' ' || end[-1] == '\t')) {
			end--;
			*end = '\0';
		}
	}
	snprintf(out, outlen, "%s", tmp);
}

static void
build_font_css_family(char *out, size_t outlen)
{
	char base[192];
	char *size_pos;
	char *p;

	if (!out || outlen == 0) {
		return;
	}
	out[0] = '\0';
	snprintf(base, sizeof(base), "%s", (font_name && *font_name) ? font_name : "sans");
	size_pos = strstr(base, ":size=");
	if (size_pos) {
		*size_pos = '\0';
	}
	for (p = base; *p != '\0'; p++) {
		if (*p == ':') {
			*p = ' ';
		}
	}
	snprintf(out, outlen, "%s", base);
}

static void text_render_options(cairo_t *cr)
{
	cairo_font_options_t *options = cairo_font_options_create();
	cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
	cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_SLIGHT);
	cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_ON);
	cairo_set_font_options(cr, options);
	cairo_font_options_destroy(options);
}

static void style_text_layout(PangoLayout *layout, PangoFontDescription *desc,
                              const struct editor_state *ed)
{
 const struct action *style=ed?ed->text_style:NULL;
 pango_font_description_set_weight(desc,style && style->font_weight?style->font_weight:PANGO_WEIGHT_NORMAL);
 pango_font_description_set_style(desc,style && style->italic?PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL);
 PangoAttrList *attrs=pango_attr_list_new();
 if(style && style->underline)pango_attr_list_insert(attrs,pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
 if(style && style->strikethrough)pango_attr_list_insert(attrs,pango_attr_strikethrough_new(TRUE));
 if(ed && ed->show_text_selection && ed->text_cursor!=ed->text_anchor) {
     int start=ed->text_cursor<ed->text_anchor?ed->text_cursor:ed->text_anchor;
     int end=ed->text_cursor>ed->text_anchor?ed->text_cursor:ed->text_anchor;
     PangoAttribute *bg=pango_attr_background_new(0x3333,0x6666,0xcccc);
     PangoAttribute *fg=pango_attr_foreground_new(65535,65535,65535);
     bg->start_index=fg->start_index=start;bg->end_index=fg->end_index=end;
     pango_attr_list_insert(attrs,bg);pango_attr_list_insert(attrs,fg);
 }
 pango_layout_set_attributes(layout,attrs);pango_attr_list_unref(attrs);
}

static int render_text_pango(const struct editor_state *ed, struct image *img,
			     int x, int y, const char *text, int scale,
			     unsigned int color, int *ink_x, int *ink_y,
			     int *ink_w, int *ink_h, int measure_only,
			     int *out_w, int *out_h)
{
	(void)ed;
	cairo_surface_t *surface;
	cairo_t *cr;
	PangoLayout *layout;
	PangoFontDescription *desc;
	PangoRectangle ink_rect;
	PangoRectangle logical_rect;
	char family[192];
	int w_px;
	int h_px;
	int draw_w;
	int draw_h;
	int draw_off_x;
	int draw_off_y;
	int ix0;
	int iy0;
	int ix1;
	int iy1;
	double sr;
	double sg;
	double sb;

	if (!text) {
		text = "";
	}
	if (scale < 1) {
		scale = 1;
	}
	w_px = 0;
	h_px = 0;

	if (!measure_only && (!img || !img->pixels || !ed)) {
		return -1;
	}

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return -1;
	}
	cr = cairo_create(surface);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}

	text_render_options(cr);
	layout = pango_cairo_create_layout(cr);
	if (!layout) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_layout_set_text(layout, text, -1);
	build_font_css_family(family, sizeof(family));
	desc = pango_font_description_from_string(family);
	if (!desc) {
		g_object_unref(layout);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_font_description_set_absolute_size(
	    desc, (double)(TEXT_SCALE_PX * scale) * PANGO_SCALE);
	style_text_layout(layout,desc,ed);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
	w_px = logical_rect.width;
	h_px = logical_rect.height;
	if (w_px <= 0) {
		w_px = ink_rect.width;
	}
	if (h_px <= 0) {
		h_px = ink_rect.height;
	}
	if (w_px < 0) {
		w_px = 0;
	}
	if (h_px < 0) {
		h_px = 0;
	}

	if (ink_x) {
		*ink_x = x + TEXT_RENDER_PAD;
	}
	if (ink_y) {
		*ink_y = y + TEXT_RENDER_PAD;
	}
	if (ink_w) {
		*ink_w = ink_rect.width > 0 ? ink_rect.width : w_px;
	}
	if (ink_h) {
		*ink_h = ink_rect.height > 0 ? ink_rect.height : h_px;
	}
	if (out_w) {
		*out_w = ink_rect.width > 0 ? ink_rect.width : w_px;
	}
	if (out_h) {
		*out_h = ink_rect.height > 0 ? ink_rect.height : h_px;
	}
	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	if (measure_only || w_px <= 0 || h_px <= 0) {
		return 0;
	}

	draw_w =
	    (ink_rect.width > 0 ? ink_rect.width : w_px) + TEXT_RENDER_PAD * 2;
	draw_h = (ink_rect.height > 0 ? ink_rect.height : h_px) +
		 TEXT_RENDER_PAD * 2;
	draw_off_x = TEXT_RENDER_PAD - ink_rect.x;
	draw_off_y = TEXT_RENDER_PAD - ink_rect.y;
	if (draw_w <= 0 || draw_h <= 0) {
		return 0;
	}

	surface =
	    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, draw_w, draw_h);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return -1;
	}
	cr = cairo_create(surface);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	text_render_options(cr);
	layout = pango_cairo_create_layout(cr);
	if (!layout) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_layout_set_text(layout, text, -1);
	build_font_css_family(family, sizeof(family));
	desc = pango_font_description_from_string(family);
	if (!desc) {
		g_object_unref(layout);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_font_description_set_absolute_size(
	    desc, (double)(TEXT_SCALE_PX * scale) * PANGO_SCALE);
	style_text_layout(layout,desc,ed);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	ix0 = draw_w;
	iy0 = draw_h;
	ix1 = -1;
	iy1 = -1;

	if (w_px > 0 && h_px > 0) {
		int ix;
		int iy;
		unsigned char *src;
		int stride;

		sr = (double)((color >> 16) & 0xff) / 255.0;
		sg = (double)((color >> 8) & 0xff) / 255.0;
		sb = (double)(color & 0xff) / 255.0;
		cairo_set_source_rgb(cr, sr, sg, sb);
		cairo_move_to(cr, (double)draw_off_x, (double)draw_off_y);
		pango_cairo_show_layout(cr, layout);
		cairo_surface_flush(surface);
		src = cairo_image_surface_get_data(surface);
		stride = cairo_image_surface_get_stride(surface);
		for (iy = 0; iy < draw_h; iy++) {
			for (ix = 0; ix < draw_w; ix++) {
				int tx = x + ix;
				int ty = y + iy;
				unsigned char *sp =
				    src + (size_t)iy * (size_t)stride +
				    (size_t)ix * 4u;
				unsigned char b = sp[0];
				unsigned char g = sp[1];
				unsigned char r = sp[2];
				unsigned char a = sp[3];
				if (a > 0 && tx >= 0 && ty >= 0 &&
				    tx < img->width && ty < img->height) {
					unsigned char *dst =
					    img->pixels +
					    ((size_t)ty * (size_t)img->width +
					     (size_t)tx) *
						4u;
					if (ix < ix0)
						ix0 = ix;
					if (iy < iy0)
						iy0 = iy;
					if (ix > ix1)
						ix1 = ix;
					if (iy > iy1)
						iy1 = iy;
                    unsigned int out=a+dst[3]*(255u-a)/255u;
                    unsigned int channels[]={r,g,b};
                    for(int channel=0;channel<3;channel++)
                        dst[channel]=(channels[channel]*255u+dst[channel]*dst[3]*(255u-a)/255u)/out;
                    dst[3]=out;
				}
			}
		}
	}

	if (ix1 >= ix0 && iy1 >= iy0) {
		if (ink_x) {
			*ink_x = x + ix0;
		}
		if (ink_y) {
			*ink_y = y + iy0;
		}
		if (ink_w) {
			*ink_w = ix1 - ix0 + 1;
		}
		if (ink_h) {
			*ink_h = iy1 - iy0 + 1;
		}
	}

	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return 0;
}

static void
text_metrics_xft(const struct editor_state *ed, const char *text, int scale, int *tw, int *th)
{
	if (render_text_pango(ed, NULL, 0, 0, text, scale, 0, NULL, NULL, NULL, NULL, 1, tw, th) != 0) {
		int len;
		if (!text) {
			text = "";
		}
		if (scale < 1) {
			scale = 1;
		}
		len = (int)strlen(text);
		if (tw) {
			*tw = len * 6 * scale;
		}
		if (th) {
			*th = 8 * scale;
		}
	}
}

static int
number_radius_for_scale(int scale)
{
	if (scale < 1) {
		scale = 1;
	}
	return scale * 6 + 6;
}

static unsigned int
inverse_color(unsigned int color)
{
	unsigned int r = (color >> 16) & 0xffu;
	unsigned int g = (color >> 8) & 0xffu;
	unsigned int b = color & 0xffu;
	unsigned int y = (r * 299u + g * 587u + b * 114u) / 1000u;
	return (y < 128u) ? 0xffffffu : 0x000000u;
}


static void
set_wm_class(Display *dpy, Window win, const char *name)
{
	XClassHint hint;
	char *res_name;
	char *res_class;

	if (!dpy || !win) {
		return;
	}
	if (!name || !*name) {
		name = "s2";
	}
	res_name = strdup(name);
	res_class = strdup(name);
	if (!res_name || !res_class) {
		free(res_name);
		free(res_class);
		return;
	}
	hint.res_name = res_name;
	hint.res_class = res_class;
	XSetClassHint(dpy, win, &hint);
	free(res_name);
	free(res_class);
}

static void
set_wm_window_type(Display *dpy, Window win, int normal_window)
{
	Atom prop;
	Atom kind;

	if (!dpy || !win) {
		return;
	}
	prop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	if (prop == None) {
		return;
	}
	kind = XInternAtom(dpy,
	                  normal_window ? "_NET_WM_WINDOW_TYPE_NORMAL" : "_NET_WM_WINDOW_TYPE_DIALOG",
	                  False);
	if (kind == None) {
		return;
	}
	XChangeProperty(dpy, win, prop, XA_ATOM, 32, PropModeReplace, (unsigned char *)&kind, 1);
}

static int
img_alloc_like(struct image *dst, const struct image *src)
{
	if (!dst || !src || !src->pixels || src->len == 0) {
		return -1;
	}
	memset(dst, 0, sizeof(*dst));
	dst->width = src->width;
	dst->height = src->height;
	dst->len = src->len;
	dst->pixels = malloc(dst->len);
	if (!dst->pixels) {
		return -1;
	}
	return 0;
}

static void
img_copy(struct image *dst, const struct image *src)
{
	if (!dst || !src || !dst->pixels || !src->pixels || dst->len != src->len) {
		return;
	}
	memcpy(dst->pixels, src->pixels, src->len);
}

static void
img_put_px(struct image *img, int x, int y, unsigned int color)
{
	unsigned char *p;

	if (!img || !img->pixels || x < 0 || y < 0 || x >= img->width || y >= img->height) {
		return;
	}
	p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
	p[0] = (unsigned char)((color >> 16) & 0xff);
	p[1] = (unsigned char)((color >> 8) & 0xff);
	p[2] = (unsigned char)(color & 0xff);
	p[3] = 0xff;
}

/* Keep capture pixels on the X server; panning/zooming only changes a transform. */
static void paint_canvas_cache(struct editor_state *ed,const struct image *src,int preview)
{
 Visual *visual=DefaultVisual(ed->dpy,ed->screen);
 int depth=DefaultDepth(ed->dpy,ed->screen);
 if(!ed->source_surface || ed->source_dirty || preview || ed->source_preview!=preview) {
     if(!ed->source_pixmap)ed->source_pixmap=XCreatePixmap(ed->dpy,ed->win,src->width,src->height,depth);
     XImage *pixels=XCreateImage(ed->dpy,visual,depth,ZPixmap,0,NULL,src->width,src->height,32,0);
     if(!pixels)return;
     pixels->data=malloc((size_t)pixels->bytes_per_line*src->height);
     if(!pixels->data) {XDestroyImage(pixels);return;}
     int fast=pixels->bits_per_pixel==32 && pixels->byte_order==LSBFirst && visual->red_mask==0xff0000 && visual->green_mask==0xff00 && visual->blue_mask==0xff;
     for(int y=0;y<src->height;y++)for(int x=0;x<src->width;x++) {
         const unsigned char *p=src->pixels+((size_t)y*src->width+x)*4;
         uint32_t rgb=((uint32_t)p[0]<<16)|((uint32_t)p[1]<<8)|p[2];
         if(fast)memcpy(pixels->data+(size_t)y*pixels->bytes_per_line+x*4,&rgb,4);
         else XPutPixel(pixels,x,y,rgb);
     }
     if(ed->source_surface)cairo_surface_flush(ed->source_surface);
     XPutImage(ed->dpy,ed->source_pixmap,ed->gc,pixels,0,0,0,0,src->width,src->height);
     XDestroyImage(pixels);
     if(!ed->source_surface)ed->source_surface=cairo_xlib_surface_create(ed->dpy,ed->source_pixmap,visual,src->width,src->height);
     cairo_surface_mark_dirty(ed->source_surface);
     ed->source_dirty=0;ed->source_preview=preview;
 }
 cairo_surface_t *surface=cairo_xlib_surface_create(ed->dpy,ed->canvasbuf,visual,ed->win_w,ed->win_h);
 cairo_t *cr=cairo_create(surface);
 cairo_pattern_t *ramp=cairo_pattern_create_linear(ed->win_w,0,0,ed->win_h);
 double light[3],dark[3];
 for(int c=0;c<3;c++) {
     double base=((themes[ed->theme].workspace>>(16-8*c))&255)/255.;
     double tint=((themes[ed->theme].accent>>(16-8*c))&255)/255.;
     light[c]=base+(tint-base)*.15;dark[c]=base*.78;
 }
 cairo_pattern_add_color_stop_rgb(ramp,0,light[0],light[1],light[2]);
 cairo_pattern_add_color_stop_rgb(ramp,1,dark[0],dark[1],dark[2]);
 cairo_set_source(cr,ramp);cairo_paint(cr);cairo_pattern_destroy(ramp);
 cairo_surface_t *tile=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,24,24);
 cairo_t *dot=cairo_create(tile);unsigned int color=themes[ed->theme].dots;
 cairo_set_source_rgb(dot,((color>>16)&255)/255.,((color>>8)&255)/255.,(color&255)/255.);
 cairo_rectangle(dot,12,12,1,1);cairo_fill(dot);cairo_destroy(dot);
 cairo_pattern_t *pattern=cairo_pattern_create_for_surface(tile);cairo_pattern_set_extend(pattern,CAIRO_EXTEND_REPEAT);
 cairo_set_source(cr,pattern);cairo_paint(cr);cairo_pattern_destroy(pattern);cairo_surface_destroy(tile);
 cairo_translate(cr,ed->canvas_x,ed->canvas_y);cairo_scale(cr,ed->scale,ed->scale);
 cairo_set_source_surface(cr,ed->source_surface,0,0);
 cairo_pattern_set_filter(cairo_get_source(cr),ed->scale<1?CAIRO_FILTER_BILINEAR:CAIRO_FILTER_NEAREST);
 cairo_rectangle(cr,0,0,src->width,src->height);cairo_fill(cr);
 cairo_destroy(cr);cairo_surface_destroy(surface);
}

static int recreate_ximg(struct editor_state *ed, Visual *vis, int depth)
{
	int draw_h;
	size_t data_size;

	if (!ed || !ed->dpy || !vis) {
		return -1;
	}
	draw_h = ed->win_h;
	if (draw_h < 1) {
		draw_h = 1;
	}
	if (ed->win_w < 1) {
		ed->win_w = 1;
	}
if(ed->chromebuf) {XFreePixmap(ed->dpy,ed->chromebuf);ed->chromebuf=0;ed->chrome_key[0]=0;}
 ed->damage_w=ed->damage_h=0;
 if (ed->canvasbuf) {
		XFreePixmap(ed->dpy, ed->canvasbuf);
		ed->canvasbuf = 0;
	}
	if (ed->ximg) {
		XDestroyImage(ed->ximg);
		ed->ximg = NULL;
	}
	data_size = (size_t)ed->win_w * (size_t)draw_h * 4;
	ed->ximg = XCreateImage(ed->dpy, vis, (unsigned int)depth, ZPixmap, 0,
				calloc(1, data_size), ed->win_w, draw_h, 32, 0);
	if (!ed->ximg || !ed->ximg->data) {
		return -1;
	}
	ed->raster_dirty = 1;
	return 0;
}

static int footer_height(const struct editor_state *ed) { return ed->win_w < 718 ? 88 : 44; }
static int toolbar_rows(const struct editor_state *ed) { return ed->win_w < 900 ? 2 : 1; }
static int toolbar_bottom(const struct editor_state *ed) { return 16 + 64 * toolbar_rows(ed); }

static void
recalc_layout(struct editor_state *ed)
{
	int draw_w;
	int draw_h;
	float sx;
	float sy;
	float s;

	if (!ed || !ed->img) {
		return;
	}
	draw_w = (int)(ed->win_w * 0.78f);
	draw_h = ed->win_h - 210 - (toolbar_rows(ed)-1)*64 - (footer_height(ed)-44);
	if (draw_w < 1) {
		draw_w = 1;
	}
	if (draw_h < 1) {
		draw_h = 1;
	}
	sx = (float)draw_w / (float)ed->img->width;
	sy = (float)draw_h / (float)ed->img->height;
	s = sx < sy ? sx : sy;
	if (s <= 0.0f) {
		s = 1.0f;
	}
	if (s > 1.0f) s = 1.0f;
	ed->scale = ed->zoom>0?ed->zoom:s;
	ed->canvas_w = (int)(ed->img->width * ed->scale);
	ed->canvas_h = (int)(ed->img->height * ed->scale);
	if (ed->canvas_w < 1) ed->canvas_w = 1;
	if (ed->canvas_h < 1) ed->canvas_h = 1;
	ed->canvas_x = (ed->win_w - ed->canvas_w) / 2;
	ed->canvas_y = 106 + (toolbar_rows(ed)-1)*64 + (draw_h - ed->canvas_h) / 2;
	if (ed->canvas_x < window_padding) ed->canvas_x = window_padding;
	if (ed->canvas_y < window_padding) ed->canvas_y = window_padding;
 ed->canvas_x+=ed->view_x;ed->canvas_y+=ed->view_y;
}

static void set_zoom(struct editor_state *ed, float value)
{
 if(value==0)ed->view_x=ed->view_y=0;
 ed->zoom=value;
 if(ed->zoom>4) ed->zoom=4;
 if(ed->zoom>0 && ed->zoom<0.1f) ed->zoom=0.1f;
 recalc_layout(ed); ed->raster_dirty=1;
}

static void set_zoom_at(struct editor_state *ed,float value,int x,int y)
{
 double ix=(x-ed->canvas_x)/(double)ed->scale,iy=(y-ed->canvas_y)/(double)ed->scale;
 set_zoom(ed,value);
 int target_x=lround(x-ix*ed->scale),target_y=lround(y-iy*ed->scale);
 ed->view_x+=target_x-ed->canvas_x;ed->view_y+=target_y-ed->canvas_y;
 ed->canvas_x=target_x;ed->canvas_y=target_y;
}
static void pan_to(struct editor_state *ed,int x,int y)
{
 ed->view_x=ed->pan_view_x+x-ed->pan_pointer_x;
 ed->view_y=ed->pan_view_y+y-ed->pan_pointer_y;
 recalc_layout(ed);ed->raster_dirty=1;
}

static int
canvas_to_image_x(const struct editor_state *ed, int x)
{
	int v;
	if (x < ed->canvas_x) {
		x = ed->canvas_x;
	}
	if (ed->canvas_w <= 0) {
		return 0;
	}
	if (x >= ed->canvas_x + ed->canvas_w) {
		x = ed->canvas_x + ed->canvas_w - 1;
	}
	if (ed->scale <= 0.0f) {
		return 0;
	}
	v = (int)((x - ed->canvas_x) / ed->scale);
	if (v < 0) {
		v = 0;
	}
	if (v >= ed->img->width) {
		v = ed->img->width - 1;
	}
	return v;
}

static int
canvas_to_image_y(const struct editor_state *ed, int y)
{
	int v;
	if (y < ed->canvas_y) {
		y = ed->canvas_y;
	}
	if (ed->canvas_h <= 0) {
		return 0;
	}
	if (y >= ed->canvas_y + ed->canvas_h) {
		y = ed->canvas_y + ed->canvas_h - 1;
	}
	if (ed->scale <= 0.0f) {
		return 0;
	}
	v = (int)((y - ed->canvas_y) / ed->scale);
	if (v < 0) {
		v = 0;
	}
	if (v >= ed->img->height) {
		v = ed->img->height - 1;
	}
	return v;
}

static void
draw_rect_guide(struct image *img, int x0, int y0, int x1, int y1, unsigned int color)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int x;
	int y;

	if (!img || !img->pixels) {
		return;
	}
	minx = x0 < x1 ? x0 : x1;
	miny = y0 < y1 ? y0 : y1;
	maxx = x0 > x1 ? x0 : x1;
	maxy = y0 > y1 ? y0 : y1;
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx >= img->width) maxx = img->width - 1;
	if (maxy >= img->height) maxy = img->height - 1;

	for (x = minx; x <= maxx; x++) {
		img_put_px(img, x, miny, color);
		img_put_px(img, x, maxy, color);
	}
	for (y = miny; y <= maxy; y++) {
		img_put_px(img, minx, y, color);
		img_put_px(img, maxx, y, color);
	}
}


static void
action_bounds(const struct editor_state *ed, const struct action *a, int *minx, int *miny, int *maxx, int *maxy)
{
	int x0;
	int y0;
	int x1;
	int y1;
	int i;

	if (!a || !minx || !miny || !maxx || !maxy) {
		if (minx) *minx = 0;
		if (miny) *miny = 0;
		if (maxx) *maxx = 0;
		if (maxy) *maxy = 0;
		return;
	}
	x0 = a->x0 < a->x1 ? a->x0 : a->x1;
	y0 = a->y0 < a->y1 ? a->y0 : a->y1;
	x1 = a->x0 > a->x1 ? a->x0 : a->x1;
	y1 = a->y0 > a->y1 ? a->y0 : a->y1;
    if(a->type==ACTION_LINE || a->type==ACTION_ARROW) {
        double cx=(a->x0+a->x1)/2. - (a->y1-a->y0)*a->bend;
        double cy=(a->y0+a->y1)/2. + (a->x1-a->x0)*a->bend;
        double mx=(a->x0+a->x1)/2.,my=(a->y0+a->y1)/2.;
        double rx=fmax(fabs((double)a->x1-a->x0)/2.,fabs(cx-mx));
        double ry=fmax(fabs((double)a->y1-a->y0)/2.,fabs(cy-my));
        x0=floor(mx-rx);y0=floor(my-ry);x1=ceil(mx+rx);y1=ceil(my+ry);
    }
	if (a->type == ACTION_TEXT) {
		int bx;
		int by;
		int bw;
		int bh;
		int scale = a->p0 < 0 ? -a->p0 : a->p0;
		x0 = a->x0;
		y0 = a->y0;
		x1 = a->x0;
		y1 = a->y0;
		struct editor_state styled=*ed;styled.text_style=a;
        text_box_metrics(&styled, a->x0, a->y0, a->text, scale, a->p0 < 0, &bx, &by, &bw, &bh);
		x0 = bx;
		y0 = by;
		x1 = bx + bw;
		y1 = by + bh;
	} else if (a->type == ACTION_NUMBER) {
		int r = a->p0 > 0 ? a->p0 : 8;
		x0 = a->x0 - r;
		y0 = a->y0 - r;
		x1 = a->x0 + r;
		y1 = a->y0 + r;
	} else if ((a->type == ACTION_PEN || a->type == ACTION_MARKER) && a->pen_points && a->pen_len > 0) {
		x0 = x1 = a->pen_points[0];
		y0 = y1 = a->pen_points[1];
		for (i = 1; i < a->pen_len; i++) {
			int px = a->pen_points[i * 2 + 0];
			int py = a->pen_points[i * 2 + 1];
			if (px < x0) x0 = px;
			if (py < y0) y0 = py;
			if (px > x1) x1 = px;
			if (py > y1) y1 = py;
		}
	}
	*minx = x0;
	*miny = y0;
	*maxx = x1;
	*maxy = y1;
}

static void
draw_pen_points(struct image *img, const int *points, int len, int thickness, unsigned int color)
{
	int i;

	if (!img || !points || len <= 0) {
		return;
	}
	if (len == 1) {
		draw_line(img, points[0], points[1], points[0], points[1], thickness, color);
		return;
	}
	for (i = 1; i < len; i++) {
		draw_line(img,
		          points[(i - 1) * 2 + 0],
		          points[(i - 1) * 2 + 1],
		          points[i * 2 + 0],
		          points[i * 2 + 1],
		          thickness,
		          color);
	}
}

/*
 * Render a freehand highlighter stroke: the thick path is rasterised into a
 * binary coverage mask first, then blended onto the image exactly once at the
 * given strength. The single blend keeps overlapping parts of one stroke from
 * darkening, so it reads like a real highlighter.
 */
static void
draw_marker_stroke(struct image *img, const int *points, int len, int thickness, unsigned int color, int strength)
{
	struct image mask;
	int *shifted;
	int minx;
	int miny;
	int maxx;
	int maxy;
	int rad;
	int w;
	int h;
	int x;
	int y;
	int i;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int s;
	unsigned int inv;

	if (!img || !img->pixels || !points || len <= 0) {
		return;
	}
	if (thickness < 1) {
		thickness = 1;
	}
	rad = thickness / 2 + 1;
	minx = maxx = points[0];
	miny = maxy = points[1];
	for (i = 1; i < len; i++) {
		int px = points[i * 2 + 0];
		int py = points[i * 2 + 1];
		if (px < minx) minx = px;
		if (px > maxx) maxx = px;
		if (py < miny) miny = py;
		if (py > maxy) maxy = py;
	}
	minx -= rad;
	miny -= rad;
	maxx += rad;
	maxy += rad;
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx > img->width - 1) maxx = img->width - 1;
	if (maxy > img->height - 1) maxy = img->height - 1;
	w = maxx - minx + 1;
	h = maxy - miny + 1;
	if (w < 1 || h < 1) {
		return;
	}

	memset(&mask, 0, sizeof(mask));
	mask.width = w;
	mask.height = h;
	mask.len = (size_t)w * (size_t)h * 4u;
	mask.pixels = calloc(1, mask.len);
	if (!mask.pixels) {
		return;
	}
	shifted = malloc((size_t)len * 2u * sizeof(*shifted));
	if (!shifted) {
		free(mask.pixels);
		return;
	}
	for (i = 0; i < len; i++) {
		shifted[i * 2 + 0] = points[i * 2 + 0] - minx;
		shifted[i * 2 + 1] = points[i * 2 + 1] - miny;
	}
	draw_pen_points(&mask, shifted, len, thickness, 0xffffffu);

	if (strength < 1) {
		strength = 1;
	}
	if (strength > 100) {
		strength = 100;
	}
	s = (unsigned int)strength;
	inv = 100u - s;
	r = (color >> 16) & 0xffu;
	g = (color >> 8) & 0xffu;
	b = color & 0xffu;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			unsigned char *mp = mask.pixels + ((size_t)y * (size_t)w + (size_t)x) * 4u;
			unsigned char *dp;
			if (mp[3] <= 8) {
				continue;
			}
			dp = img->pixels + ((size_t)(y + miny) * (size_t)img->width + (size_t)(x + minx)) * 4u;
			dp[0] = (unsigned char)(((unsigned int)dp[0] * inv + r * s) / 100u);
			dp[1] = (unsigned char)(((unsigned int)dp[1] * inv + g * s) / 100u);
			dp[2] = (unsigned char)(((unsigned int)dp[2] * inv + b * s) / 100u);
			dp[3] = 0xff;
		}
	}
	free(shifted);
	free(mask.pixels);
}

static int
text_bg_padding_for_scale(int scale)
{
	if (scale < 1) {
		scale = 1;
	}
	return text_fill_padding * scale;
}

static int
text_bg_radius_for_scale(int scale)
{
	if (scale < 1) {
		scale = 1;
	}
	return text_fill_corner_radius * scale;
}

static int action_text_padding(const struct action *a,int scale)
{
 return a && a->text_box_custom?a->text_padding:text_bg_padding_for_scale(scale);
}
static int action_text_radius(const struct action *a,int scale)
{
 return a && a->text_box_custom?a->text_radius:text_bg_radius_for_scale(scale);
}

/* Shared subtle color ramp for committed fills and their vector previews. */
static void gradient_source_strength(cairo_t *cr,unsigned int color,double x0,double y0,double x1,double y1,double alpha,double strength)
{
 double r=((color>>16)&255)/255.,g=((color>>8)&255)/255.,b=(color&255)/255.;
 cairo_pattern_t *pattern=cairo_pattern_create_linear(x1,y0,x0,fmax(y0+1,y1));
 cairo_pattern_add_color_stop_rgba(pattern,0,r+(1-r)*.18*strength,g+(1-g)*.18*strength,b+(1-b)*.18*strength,alpha);
 cairo_pattern_add_color_stop_rgba(pattern,.45,r,g,b,alpha);
 cairo_pattern_add_color_stop_rgba(pattern,1,r*(1-.24*strength),g*(1-.24*strength),b*(1-.24*strength),alpha);
 cairo_set_source(cr,pattern);cairo_pattern_destroy(pattern);
}
static void gradient_source(cairo_t *cr,unsigned int color,double x0,double y0,double x1,double y1,double alpha)
{
 gradient_source_strength(cr,color,x0,y0,x1,y1,alpha,1.);
}
static void draw_gradient_fill(struct image *img,int x,int y,int w,int h,
                               int radius,int ellipse,unsigned int color,double alpha,int gradient)
{
 if(!img || !img->pixels || w<=0 || h<=0)return;
 int left=x<0?0:x,top=y<0?0:y,right=x+w,bottom=y+h;
 if(right>img->width)right=img->width;
 if(bottom>img->height)bottom=img->height;
 if(right<=left || bottom<=top)return;
 cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,right-left,bottom-top);
 cairo_t *cr=cairo_create(surface);cairo_translate(cr,-left,-top);
 if(ellipse) {
     cairo_save(cr);cairo_translate(cr,x+w/2.,y+h/2.);cairo_scale(cr,w/2.,h/2.);
     cairo_arc(cr,0,0,1,0,2*M_PI);cairo_restore(cr);
 } else if(radius>0) {
     double r=fmin(radius,fmin(w,h)/2.);
     cairo_new_sub_path(cr);cairo_arc(cr,x+w-r,y+r,r,-M_PI/2,0);
     cairo_arc(cr,x+w-r,y+h-r,r,0,M_PI/2);cairo_arc(cr,x+r,y+h-r,r,M_PI/2,M_PI);
     cairo_arc(cr,x+r,y+r,r,M_PI,3*M_PI/2);cairo_close_path(cr);
 } else cairo_rectangle(cr,x,y,w,h);
 gradient_source_strength(cr,color,x,y,x+w,y+h,alpha,gradient?1.:0.);cairo_fill(cr);cairo_surface_flush(surface);
 unsigned char *src=cairo_image_surface_get_data(surface);int stride=cairo_image_surface_get_stride(surface);
 for(int iy=top;iy<bottom;iy++)for(int ix=left;ix<right;ix++) {
     uint32_t pixel;memcpy(&pixel,src+(iy-top)*stride+(ix-left)*4,4);
     unsigned int a=pixel>>24;if(!a)continue;
     unsigned char *dst=img->pixels+((size_t)iy*img->width+ix)*4;
     unsigned int out=a+dst[3]*(255-a)/255;
     for(int c=0;c<3;c++) {
         unsigned int premul=(pixel>>(16-8*c))&255;
         dst[c]=(premul*255+dst[c]*dst[3]*(255-a)/255)/out;
     }
     dst[3]=out;
 }
 cairo_destroy(cr);cairo_surface_destroy(surface);
}
static void draw_fill_rounded_rect(struct image *img,int x,int y,int w,int h,int radius,unsigned int color,int gradient)
{
 draw_gradient_fill(img,x,y,w,h,radius,0,color,1.,gradient);
}

static void
text_box_metrics(const struct editor_state *ed,
		 int x,
		 int y,
		 const char *text,
		 int scale,
		 int with_fill,
		 int *bx,
		 int *by,
		 int *bw,
		 int *bh)
{
	int tw;
	int th;
	int pad;
	int ink_x;
	int ink_y;
	int ink_w;
	int ink_h;

	if (scale < 1) {
		scale = 1;
	}
	ink_x = x + TEXT_RENDER_PAD;
	ink_y = y + TEXT_RENDER_PAD;
	ink_w = 0;
	ink_h = 0;
	if (render_text_pango(ed,
	                      NULL,
	                      x,
	                      y,
	                      text ? text : "",
	                      scale,
	                      0,
	                      &ink_x,
	                      &ink_y,
	                      &ink_w,
	                      &ink_h,
	                      1,
	                      &tw,
	                      &th) != 0) {
		text_metrics_xft(ed, text ? text : "", scale, &tw, &th);
		ink_x = x + TEXT_RENDER_PAD;
		ink_y = y + TEXT_RENDER_PAD;
		ink_w = tw;
		ink_h = th;
	}
	pad = with_fill ? action_text_padding(ed->text_style,scale) : 0;
	if (bx) {
		*bx = ink_x - pad;
	}
	if (by) {
		*by = ink_y - pad;
	}
	if (bw) {
		*bw = ink_w + pad * 2;
	}
	if (bh) {
		*bh = ink_h + pad * 2;
	}
}

static void clear_text_mode(struct editor_state *ed)
{
 if(ed->text_mode && ed->text_edit_index>=0)ed->dirty=1;
 ed->text_mode=0;ed->text_len=ed->text_cursor=ed->text_anchor=0;
 ed->text_edit_index=-1;ed->text_buf[0]=0;ed->raster_dirty=1;
}
static void commit_text_input(struct editor_state *ed)
{
 if(ed->text_len==0 && ed->text_edit_index>=0) {
     int index=ed->text_edit_index;clear_text_mode(ed);set_tool(ed,TOOL_SELECT);
     sel_set_single(ed,index);delete_selected(ed);return;
 }
 int selected=-1;
 if(ed->text_len>0 || ed->text_edit_index>=0) {
     struct action a=ed->text_edit_index>=0?ed->text_edit_style:(struct action){0};
     if(ed->text_edit_index<0){a.font_weight=S2_DEFAULT_TEXT_WEIGHT;a.no_gradient=!ed->gradient_mode;}
     a.type=ACTION_TEXT;a.x0=ed->text_x;a.y0=ed->text_y;
     a.p0=ed->fill_mode?-ed->text_scale:ed->text_scale;a.color=ed->color;a.text=ed->text_buf;
     if(ed->text_edit_index>=0 && (size_t)ed->text_edit_index<ed->actions.cursor) {
         char *replacement=strdup(ed->text_buf);if(!replacement)return;
         selected=ed->text_edit_index;
         free(ed->actions.items[selected].text);a.text=replacement;ed->actions.items[selected]=a;
     } else if(push_action(&ed->actions,&a)==0)selected=(int)ed->actions.cursor-1;
     else return;
     ed->dirty=ed->raster_dirty=1;
 }
 clear_text_mode(ed);set_tool(ed,TOOL_SELECT);
 if(selected>=0)sel_set_single(ed,selected);
}
static int text_delete_selection(struct editor_state *ed)
{
 int start=ed->text_cursor<ed->text_anchor?ed->text_cursor:ed->text_anchor;
 int end=ed->text_cursor>ed->text_anchor?ed->text_cursor:ed->text_anchor;
 if(start==end)return 0;
 memmove(ed->text_buf+start,ed->text_buf+end,ed->text_len-end+1);
 ed->text_len-=end-start;ed->text_cursor=ed->text_anchor=start;return 1;
}

static int
append_pen_point(struct editor_state *ed, int x, int y)
{
	int *new_points;

	if (!ed) {
		return -1;
	}
	if (ed->pen_len > 0) {
		int lx = ed->pen_points[(ed->pen_len - 1) * 2 + 0];
		int ly = ed->pen_points[(ed->pen_len - 1) * 2 + 1];
		if (lx == x && ly == y) {
			return 0;
		}
	}
	if (ed->pen_len == ed->pen_cap) {
		int new_cap = ed->pen_cap ? ed->pen_cap * 2 : 64;
		new_points = realloc(ed->pen_points, (size_t)new_cap * 2u * sizeof(*new_points));
		if (!new_points) {
			return -1;
		}
		ed->pen_points = new_points;
		ed->pen_cap = new_cap;
	}
	ed->pen_points[ed->pen_len * 2 + 0] = x;
	ed->pen_points[ed->pen_len * 2 + 1] = y;
	ed->pen_len++;
	return 0;
}

static void append_pasted_text(struct editor_state *ed, const char *s)
{
	int i;
	if (!ed || !s) {
		return;
	}
 text_delete_selection(ed);
	for (i = 0; s[i] != '\0';) {
		const char *current = s + i;
		gunichar codepoint = g_utf8_get_char_validated(current, -1);
		if (codepoint == (gunichar)-1 || codepoint == (gunichar)-2) {
			i++;
			continue;
		}
		int bytes = (int)(g_utf8_next_char(current) - current);
		if (codepoint >= 0x20 && codepoint != 0x7f) {
			if (ed->text_len + bytes >= (int)sizeof(ed->text_buf))
				break;
			memmove(ed->text_buf+ed->text_cursor+bytes,ed->text_buf+ed->text_cursor,ed->text_len-ed->text_cursor+1);
            memcpy(ed->text_buf+ed->text_cursor,current,bytes);
            ed->text_len+=bytes;ed->text_cursor+=bytes;ed->text_anchor=ed->text_cursor;
		}
		i += bytes;
	}
	ed->text_buf[ed->text_len] = '\0';
}

static int
paste_text_from_clipboard(struct editor_state *ed)
{
	char clip[2048];

	if (!ed) {
		return 0;
	}
	if (clipboard_paste_text(clip, sizeof(clip)) != 0) {
		return 0;
	}
	if (ed->text_mode) {
		append_pasted_text(ed, clip);
		ed->raster_dirty = 1;
		return 1;
	}
	set_tool(ed, TOOL_TEXT);
	ed->text_mode = 1;
	ed->text_x = ed->img->width / 2;
	ed->text_y = ed->img->height / 2;
	ed->text_len = 0;ed->text_cursor=ed->text_anchor=0;ed->text_edit_index=-1;
	ed->text_buf[0] = '\0';
	append_pasted_text(ed, clip);
	ed->raster_dirty = 1;
	return 1;
}

static void
reset_pen_input(struct editor_state *ed)
{
	if (!ed) {
		return;
	}
	ed->pen_active = 0;
	ed->pen_is_marker = 0;
	ed->pen_len = 0;
}

static void
draw_text_xft(struct editor_state *ed, struct image *img, int x, int y, const char *text, int scale, unsigned int color)
{
	if (render_text_pango(ed, img, x, y, text, scale, color, NULL, NULL, NULL, NULL, 0, NULL, NULL) != 0) {
		draw_text(img, x, y, text, scale, color);
	}
}

/* Render badges at capture resolution with smooth coverage and no outline. */
static void connector_point(const struct action *a,double t,double *x,double *y)
{
 double cx=(a->x0+a->x1)/2.-(a->y1-a->y0)*a->bend;
 double cy=(a->y0+a->y1)/2.+(a->x1-a->x0)*a->bend;
 *x=(1-t)*(1-t)*a->x0+2*(1-t)*t*cx+t*t*a->x1;
 *y=(1-t)*(1-t)*a->y0+2*(1-t)*t*cy+t*t*a->y1;
}
static void connector_fill(cairo_t *cr,unsigned int color,double size,int gradient)
{
 if(gradient)gradient_source(cr,color,-size*1.3,-size*.6,0,size*.6,1.);
 cairo_fill(cr);
}
static void connector_head(cairo_t *cr,double x,double y,double angle,double size,int style,unsigned int color,int gradient)
{
 cairo_save(cr);cairo_translate(cr,x,y);cairo_rotate(cr,angle);
 switch(style) {
 case 4:cairo_arc(cr,-size*.35,0,size*.35,0,2*M_PI);connector_fill(cr,color,size,gradient);break;
 case 5:cairo_move_to(cr,0,-size*.55);cairo_line_to(cr,0,size*.55);cairo_stroke(cr);break;
 case 3:cairo_move_to(cr,0,0);cairo_line_to(cr,-size*.65,-size*.45);cairo_line_to(cr,-size*1.3,0);cairo_line_to(cr,-size*.65,size*.45);cairo_close_path(cr);connector_fill(cr,color,size,gradient);break;
 default:
     cairo_move_to(cr,-size,-size*.55);cairo_line_to(cr,0,0);cairo_line_to(cr,-size,size*.55);
     if(style==1 || style==2)cairo_close_path(cr);
     if(style==1)connector_fill(cr,color,size,gradient);else cairo_stroke(cr);break;
 }
 cairo_restore(cr);
}
static void connector_path(cairo_t *cr,const struct action *a)
{
 double cx=(a->x0+a->x1)/2.-(a->y1-a->y0)*a->bend;
 double cy=(a->y0+a->y1)/2.+(a->x1-a->x0)*a->bend;
 double length=hypot(a->x1-a->x0,a->y1-a->y0);
 double size=fmin(fmax(12,a->p0*3.5),fmax(3,length*.35));
 double trim=a->edge_style==4?.35:(a->edge_style==1 || a->edge_style==2?1:(a->edge_style==3?1.3:0));
 double t0=0,t1=1;
 if(a->type==ACTION_ARROW && length>0) {
     if(a->edges&1)t0=fmin(.4,size*trim/fmax(1,2*hypot(cx-a->x0,cy-a->y0)));
     if(a->edges&2)t1=1-fmin(.4,size*trim/fmax(1,2*hypot(a->x1-cx,a->y1-cy)));
 }
 double sx,sy,ex,ey;connector_point(a,t0,&sx,&sy);connector_point(a,t1,&ex,&ey);
 double qx=sx+(t1-t0)*((1-t0)*(cx-a->x0)+t0*(a->x1-cx));
 double qy=sy+(t1-t0)*((1-t0)*(cy-a->y0)+t0*(a->y1-cy));
 cairo_set_line_width(cr,a->p0);cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);cairo_set_line_join(cr,CAIRO_LINE_JOIN_ROUND);
 cairo_move_to(cr,sx,sy);
 cairo_curve_to(cr,sx+2*(qx-sx)/3,sy+2*(qy-sy)/3,ex+2*(qx-ex)/3,ey+2*(qy-ey)/3,ex,ey);
 cairo_stroke(cr);
 if(a->type==ACTION_ARROW) {
     if(a->edges&1)connector_head(cr,a->x0,a->y0,atan2(a->y0-cy,a->x0-cx),size,a->edge_style,a->color,!a->no_gradient);
     if(a->edges&2)connector_head(cr,a->x1,a->y1,atan2(a->y1-cy,a->x1-cx),size,a->edge_style,a->color,!a->no_gradient);
 }
}
static void draw_connector(struct editor_state *ed,struct image *img,const struct action *a)
{
 int x0,y0,x1,y1;action_bounds(ed,a,&x0,&y0,&x1,&y1);
 int pad=fmax(20,a->p0*5);
 x0=fmax(0,x0-pad);y0=fmax(0,y0-pad);x1=fmin(img->width,x1+pad+1);y1=fmin(img->height,y1+pad+1);
 if(x1<=x0 || y1<=y0)return;
 cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,x1-x0,y1-y0);
 cairo_t *cr=cairo_create(surface);cairo_translate(cr,-x0,-y0);
 cairo_set_source_rgb(cr,((a->color>>16)&255)/255.,((a->color>>8)&255)/255.,(a->color&255)/255.);
 connector_path(cr,a);cairo_surface_flush(surface);
 unsigned char *src=cairo_image_surface_get_data(surface);int stride=cairo_image_surface_get_stride(surface);
 for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++) {
     uint32_t pixel;memcpy(&pixel,src+(y-y0)*stride+(x-x0)*4,4);
     unsigned int alpha=pixel>>24;if(!alpha)continue;
     unsigned char *dst=img->pixels+((size_t)y*img->width+x)*4;
     unsigned int out=alpha+dst[3]*(255-alpha)/255;
     for(int c=0;c<3;c++)dst[c]=(((pixel>>(16-8*c))&255)*255+dst[c]*dst[3]*(255-alpha)/255)/out;
     dst[3]=out;
 }
 cairo_destroy(cr);cairo_surface_destroy(surface);
}

static void draw_number_pin(struct image *img, const struct action *a)
{
 int radius=a->p0>0?a->p0:number_radius_for_scale(a->y1);
 int diameter=radius*2+4;
 if(diameter<1 || diameter>4096)return;
 cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,diameter,diameter);
 cairo_t *cr=cairo_create(surface);
 double r=((a->color>>16)&255)/255.,g=((a->color>>8)&255)/255.,b=(a->color&255)/255.;
 cairo_pattern_t *gradient=cairo_pattern_create_linear(diameter-2,2,2,diameter-2);
 cairo_pattern_add_color_stop_rgb(gradient,0,r+(1-r)*.24,g+(1-g)*.24,b+(1-b)*.24);
 cairo_pattern_add_color_stop_rgb(gradient,.5,r,g,b);
 cairo_pattern_add_color_stop_rgb(gradient,1,r*.68,g*.68,b*.68);
 cairo_arc(cr,diameter/2.,diameter/2.,radius,0,2*M_PI);
 if(a->no_gradient)cairo_set_source_rgb(cr,r,g,b);else cairo_set_source(cr,gradient);
 cairo_fill(cr);cairo_pattern_destroy(gradient);
 char number[32];snprintf(number,sizeof number,"%d",a->x1);
 PangoLayout *layout=pango_cairo_create_layout(cr);
 PangoFontDescription *font=pango_font_description_from_string("Geist Medium");
 double pixels=fmin((a->y1>0?a->y1:1)*TEXT_SCALE_PX,radius*1.15);
 pango_font_description_set_absolute_size(font,pixels*PANGO_SCALE);
 pango_layout_set_font_description(layout,font);pango_layout_set_text(layout,number,-1);
 PangoRectangle ink;pango_layout_get_pixel_extents(layout,&ink,NULL);
 if(ink.width>radius*1.55) {
     pixels*=radius*1.55/ink.width;
     pango_font_description_set_absolute_size(font,pixels*PANGO_SCALE);
     pango_layout_set_font_description(layout,font);pango_layout_get_pixel_extents(layout,&ink,NULL);
 }
 double linear[3]={r,g,b};
 for(int i=0;i<3;i++)linear[i]=linear[i]<=.04045?linear[i]/12.92:pow((linear[i]+.055)/1.055,2.4);
 double luminance=.2126*linear[0]+.7152*linear[1]+.0722*linear[2];
 double fg=luminance>.20?.07:1.;
 cairo_set_source_rgb(cr,fg,fg,fg);
 cairo_move_to(cr,(diameter-ink.width)/2.-ink.x,(diameter-ink.height)/2.-ink.y);
 pango_cairo_show_layout(cr,layout);
 pango_font_description_free(font);g_object_unref(layout);
 cairo_surface_flush(surface);
 unsigned char *data=cairo_image_surface_get_data(surface);int stride=cairo_image_surface_get_stride(surface);
 for(int y=0;y<diameter;y++)for(int x=0;x<diameter;x++) {
     int ix=a->x0+x-diameter/2,iy=a->y0+y-diameter/2;
     if(ix<0 || iy<0 || ix>=img->width || iy>=img->height)continue;
     uint32_t pixel;memcpy(&pixel,data+y*stride+x*4,4);
     unsigned int alpha=pixel>>24;if(!alpha)continue;
     unsigned char *dst=img->pixels+((size_t)iy*img->width+ix)*4;
     unsigned int out=alpha+dst[3]*(255-alpha)/255;
     for(int channel=0;channel<3;channel++) {
         unsigned int premul=(pixel>>(16-8*channel))&255;
         dst[channel]=(premul*255+dst[channel]*dst[3]*(255-alpha)/255)/out;
     }
     dst[3]=out;
 }
 cairo_destroy(cr);cairo_surface_destroy(surface);
}

static void
apply_action(struct editor_state *ed, struct image *img, const struct action *a)
{
	if (!ed || !img || !a) {
		return;
	}
	if (a->rotation % 360 != 0 && action_is_rotatable(a->type)) {
		apply_action_rotated(ed, img, a);
		return;
	}
	switch (a->type) {
	case ACTION_ARROW:
		draw_connector(ed,img,a);
		break;
	case ACTION_LINE:
		draw_connector(ed,img,a);
		break;
	case ACTION_PEN:
		draw_pen_points(img, a->pen_points, a->pen_len, a->p0, a->color);
		break;
	case ACTION_NUMBER:
		draw_number_pin(img,a);
		break;
	case ACTION_RECT:
		if (a->p0 < 0) {
			draw_gradient_fill(img,fmin(a->x0,a->x1),fmin(a->y0,a->y1),abs(a->x1-a->x0)+1,abs(a->y1-a->y0)+1,0,0,action_fill_color(a),1.,!a->no_gradient);
            if(!a->no_outline)draw_rect(img,a->x0,a->y0,a->x1,a->y1,a->outline_thickness>0?a->outline_thickness:1,a->color);
		} else if(!a->no_outline) {
			draw_rect(img, a->x0, a->y0, a->x1, a->y1, a->p0, a->color);
		}
		break;
	case ACTION_CIRCLE:
		if (a->p0 < 0) {
			draw_gradient_fill(img,fmin(a->x0,a->x1),fmin(a->y0,a->y1),abs(a->x1-a->x0)+1,abs(a->y1-a->y0)+1,0,1,action_fill_color(a),1.,!a->no_gradient);
            if(!a->no_outline)draw_circle(img,a->x0,a->y0,a->x1,a->y1,a->outline_thickness>0?a->outline_thickness:1,a->color);
		} else if(!a->no_outline) {
			draw_circle(img, a->x0, a->y0, a->x1, a->y1, a->p0, a->color);
		}
		break;
	case ACTION_TEXT:
		{
			int scale = a->p0 < 0 ? -a->p0 : a->p0;
			if (scale < 1) {
				scale = 1;
			}
			if (a->p0 < 0 && a->text) {
				int bx;
				int by;
				int bw;
				int bh;
				int radius;
				unsigned int bg = a->has_fill_color?a->fill_color:inverse_color(a->color);
				struct editor_state styled_bg=*ed;styled_bg.text_style=a;
                text_box_metrics(&styled_bg, a->x0, a->y0, a->text, scale, 1, &bx, &by, &bw, &bh);
				radius = action_text_radius(a,scale);
				draw_fill_rounded_rect(img, bx, by, bw, bh, radius, bg,!a->no_gradient);
			}
			struct editor_state styled_text=*ed;styled_text.text_style=a;
            draw_text_xft(&styled_text, img, a->x0, a->y0, a->text ? a->text : "", scale, a->color);
		}
		break;
	case ACTION_HIGHLIGHT:
		draw_gradient_fill(img,fmin(a->x0,a->x1),fmin(a->y0,a->y1),abs(a->x1-a->x0)+1,abs(a->y1-a->y0)+1,0,0,a->color,fmax(.01,fmin(1.,a->p0/100.)),!a->no_gradient);
		break;
	case ACTION_MARKER:
		draw_marker_stroke(img, a->pen_points, a->pen_len, a->p0, a->color, default_marker_strength);
		break;
	case ACTION_PIXELATE:
		draw_pixelate(img, a->x0, a->y0, a->x1, a->y1, a->p0);
		break;
	case ACTION_BLUR:
		draw_blur(img, a->x0, a->y0, a->x1, a->y1, a->p0);
		break;
	default:
		break;
	}
}

static int
action_is_rotatable(enum action_type t)
{
	return t == ACTION_ARROW || t == ACTION_LINE || t == ACTION_PEN || t == ACTION_NUMBER ||
	       t == ACTION_RECT || t == ACTION_CIRCLE || t == ACTION_TEXT;
}

static void
action_center(const struct editor_state *ed, const struct action *a, double *cx, double *cy)
{
	int minx;
	int miny;
	int maxx;
	int maxy;

	action_bounds(ed, a, &minx, &miny, &maxx, &maxy);
	if (cx) {
		*cx = (minx + maxx) / 2.0;
	}
	if (cy) {
		*cy = (miny + maxy) / 2.0;
	}
}

static int
action_draw_pad(const struct action *a)
{
	int t;

	if (!a) {
		return 4;
	}
	switch (a->type) {
	case ACTION_ARROW:
		t = a->p0 > 0 ? a->p0 : 1;
		return (int)fmax(20,t*5)+4;
	case ACTION_LINE:
	case ACTION_RECT:
	case ACTION_CIRCLE:
	case ACTION_PEN:
        t=a->p0>0?a->p0:((a->type==ACTION_RECT || a->type==ACTION_CIRCLE) && !a->no_outline && a->outline_thickness>0?a->outline_thickness:1);
		return t / 2 + 3;
	default:
		return 4;
	}
}

/*
 * Render a rotatable action rotated by a->rotation degrees about its center.
 * The action is first drawn into a transparent scratch layer, which is then
 * sampled back onto img with an inverse rotation (nearest-neighbour).
 */
static void
apply_action_rotated(struct editor_state *ed, struct image *img, const struct action *a)
{
	struct image tmp;
	struct action s;
	int *pts = NULL;
	int minx;
	int miny;
	int maxx;
	int maxy;
	int pad;
	int tw;
	int th;
	double cx;
	double cy;
	double tcx;
	double tcy;
	double rad;
	double cosr;
	double sinr;
	double dminx;
	double dminy;
	double dmaxx;
	double dmaxy;
	int dx0;
	int dy0;
	int dx1;
	int dy1;
	int X;
	int Y;
	int i;

	if (!ed || !img || !img->pixels || !a) {
		return;
	}

	action_bounds(ed, a, &minx, &miny, &maxx, &maxy);
	pad = action_draw_pad(a);
	minx -= pad;
	miny -= pad;
	maxx += pad;
	maxy += pad;
	tw = maxx - minx + 1;
	th = maxy - miny + 1;
	if (tw < 1 || th < 1) {
		return;
	}
	/* guard against pathological sizes */
	if ((long)tw * (long)th > 64L * 1024L * 1024L) {
		return;
	}

	memset(&tmp, 0, sizeof(tmp));
	tmp.width = tw;
	tmp.height = th;
	tmp.len = (size_t)tw * (size_t)th * 4u;
	tmp.pixels = calloc(1, tmp.len);
	if (!tmp.pixels) {
		return;
	}

	s = *a;
	s.rotation = 0;
	switch (a->type) {
	case ACTION_NUMBER:
	case ACTION_TEXT:
		s.x0 = a->x0 - minx;
		s.y0 = a->y0 - miny;
		break;
	case ACTION_PEN:
		if (a->pen_points && a->pen_len > 0) {
			pts = malloc((size_t)a->pen_len * 2u * sizeof(*pts));
			if (!pts) {
				free(tmp.pixels);
				return;
			}
			for (i = 0; i < a->pen_len; i++) {
				pts[i * 2 + 0] = a->pen_points[i * 2 + 0] - minx;
				pts[i * 2 + 1] = a->pen_points[i * 2 + 1] - miny;
			}
			s.pen_points = pts;
		}
		break;
	default:
		s.x0 = a->x0 - minx;
		s.y0 = a->y0 - miny;
		s.x1 = a->x1 - minx;
		s.y1 = a->y1 - miny;
		break;
	}

	apply_action(ed, &tmp, &s);

	cx = (minx + maxx) / 2.0;
	cy = (miny + maxy) / 2.0;
	tcx = cx - minx;
	tcy = cy - miny;
	rad = a->rotation * M_PI / 180.0;
	cosr = cos(rad);
	sinr = sin(rad);

	/* destination bounding box: forward-rotate the four corners about center */
	dminx = dminy = 1e18;
	dmaxx = dmaxy = -1e18;
	for (i = 0; i < 4; i++) {
		double px = (i == 1 || i == 2) ? maxx : minx;
		double py = (i >= 2) ? maxy : miny;
		double rx = cosr * (px - cx) - sinr * (py - cy) + cx;
		double ry = sinr * (px - cx) + cosr * (py - cy) + cy;
		if (rx < dminx) dminx = rx;
		if (ry < dminy) dminy = ry;
		if (rx > dmaxx) dmaxx = rx;
		if (ry > dmaxy) dmaxy = ry;
	}
	dx0 = (int)floor(dminx) - 1;
	dy0 = (int)floor(dminy) - 1;
	dx1 = (int)ceil(dmaxx) + 1;
	dy1 = (int)ceil(dmaxy) + 1;
	if (dx0 < 0) dx0 = 0;
	if (dy0 < 0) dy0 = 0;
	if (dx1 > img->width - 1) dx1 = img->width - 1;
	if (dy1 > img->height - 1) dy1 = img->height - 1;

	for (Y = dy0; Y <= dy1; Y++) {
		for (X = dx0; X <= dx1; X++) {
			double dxp = X - cx;
			double dyp = Y - cy;
			/* inverse rotation back into scratch space */
			double sxr = cosr * dxp + sinr * dyp + tcx;
			double syr = -sinr * dxp + cosr * dyp + tcy;
			int isx = (int)lround(sxr);
			int isy = (int)lround(syr);
			const unsigned char *sp;
			unsigned char *dp;
			unsigned int al;

			if (isx < 0 || isy < 0 || isx >= tw || isy >= th) {
				continue;
			}
			sp = tmp.pixels + ((size_t)isy * (size_t)tw + (size_t)isx) * 4u;
			al = sp[3];
			if (al <= 8u) {
				continue;
			}
			dp = img->pixels + ((size_t)Y * (size_t)img->width + (size_t)X) * 4u;
			if (al >= 255u) {
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
				dp[3] = 0xff;
			} else {
				dp[0] = (unsigned char)(((unsigned int)dp[0] * (255u - al) + (unsigned int)sp[0] * al) / 255u);
				dp[1] = (unsigned char)(((unsigned int)dp[1] * (255u - al) + (unsigned int)sp[1] * al) / 255u);
				dp[2] = (unsigned char)(((unsigned int)dp[2] * (255u - al) + (unsigned int)sp[2] * al) / 255u);
				dp[3] = 0xff;
			}
		}
	}

	free(pts);
	free(tmp.pixels);
}

static void
free_action(struct action *a)
{
	if (!a) {
		return;
	}
	if (a->text) {
		free(a->text);
		a->text = NULL;
	}
	if (a->pen_points) {
		free(a->pen_points);
		a->pen_points = NULL;
	}
	a->pen_len = 0;
}

static int
push_action(struct action_stack *st, const struct action *in)
{
	struct action *new_items;
	struct action a;
	size_t i;

	if (!st || !in) {
		return -1;
	}
	if (st->cursor < st->len) {
		for (i = st->cursor; i < st->len; i++) {
			free_action(&st->items[i]);
		}
		st->len = st->cursor;
	}
	if (st->len == st->cap) {
		size_t new_cap = st->cap ? st->cap * 2 : 32;
		new_items = realloc(st->items, new_cap * sizeof(*new_items));
		if (!new_items) {
			return -1;
		}
		st->items = new_items;
		st->cap = new_cap;
	}
	a = *in;
	a.text = NULL;
	a.pen_points = NULL;
	a.pen_len = 0;
	if (in->text) {
		a.text = strdup(in->text);
		if (!a.text) {
			return -1;
		}
	}
	if (in->pen_points && in->pen_len > 0) {
		a.pen_points = malloc((size_t)in->pen_len * 2u * sizeof(*a.pen_points));
		if (!a.pen_points) {
			free(a.text);
			a.text = NULL;
			return -1;
		}
		memcpy(a.pen_points, in->pen_points, (size_t)in->pen_len * 2u * sizeof(*a.pen_points));
		a.pen_len = in->pen_len;
	}
	st->items[st->len++] = a;
	st->cursor = st->len;
	return 0;
}

static int
tool_count(void)
{
	return 13;
}

static enum tool
tool_by_index(int idx)
{
	switch (idx) {
	case 0: return TOOL_SELECT;
	case 1: return TOOL_ARROW;
	case 2: return TOOL_LINE;
	case 3: return TOOL_PEN;
	case 4: return TOOL_NUMBER;
	case 5: return TOOL_RECT;
	case 6: return TOOL_CIRCLE;
	case 7: return TOOL_TEXT;
	case 8: return TOOL_HIGHLIGHT;
	case 9: return TOOL_MARKER;
	case 10: return TOOL_PIXELATE;
	case 11: return TOOL_BLUR;
	default: return TOOL_PICKER;
	}
}

static const char *
tool_label(enum tool t)
{
	switch (t) {
	case TOOL_SELECT: return "SEL";
	case TOOL_ARROW: return "ARW";
	case TOOL_LINE: return "LIN";
	case TOOL_PEN: return "PEN";
	case TOOL_NUMBER: return "NUM";
	case TOOL_RECT: return "REC";
	case TOOL_CIRCLE: return "CIR";
	case TOOL_TEXT: return "TXT";
	case TOOL_HIGHLIGHT: return "HL";
	case TOOL_MARKER: return "MRK";
	case TOOL_PIXELATE: return "PXL";
	case TOOL_BLUR: return "BLR";
	case TOOL_PICKER: return "PCK";
	default: return "?";
	}
}

static int
undo_action(struct action_stack *st)
{
	if (!st || st->cursor == 0) {
		return -1;
	}
	st->cursor--;
	return 0;
}

static int
redo_action(struct action_stack *st)
{
	if (!st || st->cursor >= st->len) {
		return -1;
	}
	st->cursor++;
	return 0;
}

static void free_actions(struct action_stack *st)
{
	size_t i;

	if (!st) {
		return;
	}
	for (i = 0; i < st->len; i++) {
		free_action(&st->items[i]);
	}
	free(st->items);
	st->items = NULL;
	st->len = 0;
	st->cap = 0;
}

static void rerender_from_actions(struct editor_state *ed)
{
	size_t i;

	img_copy(&ed->rendered, &ed->base);
	for (i = 0; i < ed->actions.cursor; i++) {
if((!ed->transform_preview || (int)i!=ed->selected_idx) && (!ed->text_mode || (int)i!=ed->text_edit_index))
            apply_action(ed, &ed->rendered, &ed->actions.items[i]);
	}
	ed->dirty = 0;ed->source_dirty=1;
}

static const struct action *selected_text(const struct editor_state *ed)
{
 if(ed->tool!=TOOL_SELECT)return NULL;
 for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];
     if(index>=0 && (size_t)index<ed->actions.cursor && ed->actions.items[index].type==ACTION_TEXT)return &ed->actions.items[index];
 }
 return NULL;
}

static const struct action *selected_action(const struct editor_state *ed)
{
 if(ed->tool!=TOOL_SELECT)return NULL;
 for(int i=0;i<ed->sel_len;i++)if(ed->sel[i]>=0 && (size_t)ed->sel[i]<ed->actions.cursor)return &ed->actions.items[ed->sel[i]];
 return NULL;
}
static int selection_supports_fill(const struct editor_state *ed)
{
 if(ed->tool!=TOOL_SELECT)return 0;
 for(int i=0;i<ed->sel_len;i++)if(ed->sel[i]>=0 && (size_t)ed->sel[i]<ed->actions.cursor && action_supports_fill(&ed->actions.items[ed->sel[i]]))return 1;
 return 0;
}
static int action_dimension(const struct action *a)
{
 if(a->type==ACTION_TEXT)return abs(a->p0)*TEXT_SCALE_PX;
 if(a->type==ACTION_NUMBER)return a->y1*TEXT_SCALE_PX;
 if(a->type==ACTION_RECT || a->type==ACTION_CIRCLE)return a->p0<0?(a->outline_thickness>0?a->outline_thickness:1):a->p0;
 return a->p0;
}
static int size_selected_actions(struct editor_state *ed,int value,int relative)
{
 if(!selected_action(ed))return 0;
 for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];if(index<0 || (size_t)index>=ed->actions.cursor)continue;
     struct action *a=&ed->actions.items[index];int old=action_dimension(a),step=1,low=1,high=128;
     if(a->type==ACTION_RECT || a->type==ACTION_CIRCLE)step=3;
     if(a->type==ACTION_TEXT || a->type==ACTION_NUMBER){step=TEXT_SCALE_PX;low=TEXT_SCALE_PX;high=max_text_scale*TEXT_SCALE_PX;}
     if(a->type==ACTION_MARKER){step=8;low=4;high=400;}
     if(a->type==ACTION_BLUR){step=3;high=16;}
     if(a->type==ACTION_HIGHLIGHT){step=5;high=100;}
     if(a->type==ACTION_PIXELATE){low=2;high=64;}
     int next=relative?old+value*step:value;next=next<low?low:next>high?high:next;
     if(a->type==ACTION_TEXT){int scale=next/TEXT_SCALE_PX;a->p0=a->p0<0?-scale:scale;ed->text_scale=scale;}
     else if(a->type==ACTION_NUMBER){a->y1=next/TEXT_SCALE_PX;a->p0=number_radius_for_scale(a->y1);ed->text_scale=a->y1;}
     else if(a->type==ACTION_RECT || a->type==ACTION_CIRCLE){a->outline_thickness=next;if(a->p0>=0)a->p0=next;}
     else a->p0=next;
 }
 ed->dirty=ed->raster_dirty=1;ed->chrome_key[0]=0;return 1;
}
static unsigned int action_fill_color(const struct action *a)
{
 return a->has_fill_color?a->fill_color:a->type==ACTION_TEXT?inverse_color(a->color):a->color;
}
static unsigned int picker_current_color(const struct editor_state *ed)
{
 const struct action *a=selected_action(ed);
 if(a)return ed->color_target_fill && action_supports_fill(a)?action_fill_color(a):a->color;
 return ed->color;
}
static int apply_selected_color(struct editor_state *ed,unsigned int color,int fill)
{
 if(!selected_action(ed))return 0;
 int changed=0;
 for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];if(index<0 || (size_t)index>=ed->actions.cursor)continue;
     struct action *a=&ed->actions.items[index];
     if(a->type==ACTION_BLUR || a->type==ACTION_PIXELATE)continue;
     if(fill) {
         if(!action_supports_fill(a))continue;
         a->fill_color=color;a->has_fill_color=1;
         if(a->type==ACTION_TEXT)a->p0=-abs(a->p0);
         else {if(a->p0>0)a->outline_thickness=a->p0;a->p0=-1;}
     } else {
         if(action_supports_fill(a) && !a->has_fill_color){a->fill_color=action_fill_color(a);a->has_fill_color=1;}
         a->color=color;
     }
     changed=1;
 }
 if(changed){if(fill)ed->fill_mode=1;else ed->color=color;ed->dirty=ed->raster_dirty=1;ed->chrome_key[0]=0;}
 return changed;
}
static void apply_picker_color(struct editor_state *ed,unsigned int color)
{
 if(!apply_selected_color(ed,color,ed->color_target_fill) && !ed->color_target_fill)ed->color=color;
}

/* Shared by toolbar clicks and keyboard size shortcuts. */
static int tool_size(const struct editor_state *ed)
{
 const struct action *selected=selected_action(ed);if(selected)return action_dimension(selected);
	switch (ed->tool) {
	case TOOL_TEXT:
	case TOOL_NUMBER:
		return ed->text_scale*TEXT_SCALE_PX;
	case TOOL_HIGHLIGHT:
		return ed->highlight_strength;
	case TOOL_PIXELATE:
		return ed->pixelate_block;
	case TOOL_BLUR:
		return ed->blur_radius;
	case TOOL_MARKER:
		return ed->marker_height;
	default:
		return thickness_presets[ed->thickness_idx];
	}
}
static int dimension_is_percentage(const struct editor_state *ed)
{
 const struct action *a=selected_action(ed);
 return a?a->type==ACTION_HIGHLIGHT:ed->tool==TOOL_HIGHLIGHT;
}
static void adjust_tool_size(struct editor_state *ed, int delta)
{
 if(size_selected_actions(ed,delta,1))return;
	int *value = NULL, low = 1, high = 100, step = 1;
	switch (ed->tool) {
	case TOOL_TEXT:
	case TOOL_NUMBER:
		value = &ed->text_scale;
		high = max_text_scale;
		break;
	case TOOL_HIGHLIGHT:
		value = &ed->highlight_strength;
        step=5;
		break;
	case TOOL_PIXELATE:
		value = &ed->pixelate_block;
		low = 2;
		high = 64;
		break;
	case TOOL_BLUR:
		value = &ed->blur_radius;
        step=3;
		high = 16;
		break;
	case TOOL_MARKER:
		value = &ed->marker_height;
		low = 4;
		high = 400;
		step = 8;
		break;
	default:
		value = &ed->thickness_idx;
		low = 0;
		high =
		    sizeof(thickness_presets) / sizeof(thickness_presets[0]) -
		    1;
		break;
	}
	*value += delta * step;
	if (*value < low)
		*value = low;
	if (*value > high)
		*value = high;
	ed->raster_dirty = 1;
}

#define ROTATE_HANDLE_OFFSET 22
#define ROTATE_HANDLE_RADIUS 7
#define TOOLBAR_WIDTH 1016
#define TOOLBAR_Y 16
#define TOOLBAR_HEIGHT 64
#define FOOTER_HEIGHT 44

#include "assets/icons/masks.h"

struct chrome_label {
	cui_rect bounds;
	cui_color color;
	float size;
	char text[256];
 int left;
};
static struct chrome_label chrome_labels[192];
static size_t chrome_label_count;

static cui_context *chrome;
static PangoLayout *chrome_layouts[192];
static char *chrome_strings[192];
static float chrome_sizes[192];
static unsigned chrome_next;
static cairo_surface_t *chrome_icons[8];
static float chrome_scale(const struct editor_state *ed)
{
	return fminf(1.0f, (ed->win_w - 24.0f) / (toolbar_rows(ed)==2 ? 520 : TOOLBAR_WIDTH));
}
static int toolbar_left(const struct editor_state *ed)
{
	return (ed->win_w - (int)((toolbar_rows(ed)==2 ? 520 : TOOLBAR_WIDTH) * chrome_scale(ed))) / 2;
}
static int footer_width(const struct editor_state *ed)
{
	return ed->win_w < 980 ? ed->win_w - 32 : 948;
}
static int footer_left(const struct editor_state *ed)
{
	return (ed->win_w - footer_width(ed)) / 2;
}
static int toolbar_tool_index_at_x(const struct editor_state *ed, int x)
{
	float local = (x - toolbar_left(ed)) / chrome_scale(ed) - 12;
	int index = (int)(local / 38);
	return local >= 0 && index < tool_count() && (int)local % 38 < 36
		   ? index
		   : -1;
}

static cui_rect popup_rect(const struct editor_state *ed);
static int chrome_hit(const struct editor_state *ed, int x, int y)
{
	if (ed->popup) {
		cui_rect rect = popup_rect(ed);
		if (x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
		    y < rect.y + rect.h)
			return 22;
	}

	float scale = chrome_scale(ed), local = (x - toolbar_left(ed)) / scale+108,
	      top = (y - TOOLBAR_Y) / scale;
 if(toolbar_rows(ed)==2 && top>=64) {top-=64;local+=504;}
	if (top >= 10 && top <= 54) {
		int tool = (toolbar_rows(ed)==2 && y>=TOOLBAR_Y+64*scale) ? -1 : toolbar_tool_index_at_x(ed, x);
		if (tool >= 0)
			return tool;
		if (local >= 624 && local < 656)
			return 13;
		if(local>=724 && local<759)return 21;
 if(local>=759 && local<794)return 23;
		if (local >= 662 && local < 716)
			return 14;
		if (local >= 804 && local < 914)
			return 15;
		if (local >= 924 && local < 1110)
			return 16;
	}
	int fx = footer_left(ed), fw = footer_width(ed),
	    fy = ed->win_h - footer_height(ed) - 16;
	if (y >= fy + 6 && y < fy + 38) {
		if(x>=fx+8 && x<fx+(fw<450?84:104))return 24;
		int relative = x - fx - fw;
		if (relative >= -282 && relative < -250)
			return 17;
		if (relative >= -176 && relative < -144)
			return 18;
		if (relative >= -136 && relative < -88)
			return 19;
		if (relative >= -80 && relative < -48)
			return 20;
        if(relative>=-44 && relative<-12)
            return 25;
	}
	return -1;
}

static PangoLayout *chrome_layout(cui_str text, float size)
{
	unsigned i, slot;
	for (i = 0; i < 192; i++)
		if (chrome_layouts[i] && chrome_sizes[i] == size &&
		    strlen(chrome_strings[i]) == text.length &&
		    !memcmp(chrome_strings[i], text.data, text.length))
			return chrome_layouts[i];
	slot = chrome_next++ % 192;
	if (chrome_layouts[slot])
		g_object_unref(chrome_layouts[slot]);
	free(chrome_strings[slot]);
	chrome_strings[slot] = strndup(text.data, text.length);
	PangoContext *context =
	    pango_font_map_create_context(pango_cairo_font_map_get_default());
	PangoLayout *layout = pango_layout_new(context);
	g_object_unref(context);
	PangoFontDescription *font =
	    pango_font_description_from_string("Geist Medium");
	pango_font_description_set_absolute_size(font, size * PANGO_SCALE);
	pango_layout_set_font_description(layout, font);
	pango_font_description_free(font);
	pango_layout_set_text(layout, text.data, (int)text.length);
	pango_layout_set_single_paragraph_mode(layout, TRUE);
	chrome_layouts[slot] = layout;
	chrome_sizes[slot] = size;
	return layout;
}
static cui_size chrome_measure(void *user, cui_str text, float size)
{
	int w, h;
	(void)user;
	pango_layout_get_pixel_size(chrome_layout(text, size), &w, &h);
	return (cui_size){w, h};
}
static const struct ui_theme *chrome_theme = &themes[S2_DEFAULT_THEME];
static cui_color theme_color(unsigned int rgb)
{
 return (cui_color){rgb>>16 & 255,rgb>>8 & 255,rgb & 255,255};
}
static void set_theme(struct editor_state *ed, int index)
{
 if(index<0 || (size_t)index>=THEME_COUNT)return;
 ed->theme=index;ed->window_bg=themes[index].workspace;ed->raster_dirty=1;
}
/* Remap legacy chrome roles; annotation colors and picker swatches stay literal. */
static cui_color chrome_color(cui_color c)
{
 unsigned int rgb=(c.r<<16)|(c.g<<8)|c.b, value=rgb;
 if(c.a!=255)return c;
 switch(rgb) {
 case 0x181a20: case 0x191c23:value=chrome_theme->panel;break;
 case 0xaab1c2:case 0xcdd4e2:case 0xdbe3f2:case 0xdfe5f1:case 0x919db3:value=chrome_theme->text;break;
 case 0xb3caff:case 0xc7d8ff:value=chrome_theme->accent;break;
 case 0x303d58:case 0x212b3e:value=chrome_theme->selected;break;
 case 0x272c38:case 0x272c37:case 0x252933:case 0x232731:value=chrome_theme->hover;break;
 case 0x0f1218:value=chrome_theme->workspace;break;
 case 0x414859:value=chrome_theme->border;break;
 case 0x1b2a46:value=chrome_theme->panel;break;
 }
 return theme_color(value);
}
static void ui_box(cui_context *ui, const char *id, float x, float y, float w,
		   float h, cui_color bg, const char *text, cui_color fg,
		   float size)
{
 if(strncmp(id,"pigment",7) && strncmp(id,"theme-preview",13)) {
     bg=chrome_color(bg);
     if(strcmp(id,"color"))fg=chrome_color(fg);
 }
 cui_begin_absolute_box(
	    ui, cui_id_from(ui, id),
	    (cui_box){.width = w,
		      .height = h,
		      .padding = 0,
		      .radius = (!strcmp(id,"about-card") || !strcmp(id,"about-hero"))?18:strstr(id,"shortcut")?5:9,
		      .background = bg,
		      .align_x = CUI_ALIGN_CENTER,
		      .align_y = CUI_ALIGN_CENTER,
		      .border = theme_color(chrome_theme->border),
		      .border_width =
			  (!strcmp(id, "toolbar") || !strcmp(id, "status") ||
			   !strcmp(id, "picker-panel") || !strcmp(id,"about-card") || !strcmp(id,"about-repo"))
			      ? 1
			      : 0},
	    (cui_rect){x, y, w, h});
	if (text && chrome_label_count < 192) {
		struct chrome_label *label =
		    &chrome_labels[chrome_label_count++];
		label->bounds = (cui_rect){x, y, w, h};
 label->left=0;
		label->color = fg;
		label->size = size;
		snprintf(label->text, sizeof label->text, "%s", text);
	}
	cui_end_box(ui);
}
static void top_box(struct editor_state *ed, const char *id, float x, float y,
		    float w, float h, cui_color bg, const char *text,
		    cui_color fg)
{
	float s = chrome_scale(ed);
	if(!strcmp(id,"toolbar")) {w=toolbar_rows(ed)==2?520:TOOLBAR_WIDTH;h=64*toolbar_rows(ed);}
 else {x-=108;if(toolbar_rows(ed)==2 && x>=504) {x-=504;y+=64;}}
 ui_box(chrome, id, toolbar_left(ed) + x * s, TOOLBAR_Y + y * s, w * s,
	       h * s, bg, text, fg, 14 * s);
}
static const unsigned int picker_colors[] = {
    0xffffff, 0xcbd5e1, 0x64748b, 0x111827, 0xef4444, 0xf97316,
    0xfacc15, 0x84cc16, 0x22c55e, 0x14b8a6, 0x06b6d4, 0x5689ef,
    0x6366f1, 0xa855f7, 0xec4899, 0xfda4af};
static cui_rect popup_rect(const struct editor_state *ed)
{
	float s = chrome_scale(ed),
	      x = toolbar_left(ed) + (ed->popup == 1 ? 516 : 554) * s - 110;
	if (x > ed->win_w - 284)
		x = ed->win_w - 284;
	if (x < 12)
		x = 12;
	return (cui_rect){x, TOOLBAR_Y + TOOLBAR_HEIGHT * toolbar_rows(ed) * s + 10, 272, 296};
}
static int size_options(const struct editor_state *ed, int values[8])
{
	static const int text[] = {8, 16, 24, 32, 48, 64, 96, 128},
			 strength[] = {10, 20, 30, 40, 50, 60, 80, 100},
			 blocks[] = {2, 4, 8, 12, 16, 24, 32, 64},
			 blur[] = {1, 2, 3, 4, 6, 8, 12, 16},
			 marker[] = {8, 12, 16, 24, 32, 48, 64, 96};
	const int *source = NULL;
 const struct action *a=selected_action(ed);
 enum tool tool=ed->tool;
 if(a) {
     if(a->type==ACTION_TEXT || a->type==ACTION_NUMBER){memcpy(values,text,sizeof text);return 8;}
     if(a->type==ACTION_MARKER)tool=TOOL_MARKER;
     if(a->type==ACTION_HIGHLIGHT)tool=TOOL_HIGHLIGHT;
     if(a->type==ACTION_BLUR)tool=TOOL_BLUR;
     if(a->type==ACTION_PIXELATE)tool=TOOL_PIXELATE;
 }
 switch (tool) {
	case TOOL_TEXT:
	case TOOL_NUMBER:
		source = text;
		break;
	case TOOL_HIGHLIGHT:
		source = strength;
		break;
	case TOOL_PIXELATE:
		source = blocks;
		break;
	case TOOL_BLUR:
		source = blur;
		break;
	case TOOL_MARKER:
		source = marker;
		break;
	default: {
		size_t count =
		    sizeof(thickness_presets) / sizeof(thickness_presets[0]);
		if (count > 8)
			count = 8;
		for (size_t i = 0; i < count; i++)
			values[i] = thickness_presets[i];
		return (int)count;
	}
	}
	memcpy(values, source, 8 * sizeof(int));
	return 8;
}
static void set_tool_size(struct editor_state *ed, int value)
{
 if(size_selected_actions(ed,value,0))return;
	switch (ed->tool) {
	case TOOL_TEXT:
	case TOOL_NUMBER:
		ed->text_scale = value / TEXT_SCALE_PX;
		if (ed->text_scale < 1)
			ed->text_scale = 1;
		if (ed->text_scale > max_text_scale)
			ed->text_scale = max_text_scale;
		break;
	case TOOL_HIGHLIGHT:
		ed->highlight_strength = value;
		break;
	case TOOL_PIXELATE:
		ed->pixelate_block = value;
		break;
	case TOOL_BLUR:
		ed->blur_radius = value;
		break;
	case TOOL_MARKER:
		ed->marker_height = value;
		break;
	default:
		for (size_t i = 0; i < sizeof(thickness_presets) /
					   sizeof(thickness_presets[0]);
		     i++)
			if (thickness_presets[i] == value)
				ed->thickness_idx = i;
		break;
	}
	ed->raster_dirty = 1;
}
static int popup_click(struct editor_state *ed, int x, int y)
{
	if (!ed->popup)
		return 0;
	cui_rect rect = popup_rect(ed);
	int px = x - rect.x, py = y - rect.y;
	if (px < 0 || py < 0 || px >= rect.w || py >= rect.h) {
		int hit = chrome_hit(ed, x, y);
		if (hit == 13 || hit == 14 || hit == 24)
			return 0;
		ed->popup = 0;
		return hit < 0;
	}
	if(ed->popup==3) {
        int row=(py-44)/26;
        if(py>=44 && row>=0 && (size_t)row<THEME_COUNT && py<44+26*(int)THEME_COUNT) {
            set_theme(ed,row);ed->popup=0;
        }
        return 1;
    }
 if (ed->popup == 1) {
        if(py>=8 && py<36 && selection_supports_fill(ed)) {
            ed->color_target_fill=px>=136;return 1;
        }
		if (py >= 44 && py < 196 && px >= 16) {
			int col = (px - 16) / 60, row = (py - 44) / 40;
			if (col < 4 && row < 4 && (px - 16) % 60 < 48 &&
			    (py - 44) % 40 < 32) {
				apply_picker_color(ed,picker_colors[row * 4 + col]);
				ed->popup = 0;
			}
		} else if (py >= 208 && py < 244) {
			ed->popup = 0;
			ed->color_mode = 1;
			ed->color_len = 0;
			ed->color_buf[0] = '\0';
		} else if (py >= 252 && py < 284) {
			ed->popup = 0;
			if(selected_action(ed))ed->picker_active=1;else set_tool(ed, TOOL_PICKER);
		}
	} else {
		int values[8], count = size_options(ed, values);
		if (py >= 48 && py < 224 && px >= 16) {
			int col = (px - 16) / 128, row = (py - 48) / 44,
			    index = row * 2 + col;
			if (col < 2 && index < count && (px - 16) % 128 < 112 &&
			    (py - 48) % 44 < 36) {
				set_tool_size(ed, values[index]);
				ed->popup = 0;
			}
		} else if (py >= 244 && py < 280) {
			if (px >= 16 && px < 64)
				adjust_tool_size(ed, -1);
			if (px >= 208 && px < 256)
				adjust_tool_size(ed, 1);
		}
	}
	return 1;
}
static void draw_popup(struct editor_state *ed, cui_color panel, cui_color fg,
		       cui_color accent, cui_color active)
{
	if (!ed->popup)
		return;
	cui_rect rect = popup_rect(ed);
	float x = rect.x, y = rect.y;
	char msg[64], id[32];
	ui_box(chrome, "picker-panel", x, y, rect.w, rect.h, panel, NULL, fg,
	       13);
	ui_box(chrome, "picker-title", x + 12, y + 8, 248, 28, panel,
	       ed->popup == 3 ? "Theme presets" : ed->popup == 1 ? (selection_supports_fill(ed)?NULL:"Choose a color")
			      : (ed->tool == TOOL_NUMBER ? "Number size"
				 : (ed->tool == TOOL_TEXT || selected_text(ed)) ? "Text size"
							 : "Tool dimension"),
	       fg, 14);
if(ed->popup==1 && selection_supports_fill(ed)) {
     /* Replace the title with explicit targets without moving the swatches. */
     ui_box(chrome,"color-stroke-tab",x+12,y+8,120,28,ed->color_target_fill?panel:active,"Stroke / Text",fg,12);
     ui_box(chrome,"color-fill-tab",x+136,y+8,120,28,ed->color_target_fill?active:panel,"Fill",fg,12);
 }
if(ed->popup==3) {
     for(size_t i=0;i<THEME_COUNT;i++) {
         snprintf(id,sizeof id,"theme-row%zu",i);
         snprintf(msg,sizeof msg,"%zu  %s%s",i+1,themes[i].name,(int)i==ed->theme?"  ✓":"");
         ui_box(chrome,id,x+12,y+44+i*26,248,24,(int)i==ed->theme?active:panel,NULL,fg,12);
         snprintf(id,sizeof id,"theme-name%zu",i);
         ui_box(chrome,id,x+46,y+44+i*26,208,24,(int)i==ed->theme?active:panel,msg,fg,12);
         chrome_labels[chrome_label_count-1].left=1;
         snprintf(id,sizeof id,"theme-preview%zu",i);
         ui_box(chrome,id,x+20,y+49+i*26,14,14,theme_color(themes[i].accent),NULL,fg,12);
     }
     return;
 }
 if (ed->popup == 1) {
		for (int i = 0; i < 16; i++) {
			unsigned int color = picker_colors[i];
			float sx = x + 16 + (i % 4) * 60,
			      sy = y + 44 + (i / 4) * 40;
			snprintf(id, sizeof id, "swatch%d", i);
			ui_box(chrome, id, sx, sy, 48, 32,
			       picker_current_color(ed) == color ? accent : panel, NULL, fg,
			       13);
			snprintf(id, sizeof id, "pigment%d", i);
			ui_box(chrome, id, sx + 3, sy + 3, 42, 26,
			       (cui_color){(color >> 16) & 255,
					   (color >> 8) & 255, color & 255,
					   255},
			       NULL, fg, 13);
		}
		snprintf(msg, sizeof msg, "Custom color  #%06X", picker_current_color(ed));
		ui_box(chrome, "custom-color", x + 16, y + 208, 240, 36, active,
		       msg, fg, 13);
		ui_box(chrome, "sample-color", x + 16, y + 252, 240, 32, panel,
		       "Pick from image  ·  C", fg, 13);
	} else {
		int values[8], count = size_options(ed, values);
		for (int i = 0; i < count; i++) {
			snprintf(id, sizeof id, "dimension%d", i);
			snprintf(msg, sizeof msg,
				 dimension_is_percentage(ed) ? "%d%%" : "%d px",
				 values[i]);
			ui_box(chrome, id, x + 16 + (i % 2) * 128,
			       y + 48 + (i / 2) * 44, 112, 36,
			       tool_size(ed) == values[i]
				   ? active
				   : (cui_color){35, 39, 49, 255},
			       msg, tool_size(ed) == values[i] ? accent : fg,
			       14);
		}
		ui_box(chrome, "dimension-minus", x + 16, y + 244, 48, 36,
		       active, "−", fg, 18);
		snprintf(msg, sizeof msg,
			 dimension_is_percentage(ed) ? "%d%%" : "%d px",
			 tool_size(ed));
		ui_box(chrome, "dimension-value", x + 72, y + 244, 128, 36,
		       panel, msg, fg, 14);
		ui_box(chrome, "dimension-plus", x + 208, y + 244, 48, 36,
		       active, "+", fg, 18);
	}
}

static void icon_mask(cairo_t *cr, int index, double x, double y, double size,
		      cui_color color)
{
	static const unsigned char *masks[] = {
	    icon_select, icon_pen,    icon_marker, icon_pixelate,
	    icon_blur,	 icon_picker, icon_help,   icon_save};
	if (!chrome_icons[index])
		chrome_icons[index] = cairo_image_surface_create_for_data(
		    (unsigned char *)masks[index], CAIRO_FORMAT_A8, 48, 48, 48);
	cairo_save(cr);
	cairo_translate(cr, x, y);
	cairo_scale(cr, size / 48, size / 48);
	cairo_set_source_rgba(cr, color.r / 255., color.g / 255.,
			      color.b / 255., color.a / 255.);
	cairo_mask_surface(cr, chrome_icons[index], 0, 0);
	cairo_restore(cr);
}
static void tool_icon(cairo_t *cr, int index, double x, double y, double size,
		      cui_color color)
{
	static const int generated[] = {0,  -1, -1, 1, -1, -1, -1,
					-1, -1, 2,  3, 4,  5};
	if (generated[index] >= 0) {
		icon_mask(cr, generated[index], x, y, size, color);
		return;
	}
	cairo_save(cr);
	cairo_translate(cr, x, y);
	cairo_scale(cr, size / 24, size / 24);
	cairo_set_source_rgba(cr, color.r / 255., color.g / 255.,
			      color.b / 255., 1);
	cairo_set_line_width(cr, 1.65);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	switch (index) {
	case 1:
		cairo_move_to(cr, 5, 19);
		cairo_line_to(cr, 19, 5);
		cairo_move_to(cr, 9, 5);
		cairo_line_to(cr, 19, 5);
		cairo_line_to(cr, 19, 15);
		break;
	case 2:
		cairo_move_to(cr, 5, 18);
		cairo_line_to(cr, 19, 6);
		break;
	case 4:
		cairo_arc(cr, 12, 12, 8, 0, 2 * M_PI);
		cairo_move_to(cr, 10, 9);
		cairo_line_to(cr, 12, 7);
		cairo_line_to(cr, 12, 17);
		cairo_move_to(cr, 9, 17);
		cairo_line_to(cr, 15, 17);
		break;
	case 5:
		cairo_rectangle(cr, 4, 5, 16, 14);
		break;
	case 6:
		cairo_arc(cr, 12, 12, 8, 0, 2 * M_PI);
		break;
	case 7:
		cairo_move_to(cr, 5, 6);
		cairo_line_to(cr, 19, 6);
		cairo_move_to(cr, 12, 6);
		cairo_line_to(cr, 12, 19);
		cairo_move_to(cr, 9, 19);
		cairo_line_to(cr, 15, 19);
		break;
	case 8:
		cairo_move_to(cr, 5, 15);
		cairo_line_to(cr, 10, 5);
		cairo_line_to(cr, 19, 9);
		cairo_line_to(cr, 14, 19);
		cairo_close_path(cr);
		cairo_move_to(cr, 4, 21);
		cairo_line_to(cr, 17, 21);
		break;
	}
	cairo_stroke(cr);
	cairo_restore(cr);
}
static void paint_chrome_label(cairo_t *cr,const struct chrome_label *label)
{
 PangoLayout *layout=chrome_layout(cui_string(label->text),label->size);
 PangoRectangle ink;pango_layout_get_pixel_extents(layout,&ink,NULL);
 /* Centered labels already have spacing in their layout bounds. Extra
  * fixed clipping insets truncate small buttons, especially when scaled. */
 double inset=label->left?4:0;
 cairo_save(cr);cairo_rectangle(cr,label->bounds.x+inset,label->bounds.y,label->bounds.w-2*inset,label->bounds.h);cairo_clip(cr);
 cairo_set_source_rgba(cr,label->color.r/255.,label->color.g/255.,label->color.b/255.,label->color.a/255.);
 cairo_move_to(cr,label->bounds.x+(label->left?8:(label->bounds.w-ink.width)/2)-ink.x,label->bounds.y+(label->bounds.h-ink.height)/2-ink.y);
 pango_cairo_show_layout(cr,layout);cairo_restore(cr);

}

static cui_rect help_rect(const struct editor_state *ed)
{
 return (cui_rect){(ed->win_w-fminf(700,ed->win_w-48))/2,(ed->win_h-560)/2,fminf(700,ed->win_w-48),560};
}
static void help_row(const char *id,float x,float y,float width,const char *name,const char *key)
{
 char label[64],cap[64];snprintf(label,sizeof label,"%s-label",id);snprintf(cap,sizeof cap,"%s-key",id);
 ui_box(chrome,label,x,y,width-90,28,(cui_color){25,28,35,255},name,(cui_color){205,212,226,255},13);
 chrome_labels[chrome_label_count-1].left=1;
 ui_box(chrome,cap,x+width-82,y+2,78,24,(cui_color){39,44,55,255},key,(cui_color){179,202,255,255},12);
}
static void compose_help(struct editor_state *ed)
{
 if(!ed->show_help)return;
 cui_rect r=help_rect(ed);cui_color bg={25,28,35,255},fg={219,227,242,255},muted={145,157,179,255},blue={179,202,255,255};
 ui_box(chrome,"help-scrim",0,0,ed->win_w,ed->win_h,(cui_color){0,0,0,165},NULL,fg,13);
 ui_box(chrome,"help-card",r.x,r.y,r.w,r.h,bg,NULL,fg,13);
 ui_box(chrome,"help-heading",r.x+24,r.y+22,r.w-108,32,bg,"Keyboard shortcuts",fg,21);
 chrome_labels[chrome_label_count-1].left=1;
 ui_box(chrome,"help-close",r.x+r.w-58,r.y+20,34,34,(cui_color){39,44,55,255},"×",fg,21);
 ui_box(chrome,"help-subtitle",r.x+24,r.y+62,r.w-48,24,bg,"Choose a tool, make your mark, then save or copy.",muted,13);
 chrome_labels[chrome_label_count-1].left=1;
 float col=(r.w-64)/2,left=r.x+24,right=left+col+16;
 ui_box(chrome,"help-tools-title",left,r.y+112,col,28,bg,"DRAW & MARK",blue,12);
 chrome_labels[chrome_label_count-1].left=1;
 ui_box(chrome,"help-edit-title",right,r.y+112,col,28,bg,"EDIT & FINISH",blue,12);
 chrome_labels[chrome_label_count-1].left=1;
 const char *names[]={"Select / Arrow","Line / Pencil","Rectangle / Circle","Text / Number","Highlight / Marker","Blur / Pixelate","Color / Theme","Fill / Outline","Adjust tool size"};
 const char *keys[]={"S / A","L / P","R / O","T / N","H / M","B / X","C / Alt C","f / ⇧F","[ / ]"};
 const char *edit[]={"Undo / Redo","Delete selection","Paste text","Copy & close","Copy, keep editing","Save timestamped","Finish text entry","Zoom / Fit","Cancel placement"};
 const char *ekeys[]={"⌃Z / ⇧⌃Z","Del","⌃V","⌃C","⌃Y","⌃S","Enter","+ − / =","X"};
 for(int i=0;i<9;i++){char id[24];snprintf(id,sizeof id,"help-tool-%d",i);help_row(id,left,r.y+154+i*32,col,names[i],keys[i]);snprintf(id,sizeof id,"help-edit-%d",i);help_row(id,right,r.y+154+i*32,col,edit[i],ekeys[i]);}
 ui_box(chrome,"help-tip",r.x+24,r.y+460,r.w-48,28,(cui_color){33,43,62,255},"Shift: square shapes · 45° rotation · proportional resizing",blue,12);
 ui_box(chrome,"help-text-style",r.x+24,r.y+492,r.w-48,24,bg,"Selected text: b/B weight · p/P padding · c/C corners · Shift+1–9 fill",fg,12);
 ui_box(chrome,"help-dismiss",r.x+24,r.y+526,r.w-48,20,bg,"Esc or ? to return to your capture",muted,12);
}

static cui_rect about_rect(const struct editor_state *ed)
{
 float scale=fminf(1,fminf((ed->win_w-32)/560.f,(ed->win_h-32)/500.f));
 return (cui_rect){(ed->win_w-560*scale)/2,(ed->win_h-500*scale)/2,560*scale,500*scale};
}
static int about_hit(const struct editor_state *ed,int x,int y)
{
 cui_rect r=about_rect(ed);double s=r.w/560.,px=(x-r.x)/s,py=(y-r.y)/s;
 if(px<0 || py<0 || px>=560 || py>=500)return -1;
 if((px>=494 && px<536 && py>=20 && py<62) || (px>=32 && px<528 && py>=448 && py<484))return 2;
 if(px>=32 && px<528 && py>=300 && py<354)return 1;
 return 0;
}
static void about_box(const struct editor_state *ed,const char *id,float x,float y,float w,float h,cui_color bg,const char *text,cui_color fg,float size)
{
 cui_rect r=about_rect(ed);float s=r.w/560;
 ui_box(chrome,id,r.x+x*s,r.y+y*s,w*s,h*s,bg,text,fg,fmaxf(size>=10 && size<=18?10:0,size*s));
}
static void compose_about(struct editor_state *ed)
{
 if(!ed->show_about)return;
 const struct ui_theme *theme=&themes[ed->theme];
 cui_color bg=theme_color(theme->panel),fg=theme_color(theme->text),accent=theme_color(theme->accent),selected=theme_color(theme->selected),clear={0,0,0,0};
 cui_rect r=about_rect(ed);
 ui_box(chrome,"about-scrim",0,0,ed->win_w,ed->win_h,(cui_color){0,0,0,180},NULL,fg,13);
 ui_box(chrome,"about-card",r.x,r.y,r.w,r.h,bg,NULL,fg,13);
 about_box(ed,"about-hero",16,16,528,218,selected,NULL,fg,13);
 about_box(ed,"about-eyebrow",112,30,336,24,clear,"CAPTURE A LITTLE. SAY A LOT.",accent,10);
 about_box(ed,"about-logo",180,64,200,88,clear,"s2",accent,84);
 about_box(ed,"about-name",60,154,440,36,clear,"Super Screenshot",fg,26);
 about_box(ed,"about-tagline",32,198,496,24,clear,"Small app. Big screenshot energy.",fg,13);
 about_box(ed,"about-close",494,24,34,34,bg,"×",fg,21);
 about_box(ed,"about-capture",32,252,150,28,selected,"CAPTURE",accent,11);
 about_box(ed,"about-annotate",192,252,150,28,selected,"ANNOTATE",accent,11);
 about_box(ed,"about-personalize",352,252,176,28,selected,"MAKE IT YOURS",accent,11);
 int link_hot=ed->about_focus==1 || about_hit(ed,ed->pointer_x,ed->pointer_y)==1;
 about_box(ed,"about-repo",32,300,496,54,link_hot?selected:bg,"github.com/mattmezza/s2  ↗",accent,18);
 about_box(ed,"about-credit",32,374,496,28,clear,"Made by Matteo Merola",fg,18);
 char license[128];snprintf(license,sizeof license,"© 2026 Matteo Merola  ·  MIT License  ·  v%s",VERSION);
 about_box(ed,"about-license",32,408,496,20,clear,license,fg,12);
 about_box(ed,"about-back",32,448,496,36,ed->about_focus==2?selected:bg,"Back to my masterpiece  →",accent,13);
}
/* Launch the literal repository URL without invoking a shell. A detached child
 * avoids holding up the editor or leaving browser launcher zombies behind. */
static void open_about_repo(struct editor_state *ed)
{
 pid_t child=fork();if(child<0)return;
 if(child==0) {
     pid_t browser=fork();if(browser<0)_exit(127);if(browser>0)_exit(0);
     if(ed->dpy)close(ConnectionNumber(ed->dpy));
     int nullfd=open("/dev/null",O_RDWR);
     if(nullfd>=0){dup2(nullfd,STDIN_FILENO);dup2(nullfd,STDOUT_FILENO);dup2(nullfd,STDERR_FILENO);if(nullfd>2)close(nullfd);}
     execlp("xdg-open","xdg-open","https://github.com/mattmezza/s2",(char *)NULL);_exit(127);
 }
 while(waitpid(child,NULL,0)<0 && errno==EINTR){}
}
static void draw_footer_symbol(cairo_t *cr,double x,double y,int info,cui_color color)
{
 cairo_save(cr);cairo_translate(cr,x,y);cairo_set_source_rgb(cr,color.r/255.,color.g/255.,color.b/255.);
 cairo_set_line_width(cr,1.65);cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);cairo_set_line_join(cr,CAIRO_LINE_JOIN_ROUND);
 cairo_arc(cr,0,0,8.5,0,2*M_PI);cairo_stroke(cr);
 if(info){cairo_arc(cr,0,-4,1,0,2*M_PI);cairo_fill(cr);cairo_move_to(cr,0,-.5);cairo_line_to(cr,0,4.5);cairo_stroke(cr);}
 else {cairo_move_to(cr,-2.7,-3.2);cairo_curve_to(cr,-2.3,-6.5,3.9,-5.7,3.1,-2.1);cairo_curve_to(cr,2.7,-.4,0,-.6,0,1.6);cairo_stroke(cr);cairo_arc(cr,0,4.7,.9,0,2*M_PI);cairo_fill(cr);}
 cairo_restore(cr);
}
static void draw_about_decoration(cairo_t *cr,const struct editor_state *ed)
{
 if(!ed->show_about)return;
 cui_rect r=about_rect(ed);double s=r.w/560.;unsigned int rgb=themes[ed->theme].accent;
 cairo_save(cr);cairo_translate(cr,r.x,r.y);cairo_scale(cr,s,s);
 cairo_set_source_rgba(cr,((rgb>>16)&255)/255.,((rgb>>8)&255)/255.,(rgb&255)/255.,.7);
 cairo_set_line_width(cr,3);cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
 /* Cropping corners frame the logo; scattered marks keep the card playful. */
 for(int i=0;i<4;i++){
     double x=i&1?380:180,y=i&2?148:66,dx=i&1?-16:16,dy=i&2?-16:16;
     cairo_move_to(cr,x,y+dy);cairo_line_to(cr,x,y);cairo_line_to(cr,x+dx,y);cairo_stroke(cr);
 }
 const double stars[][3]={{92,116,7},{442,166,6},{416,90,4},{128,178,3}};
 for(size_t i=0;i<4;i++){
     double x=stars[i][0],y=stars[i][1],d=stars[i][2];
     cairo_move_to(cr,x-d,y);cairo_line_to(cr,x+d,y);cairo_move_to(cr,x,y-d);cairo_line_to(cr,x,y+d);cairo_stroke(cr);
 }
 cairo_restore(cr);
}

static double monotonic_seconds(void)
{
 struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
 return now.tv_sec+now.tv_nsec/1e9;
}
static void copy_chrome(struct editor_state *ed,Drawable source,Drawable target)
{
 int left=toolbar_left(ed),width=lround((toolbar_rows(ed)==2?520:TOOLBAR_WIDTH)*chrome_scale(ed));
 XCopyArea(ed->dpy,source,target,ed->gc,left-1,TOOLBAR_Y-1,width+2,lround(TOOLBAR_HEIGHT*toolbar_rows(ed)*chrome_scale(ed))+2,left-1,TOOLBAR_Y-1);
 int fx=footer_left(ed),fy=ed->win_h-footer_height(ed)-16;
 XCopyArea(ed->dpy,source,target,ed->gc,fx-1,fy-1,footer_width(ed)+2,footer_height(ed)+2,fx-1,fy-1);
}

static void paint_chrome_icons(cairo_t *cr,struct editor_state *ed,int left,float s,int fx,int fw,int fy)
{
	for (int i = 0; i < tool_count(); i++)
		tool_icon(cr, i, left + (18 + i * 38) * s, TOOLBAR_Y + 20 * s,
			  24 * s, tool_by_index(i) == ed->tool ? theme_color(chrome_theme->accent) : theme_color(chrome_theme->text));
 for(int filled=0;filled<2;filled++) {
     int enabled=filled?ed->fill_mode:ed->outline_mode;
     cui_color color=theme_color(enabled?chrome_theme->accent:chrome_theme->text);
     cairo_save(cr);cairo_set_source_rgb(cr,color.r/255.,color.g/255.,color.b/255.);
     double ix=left+((filled?775.5:742.5)-108-6-(toolbar_rows(ed)==2?504:0))*s;
     double iy=TOOLBAR_Y+(26+(toolbar_rows(ed)==2?64:0))*s;
     cairo_set_line_width(cr,1.6*s);
     if(filled && ed->gradient_mode) {
         cairo_rectangle(cr,ix,iy,6*s,12*s);cairo_fill(cr);
         for(int row=0;row<4;row++)for(int col=0;col<2;col++) {
             cairo_arc(cr,ix+(8+col*3)*s,iy+(1.5+row*3)*s,(col?.55:.85)*s,0,2*M_PI);cairo_fill(cr);
         }
         cairo_rectangle(cr,ix,iy,12*s,12*s);cairo_stroke(cr);
     } else {
         cairo_rectangle(cr,ix,iy,12*s,12*s);
         if(filled)cairo_fill(cr);else cairo_stroke(cr);
     }
     if(!filled && !ed->outline_mode) {
         cairo_move_to(cr,ix-2*s,iy+14*s);cairo_line_to(cr,ix+14*s,iy-2*s);
         cairo_set_line_width(cr,4*s);
         /* A panel-colored halo keeps the diagonal distinct from the square. */
         cairo_set_source_rgb(cr,((chrome_theme->panel>>16)&255)/255.,((chrome_theme->panel>>8)&255)/255.,(chrome_theme->panel&255)/255.);
         cairo_stroke_preserve(cr);cairo_set_source_rgb(cr,color.r/255.,color.g/255.,color.b/255.);
         cairo_set_line_width(cr,1.8*s);cairo_stroke(cr);
     }
     cairo_restore(cr);
 }

 draw_footer_symbol(cr,fx+fw-64,fy+22,0,theme_color(chrome_theme->text));
 draw_footer_symbol(cr,fx+fw-28,fy+22,1,theme_color(chrome_theme->text));
}

static void draw_status(struct editor_state *ed)
{
	chrome_theme=&themes[ed->theme];
 double now=monotonic_seconds();
 if(!ed->motion_only || now-ed->chrome_time>.1) {
     ed->chrome_cursor_x=ed->cursor_x;ed->chrome_cursor_y=ed->cursor_y;ed->chrome_time=now;
 }
 char key[768];
 snprintf(key,sizeof key,"%d %d %d %d %u %d %d %d %d %.5f %d %d %d %d %d %d %d %d %s %s",
     ed->win_w,ed->win_h,ed->tool,tool_size(ed),ed->color,ed->fill_mode,ed->theme,
     ed->sel_len,ed->number_next,ed->scale,ed->chrome_hover,ed->text_mode,ed->color_mode,
     ed->chrome_cursor_x,ed->chrome_cursor_y,ed->selected_idx,ed->gradient_mode,ed->outline_mode,ed->text_buf,ed->color_buf);
 int cacheable=!ed->popup && !(ed->show_help || ed->show_about) && ed->chrome_hover<0;
 if(cacheable && ed->chromebuf && !strcmp(key,ed->chrome_key)) {
     copy_chrome(ed,ed->chromebuf,ed->backbuf);return;
 }
 ed->chrome_key[0]=0;
 cui_color panel = {24, 26, 32, 255}, fg = {170, 177, 194, 255},
		  accent = {179, 202, 255, 255}, active = {48, 61, 88, 255},
		  hover = {39, 44, 56, 255};
	float s = chrome_scale(ed);
 chrome_label_count=0;
	int left = toolbar_left(ed), fw = footer_width(ed),
	    fx = footer_left(ed), fy = ed->win_h - footer_height(ed) - 16;
	int i;
	size_t j;
	char msg[256];
	if (!chrome)
		chrome = cui_create((cui_config){.max_elements = 256,
						 .max_commands = 1024,
						 .frame_bytes = 65536,
						 .measure = chrome_measure});
	if (!chrome || !cui_begin_frame(chrome, ed->win_w, ed->win_h, 0))
		return;
	cui_begin_box(chrome, cui_id_from(chrome, "root"),
		      (cui_box){.width = ed->win_w, .height = ed->win_h});
	top_box(ed, "toolbar", 0, 0, TOOLBAR_WIDTH, 64, panel, NULL, fg);

	for (i = 0; i < tool_count(); i++) {
		char id[24];
		snprintf(id, sizeof id, "tool%d", i);
		top_box(ed, id, 120 + i * 38, 10, 36, 44,
			tool_by_index(i) == ed->tool
			    ? active
			    : (ed->toolbar_hover == i ? hover : panel),
			NULL, fg);
	}
	top_box(ed, "color", 624, 10, 32, 44,
		ed->chrome_hover == 13 ? hover : panel, "●",
		(cui_color){(ed->color >> 16) & 255, (ed->color >> 8) & 255,
			    ed->color & 255, 255});
	snprintf(msg, sizeof msg, dimension_is_percentage(ed) ? "%d%%" : "%dpx",
		 tool_size(ed));
	top_box(ed, "size", 662, 10, 54, 44,
		ed->chrome_hover == 14 ? hover : panel, msg, fg);
 top_box(ed,"fill-track",724,14,70,36,(cui_color){15,18,24,255},NULL,fg);
 top_box(ed,"outline-segment",727,17,31,30,ed->outline_mode?active:panel,NULL,ed->outline_mode?accent:fg);
 top_box(ed,"fill-segment",760,17,31,30,ed->fill_mode?active:panel,NULL,ed->fill_mode?accent:fg);
 top_box(ed,"actions-divider",798,20,1,24,(cui_color){65,72,89,255},NULL,fg);
 cui_color save_bg=ed->chrome_hover==15?hover:(cui_color){37,41,51,255};
 cui_color copy_bg=ed->chrome_hover==16?(cui_color){199,216,255,255}:accent;
 top_box(ed,"save",804,10,110,44,save_bg,NULL,fg);
 top_box(ed,"save-label",816,21,34,22,(cui_color){0,0,0,0},"Save",fg);
 top_box(ed,"save-shortcut",860,20,44,24,panel,"Ctrl S",fg);
 chrome_labels[chrome_label_count-1].size=10*s;
 top_box(ed,"copy",924,10,186,44,copy_bg,NULL,fg);
 top_box(ed,"copy-label",936,21,94,22,(cui_color){0,0,0,0},"Copy & Close",(cui_color){27,42,70,255});
 top_box(ed,"copy-shortcut",1046,20,52,24,(cui_color){255,255,255,42},"Ctrl C",(cui_color){27,42,70,255});
 chrome_labels[chrome_label_count-1].size=10*s;
	ui_box(chrome, "status", fx, fy, fw, footer_height(ed), panel, NULL, fg,
	       13);
	snprintf(msg, sizeof msg,
		 "PNG   %d × %d   ·   X %d  Y %d   ·   %s   ·   %d selected",
		 ed->img->width, ed->img->height, ed->chrome_cursor_x, ed->chrome_cursor_y,
		 tool_label(ed->tool), ed->sel_len);
	if (fw < 800 && footer_height(ed)==44)
		snprintf(msg, sizeof msg, "PNG  %d × %d  ·  %s  ·  %d selected",
			 ed->img->width, ed->img->height, tool_label(ed->tool),
			 ed->sel_len);
 if(ed->tool==TOOL_NUMBER)snprintf(msg,sizeof msg,"Number %d next  ·  %d px  ·  Click Number to reset",ed->number_next,tool_size(ed));
 if(ed->selected_idx>=0 && (size_t)ed->selected_idx<ed->actions.cursor && ed->actions.items[ed->selected_idx].type==ACTION_TEXT) {
     const struct action *text=&ed->actions.items[ed->selected_idx];
     snprintf(msg,sizeof msg,"Text %dpx · Weight %d%s%s%s · b/B p/P c/C",abs(text->p0)*TEXT_SCALE_PX,
         text->font_weight?text->font_weight:400,text->italic?" · Italic":"",
         text->underline?" · Underline":"",text->strikethrough?" · Strike":"");
 }
 if(ed->tool==TOOL_SELECT && ed->selected_idx>=0 && (size_t)ed->selected_idx<ed->actions.cursor) {
     const struct action *a=&ed->actions.items[ed->selected_idx];
     static const char *ends[]={"Open","Filled","Outline","Diamond","Dot","Bar"};
     if(a->type==ACTION_ARROW)snprintf(msg,sizeof msg,"%s ends · e style · < > ends · drag diamond to curve",ends[a->edge_style>=0 && a->edge_style<6?a->edge_style:0]);
     else if(a->type==ACTION_LINE)snprintf(msg,sizeof msg,"Drag diamond to curve · Shift-drag to straighten");
 }
	if (ed->color_mode)
		snprintf(msg, sizeof msg,
			 "Color #%s  ·  Enter to apply  ·  Esc to cancel",
			 ed->color_buf);
	else if (ed->text_mode)
		snprintf(msg, sizeof msg, "Text: %.28s  ·  Enter to place",
			 ed->text_buf);
	ui_box(chrome,"theme-button",fx+8,fy+6,fw<450?76:96,32,ed->chrome_hover==24?hover:panel,themes[ed->theme].name,fg,12);
 int stacked=footer_height(ed)>44;
 ui_box(chrome, "metadata", stacked?fx+8:fx+110, stacked?fy+44:fy,
        stacked?fw-16:fw-400, FOOTER_HEIGHT, panel, msg, fg, 13);
 if(stacked) {
     int width; pango_layout_get_pixel_size(chrome_layout(cui_string(msg),13),&width,NULL);
     if(width>fw-32) chrome_labels[chrome_label_count-1].size=13.f*(fw-32)/width;
 }
	ui_box(chrome, "zoom-out", fx + fw - 282, fy + 6, 32, 32,
	       ed->chrome_hover == 17 ? hover : panel, "−", fg, 17);
	snprintf(msg, sizeof msg, "%.0f%%", ed->scale * 100);
	ui_box(chrome, "zoom-value", fx + fw - 246, fy + 6, 66, 32, panel, msg,
	       fg, 13);
	ui_box(chrome, "zoom-in", fx + fw - 176, fy + 6, 32, 32,
	       ed->chrome_hover == 18 ? hover : panel, "+", fg, 17);
	ui_box(chrome, "zoom-fit", fx + fw - 136, fy + 6, 48, 32,
	       ed->chrome_hover == 19 ? hover : panel, "Fit", fg, 13);
	ui_box(chrome, "help", fx + fw - 80, fy + 6, 32, 32,
	       ed->chrome_hover == 20 ? hover : panel, NULL, fg, 13);
 ui_box(chrome,"about",fx+fw-44,fy+6,32,32,ed->chrome_hover==25?hover:panel,NULL,fg,13);
	if (!ed->popup && ed->toolbar_hover >= 0 && ed->toolbar_hover < tool_count()) {
		static const char *tips[] = {
		    "Select · S",      "Arrow · A",    "Line · L",
		    "Pen · P",	       "Number · N",   "Rectangle · R",
		    "Circle · O",      "Text · T",     "Highlight · H",
		    "Marker · M",      "Pixelate · X", "Blur · B",
		    "Color picker · C"};
		ui_box(chrome, "tooltip",
		       fminf(ed->win_w-158,fmaxf(12,left + (30 + ed->toolbar_hover * 38) * s-73)),
		       TOOLBAR_Y + (64*toolbar_rows(ed)+10) * s, 146, 34, panel,
		       tips[ed->toolbar_hover], fg, 13);
	}
 if(!ed->popup && !(ed->show_help || ed->show_about) && (ed->chrome_hover==21 || ed->chrome_hover==23))ui_box(chrome,"fill-tooltip",left+(759-108)*s-70,TOOLBAR_Y+(64*toolbar_rows(ed)+10)*s,140,34,panel,ed->chrome_hover==21?(ed->outline_mode?"Outline on · Shift F":"Outline off · Shift F"):(ed->gradient_mode?"Gradient fill · f / g":"Flat fill · f / g"),fg,13);
 if(!ed->show_about && !ed->show_help && !ed->popup && (ed->chrome_hover==20 || ed->chrome_hover==25))
     ui_box(chrome,"footer-tip",fminf(ed->win_w-180,fx+fw-170),fy-42,164,32,panel,ed->chrome_hover==20?"Shortcuts · ?":"About s2",fg,12);
	draw_popup(ed,panel,fg,accent,active);
 size_t modal_label_start=chrome_label_count;
 compose_help(ed);compose_about(ed);
	cui_end_box(chrome);
	cui_draw_list list = cui_end_frame(chrome);
	cairo_surface_t *surface = cairo_xlib_surface_create(
	    ed->dpy, ed->backbuf, DefaultVisual(ed->dpy, ed->screen), ed->win_w,
	    ed->win_h);
	cairo_t *cr = cairo_create(surface);
 int chrome_painted=0;
	for (j = 0; j < list.count; j++) {
		const cui_draw_command *c = &list.commands[j];
        if(!chrome_painted && (ed->show_help || ed->show_about) && c->type==CUI_DRAW_RECT &&
           c->bounds.x==0 && c->bounds.y==0 && c->bounds.w==ed->win_w && c->bounds.h==ed->win_h && c->color.a<255) {
            for(size_t li=0;li<modal_label_start;li++)paint_chrome_label(cr,&chrome_labels[li]);
            paint_chrome_icons(cr,ed,left,s,fx,fw,fy);chrome_painted=1;
        }

		cairo_set_source_rgba(cr, c->color.r / 255., c->color.g / 255.,
				      c->color.b / 255., c->color.a / 255.);
		if (c->type == CUI_DRAW_RECT || c->type == CUI_DRAW_BORDER) {
			double x = c->bounds.x, y = c->bounds.y,
			       w = c->bounds.w, h = c->bounds.h, r = fmin(c->radius,fmin(c->bounds.w,c->bounds.h)/2);
			cairo_new_sub_path(cr);
			cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
			cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
			cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
			cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
			cairo_close_path(cr);
			if (c->type == CUI_DRAW_BORDER) {
				cairo_set_line_width(
				    cr, c->stroke > 0 ? c->stroke : 1);
				cairo_stroke(cr);
			} else {
                unsigned int rgb=(c->color.r<<16)|(c->color.g<<8)|c->color.b;
                if(rgb==chrome_theme->panel && c->color.a==255) {
                    double gy=c->bounds.y<toolbar_bottom(ed)?TOOLBAR_Y:fy;
                    double gh=gy==TOOLBAR_Y?TOOLBAR_HEIGHT*toolbar_rows(ed):footer_height(ed);
                    gradient_source_strength(cr,rgb,gy==TOOLBAR_Y?left:fx,gy,(gy==TOOLBAR_Y?left+(toolbar_rows(ed)==2?520:TOOLBAR_WIDTH)*s:fx+fw),gy+gh,1.,.4);
                }
                cairo_fill(cr);
            }
		} else if (c->type == CUI_DRAW_TEXT) {
			cairo_move_to(cr, c->bounds.x, c->bounds.y);
			pango_cairo_show_layout(
			    cr, chrome_layout(c->text, c->font_size));
		}
	}
 for(size_t li=chrome_painted?modal_label_start:0;li<chrome_label_count;li++)paint_chrome_label(cr,&chrome_labels[li]);
 if(!chrome_painted)paint_chrome_icons(cr,ed,left,s,fx,fw,fy);
 draw_about_decoration(cr,ed);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
 if(cacheable) {
     if(!ed->chromebuf)ed->chromebuf=XCreatePixmap(ed->dpy,ed->win,ed->win_w,ed->win_h,DefaultDepth(ed->dpy,ed->screen));
     copy_chrome(ed,ed->backbuf,ed->chromebuf);
     snprintf(ed->chrome_key,sizeof ed->chrome_key,"%s",key);
 }
}

static void chrome_cleanup(void)
{
	unsigned i;
	if (chrome) {
		cui_destroy(chrome);
		chrome = NULL;
	}
	for (i = 0; i < 192; i++) {
		if (chrome_layouts[i])
			g_object_unref(chrome_layouts[i]);
		chrome_layouts[i] = NULL;
		free(chrome_strings[i]);
		chrome_strings[i] = NULL;
	}
	for (i = 0; i < 8; i++) {
		if (chrome_icons[i])
			cairo_surface_destroy(chrome_icons[i]);
		chrome_icons[i] = NULL;
	}
	chrome_next = 0;
}


static void
selection_geometry(struct editor_state *ed, const struct action *a, int *corners, int *handle)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	double cx;
	double cy;
	double rad;
	double cosr;
	double sinr;
	double sxoff;
	double syoff;
	double tmx;
	double tmy;
	double upx;
	double upy;
	int i;
	const double bx[4] = { 0, 1, 1, 0 };
	const double by[4] = { 0, 0, 1, 1 };

	action_bounds(ed, a, &minx, &miny, &maxx, &maxy);
	if (ed->drag_active) {
		minx += ed->drag_dx;
		maxx += ed->drag_dx;
		miny += ed->drag_dy;
		maxy += ed->drag_dy;
	}
	cx = (minx + maxx) / 2.0;
	cy = (miny + maxy) / 2.0;
	rad = a->rotation * M_PI / 180.0;
	cosr = cos(rad);
	sinr = sin(rad);
	sxoff = ed->canvas_x;
	syoff = ed->canvas_y;

	for (i = 0; i < 4; i++) {
		double px = minx + bx[i] * (maxx - minx);
		double py = miny + by[i] * (maxy - miny);
		double rx = cosr * (px - cx) - sinr * (py - cy) + cx;
		double ry = sinr * (px - cx) + cosr * (py - cy) + cy;
		corners[i * 2 + 0] = (int)lround(sxoff + rx * ed->scale);
		corners[i * 2 + 1] = (int)lround(syoff + ry * ed->scale);
	}
	/* handle sits beyond the rotated top edge midpoint */
	tmx = (corners[0] + corners[2]) / 2.0;
	tmy = (corners[1] + corners[3]) / 2.0;
	upx = sinr;
	upy = -cosr;
	handle[0] = (int)lround(tmx + upx * ROTATE_HANDLE_OFFSET);
	handle[1] = (int)lround(tmy + upy * ROTATE_HANDLE_OFFSET);
}

static struct action *selected_connector(struct editor_state *ed)
{
 if(ed->tool!=TOOL_SELECT || ed->sel_len!=1 || ed->selected_idx<0 || (size_t)ed->selected_idx>=ed->actions.cursor)return NULL;
 struct action *a=&ed->actions.items[ed->selected_idx];
 return a->type==ACTION_LINE || a->type==ACTION_ARROW?a:NULL;
}
static int curve_handle_position(struct editor_state *ed,int *x,int *y)
{
 struct action *a=selected_connector(ed);if(!a)return 0;
 double px,py,cx,cy;connector_point(a,.5,&px,&py);action_center(ed,a,&cx,&cy);
 double r=a->rotation*M_PI/180.,c=cos(r),s=sin(r);
 *x=lround(ed->canvas_x+(cx+c*(px-cx)-s*(py-cy))*ed->scale);
 *y=lround(ed->canvas_y+(cy+s*(px-cx)+c*(py-cy))*ed->scale);
 return 1;
}
static int curve_handle_hit(struct editor_state *ed,int x,int y)
{
 int hx,hy;if(!curve_handle_position(ed,&hx,&hy))return 0;
 return (x-hx)*(x-hx)+(y-hy)*(y-hy)<=100;
}
static void curve_to_pointer(struct editor_state *ed,int x,int y,unsigned state)
{
 struct action *a=selected_connector(ed);if(!a || ed->scale<=0)return;
 double cx,cy;action_center(ed,a,&cx,&cy);
 double px=(x-ed->canvas_x)/ed->scale-cx,py=(y-ed->canvas_y)/ed->scale-cy;
 double r=-a->rotation*M_PI/180.,rx=cx+cos(r)*px-sin(r)*py,ry=cy+sin(r)*px+cos(r)*py;
 double dx=a->x1-a->x0,dy=a->y1-a->y0,len=dx*dx+dy*dy;
 if(len<1)return;
 double bend=2*((rx-(a->x0+a->x1)/2.)*(-dy)+(ry-(a->y0+a->y1)/2.)*dx)/len;
 if(state&ShiftMask)bend=0; /* easy straightening */
 a->bend=fmax(-2,fmin(2,bend));
}
static int style_selected_arrow(struct editor_state *ed,KeySym sym)
{
 struct action *a=selected_connector(ed);if(!a || a->type!=ACTION_ARROW)return 0;
 if(sym==XK_less)a->edges^=1;
 else if(sym==XK_greater)a->edges^=2;
 else if(sym==XK_e || sym==XK_E) {
     size_t count=sizeof arrow_edge_cycle/sizeof arrow_edge_cycle[0],i;
     for(i=0;i<count;i++)if(arrow_edge_cycle[i]==a->edge_style)break;
     a->edge_style=arrow_edge_cycle[i<count?(i+count+(sym==XK_E?-1:1))%count:(sym==XK_E?count-1:0)];
 } else return 0;
 ed->dirty=ed->raster_dirty=1;return 1;
}

static int
rotation_handle_hit(struct editor_state *ed, int sx, int sy)
{
	int corners[8];
	int handle[2];
	int dx;
	int dy;

	if (ed->selected_idx < 0 || (size_t)ed->selected_idx >= ed->actions.cursor) {
		return 0;
	}
	if (!action_is_rotatable(ed->actions.items[ed->selected_idx].type)) {
		return 0;
	}
	selection_geometry(ed, &ed->actions.items[ed->selected_idx], corners,
			   handle);
	dx = sx - handle[0];
	dy = sy - handle[1];
	return dx * dx + dy * dy <=
	       (ROTATE_HANDLE_RADIUS + 3) * (ROTATE_HANDLE_RADIUS + 3);
}

static int action_is_resizable(enum action_type t)
{
 return t==ACTION_RECT || t==ACTION_CIRCLE || t==ACTION_LINE ||
        t==ACTION_ARROW || t==ACTION_HIGHLIGHT || t==ACTION_BLUR ||
        t==ACTION_PIXELATE || t==ACTION_TEXT || t==ACTION_NUMBER;
}
static int resize_handle_hit(struct editor_state *ed, int x, int y)
{
 if(ed->tool!=TOOL_SELECT || ed->sel_len!=1 || ed->selected_idx<0 ||
    (size_t)ed->selected_idx>=ed->actions.cursor)return -1;
 struct action *a=&ed->actions.items[ed->selected_idx];
 if(!action_is_resizable(a->type))return -1;
 int corners[8],handle[2];selection_geometry(ed,a,corners,handle);
 for(int i=0;i<4;i++)if(abs(x-corners[i*2])<=7 && abs(y-corners[i*2+1])<=7)return i;
 return -1;
}

/* Repaint just the transformed object's tile over a cached capture. */
static void draw_transform_preview(struct editor_state *ed)
{
 struct action pending={0};
 const struct action *a;
 if(ed->transform_preview && ed->selected_idx>=0 && (size_t)ed->selected_idx<ed->actions.cursor)a=&ed->actions.items[ed->selected_idx];
 else if(ed->text_mode && ed->text_len>0) {
     if(ed->text_edit_index>=0)pending=ed->text_edit_style;
     if(ed->text_edit_index<0){pending.font_weight=S2_DEFAULT_TEXT_WEIGHT;pending.no_gradient=!ed->gradient_mode;}
     pending.type=ACTION_TEXT;pending.x0=ed->text_x;pending.y0=ed->text_y;
     pending.p0=ed->fill_mode?-ed->text_scale:ed->text_scale;
     pending.color=ed->color;pending.text=ed->text_buf;a=&pending;
 } else return;
 int x0,y0,x1,y1;action_bounds(ed,a,&x0,&y0,&x1,&y1);
 double cx=(x0+x1)/2.,cy=(y0+y1)/2.,rad=a->rotation*M_PI/180.;
 double c=cos(rad),s=sin(rad),left=1e9,top=1e9,right=-1e9,bottom=-1e9;
 for(int i=0;i<4;i++) {
     double x=(i&1?x1:x0)-cx,y=(i&2?y1:y0)-cy;
     double rx=c*x-s*y+cx,ry=s*x+c*y+cy;
     left=fmin(left,rx);right=fmax(right,rx);top=fmin(top,ry);bottom=fmax(bottom,ry);
 }
 int pad=action_draw_pad(a)+4;
 int tx=fmax(0,floor(left)-pad),ty=fmax(0,floor(top)-pad);
 int width=fmin(ed->img->width,ceil(right)+pad)-tx+1;
 int height=fmin(ed->img->height,ceil(bottom)+pad)-ty+1;
 if(width<=0 || height<=0 || (size_t)width*height>64u*1024u*1024u)return;
 struct image tile={.width=width,.height=height,.len=(size_t)width*height*4};
 tile.pixels=calloc(1,tile.len);if(!tile.pixels)return;
 struct action shifted=*a;
 if(a->pen_points && a->pen_len) {
     shifted.pen_points=malloc((size_t)a->pen_len*2*sizeof(int));
     if(!shifted.pen_points) {free(tile.pixels);return;}
     memcpy(shifted.pen_points,a->pen_points,(size_t)a->pen_len*2*sizeof(int));
 }
 action_move_by(&shifted,-tx,-ty);
 struct editor_state preview_ed=*ed;preview_ed.show_text_selection=ed->text_mode;
 apply_action(&preview_ed,&tile,&shifted);
 if(shifted.pen_points!=a->pen_points)free(shifted.pen_points);
 cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);
 unsigned char *data=cairo_image_surface_get_data(surface);int stride=cairo_image_surface_get_stride(surface);
 for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
     unsigned char *p=tile.pixels+((size_t)y*width+x)*4;
     uint32_t argb=((uint32_t)p[3]<<24)|((p[0]*p[3]/255)<<16)|((p[1]*p[3]/255)<<8)|(p[2]*p[3]/255);
     memcpy(data+y*stride+x*4,&argb,4);
 }
 cairo_surface_mark_dirty(surface);
 cairo_surface_t *target=cairo_xlib_surface_create(ed->dpy,ed->backbuf,DefaultVisual(ed->dpy,ed->screen),ed->win_w,ed->win_h);
 cairo_t *cr=cairo_create(target);
 cairo_rectangle(cr,ed->canvas_x,ed->canvas_y,ed->canvas_w,ed->canvas_h);cairo_clip(cr);
 cairo_translate(cr,ed->canvas_x+tx*ed->scale,ed->canvas_y+ty*ed->scale);cairo_scale(cr,ed->scale,ed->scale);
 cairo_set_source_surface(cr,surface,0,0);cairo_paint(cr);
 cairo_destroy(cr);cairo_surface_destroy(target);cairo_surface_destroy(surface);free(tile.pixels);
}

/* Region effects preview at display resolution; exports retain capture resolution. */
static void draw_effect_preview(struct editor_state *ed,cairo_t *cr)
{
 int x0=fmin(ed->anchor_x,ed->cursor_x),y0=fmin(ed->anchor_y,ed->cursor_y);
 int x1=fmax(ed->anchor_x,ed->cursor_x),y1=fmax(ed->anchor_y,ed->cursor_y);
 int width=fmax(1,ceil((x1-x0+1)*ed->scale)),height=fmax(1,ceil((y1-y0+1)*ed->scale));
 struct image tile={.width=width,.height=height,.len=(size_t)width*height*4};
 tile.pixels=malloc(tile.len);if(!tile.pixels)return;
 for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
     int sx=x0+floor(x/ed->scale),sy=y0+floor(y/ed->scale);
     if(sx>ed->img->width-1)sx=ed->img->width-1;
     if(sy>ed->img->height-1)sy=ed->img->height-1;
     memcpy(tile.pixels+((size_t)y*width+x)*4,ed->rendered.pixels+((size_t)sy*ed->img->width+sx)*4,4);
 }
 if(ed->tool==TOOL_BLUR)draw_blur(&tile,0,0,width-1,height-1,fmax(1,lround(ed->blur_radius*ed->scale)));
 else draw_pixelate(&tile,0,0,width-1,height-1,fmax(1,lround(ed->pixelate_block*ed->scale)));
 cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);
 unsigned char *data=cairo_image_surface_get_data(surface);int stride=cairo_image_surface_get_stride(surface);
 for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
     unsigned char *p=tile.pixels+((size_t)y*width+x)*4;
     uint32_t argb=0xff000000u|((uint32_t)p[0]<<16)|((uint32_t)p[1]<<8)|p[2];
     memcpy(data+y*stride+x*4,&argb,4);
 }
 cairo_surface_mark_dirty(surface);
 cairo_set_source_surface(cr,surface,ed->canvas_x+x0*ed->scale,ed->canvas_y+y0*ed->scale);cairo_paint(cr);
 cairo_surface_destroy(surface);free(tile.pixels);
}

static void draw_text_caret(struct editor_state *ed)
{
 if(!ed->text_mode)return;
 struct action a=ed->text_edit_index>=0?ed->text_edit_style:(struct action){0};
 if(ed->text_edit_index<0)a.font_weight=S2_DEFAULT_TEXT_WEIGHT;
 a.type=ACTION_TEXT;a.x0=ed->text_x;a.y0=ed->text_y;a.text=ed->text_buf;
 a.p0=ed->fill_mode?-ed->text_scale:ed->text_scale;
 cairo_surface_t *surface=cairo_xlib_surface_create(ed->dpy,ed->backbuf,DefaultVisual(ed->dpy,ed->screen),ed->win_w,ed->win_h);
 cairo_t *cr=cairo_create(surface);text_render_options(cr);
 PangoLayout *layout=pango_cairo_create_layout(cr);char family[192];build_font_css_family(family,sizeof family);
 PangoFontDescription *desc=pango_font_description_from_string(family);
 pango_font_description_set_absolute_size(desc,ed->text_scale*TEXT_SCALE_PX*PANGO_SCALE);
 struct editor_state styled=*ed;styled.text_style=&a;styled.show_text_selection=0;
 style_text_layout(layout,desc,&styled);pango_layout_set_font_description(layout,desc);
 pango_layout_set_text(layout,ed->text_buf,-1);
 PangoRectangle ink,strong;pango_layout_get_pixel_extents(layout,&ink,NULL);
 pango_layout_get_cursor_pos(layout,ed->text_cursor,&strong,NULL);
 double x=ed->text_x+TEXT_RENDER_PAD-ink.x+(double)strong.x/PANGO_SCALE;
 double y=ed->text_y+TEXT_RENDER_PAD-ink.y+(double)strong.y/PANGO_SCALE;
 double h=fmax(1,(double)strong.height/PANGO_SCALE);
 cairo_rectangle(cr,ed->canvas_x,ed->canvas_y,ed->canvas_w,ed->canvas_h);cairo_clip(cr);
 cairo_translate(cr,ed->canvas_x,ed->canvas_y);cairo_scale(cr,ed->scale,ed->scale);
 if(a.rotation) {double cx,cy;action_center(ed,&a,&cx,&cy);cairo_translate(cr,cx,cy);cairo_rotate(cr,a.rotation*M_PI/180.);cairo_translate(cr,-cx,-cy);}
 cairo_move_to(cr,x,y);cairo_line_to(cr,x,y+h);
 cairo_set_source_rgba(cr,0,0,0,.8);cairo_set_line_width(cr,3/ed->scale);cairo_stroke_preserve(cr);
 cairo_set_source_rgb(cr,1,1,1);cairo_set_line_width(cr,1/ed->scale);cairo_stroke(cr);
 pango_font_description_free(desc);g_object_unref(layout);cairo_destroy(cr);cairo_surface_destroy(surface);
}

static void render_frame(struct editor_state *ed)
{
 update_cursor(ed);
 double now=monotonic_seconds();
 if(ed->motion_only && !ed->dirty && !ed->raster_dirty && !ed->anchor_active &&
    !ed->pen_active && !ed->marquee_active && !ed->drag_active &&
    !ed->rotating && !ed->resizing && !ed->curving && ed->tool!=TOOL_MARKER && !ed->popup && !(ed->show_help || ed->show_about) &&
    ed->chrome_hover<0 && ed->chrome_key[0]) {
     if(now-ed->idle_time<.1) {XFlush(ed->dpy);return;}
     ed->idle_time=now;
 }
	struct timespec frame_start, frame_end;
	if (ed->profile)
		clock_gettime(CLOCK_MONOTONIC, &frame_start);
	if (ed->dirty) {
		rerender_from_actions(ed);
		ed->raster_dirty = 1;
	}
	if (!ed->ximg || !ed->ximg->data) {
		return;
	}

	int vector_preview =
	    ed->pen_active || ed->anchor_active;
	int raster_preview = 0;
	if (raster_preview)
		img_copy(&ed->preview, &ed->rendered);
	if (raster_preview)
		ed->raster_dirty = 1;

	if (ed->anchor_active && !vector_preview) {
		struct action a;
		memset(&a, 0, sizeof(a));
		a.x0 = ed->anchor_x;
		a.y0 = ed->anchor_y;
		a.x1 = ed->cursor_x;
		a.y1 = ed->cursor_y;
		a.color = ed->color;
		switch (ed->tool) {
		case TOOL_ARROW:
			a.type = ACTION_ARROW;a.no_gradient=!ed->gradient_mode;
			a.p0 = thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			break;
		case TOOL_LINE:
			a.type = ACTION_LINE;
			a.p0 = thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			break;
		case TOOL_RECT:
			a.type = ACTION_RECT;a.no_gradient=!ed->gradient_mode;a.no_outline=!ed->outline_mode;
			a.p0 = ed->fill_mode
				   ? -1
				   : thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x,
					ed->anchor_y, ed->cursor_x,
					ed->cursor_y, 0xffffff);
			break;
		case TOOL_CIRCLE:
			a.type = ACTION_CIRCLE;a.no_gradient=!ed->gradient_mode;a.no_outline=!ed->outline_mode;
			a.p0 = thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x,
					ed->anchor_y, ed->cursor_x,
					ed->cursor_y, 0xffffff);
			break;
		case TOOL_HIGHLIGHT:
			a.type = ACTION_HIGHLIGHT;a.no_gradient=!ed->gradient_mode;
			a.p0 = ed->highlight_strength;
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x,
					ed->anchor_y, ed->cursor_x,
					ed->cursor_y, 0xffffff);
			break;
		case TOOL_PIXELATE:
			a.type = ACTION_PIXELATE;
			a.p0 = ed->pixelate_block;
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x,
					ed->anchor_y, ed->cursor_x,
					ed->cursor_y, 0xffffff);
			break;
		case TOOL_BLUR:
			a.type = ACTION_BLUR;
			a.p0 = ed->blur_radius;
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x,
					ed->anchor_y, ed->cursor_x,
					ed->cursor_y, 0xffffff);
			break;
		default:
			break;
		}
	}

 int upload=ed->raster_dirty || !ed->canvasbuf;
 if(!ed->canvasbuf)ed->canvasbuf=XCreatePixmap(ed->dpy,ed->win,ed->win_w,ed->win_h,DefaultDepth(ed->dpy,ed->screen));
 if(upload) {
     paint_canvas_cache(ed,raster_preview?&ed->preview:&ed->rendered,raster_preview);
     ed->raster_dirty=0;
 }
 int dx=0,dy=0,dw=ed->win_w,dh=ed->win_h;
 int partial=ed->motion_only && vector_preview && !ed->sel_len && !ed->popup && !(ed->show_help || ed->show_about);
 if(partial) {
     int minx=ed->anchor_x,maxx=ed->cursor_x,miny=ed->anchor_y,maxy=ed->cursor_y;
     if(ed->pen_active && ed->pen_len>0) {
         minx=maxx=ed->pen_points[0];miny=maxy=ed->pen_points[1];
         for(int i=1;i<ed->pen_len;i++) {
             int x=ed->pen_points[i*2],y=ed->pen_points[i*2+1];
             if(x<minx)minx=x;
             if(x>maxx)maxx=x;
             if(y<miny)miny=y;
             if(y>maxy)maxy=y;
         }
     }
     int margin=ceil((30+tool_size(ed)*2)*ed->scale)+4;
     int x0=ed->canvas_x+floor(fmin(minx,maxx)*ed->scale)-margin;
     int y0=ed->canvas_y+floor(fmin(miny,maxy)*ed->scale)-margin;
     int x1=ed->canvas_x+ceil(fmax(minx,maxx)*ed->scale)+margin;
     int y1=ed->canvas_y+ceil(fmax(miny,maxy)*ed->scale)+margin;
     if(ed->damage_w>0) {
         if(ed->damage_x<x0)x0=ed->damage_x;
         if(ed->damage_y<y0)y0=ed->damage_y;
         if(ed->damage_x+ed->damage_w>x1)x1=ed->damage_x+ed->damage_w;
         if(ed->damage_y+ed->damage_h>y1)y1=ed->damage_y+ed->damage_h;
     }
     dx=x0<0?0:x0;dy=y0<0?0:y0;
     if(x1>ed->win_w)x1=ed->win_w;
     if(y1>ed->win_h)y1=ed->win_h;
     dw=x1-dx;dh=y1-dy;
     if(dw<1 || dh<1) {dx=dy=0;dw=ed->win_w;dh=ed->win_h;}
     ed->damage_x=dx;ed->damage_y=dy;ed->damage_w=dw;ed->damage_h=dh;
 } else ed->damage_w=ed->damage_h=0;
 XCopyArea(ed->dpy,ed->canvasbuf,ed->backbuf,ed->gc,dx,dy,dw,dh,dx,dy);
	if (vector_preview) {
		cairo_surface_t *surface = cairo_xlib_surface_create(
		    ed->dpy, ed->backbuf, DefaultVisual(ed->dpy, ed->screen),
		    ed->win_w, ed->win_h);
		cairo_t *cr = cairo_create(surface);
        if(partial) {cairo_rectangle(cr,dx,dy,dw,dh);cairo_clip(cr);}
		double x0 = ed->canvas_x + ed->anchor_x * ed->scale,
		       y0 = ed->canvas_y + ed->anchor_y * ed->scale,
		       x1 = ed->canvas_x + ed->cursor_x * ed->scale,
		       y1 = ed->canvas_y + ed->cursor_y * ed->scale;
		unsigned int color =
		    ed->pen_active ? (unsigned int)ed->pen_color : ed->color;
		double alpha = ed->pen_active && ed->pen_is_marker
				   ? default_marker_strength / 100.0
				   : (ed->tool == TOOL_HIGHLIGHT
					  ? ed->highlight_strength / 100.0
					  : 1.0);
		int thickness = ed->pen_active
				    ? ed->pen_thickness
				    : thickness_presets[ed->thickness_idx];
		cairo_set_source_rgba(cr, ((color >> 16) & 255) / 255.,
				      ((color >> 8) & 255) / 255.,
				      (color & 255) / 255., alpha);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
		cairo_set_line_width(cr, (thickness / 2 + 1) * 2 * ed->scale);
		cairo_rectangle(cr, ed->canvas_x, ed->canvas_y, ed->canvas_w,
				ed->canvas_h);
		cairo_clip(cr);
		if(ed->tool==TOOL_BLUR || ed->tool==TOOL_PIXELATE) {
            draw_effect_preview(ed,cr);
            unsigned int guide=themes[ed->theme].accent;
            cairo_set_source_rgb(cr,((guide>>16)&255)/255.,((guide>>8)&255)/255.,(guide&255)/255.);
            cairo_set_line_width(cr,1.);
            cairo_rectangle(cr,fmin(x0,x1),fmin(y0,y1),fmax(1,fabs(x1-x0)),fmax(1,fabs(y1-y0)));
        } else if (ed->pen_active && ed->pen_len > 0) {
			cairo_move_to(
			    cr, ed->canvas_x + ed->pen_points[0] * ed->scale,
			    ed->canvas_y + ed->pen_points[1] * ed->scale);
			if (ed->pen_len == 1)
				cairo_rel_line_to(cr, .01, 0);
			for (int pi = 1; pi < ed->pen_len; pi++)
				cairo_line_to(
				    cr,
				    ed->canvas_x +
					ed->pen_points[pi * 2] * ed->scale,
				    ed->canvas_y +
					ed->pen_points[pi * 2 + 1] * ed->scale);
		} else if (ed->tool == TOOL_RECT ||
			   ed->tool == TOOL_HIGHLIGHT) {
			cairo_rectangle(cr, fmin(x0, x1), fmin(y0, y1),
					fmax(1, fabs(x1 - x0)),
					fmax(1, fabs(y1 - y0)));
			if (ed->fill_mode || ed->tool == TOOL_HIGHLIGHT) {
                gradient_source_strength(cr,color,fmin(x0,x1),fmin(y0,y1),fmax(x0,x1),fmax(y0,y1),alpha,ed->gradient_mode?1.:0.);
                cairo_fill_preserve(cr);
            }
		} else if (ed->tool == TOOL_CIRCLE) {
			double rx = fmax(.5, fabs(x1 - x0) / 2),
			       ry = fmax(.5, fabs(y1 - y0) / 2);
			cairo_save(cr);
			cairo_translate(cr, (x0 + x1) / 2, (y0 + y1) / 2);
			cairo_scale(cr, rx, ry);
			cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
			cairo_restore(cr);
			if (ed->fill_mode) {
                gradient_source_strength(cr,color,fmin(x0,x1),fmin(y0,y1),fmax(x0,x1),fmax(y0,y1),alpha,ed->gradient_mode?1.:0.);
                cairo_fill_preserve(cr);
            }
        } else {
            struct action pending={.type=ed->tool==TOOL_ARROW?ACTION_ARROW:ACTION_LINE,
                .x0=ed->anchor_x,.y0=ed->anchor_y,.x1=ed->cursor_x,.y1=ed->cursor_y,
                .p0=thickness,.edges=2,.edge_style=S2_DEFAULT_ARROW_EDGE_STYLE,.no_gradient=!ed->gradient_mode};
            cairo_save(cr);cairo_translate(cr,ed->canvas_x,ed->canvas_y);cairo_scale(cr,ed->scale,ed->scale);
            connector_path(cr,&pending);cairo_restore(cr);
        }
        if(ed->tool==TOOL_HIGHLIGHT || ((ed->tool==TOOL_RECT || ed->tool==TOOL_CIRCLE) && !ed->outline_mode))cairo_new_path(cr);
        cairo_set_source_rgba(cr,((color>>16)&255)/255.,((color>>8)&255)/255.,(color&255)/255.,alpha);
		cairo_stroke(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
	}

 draw_transform_preview(ed);draw_text_caret(ed);
	draw_status(ed);
XCopyArea(ed->dpy,ed->backbuf,ed->win,ed->gc,dx,dy,dw,dh,dx,dy);
 if(partial)copy_chrome(ed,ed->backbuf,ed->win);
 if(ed->show_help || ed->show_about){XFlush(ed->dpy);return;}
	{
		int k;
		XSetForeground(
		    ed->dpy, ed->gc,
		    (unsigned long)themes[ed->theme].accent);
		for (k = 0; k < ed->sel_len; k++) {
			struct action *sa;
			int corners[8];
			int handle[2];
			int i;
			if (ed->sel[k] < 0 ||
			    (size_t)ed->sel[k] >= ed->actions.cursor) {
				continue;
			}
			sa = &ed->actions.items[ed->sel[k]];
			selection_geometry(ed, sa, corners, handle);
			for (i = 0; i < 4; i++) {
				int j = (i + 1) % 4;
				XDrawLine(
				    ed->dpy, ed->win, ed->gc,
				    corners[i * 2 + 0], corners[i * 2 + 1],
				    corners[j * 2 + 0], corners[j * 2 + 1]);
			}
			if(ed->sel_len==1 && action_is_resizable(sa->type)) {
                for(int corner=0;corner<4;corner++) {
                    XSetForeground(ed->dpy,ed->gc,themes[ed->theme].panel);
                    XFillRectangle(ed->dpy,ed->win,ed->gc,corners[corner*2]-4,corners[corner*2+1]-4,8,8);
                    XSetForeground(ed->dpy,ed->gc,themes[ed->theme].accent);
                    XDrawRectangle(ed->dpy,ed->win,ed->gc,corners[corner*2]-4,corners[corner*2+1]-4,8,8);
                }
            }
            /* rotation handle only when a single object is selected
			 */
			if (ed->sel_len == 1 && action_is_rotatable(sa->type)) {
                int center_x=(corners[0]+corners[4])/2,center_y=(corners[1]+corners[5])/2;
                if(ed->rotating) {
                    XDrawLine(ed->dpy,ed->win,ed->gc,center_x-5,center_y,center_x+5,center_y);
                    XDrawLine(ed->dpy,ed->win,ed->gc,center_x,center_y-5,center_x,center_y+5);
                }
				int tmx = (corners[0] + corners[2]) / 2;
				int tmy = (corners[1] + corners[3]) / 2;
				XDrawLine(ed->dpy, ed->win, ed->gc, tmx, tmy, handle[0], handle[1]);
				XDrawArc(ed->dpy,
				         ed->win,
				         ed->gc,
				         handle[0] - ROTATE_HANDLE_RADIUS,
				         handle[1] - ROTATE_HANDLE_RADIUS,
				         (unsigned int)(ROTATE_HANDLE_RADIUS * 2),
				         (unsigned int)(ROTATE_HANDLE_RADIUS * 2),
				         0,
				         360 * 64);
			}
		}
	}
    {
        int hx,hy;
        if(ed->tool==TOOL_SELECT && curve_handle_position(ed,&hx,&hy)) {
            XPoint diamond[]={{hx,hy-6},{hx+6,hy},{hx,hy+6},{hx-6,hy},{hx,hy-6}};
            XSetForeground(ed->dpy,ed->gc,themes[ed->theme].panel);
            XFillPolygon(ed->dpy,ed->win,ed->gc,diamond,4,Convex,CoordModeOrigin);
            XSetForeground(ed->dpy,ed->gc,themes[ed->theme].accent);
            XDrawLines(ed->dpy,ed->win,ed->gc,diamond,5,CoordModeOrigin);
        }
    }
	if (ed->marquee_active) {
		int x0 = ed->canvas_x + (int)(ed->marquee_x * ed->scale);
		int y0 = ed->canvas_y + (int)(ed->marquee_y * ed->scale);
		int x1 = ed->canvas_x + (int)(ed->cursor_x * ed->scale);
		int y1 = ed->canvas_y + (int)(ed->cursor_y * ed->scale);
		int minx = x0 < x1 ? x0 : x1;
		int miny = y0 < y1 ? y0 : y1;
		unsigned int w = (unsigned int)(x0 < x1 ? (x1 - x0) : (x0 - x1));
		unsigned int h = (unsigned int)(y0 < y1 ? (y1 - y0) : (y0 - y1));
		XSetForeground(ed->dpy, ed->gc, (unsigned long)themes[ed->theme].accent);
		XDrawRectangle(ed->dpy, ed->win, ed->gc, minx, miny, w, h);
	}
	if (ed->anchor_active && tool_uses_anchor(ed->tool) && !vector_preview) {
		int x0 = ed->canvas_x + (int)(ed->anchor_x * ed->scale);
		int y0 = ed->canvas_y + (int)(ed->anchor_y * ed->scale);
		int x1 = ed->canvas_x + (int)(ed->cursor_x * ed->scale);
		int y1 = ed->canvas_y + (int)(ed->cursor_y * ed->scale);
		int minx = x0 < x1 ? x0 : x1;
		int miny = y0 < y1 ? y0 : y1;
		unsigned int w = (unsigned int)(x0 < x1 ? (x1 - x0) : (x0 - x1));
		unsigned int h = (unsigned int)(y0 < y1 ? (y1 - y0) : (y0 - y1));
		XSetForeground(ed->dpy, ed->gc, (unsigned long)themes[ed->theme].accent);
		XDrawRectangle(ed->dpy, ed->win, ed->gc, minx, miny, w, h);
		if (ed->tool == TOOL_ARROW || ed->tool == TOOL_LINE) {
			XDrawLine(ed->dpy, ed->win, ed->gc, x0, y0, x1, y1);
		}
	}
	if (ed->tool == TOOL_MARKER && !ed->pen_active) {
		int mh = ed->marker_height > 0 ? ed->marker_height : 1;
		int cx = ed->canvas_x + (int)(ed->cursor_x * ed->scale);
		int yt = ed->canvas_y + (int)((ed->cursor_y - mh / 2) * ed->scale);
		int yb = ed->canvas_y + (int)((ed->cursor_y + mh / 2) * ed->scale);
		XSetForeground(ed->dpy, ed->gc, (unsigned long)(ed->color & 0xffffffu));
		XDrawLine(ed->dpy, ed->win, ed->gc, cx, yt, cx, yb);
		XDrawLine(ed->dpy, ed->win, ed->gc, cx - 3, yt, cx + 3, yt);
		XDrawLine(ed->dpy, ed->win, ed->gc, cx - 3, yb, cx + 3, yb);
	}

	XFlush(ed->dpy);
 if(ed->profile) {
 clock_gettime(CLOCK_MONOTONIC,&frame_end);
 double ms=(frame_end.tv_sec-frame_start.tv_sec)*1000.0+(frame_end.tv_nsec-frame_start.tv_nsec)/1e6;
 ed->frame_count++; ed->frame_ms+=ms; if(ms>ed->frame_max_ms) ed->frame_max_ms=ms;
 }
}

static int
parse_hex_color(const char *s, unsigned int *out)
{
	char *end;
	unsigned long v;

	if (!s || !out || strlen(s) != 6) {
		return -1;
	}
	v = strtoul(s, &end, 16);
	if (*end != '\0' || v > 0xffffff) {
		return -1;
	}
	*out = (unsigned int)v;
	return 0;
}

static void
update_cursor(struct editor_state *ed)
{
	Cursor c;

	if (!ed || !ed->dpy || !ed->win) {
		return;
	}
	c = (ed->show_help || ed->show_about)?ed->cursor_arrow:ed->chrome_hover>=0?ed->cursor_hand:(((ed->tool == TOOL_MARKER || ed->tool==TOOL_TEXT) && ed->cursor_text) ? ed->cursor_text : ed->cursor);
if(!(ed->show_help || ed->show_about) && (ed->chrome_hover<0 || ed->rotating || ed->resizing) && ed->tool==TOOL_SELECT) {
     int corner=resize_handle_hit(ed,ed->pointer_x,ed->pointer_y);
     if(ed->curving || curve_handle_hit(ed,ed->pointer_x,ed->pointer_y))c=ed->cursor_hand;
     else if(ed->rotating || rotation_handle_hit(ed,ed->pointer_x,ed->pointer_y))c=ed->cursor_rotate;
     else if(ed->resizing || corner>=0) {
         if(ed->resizing)corner=ed->resize_corner;
         int quadrant=0;
         if(ed->selected_idx>=0 && (size_t)ed->selected_idx<ed->actions.cursor)
             quadrant=(int)lround(ed->actions.items[ed->selected_idx].rotation/90.);
         c=ed->cursor_resize[(corner+quadrant)%4];
     } else if(ed->drag_active)c=ed->cursor_hand;
 }
 if(ed->picker_active)c=ed->cursor;
 if(ed->show_about)c=about_hit(ed,ed->pointer_x,ed->pointer_y)>0?ed->cursor_hand:ed->cursor_arrow;
 if (c && c!=ed->current_cursor) {
 ed->current_cursor=c;
		XDefineCursor(ed->dpy, ed->win, c);
	}
}

static void
set_tool(struct editor_state *ed, enum tool tool)
{
	if (!ed) {
		return;
	}
	if (ed->tool == TOOL_SELECT && tool != TOOL_SELECT) {
		sel_clear(ed);
		ed->drag_active = 0;
		ed->drag_dx = 0;
		ed->drag_dy = 0;
		ed->rotating = 0;
        ed->resizing=ed->curving=0;
        if(ed->transform_preview) {ed->transform_preview=0;ed->dirty=ed->raster_dirty=1;}
		ed->marquee_active = 0;
	}
	if (ed->tool == TOOL_TEXT && ed->text_mode) {
		commit_text_input(ed);
	}
	if (tool == TOOL_NUMBER) {
		ed->number_next = 1;
	}
	ed->popup=0;
	if(tool!=TOOL_SELECT)sel_clear(ed);
	ed->tool = tool;
	ed->anchor_active = 0;
	clear_text_mode(ed);
	reset_pen_input(ed);
	ed->color_mode = 0;ed->picker_active=0;
	update_cursor(ed);
}

static int
build_timestamp_name(char *out, size_t outlen)
{
 struct timespec now;struct tm local;char seconds[32];
 if(!out || clock_gettime(CLOCK_REALTIME,&now)!=0 || !localtime_r(&now.tv_sec,&local))return -1;
 if(!strftime(seconds,sizeof seconds,"%Y-%m-%dT%H%M%S",&local))return -1;
 int count=snprintf(out,outlen,"%s.%06ld.png",seconds,now.tv_nsec/1000);
 return count<0 || (size_t)count>=outlen?-1:0;
}

static int
build_timestamp_path(const struct app_config *cfg, char *out, size_t outlen)
{
	char name[64];
	char dirbuf[PATH_MAX];
	const char *dir;

	if (!out || outlen < 64) {
		return -1;
	}
	if (build_timestamp_name(name, sizeof(name)) != 0) {
		return -1;
	}
	dir = (cfg && cfg->save_dir_override && cfg->save_dir_override[0] == '/') ? cfg->save_dir_override :
	                                                                           default_save_directory;
	if (!dir || !*dir) {
		dir = ".";
	}
	if (dir[0] == '~' && (dir[1] == '\0' || dir[1] == '/')) {
		const char *home = getenv("HOME");
		if (home && *home) {
			if (dir[1] == '\0') {
				snprintf(dirbuf, sizeof(dirbuf), "%s", home);
			} else {
				snprintf(dirbuf, sizeof(dirbuf), "%s/%s", home, dir + 2);
			}
			dir = dirbuf;
		}
	}
	if (snprintf(out, outlen, "%s/%s", dir, name) >= (int)outlen) {
		return -1;
	}
	return 0;
}

/* Exclusively create timestamped captures: never truncate an earlier save. */
static int save_unique_png(const struct image *img,const char *base,char *out,size_t capacity)
{
 size_t length=strlen(base);int fd=-1;
 for(unsigned int suffix=0;suffix<10000;suffix++) {
     int count=suffix?snprintf(out,capacity,"%.*s-%u.png",(int)(length>=4?length-4:length),base,suffix):snprintf(out,capacity,"%s",base);
     if(count<0 || (size_t)count>=capacity)return -1;
     fd=open(out,O_WRONLY|O_CREAT|O_EXCL,0666);
     if(fd>=0)break;
     if(errno!=EEXIST) {fprintf(stderr,"s2: cannot create '%s': %s\n",out,strerror(errno));return -1;}
 }
 if(fd<0) {fprintf(stderr,"s2: cannot find an unused screenshot filename\n");return -1;}
 FILE *stream=fdopen(fd,"wb");
 if(!stream) {close(fd);unlink(out);return -1;}
 int result=image_write_png_stream(img,stream);
 if(fclose(stream)!=0)result=-1;
 if(result!=0)unlink(out);
 return result;
}

static void
print_saved_abs_path(const char *path)
{
	char resolved[PATH_MAX];
	char cwd[PATH_MAX];

	if (!path || !*path) {
		return;
	}
	if (realpath(path, resolved)) {
		printf("%s\n", resolved);
		fflush(stdout);
		return;
	}
	if (path[0] == '/') {
		printf("%s\n", path);
		fflush(stdout);
		return;
	}
	if (getcwd(cwd, sizeof(cwd))) {
		printf("%s/%s\n", cwd, path);
		fflush(stdout);
		return;
	}
	printf("%s\n", path);
	fflush(stdout);
}


static enum tool
tool_from_config_value(int value)
{
	switch (value) {
	case 0: return TOOL_SELECT;
	case 1: return TOOL_ARROW;
	case 2: return TOOL_LINE;
	case 3: return TOOL_PEN;
	case 4: return TOOL_NUMBER;
	case 5: return TOOL_RECT;
	case 6: return TOOL_CIRCLE;
	case 7: return TOOL_TEXT;
	case 8: return TOOL_HIGHLIGHT;
	case 9: return TOOL_MARKER;
	case 10: return TOOL_PIXELATE;
	case 11: return TOOL_BLUR;
	case 12: return TOOL_PICKER;
	default: return TOOL_ARROW;
	}
}

static int
add_simple_action(struct editor_state *ed, enum action_type type, int x0, int y0, int x1, int y1, int p0)
{
	struct action a;

	memset(&a, 0, sizeof(a));
	a.type = type;a.no_gradient=!ed->gradient_mode;a.no_outline=!ed->outline_mode;
 if(type==ACTION_RECT || type==ACTION_CIRCLE)a.outline_thickness=thickness_presets[ed->thickness_idx];
 if(type==ACTION_ARROW) {a.edges=2;a.edge_style=S2_DEFAULT_ARROW_EDGE_STYLE;}
	a.x0 = x0;
	a.y0 = y0;
	a.x1 = x1;
	a.y1 = y1;
	a.p0 = p0;
	a.color = ed->color;
	if (push_action(&ed->actions, &a) != 0) {
		fprintf(stderr, "s2: out of memory for action stack\n");
		return -1;
	}
	ed->tool=TOOL_SELECT;sel_set_single(ed,ed->actions.cursor-1);
	ed->dirty = 1;
	ed->raster_dirty = 1;
	return 0;
}

static int
distance_sq_point_segment(int px, int py, int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	if (dx == 0 && dy == 0) {
		int ux = px - x0;
		int uy = py - y0;
		return ux * ux + uy * uy;
	}
	{
		double t = ((double)(px - x0) * dx + (double)(py - y0) * dy) / ((double)dx * dx + (double)dy * dy);
		double qx;
		double qy;
		int rx;
		int ry;
		if (t < 0.0) t = 0.0;
		if (t > 1.0) t = 1.0;
		qx = x0 + t * dx;
		qy = y0 + t * dy;
		rx = px - (int)qx;
		ry = py - (int)qy;
		return rx * rx + ry * ry;
	}
}

static int
action_hit_test(const struct editor_state *ed, const struct action *a, int x, int y)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int pad;

	if (!a) {
		return 0;
	}
	if (a->rotation % 360 != 0 && action_is_rotatable(a->type)) {
		double cx;
		double cy;
		double rad = -a->rotation * M_PI / 180.0;
		double cosr = cos(rad);
		double sinr = sin(rad);
		double dxp;
		double dyp;
		action_center(ed, a, &cx, &cy);
		dxp = x - cx;
		dyp = y - cy;
		x = (int)lround(cosr * dxp - sinr * dyp + cx);
		y = (int)lround(sinr * dxp + cosr * dyp + cy);
	}
	pad = 6 + a->p0;
	minx = a->x0 < a->x1 ? a->x0 : a->x1;
	miny = a->y0 < a->y1 ? a->y0 : a->y1;
	maxx = a->x0 > a->x1 ? a->x0 : a->x1;
	maxy = a->y0 > a->y1 ? a->y0 : a->y1;

	switch (a->type) {
	case ACTION_ARROW:
	case ACTION_LINE:
		{
            double px=a->x0,py=a->y0;
            for(int i=1;i<=64;i++) {
                double qx,qy;connector_point(a,i/64.,&qx,&qy);
                if(distance_sq_point_segment(x,y,lround(px),lround(py),lround(qx),lround(qy))<=pad*pad)return 1;
                px=qx;py=qy;
            }
            return 0;
        }
	case ACTION_PEN:
	case ACTION_MARKER:
		if (a->pen_points && a->pen_len > 0) {
			int i;
			if (a->pen_len == 1) {
				int ux = x - a->pen_points[0];
				int uy = y - a->pen_points[1];
				return ux * ux + uy * uy <= pad * pad;
			}
			for (i = 1; i < a->pen_len; i++) {
				if (distance_sq_point_segment(x,
				                             y,
				                             a->pen_points[(i - 1) * 2 + 0],
				                             a->pen_points[(i - 1) * 2 + 1],
				                             a->pen_points[i * 2 + 0],
				                             a->pen_points[i * 2 + 1]) <= pad * pad) {
					return 1;
				}
			}
		}
		return 0;
	case ACTION_CIRCLE:
	case ACTION_RECT:
	case ACTION_NUMBER:
	case ACTION_HIGHLIGHT:
	case ACTION_PIXELATE:
	case ACTION_BLUR:
		return x >= minx - pad && x <= maxx + pad && y >= miny - pad && y <= maxy + pad;
	case ACTION_TEXT:
		{
			int bx;
			int by;
			int bw;
			int bh;
			int scale = a->p0 < 0 ? -a->p0 : a->p0;
			int text_pad;
			if (scale < 1) {
				scale = 1;
			}
			struct editor_state styled=*ed;styled.text_style=a;
            text_box_metrics(&styled, a->x0, a->y0, a->text ? a->text : "", scale, a->p0 < 0, &bx, &by, &bw, &bh);
			text_pad = pad;
			if (text_pad < 4) {
				text_pad = 4;
			}
			if (a->p0 < 0) {
				int pad_fill = action_text_padding(a,scale);
				int radius = action_text_radius(a,scale);
				int left = bx - text_pad;
				int top = by - text_pad;
				int right = bx + bw + text_pad;
				int bottom = by + bh + text_pad;
				int x0 = bx + pad_fill;
				int y0 = by + pad_fill;
				int w = bw - pad_fill * 2;
				int h = bh - pad_fill * 2;
				if (w < 1) {
					w = 1;
				}
				if (h < 1) {
					h = 1;
				}
				if (radius < 0) {
					radius = 0;
				}
				if (radius > w / 2) {
					radius = w / 2;
				}
				if (radius > h / 2) {
					radius = h / 2;
				}
				if (x < left || x > right || y < top || y > bottom) {
					return 0;
				}
				if (radius > 0) {
					if (x < x0 + radius && y < y0 + radius) {
						int dx = (x0 + radius - 1) - x;
						int dy = (y0 + radius - 1) - y;
						return dx * dx + dy * dy <= radius * radius;
					}
					if (x >= x0 + w - radius && y < y0 + radius) {
						int dx = x - (x0 + w - radius);
						int dy = (y0 + radius - 1) - y;
						return dx * dx + dy * dy <= radius * radius;
					}
					if (x < x0 + radius && y >= y0 + h - radius) {
						int dx = (x0 + radius - 1) - x;
						int dy = y - (y0 + h - radius);
						return dx * dx + dy * dy <= radius * radius;
					}
					if (x >= x0 + w - radius && y >= y0 + h - radius) {
						int dx = x - (x0 + w - radius);
						int dy = y - (y0 + h - radius);
						return dx * dx + dy * dy <= radius * radius;
					}
				}
				return 1;
			}
			{
				int extra = scale * 4 + text_bg_padding_for_scale(scale) + TEXT_RENDER_PAD;
				return x >= bx - text_pad - extra && x <= bx + bw + text_pad + extra && y >= by - text_pad - extra &&
				       y <= by + bh + text_pad + extra;
			}
		}
	default:
		return 0;
	}
}

static int
find_action_at(const struct editor_state *ed, int x, int y)
{
	int i;
	for (i = (int)ed->actions.cursor - 1; i >= 0; i--) {
		if (action_hit_test(ed, &ed->actions.items[i], x, y)) {
			return i;
		}
	}
	return -1;
}

static void
sel_sync_primary(struct editor_state *ed)
{
	/* selected_idx mirrors the selection only when exactly one item is held;
	 * single-object affordances (rotation handle) key off it. */
	ed->selected_idx = (ed->sel_len == 1) ? ed->sel[0] : -1;
 if(ed->selected_idx>=0 && (size_t)ed->selected_idx<ed->actions.cursor) {
     struct action *a=&ed->actions.items[ed->selected_idx];
     if(a->type!=ACTION_BLUR && a->type!=ACTION_PIXELATE)ed->color=a->color;
     if(action_supports_gradient(a))ed->gradient_mode=!a->no_gradient;
     if(a->type==ACTION_RECT || a->type==ACTION_CIRCLE)ed->outline_mode=!a->no_outline;
     if(a->type==ACTION_RECT || a->type==ACTION_CIRCLE || a->type==ACTION_TEXT)ed->fill_mode=a->p0<0;
 }
}

static void
sel_clear(struct editor_state *ed)
{
	ed->sel_len = 0;
	ed->selected_idx = -1;
}

static int
sel_contains(const struct editor_state *ed, int idx)
{
	int i;
	for (i = 0; i < ed->sel_len; i++) {
		if (ed->sel[i] == idx) {
			return 1;
		}
	}
	return 0;
}

static int
sel_add(struct editor_state *ed, int idx)
{
	if (idx < 0 || sel_contains(ed, idx)) {
		return 0;
	}
	if (ed->sel_len == ed->sel_cap) {
		int new_cap = ed->sel_cap ? ed->sel_cap * 2 : 16;
		int *p = realloc(ed->sel, (size_t)new_cap * sizeof(*p));
		if (!p) {
			return -1;
		}
		ed->sel = p;
		ed->sel_cap = new_cap;
	}
	ed->sel[ed->sel_len++] = idx;
	return 0;
}

static void
sel_set_single(struct editor_state *ed, int idx)
{
	sel_clear(ed);
	if (idx >= 0) {
		sel_add(ed, idx);
	}
	sel_sync_primary(ed);
}

static int
rects_intersect(int ax0, int ay0, int ax1, int ay1, int bx0, int by0, int bx1, int by1)
{
	if (ax1 < bx0 || bx1 < ax0 || ay1 < by0 || by1 < ay0) {
		return 0;
	}
	return 1;
}

/* select every object whose bounding box intersects the marquee rectangle */
static void
selection_from_marquee(struct editor_state *ed, int x0, int y0, int x1, int y1)
{
	int rx0 = x0 < x1 ? x0 : x1;
	int ry0 = y0 < y1 ? y0 : y1;
	int rx1 = x0 > x1 ? x0 : x1;
	int ry1 = y0 > y1 ? y0 : y1;
	int i;

	sel_clear(ed);
	for (i = 0; i < (int)ed->actions.cursor; i++) {
		int minx;
		int miny;
		int maxx;
		int maxy;
		action_bounds(ed, &ed->actions.items[i], &minx, &miny, &maxx, &maxy);
		if (rects_intersect(minx, miny, maxx, maxy, rx0, ry0, rx1, ry1)) {
			sel_add(ed, i);
		}
	}
	sel_sync_primary(ed);
}

static void
action_move_by(struct action *a, int dx, int dy)
{
	int i;

	if (!a) {
		return;
	}
	if ((a->type == ACTION_PEN || a->type == ACTION_MARKER) && a->pen_points && a->pen_len > 0) {
		for (i = 0; i < a->pen_len; i++) {
			a->pen_points[i * 2 + 0] += dx;
			a->pen_points[i * 2 + 1] += dy;
		}
		a->x0 += dx;
		a->y0 += dy;
		a->x1 += dx;
		a->y1 += dy;
		return;
	}
	if (a->type == ACTION_NUMBER) {
		a->x0 += dx;
		a->y0 += dy;
		return;
	}
	a->x0 += dx;
	a->y0 += dy;
	a->x1 += dx;
	a->y1 += dy;
}

static int
delete_selected(struct editor_state *ed)
{
	int i;
	int j;
	int removed = 0;

	if (ed->sel_len <= 0) {
		return -1;
	}
	/* delete highest indices first so the remaining indices stay valid */
	for (i = 0; i < ed->sel_len; i++) {
		for (j = i + 1; j < ed->sel_len; j++) {
			if (ed->sel[j] > ed->sel[i]) {
				int t = ed->sel[i];
				ed->sel[i] = ed->sel[j];
				ed->sel[j] = t;
			}
		}
	}
	for (i = 0; i < ed->sel_len; i++) {
		int idx = ed->sel[i];
		if (idx < 0 || (size_t)idx >= ed->actions.cursor) {
			continue;
		}
		free_action(&ed->actions.items[idx]);
		for (j = idx; (size_t)j + 1 < ed->actions.cursor; j++) {
			ed->actions.items[j] = ed->actions.items[j + 1];
		}
		ed->actions.cursor--;
		removed++;
	}
	ed->actions.len = ed->actions.cursor;
	sel_clear(ed);
	if (removed == 0) {
		return -1;
	}
	ed->dirty = 1;
	ed->raster_dirty = 1;
	return 0;
}

static int
commit_current_tool(struct editor_state *ed)
{
	switch (ed->tool) {
	case TOOL_SELECT:
		sel_set_single(ed, find_action_at(ed, ed->cursor_x, ed->cursor_y));
		return 0;
	case TOOL_ARROW:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_ARROW,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         thickness_presets[ed->thickness_idx]);
	case TOOL_LINE:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_LINE,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         thickness_presets[ed->thickness_idx]);
	case TOOL_PEN:
		return 0;
	case TOOL_NUMBER:
		{
			struct action a;
			int scale = ed->text_scale > 0 ? ed->text_scale : 1;
			int radius = number_radius_for_scale(scale);
			memset(&a, 0, sizeof(a));
			a.type = ACTION_NUMBER;a.no_gradient=!ed->gradient_mode;
			a.x0 = ed->cursor_x;
			a.y0 = ed->cursor_y;
			a.x1 = ed->number_next > 0 ? ed->number_next : 1;
			a.y1 = scale;
			a.p0 = radius;
			a.color = ed->color;
			if (push_action(&ed->actions, &a) != 0) {
				fprintf(stderr, "s2: out of memory for number action\n");
				return -1;
			}
			ed->number_next = a.x1 + 1;
            ed->tool=TOOL_SELECT;sel_set_single(ed,ed->actions.cursor-1);
			ed->dirty = 1;
			ed->raster_dirty = 1;
			return 0;
		}
	case TOOL_RECT:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_RECT,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->fill_mode ? -1 : thickness_presets[ed->thickness_idx]);
	case TOOL_CIRCLE:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_CIRCLE,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->fill_mode ? -1 : thickness_presets[ed->thickness_idx]);
	case TOOL_PIXELATE:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_PIXELATE,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->pixelate_block);
	case TOOL_BLUR:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_BLUR,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->blur_radius);
	case TOOL_TEXT:
		ed->text_mode = 1;
		ed->text_x = ed->cursor_x;
		ed->text_y = ed->cursor_y;
		ed->text_len = 0;ed->text_cursor=ed->text_anchor=0;ed->text_edit_index=-1;
		ed->text_buf[0] = '\0';
		return 0;
	case TOOL_HIGHLIGHT:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_HIGHLIGHT,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->highlight_strength);
	case TOOL_MARKER:
		return 0;
	case TOOL_PICKER:
		ed->color = draw_sample_color(&ed->rendered, ed->cursor_x, ed->cursor_y);
		return 0;
	default:
		return 0;
	}
}

static int
handle_text_mode(struct editor_state *ed, XKeyEvent *kev)
{
	char buf[64];
	KeySym sym;
	int n;
        Status status;

	sym = XLookupKeysym(kev, 0);
	if (sym == XK_Escape) {
		int selected=ed->text_edit_index;
        clear_text_mode(ed);set_tool(ed,TOOL_SELECT);
        if(selected>=0)sel_set_single(ed,selected);
		ed->suppress_escape_once = 1;
		return 1;
	}
	if (sym == XK_Return) {
		commit_text_input(ed);
		return 1;
	}
 if((kev->state&ControlMask) && sym==XK_a) {
     ed->text_anchor=0;ed->text_cursor=ed->text_len;ed->raster_dirty=1;return 1;
 }
 if(sym==XK_Left || sym==XK_Right || sym==XK_Home || sym==XK_End) {
     int next=ed->text_cursor;
     if(!(kev->state&ShiftMask) && ed->text_cursor!=ed->text_anchor && (sym==XK_Left || sym==XK_Right))
         next=sym==XK_Left? (ed->text_cursor<ed->text_anchor?ed->text_cursor:ed->text_anchor):(ed->text_cursor>ed->text_anchor?ed->text_cursor:ed->text_anchor);
     else if(sym==XK_Left && next>0)next=g_utf8_find_prev_char(ed->text_buf,ed->text_buf+next)-ed->text_buf;
     else if(sym==XK_Right && next<ed->text_len)next=g_utf8_next_char(ed->text_buf+next)-ed->text_buf;
     else if(sym==XK_Home)next=0;
     else if(sym==XK_End)next=ed->text_len;
     ed->text_cursor=next;if(!(kev->state&ShiftMask))ed->text_anchor=next;
     ed->raster_dirty=1;return 1;
 }
 if(sym==XK_BackSpace || sym==XK_Delete) {
     if(!text_delete_selection(ed)) {
         int next=ed->text_cursor;
         if(sym==XK_BackSpace && next>0)next=g_utf8_find_prev_char(ed->text_buf,ed->text_buf+next)-ed->text_buf;
         if(sym==XK_Delete && next<ed->text_len)next=g_utf8_next_char(ed->text_buf+next)-ed->text_buf;
         ed->text_anchor=next;text_delete_selection(ed);
     }
     ed->raster_dirty=1;return 1;
 }
	if ((kev->state & ControlMask) && (sym == XK_v || sym == XK_V)) {
		paste_text_from_clipboard(ed);
		ed->raster_dirty = 1;
		return 1;
	}

	if (ed->xic) {
		n = Xutf8LookupString(ed->xic, kev, buf, (int)sizeof(buf) - 1, &sym, &status);
		if (status == XBufferOverflow) {
			n = 0;
		}
	} else {
		n = XLookupString(kev, buf, (int)sizeof(buf) - 1, NULL, NULL);
	}
	if (n > 0) {
		buf[n] = '\0';
 append_pasted_text(ed,buf);
		ed->raster_dirty = 1;
	}
	return 1;
}

static int
handle_color_mode(struct editor_state *ed, XKeyEvent *kev)
{
	char buf[32];
	KeySym sym;
	int n;

	sym = XLookupKeysym(kev, 0);
	if (sym == XK_Escape) {
		ed->color_mode = 0;
		ed->color_len = 0;
		ed->color_buf[0] = '\0';
		return 1;
	}
	if (sym == XK_Return) {
		unsigned int parsed;
		if (parse_hex_color(ed->color_buf, &parsed) == 0) {
			apply_picker_color(ed,parsed);
		}
		ed->color_mode = 0;
		ed->color_len = 0;
		ed->color_buf[0] = '\0';
		return 1;
	}
	if (sym == XK_BackSpace) {
		if (ed->color_len > 0) {
			ed->color_len--;
			ed->color_buf[ed->color_len] = '\0';
		}
		return 1;
	}

	n = XLookupString(kev, buf, (int)sizeof(buf), NULL, NULL);
	if (n > 0) {
		int i;
		for (i = 0; i < n; i++) {
			char c = buf[i];
			if (isxdigit((unsigned char)c) && ed->color_len < 6) {
				ed->color_buf[ed->color_len++] = c;
				ed->color_buf[ed->color_len] = '\0';
			}
		}
	}
	return 1;
}

static void
move_cursor(struct editor_state *ed, int dx, int dy)
{
	ed->cursor_x += dx;
	ed->cursor_y += dy;
	if (ed->cursor_x < 0) {
		ed->cursor_x = 0;
	}
	if (ed->cursor_y < 0) {
		ed->cursor_y = 0;
	}
	if (ed->cursor_x >= ed->img->width) {
		ed->cursor_x = ed->img->width - 1;
	}
	if (ed->cursor_y >= ed->img->height) {
		ed->cursor_y = ed->img->height - 1;
	}
}

static int
tool_uses_anchor(enum tool tool)
{
	return tool == TOOL_ARROW || tool == TOOL_LINE || tool == TOOL_RECT || tool == TOOL_CIRCLE ||
	       tool == TOOL_HIGHLIGHT || tool == TOOL_PIXELATE || tool == TOOL_BLUR;
}

static int
finalize_pending_anchor(struct editor_state *ed)
{
	if (!ed) {
		return -1;
	}
	if(ed->text_mode) { commit_text_input(ed); return 0; }
 if (tool_uses_anchor(ed->tool) && ed->anchor_active) {
		return commit_current_tool(ed);
	}
	return 0;
}

static int action_supports_fill(const struct action *a)
{
 return a->type==ACTION_RECT || a->type==ACTION_CIRCLE || a->type==ACTION_TEXT;
}
static int action_supports_gradient(const struct action *a)
{
 return action_supports_fill(a) || a->type==ACTION_NUMBER || a->type==ACTION_HIGHLIGHT || a->type==ACTION_ARROW;
}
static void toggle_gradient_mode(struct editor_state *ed)
{
 ed->gradient_mode=!ed->gradient_mode;
 if(ed->tool==TOOL_SELECT)for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];if(index<0 || (size_t)index>=ed->actions.cursor)continue;
     struct action *a=&ed->actions.items[index];
     if(action_supports_gradient(a)){a->no_gradient=!ed->gradient_mode;ed->dirty=1;}
 }
 ed->raster_dirty=1;ed->chrome_key[0]=0;
}
static void toggle_outline_mode(struct editor_state *ed)
{
 ed->outline_mode=!ed->outline_mode;
 if(ed->tool==TOOL_SELECT)for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];if(index<0 || (size_t)index>=ed->actions.cursor)continue;
     struct action *a=&ed->actions.items[index];
     if(a->type==ACTION_RECT || a->type==ACTION_CIRCLE){a->no_outline=!ed->outline_mode;ed->dirty=1;}
 }
 ed->raster_dirty=1;ed->chrome_key[0]=0;
}
static void set_fill_mode(struct editor_state *ed,int fill)
{
 ed->fill_mode=fill;
 for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];
     if(index<0 || (size_t)index>=ed->actions.cursor)continue;
     struct action *a=&ed->actions.items[index];
     if(!action_supports_fill(a))continue;
     if(a->type==ACTION_TEXT)a->p0=fill?-abs(a->p0):abs(a->p0);
     else if(fill) {
         if(a->p0>0)a->outline_thickness=a->p0;
         a->p0=-1;
     } else a->p0=a->outline_thickness>0?a->outline_thickness:thickness_presets[ed->thickness_idx];
     ed->dirty=1;
 }
 ed->raster_dirty=1;
}
static void toggle_fill_mode(struct editor_state *ed)
{
 int fill=!ed->fill_mode;
 for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];
     if(index>=0 && (size_t)index<ed->actions.cursor && action_supports_fill(&ed->actions.items[index])) {
         fill=ed->actions.items[index].p0>=0;break;
     }
 }
 set_fill_mode(ed,fill);
}
static int style_selected_text(struct editor_state *ed,KeySym sym)
{
 if(sym!=XK_b && sym!=XK_B && sym!=XK_p && sym!=XK_P && sym!=XK_c && sym!=XK_C && sym!=XK_i && sym!=XK_u && sym!=XK_s)return 0;
 int changed=0;
 for(int i=0;i<ed->sel_len;i++) {
     int index=ed->sel[i];if(index<0 || (size_t)index>=ed->actions.cursor)continue;
     struct action *a=&ed->actions.items[index];if(a->type!=ACTION_TEXT)continue;
     int weight=a->font_weight?a->font_weight:400;
     if(sym==XK_b)a->font_weight=weight>100?weight-100:100;
     if(sym==XK_B)a->font_weight=weight<900?weight+100:900;
     if(sym==XK_p || sym==XK_P || sym==XK_c || sym==XK_C) {
         if(!a->text_box_custom) {
             a->text_padding=text_bg_padding_for_scale(abs(a->p0));
             a->text_radius=text_bg_radius_for_scale(abs(a->p0));a->text_box_custom=1;
         }
         if(sym==XK_p && a->text_padding>0)a->text_padding--;
         if(sym==XK_P && a->text_padding<256)a->text_padding++;
         if(sym==XK_c)a->text_radius=a->text_radius>=4?a->text_radius-4:0;
         if(sym==XK_C)a->text_radius=a->text_radius<=252?a->text_radius+4:256;
     }
     if(sym==XK_i)a->italic=!a->italic;
     if(sym==XK_u)a->underline=!a->underline;
     if(sym==XK_s)a->strikethrough=!a->strikethrough;
     changed=1;
 }
 if(changed) {ed->dirty=ed->raster_dirty=1;ed->chrome_key[0]=0;}
 return changed;
}

static int color_selected_palette(struct editor_state *ed,KeySym number,int background)
{
 if(number<XK_1 || number>XK_9 || (size_t)(number-XK_1)>=sizeof palette/sizeof *palette)return 0;
 if(!selected_action(ed))return 0;
 apply_selected_color(ed,palette[number-XK_1],background);return 1;
}

static int edit_selected_text(struct editor_state *ed)
{
 int index=ed->selected_idx;
 if(index<0 || (size_t)index>=ed->actions.cursor || ed->actions.items[index].type!=ACTION_TEXT)return 0;
 struct action *a=&ed->actions.items[index];
 if(strlen(a->text?a->text:"")>=sizeof ed->text_buf)return 0;
 ed->text_edit_style=*a;
 set_tool(ed,TOOL_TEXT);ed->text_mode=1;ed->text_edit_index=index;
 ed->text_x=a->x0;ed->text_y=a->y0;ed->text_scale=abs(a->p0);ed->color=a->color;ed->fill_mode=a->p0<0;
 snprintf(ed->text_buf,sizeof ed->text_buf,"%s",a->text?a->text:"");
 ed->text_len=strlen(ed->text_buf);ed->text_cursor=ed->text_anchor=ed->text_len;
 ed->dirty=ed->raster_dirty=1;return 1;
}

static int
handle_keypress(struct editor_state *ed, XKeyEvent *kev)
{
	KeySym sym;
	char kbuf[8];
	int kn;
	sym = XLookupKeysym(kev, 0);
 if(sym==XK_Control_L || sym==XK_Control_R) {ed->control_down=1;update_cursor(ed);return 1;}
	kn = XLookupString(kev, kbuf, (int)sizeof(kbuf), &sym, NULL);

 if(sym==XK_Escape && ed->show_about){ed->show_about=0;return 1;}
 if(sym==XK_Escape && (!ed->cfg || ed->cfg->normal_window)) {
     clear_text_mode(ed);reset_pen_input(ed);sel_clear(ed);
     ed->tool=TOOL_SELECT;ed->popup=ed->show_help=ed->show_about=ed->color_mode=ed->picker_active=0;
     ed->anchor_active=ed->marquee_active=ed->drag_active=ed->rotating=ed->resizing=ed->curving=0;
     ed->mouse_b1_down=ed->mouse_anchor_set_on_press=ed->mouse_drag_moved=0;
     ed->drag_dx=ed->drag_dy=0;ed->panning=0;ed->transform_preview=0;
     ed->dirty=ed->raster_dirty=1;return 1;
 }
 if(ed->show_about) {
     if(sym==XK_Escape || sym==XK_q || sym==XK_question){ed->show_about=0;return 1;}
     if(sym==XK_Tab || sym==XK_ISO_Left_Tab){ed->about_focus=ed->about_focus==1?2:1;return 1;}
     if(sym==XK_Return || sym==XK_KP_Enter || sym==XK_space){if(ed->about_focus==1)open_about_repo(ed);else ed->show_about=0;}
     return 1;
 }
 if(ed->popup && sym==XK_Escape){ed->popup=0;return 1;}
 if(ed->color_mode)return handle_color_mode(ed,kev);
 if(ed->show_help){if(sym==XK_Escape || sym==XK_question || (kn>0 && kbuf[0]=='?'))ed->show_help=0;return 1;}
	if ((kev->state & ControlMask) && (sym == XK_c || sym == XK_C)) {
		finalize_pending_anchor(ed);
		ed->should_copy = 1;
		ed->running = 0;
		return 1;
	}

	if ((kev->state & ControlMask) && (sym == XK_s || sym == XK_S)) {
		finalize_pending_anchor(ed);
		ed->should_save = 1;
		ed->save_timestamp = !(ed->cfg && ed->cfg->output_path_explicit);
		ed->running = 0;
		return 1;
	}

	if (ed->text_mode) {
		return handle_text_mode(ed, kev);
	}

 if((kev->state&S2_THEME_MODIFIER)==S2_THEME_MODIFIER) {
     if(sym>=XK_1 && sym<=XK_9) {set_theme(ed,sym-XK_1);return 1;}
     if(sym==XK_c || sym==XK_C) {ed->popup=ed->popup==3?0:3;return 1;}
 }
	if (!(kev->state & (ControlMask | Mod1Mask))) {
		if (sym == XK_plus || sym == XK_KP_Add) {
			set_zoom(ed, ed->scale * 1.25f);
			return 1;
		}
		if (sym == XK_minus || sym == XK_KP_Subtract) {
			set_zoom(ed, ed->scale / 1.25f);
			return 1;
		}
		if (sym == XK_equal || sym == XK_KP_Equal || sym == XK_0) {
			set_zoom(ed, 0);
			return 1;
		}
	}

	if (sym == XK_question || (kn > 0 && kbuf[0] == '?')) {
		ed->show_help = !ed->show_help;ed->show_about=0;
 ed->popup=0;
		return 1;
	}
	if (ed->suppress_escape_once && sym == XK_Escape) {
		ed->suppress_escape_once = 0;
		return 1;
	}
	ed->suppress_escape_once = 0;
	if (ed->color_mode) {
		return handle_color_mode(ed, kev);
	}


	if ((kev->state & ControlMask) && (sym == XK_y || sym == XK_Y)) {
		finalize_pending_anchor(ed);
 if(ed->dirty) rerender_from_actions(ed);
 clipboard_copy_png(&ed->rendered);
		return 1;
	}
	if ((kev->state & ControlMask) && (sym == XK_z || sym == XK_Z)) {
		if (kev->state & ShiftMask) {
			if (redo_action(&ed->actions) == 0) {
				ed->dirty = 1;
				ed->raster_dirty = 1;
			}
		} else {
			if (undo_action(&ed->actions) == 0) {
				ed->dirty = 1;
				ed->raster_dirty = 1;
			}
		}
		ed->anchor_active = 0;
		sel_clear(ed);
		ed->marquee_active = 0;
		return 1;
	}
	if ((kev->state & ControlMask) && (sym == XK_v || sym == XK_V)) {
		return paste_text_from_clipboard(ed);
	}

	if (sym == XK_Return || sym == XK_KP_Enter) return 1;
 if(sym==XK_Escape && ed->picker_active){ed->picker_active=0;return 1;}
 if(sym==XK_Escape && ed->popup){ed->popup=0;return 1;}
 if (sym == XK_q || sym == XK_Escape) {
		ed->cancelled = 1;
		ed->running = 0;
		return 1;
	}

if(!(kev->state&(ControlMask|Mod1Mask|Mod4Mask))) {
     if((kev->state&ShiftMask) && color_selected_palette(ed,XLookupKeysym(kev,0),1))return 1;
     if(!(kev->state&ShiftMask) && color_selected_palette(ed,sym,0))return 1;
     if(style_selected_text(ed,sym))return 1;
     if(style_selected_arrow(ed,sym))return 1;
 }
	if (sym == XK_a) {
		set_tool(ed, TOOL_ARROW);
		return 1;
	}
	if (sym == XK_l) {
		set_tool(ed, TOOL_LINE);
		return 1;
	}
	if (sym == XK_p) {
		set_tool(ed, TOOL_PEN);
		return 1;
	}
	if (sym == XK_n) {
		set_tool(ed, TOOL_NUMBER);
		return 1;
	}
	if (sym == XK_r) {
		set_tool(ed, TOOL_RECT);
		return 1;
	}
	if (sym == XK_s) {
		set_tool(ed, TOOL_SELECT);
		return 1;
	}
	if (sym == XK_o || sym == XK_O) {
		set_tool(ed, TOOL_CIRCLE);
		return 1;
	}
	if (sym == XK_t) {
        if(edit_selected_text(ed))return 1;
		set_tool(ed, TOOL_TEXT);
		return 1;
	}
	if (sym == XK_h) {
		set_tool(ed, TOOL_HIGHLIGHT);
		return 1;
	}
	if (sym == XK_b) {
		set_tool(ed, TOOL_BLUR);
		return 1;
	}
	if (sym == XK_m) {
		set_tool(ed, TOOL_MARKER);
		return 1;
	}
	if (sym == XK_x) {
		set_tool(ed, TOOL_PIXELATE);
		return 1;
	}
	if (sym == XK_c) {
        ed->color_target_fill=0;
        if(selected_action(ed)){ed->picker_active=1;return 1;}
		set_tool(ed, TOOL_PICKER);
		return 1;
	}

	if (sym == XK_Left) {
		move_cursor(ed, -1, 0);
		return 1;
	}
	if (sym == XK_Right) {
		move_cursor(ed, 1, 0);
		return 1;
	}
	if (sym == XK_Up) {
		move_cursor(ed, 0, -1);
		return 1;
	}
	if (sym == XK_Down) {
		move_cursor(ed, 0, 1);
		return 1;
	}

	if (sym == XK_space) {
		commit_current_tool(ed);
		return 1;
	}
	if(sym==XK_g || sym==XK_G){toggle_gradient_mode(ed);return 1;}
 if(sym==XK_F){toggle_outline_mode(ed);return 1;}
	if (sym == XK_f) {
		toggle_fill_mode(ed);
		return 1;
	}

	if (sym == XK_BackSpace || sym == XK_Delete) {
		delete_selected(ed);
		return 1;
	}

 if(sym==XK_bracketleft || sym==XK_bracketright) {
 adjust_tool_size(ed,sym==XK_bracketright?1:-1); return 1;
 }

	if (sym == XK_comma) {
		rotate_selected_by(ed, -15);
		return 1;
	}
	if (sym == XK_period) {
		rotate_selected_by(ed, 15);
		return 1;
	}
	if (sym == XK_less) {
		rotate_selected_by(ed, -1);
		return 1;
	}
	if (sym == XK_greater) {
		rotate_selected_by(ed, 1);
		return 1;
	}

	if (sym >= XK_1 && sym <= XK_9) {
		size_t idx = (size_t)(sym - XK_1);
		size_t n = sizeof(palette) / sizeof(palette[0]);
		if (idx < n) {
			ed->color = palette[idx];
		}
		return 1;
	}

	if (sym == XK_numbersign) {
        ed->color_target_fill=0;
		ed->color_mode = 1;
		ed->color_len = 0;
		ed->color_buf[0] = '\0';
		return 1;
	}

	if (sym == XK_X) {
		ed->anchor_active = 0;
		reset_pen_input(ed);
		if (ed->text_mode) {
			clear_text_mode(ed);
		}
		ed->raster_dirty = 1;
		return 1;
	}

	return 0;
}

void editor_print_licenses(void)
{
 puts("Geist variable fonts — SIL Open Font License 1.1");
 puts(geist_license);
}
static int load_ui_fonts(void)
{
 /* Keep anonymous font files open: Fontconfig/Pango may open a face lazily. */
 static int loaded;
 static int font_fds[2]={-1,-1};
 if(loaded)return 0;
 const unsigned char *data[]={geist_regular,geist_italic};
 const size_t sizes[]={sizeof geist_regular,sizeof geist_italic};
 for(int i=0;i<2;i++) {
     int fd=syscall(SYS_memfd_create,"s2-geist",1u);
     if(fd<0)return -1;
     size_t offset=0;
     while(offset<sizes[i]) {
         ssize_t count=write(fd,data[i]+offset,sizes[i]-offset);
         if(count<0 && errno==EINTR)continue;
         if(count<=0) {close(fd);return -1;}
         offset+=(size_t)count;
     }
     char path[64];snprintf(path,sizeof path,"/proc/self/fd/%d",fd);
     if(!FcConfigAppFontAddFile(NULL,(const FcChar8 *)path)) {close(fd);return -1;}
     font_fds[i]=fd;
 }
 (void)font_fds;
 FcConfigBuildFonts(NULL);loaded=1;return 0;
}

static Cursor hand_cursor(struct editor_state *ed,const unsigned char *bits,const unsigned char *mask)
{
 Pixmap source=XCreateBitmapFromData(ed->dpy,ed->win,(const char *)bits,32,32);
 Pixmap outline=XCreateBitmapFromData(ed->dpy,ed->win,(const char *)mask,32,32);
 XColor black={.red=0,.green=0,.blue=0},white={.red=65535,.green=65535,.blue=65535};
 Cursor cursor=XCreatePixmapCursor(ed->dpy,source,outline,&black,&white,16,16);
 XFreePixmap(ed->dpy,source);XFreePixmap(ed->dpy,outline);return cursor;
}

static int x11_setup(struct editor_state *ed)
{
	Visual *vis;
	int depth;

	if(load_ui_fonts()!=0) {fprintf(stderr,"s2: cannot load embedded fonts\n");return -1;}
	ed->dpy = XOpenDisplay(NULL);
	if (!ed->dpy) {
		fprintf(stderr, "s2: cannot open X display\n");
		return -1;
	}
	ed->screen = DefaultScreen(ed->dpy);
	depth = DefaultDepth(ed->dpy, ed->screen);
	vis = DefaultVisual(ed->dpy, ed->screen);
	ed->cmap = DefaultColormap(ed->dpy, ed->screen);
	/* The annotation workspace intentionally uses a stable dark palette. */
	{
		set_theme(ed, S2_DEFAULT_THEME);
		ed->status_bg = 0x000000ul;
		ed->status_fg = 0xfffffful;
	}

	{
		XSizeHints hints = {0};
		int min_w = 400;
		int min_h = 320;
		int max_w =
		    DisplayWidth(ed->dpy, ed->screen) - window_padding * 2;
		int max_h =
		    DisplayHeight(ed->dpy, ed->screen) - window_padding * 2;
		int pref_w = ed->img->width + 120;
        if(pref_w<720)pref_w=720;
		int pref_h = ed->img->height + ed->status_h + ed->status_pad +
			     window_padding * 2;
		int win_w;
		int win_h;
        if(pref_h<700)pref_h=700;
		if (max_w < min_w) {
			max_w = min_w;
		}
		if (max_h < min_h) {
			max_h = min_h;
		}
		win_w = pref_w > min_w ? pref_w : min_w;
		win_h = pref_h > min_h ? pref_h : min_h;
		if (win_w > max_w) {
			win_w = max_w;
		}
		if (win_h > max_h) {
			win_h = max_h;
		}
		ed->win_w = win_w;
		ed->win_h = win_h;
		ed->win = XCreateSimpleWindow(ed->dpy,
	                              RootWindow(ed->dpy, ed->screen),
	                              0,
	                              0,
	                              (unsigned int)win_w,
	                              (unsigned int)win_h,
	                              0,
	                              BlackPixel(ed->dpy, ed->screen),
	                              ed->window_bg);
		hints.flags = PMinSize;
        if(ed->cfg && !ed->cfg->normal_window) {
            hints.flags |= USPosition | PWinGravity;
            hints.x=(DisplayWidth(ed->dpy,ed->screen)-win_w)/2;
            hints.y=(DisplayHeight(ed->dpy,ed->screen)-win_h)/2;
            hints.win_gravity=CenterGravity;
            XMoveWindow(ed->dpy,ed->win,hints.x,hints.y);
        }
		hints.min_width = min_w;
		hints.min_height = min_h;
		XSetWMNormalHints(ed->dpy, ed->win, &hints);
	}
	set_wm_class(ed->dpy,
	             ed->win,
	             (ed->cfg && ed->cfg->window_class && *ed->cfg->window_class) ? ed->cfg->window_class :
	                                                                                 "s2");
	set_wm_window_type(ed->dpy, ed->win, ed->cfg && ed->cfg->normal_window);
    if(ed->cfg && !ed->cfg->normal_window) {
        Atom state=XInternAtom(ed->dpy,"_NET_WM_STATE",False);
        Atom modal=XInternAtom(ed->dpy,"_NET_WM_STATE_MODAL",False);
        XChangeProperty(ed->dpy,ed->win,state,XA_ATOM,32,PropModeReplace,(unsigned char *)&modal,1);
    }
	ed->scale = 1.0f;
	ed->canvas_x = 0;
	ed->canvas_y = 0;
	ed->canvas_w = ed->img->width;
	ed->canvas_h = ed->img->height;
	ed->view_x = 0;
	ed->view_y = 0;
	recalc_layout(ed);
	XStoreName(ed->dpy, ed->win, "s2");
	XSelectInput(ed->dpy,
	             ed->win,
	             ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
	                 PointerMotionMask);
	ed->wm_protocols=XInternAtom(ed->dpy,"WM_PROTOCOLS",False);
	ed->wm_delete_window=XInternAtom(ed->dpy,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(ed->dpy,ed->win,&ed->wm_delete_window,1);
	XMapRaised(ed->dpy, ed->win);
	ed->backbuf = XCreatePixmap(ed->dpy,
	                           ed->win,
	                           (unsigned int)ed->win_w,
	                           (unsigned int)ed->win_h,
	                           (unsigned int)depth);
	ed->cursor = XCreateFontCursor(ed->dpy, XC_crosshair);
	ed->cursor_text = XCreateFontCursor(ed->dpy, XC_xterm);
 ed->cursor_hand=XCreateFontCursor(ed->dpy,XC_hand2);
 ed->cursor_grab=hand_cursor(ed,grab_cursor_bits,grab_cursor_mask);
 ed->cursor_pan=hand_cursor(ed,pan_cursor_bits,pan_cursor_mask);
 ed->cursor_arrow=XCreateFontCursor(ed->dpy,XC_left_ptr);
 Pixmap rotate_source=XCreateBitmapFromData(ed->dpy,ed->win,(const char *)rotate_cursor_bits,32,32);
 Pixmap rotate_mask=XCreateBitmapFromData(ed->dpy,ed->win,(const char *)rotate_cursor_mask,32,32);
 XColor cursor_fg={.red=0,.green=0,.blue=0},cursor_bg={.red=65535,.green=65535,.blue=65535};
 ed->cursor_rotate=XCreatePixmapCursor(ed->dpy,rotate_source,rotate_mask,&cursor_fg,&cursor_bg,16,16);
 XFreePixmap(ed->dpy,rotate_source);XFreePixmap(ed->dpy,rotate_mask);
 const unsigned int sizing[]={XC_top_left_corner,XC_top_right_corner,XC_bottom_right_corner,XC_bottom_left_corner};
 for(int i=0;i<4;i++)ed->cursor_resize[i]=XCreateFontCursor(ed->dpy,sizing[i]);
	if (ed->cursor) {
		XDefineCursor(ed->dpy, ed->win, ed->cursor);
	}

	ed->gc = XCreateGC(ed->dpy, ed->win, 0, NULL);
	if (!ed->gc) {
		fprintf(stderr, "s2: cannot create graphics context\n");
		return -1;
	}

	if (recreate_ximg(ed, vis, depth) != 0) {
		fprintf(stderr, "s2: cannot create X image\n");
		return -1;
	}

	ed->xftdraw = NULL;
	{
		char pat[192];
		build_font_pattern_primary(pat, sizeof(pat), 14);
		ed->xftfont_status = XftFontOpenName(ed->dpy, ed->screen, pat);
		if (!ed->xftfont_status) {
			build_font_pattern(pat, sizeof(pat), 14);
			ed->xftfont_status = XftFontOpenName(ed->dpy, ed->screen, pat);
		}
	}
	if (!ed->xftfont_status) {
		ed->xftfont_status = XftFontOpenName(ed->dpy, ed->screen, "monospace:size=14");
	}
	if (!ed->xftfont_status) {
		fprintf(stderr, "s2: cannot load Xft font\n");
		return -1;
	}

	XSetLocaleModifiers("");
	ed->xim = XOpenIM(ed->dpy, NULL, NULL, NULL);
	if (ed->xim) {
		ed->xic = XCreateIC(ed->xim,
		                   XNInputStyle,
		                   XIMPreeditNothing | XIMStatusNothing,
		                   XNClientWindow,
		                   ed->win,
		                   XNFocusWindow,
		                   ed->win,
		                   NULL);
	}

	return 0;
}

static void
x11_teardown(struct editor_state *ed)
{
	int i;

	if (!ed) {
		return;
	}
 chrome_cleanup();
 if(ed->profile && ed->frame_count) fprintf(stderr,"s2 profile: frames=%lu avg_ms=%.3f max_ms=%.3f total_ms=%.3f\n",ed->frame_count,ed->frame_ms/ed->frame_count,ed->frame_max_ms,ed->frame_ms);
	for (i = 0; i < 9; i++) {
		if (ed->xftfont_tool[i] && ed->xftfont_tool[i] != ed->xftfont_status) {
			XftFontClose(ed->dpy, ed->xftfont_tool[i]);
			ed->xftfont_tool[i] = NULL;
		}
	}
	if (ed->xftfont_status) {
		XftFontClose(ed->dpy, ed->xftfont_status);
		ed->xftfont_status = NULL;
	}
	if (ed->cursor) {
		XFreeCursor(ed->dpy, ed->cursor);
		ed->cursor = 0;
	}
	if(ed->cursor_rotate)XFreeCursor(ed->dpy,ed->cursor_rotate);
 for(int i=0;i<4;i++)if(ed->cursor_resize[i])XFreeCursor(ed->dpy,ed->cursor_resize[i]);
 if(ed->cursor_arrow){XFreeCursor(ed->dpy,ed->cursor_arrow);ed->cursor_arrow=0;}
 if(ed->cursor_grab)XFreeCursor(ed->dpy,ed->cursor_grab);
 if(ed->cursor_pan)XFreeCursor(ed->dpy,ed->cursor_pan);
 if(ed->cursor_hand) { XFreeCursor(ed->dpy,ed->cursor_hand);ed->cursor_hand=0; }
	if (ed->cursor_text) {
		XFreeCursor(ed->dpy, ed->cursor_text);
		ed->cursor_text = 0;
	}
	if (ed->xic) {
		XDestroyIC(ed->xic);
		ed->xic = NULL;
	}
	if (ed->xim) {
		XCloseIM(ed->xim);
		ed->xim = NULL;
	}
	if (ed->xftdraw) {
		XftDrawDestroy(ed->xftdraw);
		ed->xftdraw = NULL;
	}
	if (ed->backbuf) {
		XFreePixmap(ed->dpy, ed->backbuf);
		ed->backbuf = 0;
	}
	if(ed->source_surface) {cairo_surface_destroy(ed->source_surface);ed->source_surface=NULL;}
 if(ed->source_pixmap) {XFreePixmap(ed->dpy,ed->source_pixmap);ed->source_pixmap=0;}
 if(ed->chromebuf) {XFreePixmap(ed->dpy,ed->chromebuf);ed->chromebuf=0;}
 if(ed->canvasbuf) { XFreePixmap(ed->dpy,ed->canvasbuf);ed->canvasbuf=0; }
	if (ed->ximg) {
		XDestroyImage(ed->ximg);
		ed->ximg = NULL;
	}
	if (ed->gc) {
		XFreeGC(ed->dpy, ed->gc);
		ed->gc = 0;
	}
	if (ed->win) {
		XDestroyWindow(ed->dpy, ed->win);
		ed->win = 0;
	}
	if (ed->dpy) {
		XCloseDisplay(ed->dpy);
		ed->dpy = NULL;
	}
	image_free(&ed->base);
	image_free(&ed->rendered);
	image_free(&ed->preview);
	free(ed->pen_points);
	ed->pen_points = NULL;
	ed->pen_len = 0;
	ed->pen_cap = 0;
	free(ed->sel);
	ed->sel = NULL;
	ed->sel_len = 0;
	ed->sel_cap = 0;
	free_actions(&ed->actions);
}

static void
set_cursor_from_xy(struct editor_state *ed, int x, int y)
{
	ed->cursor_x = canvas_to_image_x(ed, x);
	ed->cursor_y = canvas_to_image_y(ed, y);
	if (ed->cursor_x < 0) {
		ed->cursor_x = 0;
	}
	if (ed->cursor_y < 0) {
		ed->cursor_y = 0;
	}
	if (ed->cursor_x >= ed->img->width) {
		ed->cursor_x = ed->img->width - 1;
	}
	if (ed->cursor_y >= ed->img->height) {
		ed->cursor_y = ed->img->height - 1;
	}
}

static void
rotate_selected_by(struct editor_state *ed, int delta)
{
	struct action *a;
	int deg;

	if (ed->selected_idx < 0 || (size_t)ed->selected_idx >= ed->actions.cursor) {
		return;
	}
	a = &ed->actions.items[ed->selected_idx];
	if (!action_is_rotatable(a->type)) {
		return;
	}
	deg = (a->rotation + delta) % 360;
	if (deg < 0) {
		deg += 360;
	}
	a->rotation = deg;
	ed->dirty = 1;
	ed->raster_dirty = 1;
}

static void
rotate_selected_to_pointer(struct editor_state *ed, int sx, int sy, int snap)
{
	struct action *a;
	double cx;
	double cy;
	double scx;
	double scy;
	double ang;
	int deg;

	if (ed->selected_idx < 0 ||
	    (size_t)ed->selected_idx >= ed->actions.cursor) {
		return;
	}
	a = &ed->actions.items[ed->selected_idx];
	if (!action_is_rotatable(a->type)) {
		return;
	}
	action_center(ed, a, &cx, &cy);
	scx = ed->canvas_x + cx * ed->scale;
	scy = ed->canvas_y + cy * ed->scale;
	ang = atan2((double)sy - scy, (double)sx - scx) * 180.0 / M_PI + 90.0;
	if (snap) {
		deg = (int)lround(ang / 45.0) * 45;
	} else {
		deg = (int)lround(ang);
	}
	deg %= 360;
	if (deg < 0) {
		deg += 360;
	}
	a->rotation = deg;
	if(!ed->transform_preview)ed->dirty=ed->raster_dirty=1;
}

static void resize_selected_to_pointer(struct editor_state *ed, int sx, int sy, int preserve)
{
 if(ed->selected_idx<0 || (size_t)ed->selected_idx>=ed->actions.cursor)return;
 struct action original=ed->resize_original;
 struct action *a=&ed->actions.items[ed->selected_idx];
 int x0,y0,x1,y1;action_bounds(ed,&original,&x0,&y0,&x1,&y1);
 double width=fmax(1,x1-x0),height=fmax(1,y1-y0);
 double cx=(x0+x1)/2.,cy=(y0+y1)/2.;
 double rad=original.rotation*M_PI/180.,c=cos(rad),s=sin(rad);
 double px=(sx-ed->canvas_x)/ed->scale-cx,py=(sy-ed->canvas_y)/ed->scale-cy;
 double lx=c*px+s*py+cx,ly=-s*px+c*py+cy;
 int right=ed->resize_corner==1 || ed->resize_corner==2;
 int bottom=ed->resize_corner>=2;
 double fx=right?x0:x1,fy=bottom?y0:y1;
 double scale_x=fmax(.02,(right?lx-fx:fx-lx)/width);
 double scale_y=fmax(.02,(bottom?ly-fy:fy-ly)/height);
 scale_x=fmin(scale_x,16);scale_y=fmin(scale_y,16);
 if(preserve || original.type==ACTION_TEXT || original.type==ACTION_NUMBER)
     scale_x=scale_y=fmax(scale_x,scale_y);
 if(original.type==ACTION_TEXT) {
     int size=abs(original.p0),next=(int)lround(size*scale_x);
     if(next<1)next=1;
     if(next>max_text_scale)next=max_text_scale;
     scale_x=scale_y=(double)next/(size?size:1);
 }
 double ncx=fx+(cx-fx)*scale_x,ncy=fy+(cy-fy)*scale_y;
 double tx=c*(ncx-cx)-s*(ncy-cy)+cx-ncx;
 double ty=s*(ncx-cx)+c*(ncy-cy)+cy-ncy;
 *a=original;
 a->x0=lround(fx+(original.x0-fx)*scale_x+tx);
 a->y0=lround(fy+(original.y0-fy)*scale_y+ty);
 if(original.type==ACTION_TEXT) {
     a->p0=lround(original.p0*scale_x);
 } else if(original.type==ACTION_NUMBER) {
     a->p0=fmax(1,lround(original.p0*scale_x));
     a->y1=fmax(1,lround(original.y1*scale_x));
 } else {
     a->x1=lround(fx+(original.x1-fx)*scale_x+tx);
     a->y1=lround(fy+(original.y1-fy)*scale_y+ty);
 }
 if(!ed->transform_preview)ed->dirty=ed->raster_dirty=1;
}

static void handle_button_press(struct editor_state *ed, XButtonEvent *bev)
{
 ed->pointer_x=bev->x;ed->pointer_y=bev->y;
 if((bev->state&ControlMask) && !(ed->show_help || ed->show_about) && !ed->popup && chrome_hit(ed,bev->x,bev->y)<0 && bev->y>=toolbar_bottom(ed) && bev->y<ed->win_h-footer_height(ed)-16) {
     ed->control_down=1;
     if(bev->button==Button4 || bev->button==Button5) {
         set_zoom_at(ed,ed->scale*(bev->button==Button4?1.15f:1/1.15f),bev->x,bev->y);
         update_cursor(ed);return;
     }
     if(bev->button==Button1) {
         ed->panning=1;ed->pan_pointer_x=bev->x;ed->pan_pointer_y=bev->y;
         ed->pan_view_x=ed->view_x;ed->pan_view_y=ed->view_y;
         update_cursor(ed);return;
     }
 }

	if (bev->button != Button1) {
		return;
	}
 if(ed->show_about){int hit=about_hit(ed,bev->x,bev->y);if(hit==-1 || hit==2)ed->show_about=0;else if(hit==1){ed->about_focus=1;open_about_repo(ed);}return;}
 if(popup_click(ed,bev->x,bev->y))return;
 if(ed->show_help){cui_rect r=help_rect(ed);if(bev->x<r.x || bev->x>r.x+r.w || bev->y<r.y || bev->y>r.y+r.h || (bev->x>=r.x+r.w-62 && bev->y<r.y+60))ed->show_help=0;return;}
	int hit = chrome_hit(ed, bev->x, bev->y);
	if (hit >= 0) {
		if (hit < 13) {
			set_tool(ed, tool_by_index(hit));
			return;
		}
		switch (hit) {
 case 25:ed->show_about=1;ed->show_help=ed->popup=0;ed->about_focus=2;break;
 case 24:ed->popup=ed->popup==3?0:3;break;
 case 13: {int menu=(bev->state&S2_THEME_MODIFIER)?3:1;
     int fill=selection_supports_fill(ed) && (bev->state&ShiftMask);
     if(menu==1 && ed->popup==1 && ed->color_target_fill!=fill){ed->color_target_fill=fill;break;}
     if(menu==1)ed->color_target_fill=fill;
     ed->popup=ed->popup==menu?0:menu;break;}
 case 14: ed->popup=ed->popup==2?0:2;break;
 case 21: toggle_outline_mode(ed);break;
		case 23: toggle_fill_mode(ed);break;
 case 15:
			finalize_pending_anchor(ed);
			ed->should_save = 1;
			ed->save_timestamp = !(ed->cfg && ed->cfg->output_path_explicit);
			ed->running = 0;
			break;
		case 16:
			finalize_pending_anchor(ed);
			ed->should_copy = 1;
			ed->running = 0;
			break;
		case 17:
			set_zoom(ed, ed->scale / 1.25f);
			break;
		case 18:
			set_zoom(ed, ed->scale * 1.25f);
			break;
		case 19:
			set_zoom(ed, 0);
			break;
		case 20:
			ed->show_help = !ed->show_help;ed->show_about=0;
 ed->popup=0;
			break;
		}
		return;
	}
	if (bev->y < TOOLBAR_Y + TOOLBAR_HEIGHT * toolbar_rows(ed) * chrome_scale(ed) + 8 ||
	    bev->y >= ed->win_h - footer_height(ed) - 16)
		return;
 /* Handles can extend beyond the capture; hit-test them before canvas bounds. */
 if(ed->picker_active) {
     if(bev->x>=ed->canvas_x && bev->x<ed->canvas_x+ed->canvas_w && bev->y>=ed->canvas_y && bev->y<ed->canvas_y+ed->canvas_h) {
         set_cursor_from_xy(ed,bev->x,bev->y);
         apply_picker_color(ed,draw_sample_color(&ed->rendered,ed->cursor_x,ed->cursor_y));
         ed->picker_active=0;
     }
     return;
 }
 if(ed->tool==TOOL_SELECT) {
     if(curve_handle_hit(ed,bev->x,bev->y)) {
         ed->curving=1;ed->mouse_b1_down=1;ed->drag_active=0;
         ed->transform_preview=1;ed->dirty=ed->raster_dirty=1;
         update_cursor(ed);return;
     }
     int corner=resize_handle_hit(ed,bev->x,bev->y);
     if(corner>=0 || rotation_handle_hit(ed,bev->x,bev->y)) {
         ed->mouse_b1_down=1;ed->drag_active=0;
         ed->transform_preview=1;ed->dirty=ed->raster_dirty=1;
         if(corner>=0) {
             ed->resizing=1;ed->resize_corner=corner;
             ed->resize_original=ed->actions.items[ed->selected_idx];
         } else ed->rotating=1;
         update_cursor(ed);return;
     }
 }
	if (bev->x < ed->canvas_x || bev->x >= ed->canvas_x + ed->canvas_w ||
	    bev->y < ed->canvas_y || bev->y >= ed->canvas_y + ed->canvas_h)
		return;
	set_cursor_from_xy(ed, bev->x, bev->y);
	if (ed->tool == TOOL_TEXT && ed->text_mode) {
		commit_text_input(ed);
		ed->mouse_b1_down = 0;
		return;
	}
	ed->mouse_b1_down = 1;
	ed->mouse_drag_moved = 0;
	if (ed->tool == TOOL_SELECT) {
		int hit;
		if (rotation_handle_hit(ed, bev->x, bev->y)) {
			ed->rotating = 1;
			ed->drag_active = 0;
			ed->drag_dx = 0;
			ed->drag_dy = 0;
			return;
		}
		hit = find_action_at(ed, ed->cursor_x, ed->cursor_y);
		if (hit >= 0) {
			/* clicking an unselected object replaces the selection;
			 * clicking one already in the set keeps the whole group
			 */
			if (!sel_contains(ed, hit)) {
				sel_set_single(ed, hit);
			}
			ed->drag_active = 1;
			ed->drag_origin_x = ed->cursor_x;
			ed->drag_origin_y = ed->cursor_y;
			ed->drag_dx = 0;
			ed->drag_dy = 0;
		} else {
			/* empty space: start a rubber-band marquee selection */
			sel_clear(ed);
			ed->drag_active = 0;
			ed->marquee_active = 1;
			ed->marquee_x = ed->cursor_x;
			ed->marquee_y = ed->cursor_y;
		}
		return;
	}
	if (ed->tool == TOOL_PEN || ed->tool == TOOL_MARKER) {
		ed->pen_active = 1;
		ed->pen_is_marker = (ed->tool == TOOL_MARKER);
		ed->pen_color = (int)ed->color;
		ed->pen_thickness = ed->pen_is_marker ? (ed->marker_height > 0 ? ed->marker_height : 1)
		                                       : thickness_presets[ed->thickness_idx];
		ed->pen_len = 0;
		if (append_pen_point(ed, ed->cursor_x, ed->cursor_y) != 0) {
			fprintf(stderr, "s2: out of memory for pen tool\n");
			reset_pen_input(ed);
		}
		ed->raster_dirty = 1;
		return;
	}
	if (ed->tool == TOOL_NUMBER) {
		ed->mouse_anchor_set_on_press = 0;
		commit_current_tool(ed);
		return;
	}
	if (tool_uses_anchor(ed->tool)) {
		if (!ed->anchor_active) {
			if (commit_current_tool(ed) == 0) {
				ed->mouse_anchor_set_on_press = 1;
			}
		} else {
			ed->mouse_anchor_set_on_press = 0;
		}
		return;
	}
	ed->mouse_anchor_set_on_press = 0;
	commit_current_tool(ed);
}

/* Apply the same endpoint to both the preview and committed geometry. */
static void
constrain_shape_cursor(struct editor_state *ed, unsigned int state)
{
	if (!(state & ShiftMask) || !ed->anchor_active ||
	    !tool_uses_anchor(ed->tool))
		return;
	int dx = ed->cursor_x - ed->anchor_x;
	int dy = ed->cursor_y - ed->anchor_y;
	int ax = abs(dx), ay = abs(dy);
	if (ed->tool == TOOL_LINE || ed->tool == TOOL_ARROW) {
		if (ax > ay * 2) { ed->cursor_y = ed->anchor_y; return; }
		if (ay > ax * 2) { ed->cursor_x = ed->anchor_x; return; }
	}
	int side = ax > ay ? ax : ay;
	/* Keep a square inside the capture even when dragging near an edge. */
	int room_x = dx < 0 ? ed->anchor_x : ed->img->width - 1 - ed->anchor_x;
	int room_y = dy < 0 ? ed->anchor_y : ed->img->height - 1 - ed->anchor_y;
	if (side > room_x) side = room_x;
	if (side > room_y) side = room_y;
	ed->cursor_x = ed->anchor_x + (dx < 0 ? -side : side);
	ed->cursor_y = ed->anchor_y + (dy < 0 ? -side : side);
}

static void
handle_button_release(struct editor_state *ed, XButtonEvent *bev)
{
 if(ed->panning && bev->button==Button1) {pan_to(ed,bev->x,bev->y);ed->panning=0;update_cursor(ed);return;}
	if (bev->button != Button1) {
		return;
	}

	set_cursor_from_xy(ed, bev->x, bev->y);
	constrain_shape_cursor(ed, bev->state);
if(ed->curving) {
     curve_to_pointer(ed,bev->x,bev->y,bev->state);
     ed->curving=0;ed->mouse_b1_down=0;ed->transform_preview=0;ed->dirty=ed->raster_dirty=1;return;
 }
 if(ed->resizing) {
     resize_selected_to_pointer(ed,bev->x,bev->y,bev->state&ShiftMask);
     ed->resizing=0;ed->mouse_b1_down=0;ed->transform_preview=0;ed->dirty=ed->raster_dirty=1;return;
 }
 if (ed->rotating) {
     rotate_selected_to_pointer(ed,bev->x,bev->y,bev->state&ShiftMask);
        ed->transform_preview=0;ed->dirty=ed->raster_dirty=1;
		ed->rotating = 0;
		ed->mouse_b1_down = 0;
		ed->raster_dirty = 1;
		return;
	}
	if (!ed->mouse_b1_down) {
		return;
	}
	if (ed->tool == TOOL_SELECT) {
		if (ed->marquee_active) {
			if (ed->mouse_drag_moved) {
				selection_from_marquee(ed, ed->marquee_x, ed->marquee_y, ed->cursor_x, ed->cursor_y);
			}
			ed->marquee_active = 0;
		} else if (ed->drag_active && (ed->drag_dx != 0 || ed->drag_dy != 0)) {
			int k;
			for (k = 0; k < ed->sel_len; k++) {
				int idx = ed->sel[k];
				if (idx >= 0 && (size_t)idx < ed->actions.cursor) {
					action_move_by(&ed->actions.items[idx], ed->drag_dx, ed->drag_dy);
				}
			}
			ed->dirty = 1;
		}
		ed->drag_active = 0;
		ed->drag_dx = 0;
		ed->drag_dy = 0;
		ed->mouse_b1_down = 0;
		ed->mouse_anchor_set_on_press = 0;
		ed->mouse_drag_moved = 0;
		ed->raster_dirty = 1;
		return;
	}
	if (ed->tool == TOOL_PEN || ed->tool == TOOL_MARKER) {
		if (ed->pen_active && ed->pen_len > 0) {
			struct action a;
			memset(&a, 0, sizeof(a));
			a.type = ed->pen_is_marker ? ACTION_MARKER : ACTION_PEN;
			a.color = (unsigned int)ed->pen_color;
			a.p0 = ed->pen_thickness;
			a.pen_points = ed->pen_points;
			a.pen_len = ed->pen_len;
			a.x0 = ed->pen_points[0];
			a.y0 = ed->pen_points[1];
			a.x1 = ed->pen_points[(ed->pen_len - 1) * 2 + 0];
			a.y1 = ed->pen_points[(ed->pen_len - 1) * 2 + 1];
			if (push_action(&ed->actions, &a) == 0) {
                ed->tool=TOOL_SELECT;sel_set_single(ed,ed->actions.cursor-1);
				ed->dirty = 1;
				ed->raster_dirty = 1;
			} else {
				fprintf(stderr, "s2: out of memory for pen action\n");
			}
		}
		reset_pen_input(ed);
		ed->mouse_b1_down = 0;
		ed->mouse_anchor_set_on_press = 0;
		ed->mouse_drag_moved = 0;
		return;
	}
	if (ed->tool == TOOL_TEXT) {
		commit_current_tool(ed);
		ed->mouse_b1_down = 0;
		ed->mouse_anchor_set_on_press = 0;
		ed->mouse_drag_moved = 0;
		return;
	}
	if (tool_uses_anchor(ed->tool) && ed->anchor_active) {
		if (!ed->mouse_anchor_set_on_press || ed->mouse_drag_moved) {
			commit_current_tool(ed);
		}
	}
	ed->mouse_b1_down = 0;
	ed->mouse_anchor_set_on_press = 0;
	ed->mouse_drag_moved = 0;
}

static void
handle_motion(struct editor_state *ed, XMotionEvent *mev)
{
	int oldx;
	int oldy;
 int old_hover=ed->chrome_hover;

 ed->pointer_x=mev->x;ed->pointer_y=mev->y;
 if(ed->show_about){ed->chrome_hover=-1;return;}
 ed->control_down=(mev->state&ControlMask)!=0;
 if(ed->panning) {pan_to(ed,mev->x,mev->y);update_cursor(ed);return;}
 oldx = ed->cursor_x;
 oldy = ed->cursor_y;
 set_cursor_from_xy(ed, mev->x, mev->y);
 constrain_shape_cursor(ed, mev->state);
 if(ed->curving)curve_to_pointer(ed,mev->x,mev->y,mev->state);
 if(ed->resizing)resize_selected_to_pointer(ed,mev->x,mev->y,mev->state&ShiftMask);
 if (ed->tool == TOOL_SELECT && ed->rotating) {
	 rotate_selected_to_pointer(ed, mev->x, mev->y, mev->state & ShiftMask);
 }
 if (ed->tool == TOOL_SELECT && ed->marquee_active) {
	 selection_from_marquee(ed, ed->marquee_x, ed->marquee_y, ed->cursor_x,
				ed->cursor_y);
 }
 if (ed->tool == TOOL_SELECT && ed->drag_active && ed->sel_len > 0) {
	 int dx = ed->cursor_x - ed->drag_origin_x;
	 int dy = ed->cursor_y - ed->drag_origin_y;
	 if (mev->state & ShiftMask) {
		 int adx = dx < 0 ? -dx : dx;
		 int ady = dy < 0 ? -dy : dy;
		 if (adx >= ady) {
			 dy = 0;
		 } else {
			 dx = 0;
		 }
	 }
	 ed->drag_dx = dx;
	 ed->drag_dy = dy;
 }
 if ((ed->tool == TOOL_PEN || ed->tool == TOOL_MARKER) && ed->pen_active) {
	 if (append_pen_point(ed, ed->cursor_x, ed->cursor_y) != 0) {
		 fprintf(stderr, "s2: out of memory for pen tool\n");
		 reset_pen_input(ed);
	 }
 }
 if (ed->mouse_b1_down && (ed->cursor_x != oldx || ed->cursor_y != oldy)) {
	 ed->mouse_drag_moved = 1;
 }
 ed->chrome_hover = chrome_hit(ed, mev->x, mev->y);
 ed->toolbar_hover = ed->chrome_hover < 13 ? ed->chrome_hover : -1;
 if (old_hover != ed->chrome_hover)
	 update_cursor(ed);
}

int editor_run(const struct app_config *cfg, struct image *img)
{
	struct editor_state ed;
	XEvent ev;
	(void)default_window_class;

	if (!cfg || !img || !img->pixels || img->width <= 0 ||
	    img->height <= 0) {
		fprintf(stderr, "s2: invalid editor input\n");
		return -1;
	}

	memset(&ed, 0, sizeof(ed));
	ed.profile = getenv("S2_PROFILE") != NULL;
	ed.cfg = cfg;
	ed.img = img;
	ed.tool = tool_from_config_value(default_tool_index);
	{
		size_t pn = sizeof(palette) / sizeof(palette[0]);
		size_t tn =
		    sizeof(thickness_presets) / sizeof(thickness_presets[0]);
		int pidx = default_palette_index;
		int tidx = default_thickness_index;
		if (pidx < 0 || (size_t)pidx >= pn) {
			pidx = 0;
		}
		if (tidx < 0 || (size_t)tidx >= tn) {
			tidx = 0;
		}
		ed.color = palette[pidx];
		ed.thickness_idx = tidx;
	}
	ed.text_scale = (default_text_scale > 0) ? default_text_scale : 1;
	if (ed.text_scale > max_text_scale) {
		ed.text_scale = max_text_scale;
	}
	ed.pixelate_block = default_pixelate_block;
	if (ed.pixelate_block < 2) {
		ed.pixelate_block = 2;
	}
	if (ed.pixelate_block > 64) {
		ed.pixelate_block = 64;
	}
	ed.blur_radius = default_blur_radius;
	if (ed.blur_radius < 1) {
		ed.blur_radius = 1;
	}
	if (ed.blur_radius > 16) {
		ed.blur_radius = 16;
	}
	ed.highlight_strength = default_highlight_strength;
	if (ed.highlight_strength < 1) {
		ed.highlight_strength = 1;
	}
	if (ed.highlight_strength > 100) {
		ed.highlight_strength = 100;
	}
	ed.marker_height = default_marker_height;
	if (ed.marker_height < 4) {
		ed.marker_height = 4;
	}
	if (ed.marker_height > 400) {
		ed.marker_height = 400;
	}
	ed.fill_mode = default_fill_mode ? 1 : 0;
 ed.gradient_mode=!!S2_DEFAULT_GRADIENT_FILL;ed.outline_mode=!!S2_DEFAULT_SHAPE_OUTLINE;
	ed.status_h = 28;
	ed.status_pad = 10;
	ed.scale = 1.0f;
	ed.cursor_x = img->width / 2;
	ed.cursor_y = img->height / 2;
	ed.running = 1;
	ed.dirty = 1;
	ed.raster_dirty = 1;
	ed.selected_idx = -1;ed.text_edit_index=-1;
	ed.toolbar_hover = -1;
 ed.chrome_hover=-1;
	ed.save_timestamp = 0;
	ed.number_next = 1;

	if (img_alloc_like(&ed.base, img) != 0 ||
	    img_alloc_like(&ed.rendered, img) != 0 ||
	    img_alloc_like(&ed.preview, img) != 0) {
		fprintf(stderr, "s2: out of memory for working buffers\n");
		x11_teardown(&ed);
		return -1;
	}
	img_copy(&ed.base, img);

	if (x11_setup(&ed) != 0) {
		x11_teardown(&ed);
		return -1;
	}
	setlocale(LC_CTYPE, "");
	setlocale(LC_ALL, "");
	if (ed.xic) {
		XSetICFocus(ed.xic);
	}
	update_cursor(&ed);

	render_frame(&ed);
	while (ed.running) {
		XNextEvent(ed.dpy, &ev);
		switch (ev.type) {
		case Expose:
		case ConfigureNotify:
			if (ev.type == ConfigureNotify) {
				Visual *vis;
				int depth;
				vis = DefaultVisual(ed.dpy, ed.screen);
				depth = DefaultDepth(ed.dpy, ed.screen);
				ed.win_w = ev.xconfigure.width;
				ed.win_h = ev.xconfigure.height;
				recalc_layout(&ed);

				if (ed.backbuf) {
					XFreePixmap(ed.dpy, ed.backbuf);
					ed.backbuf = XCreatePixmap(
					    ed.dpy, ed.win,
					    (unsigned int)ed.win_w,
					    (unsigned int)ed.win_h,
					    (unsigned int)DefaultDepth(
						ed.dpy, ed.screen));
				}
				recreate_ximg(&ed, vis, depth);
				ed.raster_dirty = 1;
			}
			render_frame(&ed);
			break;
		case KeyPress:
			handle_keypress(&ed, &ev.xkey);
			render_frame(&ed);
			break;
        case KeyRelease:
            if(XLookupKeysym(&ev.xkey,0)==XK_Control_L || XLookupKeysym(&ev.xkey,0)==XK_Control_R) {
                ed.control_down=0;update_cursor(&ed);XFlush(ed.dpy);
            }
            break;
        case ClientMessage:
            if(ev.xclient.window==ed.win && ev.xclient.message_type==ed.wm_protocols &&
               ev.xclient.format==32 && (Atom)ev.xclient.data.l[0]==ed.wm_delete_window) {
                ed.cancelled=1;ed.running=0;
            }
            break;
		case ButtonPress:
			handle_button_press(&ed, &ev.xbutton);
			render_frame(&ed);
			break;
		case ButtonRelease:
			handle_button_release(&ed, &ev.xbutton);
			render_frame(&ed);
			break;
		case MotionNotify: {
			/* Consume only consecutive motion events: releases/keys
			 * keep their order. Freehand tools retain every point,
			 * but paint only the latest frame. */
			int motion_count=0;double batch_start=monotonic_seconds();
            while (motion_count++<256 && monotonic_seconds()-batch_start<.002 && XPending(ed.dpy)) {
				XEvent next;
				XPeekEvent(ed.dpy, &next);
				if (next.type != MotionNotify ||
				    next.xmotion.window != ed.win)
					break;
				if (ed.pen_active)
					handle_motion(&ed, &ev.xmotion);
				XNextEvent(ed.dpy, &ev);
			}
			handle_motion(&ed, &ev.xmotion);
            ed.motion_only=1;
			render_frame(&ed);
            ed.motion_only=0;
			break;
		}

		default:
			break;
		}
	}

	/* Dismissing the editor is a successful cancellation, without output. */
	if (ed.cancelled) {
		x11_teardown(&ed);
		return 0;
	}

 if(ed.transform_preview) {ed.transform_preview=0;ed.dirty=1;}
	if (ed.dirty) {
		rerender_from_actions(&ed);
	}
	img_copy(img, &ed.rendered);

 if(ed.should_save) {
     char timestamp[PATH_MAX],saved[PATH_MAX];const char *outpath=cfg->output_path;
     int result;
     if(ed.save_timestamp) {
         if(build_timestamp_path(cfg,timestamp,sizeof timestamp)!=0) {x11_teardown(&ed);return -1;}
         result=save_unique_png(img,timestamp,saved,sizeof saved);outpath=saved;
     } else result=image_save_png(img,outpath);
     if(result!=0) {x11_teardown(&ed);return -1;}
     print_saved_abs_path(outpath);
 }

	if (ed.should_copy || cfg->copy_on_finish) {
		if (clipboard_copy_png(img) != 0) {
			x11_teardown(&ed);
			return -1;
		}
	}

	x11_teardown(&ed);
	return 0;
}
