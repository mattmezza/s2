/* Native Pango/Cairo regression coverage, no desktop or X server required. */
#include "../editor.c"
#include <assert.h>
static void check_center(cairo_t *cr, cairo_surface_t *surface,
			 const char *text, float width, float height)
{
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);
	struct chrome_label label = {.bounds = {10, 10, width, height},
				     .color = {255, 255, 255, 255},
				     .size = 13};
	snprintf(label.text, sizeof label.text, "%s", text);
	paint_chrome_label(cr, &label);
	cairo_surface_flush(surface);
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface), minx = 800,
	    maxx = -1, miny = 80, maxy = -1;
	for (int y = 0; y < 80; y++)
		for (int x = 0; x < 800; x++)
			if (data[y * stride + x * 4 + 3] > 8) {
				if (x < minx)
					minx = x;
				if (x > maxx)
					maxx = x;
				if (y < miny)
					miny = y;
				if (y > maxy)
					maxy = y;
			}
	assert(maxx >= minx && maxy >= miny);
	assert(fabs((minx + maxx + 1) / 2.0 - (10 + width / 2)) <= 1.1);
	assert(fabs((miny + maxy + 1) / 2.0 - (10 + height / 2)) <= 1.1);
}
int main(void)
{
 assert(load_ui_fonts()==0);
	FcConfigAppFontAddFile(
	    NULL, (const FcChar8 *)"assets/fonts/Geist-Regular.ttf");
	FcConfigAppFontAddFile(
	    NULL, (const FcChar8 *)"assets/fonts/Geist-Medium.ttf");
	cairo_surface_t *surface =
	    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 800, 80);
	cairo_t *cr = cairo_create(surface);
	for (int i = 0; i < 500; i++) {
		char text[128];
		snprintf(text, sizeof text,
			 "PNG 1920 × 1080 · X %d  Y %d · %d selected", i, i * 3,
			 i % 9);
		check_center(cr, surface, text, 640, 44);
	}
	check_center(cr, surface, "Color picker · C", 146, 34);
	check_center(cr, surface, "Number · N", 146, 34);
	check_center(cr, surface, "Copy & Close", 154, 44);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	struct editor_state ed = {0};
	struct image light = {.width = 600,
			      .height = 180,
			      .len = 600 * 180 * 4},
		     dark = light;
	light.pixels = malloc(light.len);
	dark.pixels = malloc(dark.len);
	assert(light.pixels && dark.pixels);
	memset(light.pixels, 255, light.len);
	memset(dark.pixels, 0, dark.len);
 for(size_t i=3;i<dark.len;i+=4)dark.pixels[i]=255;
	assert(render_text_pango(&ed, &dark, 20, 20, "Smooth text 123", 4,
				 0xffffff, NULL, NULL, NULL, NULL, 0, NULL,
				 NULL) == 0);
	assert(render_text_pango(&ed, &light, 20, 20, "Smooth text 123", 4, 0,
				 NULL, NULL, NULL, NULL, 0, NULL, NULL) == 0);
	int edges = 0;
	for (size_t i = 0; i < dark.len; i += 4) {
		if (dark.pixels[i] > 0 && dark.pixels[i] < 255)
			edges++;
		assert((int)dark.pixels[i] + light.pixels[i] == 255);
	}
	assert(edges > 100);
	free(light.pixels);
	free(dark.pixels);
	append_pasted_text(&ed, "aé🙂");
	assert(g_utf8_validate(ed.text_buf, -1, NULL));
	assert(ed.text_len == 7);
	memset(ed.text_buf, 'a', 254);
	ed.text_buf[254] = 0;
	ed.text_len = 254;
	append_pasted_text(&ed, "🙂");
	assert(ed.text_len == 254 && g_utf8_validate(ed.text_buf, -1, NULL));
	struct editor_state number = {0};
	set_tool(&number, TOOL_NUMBER);
	set_tool_size(&number, 16);
	assert(tool_size(&number) == 16);
	assert(commit_current_tool(&number) == 0 &&
	       number.actions.items[0].x1 == 1);
    assert(number.tool==TOOL_SELECT);
    set_tool(&number,TOOL_NUMBER);
	assert(commit_current_tool(&number) == 0 &&
	       number.actions.items[1].x1 == 1);
	int small_radius = number.actions.items[1].p0;
    set_tool(&number,TOOL_NUMBER);
	set_tool_size(&number, 64);
	assert(commit_current_tool(&number) == 0 &&
	       number.actions.items[2].y1 == 8 &&
	       number.actions.items[2].p0 > small_radius);
	set_tool(&number, TOOL_NUMBER);
	assert(number.number_next == 1);
	assert(commit_current_tool(&number) == 0 &&
	       number.actions.items[3].x1 == 1);
	set_tool(&number,TOOL_NUMBER);
	number.win_w = 1168;
	number.win_h = 790;
	number.popup = 1;
	cui_rect menu = popup_rect(&number);
	assert(popup_click(&number, menu.x + 220, menu.y + 140));
	assert(number.color == 0x5689ef && number.popup == 0);
	number.popup = 2;
	menu = popup_rect(&number);
	assert(popup_click(&number, menu.x + 200, menu.y + 154));
	assert(tool_size(&number) == 64 && number.popup == 0);
	number.outline_mode=1;
	XButtonEvent button = {.button = Button1,
			       .x = toolbar_left(&number) +
				    (776-108) * chrome_scale(&number),
			       .y = TOOLBAR_Y + 32 * chrome_scale(&number)};
	handle_button_press(&number, &button);
	assert(number.fill_mode);
	button.x=toolbar_left(&number)+(740-108)*chrome_scale(&number);
 handle_button_press(&number, &button);
	assert(number.fill_mode && !number.outline_mode);
 number.show_help=1;cui_rect help=help_rect(&number);
 button.x=help.x+help.w-40;button.y=help.y+36;handle_button_press(&number,&button);assert(!number.show_help);
 number.show_help=1;button.x=help.x+60;button.y=help.y+150;handle_button_press(&number,&button);assert(number.show_help);
 button.x=0;button.y=0;handle_button_press(&number,&button);assert(!number.show_help);
 struct image shape_image = {.width=400, .height=300};
 struct editor_state shape = {.img=&shape_image, .anchor_active=1,
     .anchor_x=100, .anchor_y=100};
 enum tool square_tools[]={TOOL_RECT,TOOL_CIRCLE,TOOL_HIGHLIGHT,TOOL_BLUR,TOOL_PIXELATE};
 for(size_t i=0;i<sizeof square_tools/sizeof *square_tools;i++) {
     shape.tool=square_tools[i];
     for(int quadrant=0;quadrant<4;quadrant++) {
         shape.tool=square_tools[i];
         shape.anchor_active=1;
         shape.cursor_x=100+(quadrant&1?-70:70);
         shape.cursor_y=100+(quadrant&2?-30:30);
         constrain_shape_cursor(&shape,0);
         assert(abs(shape.cursor_x-100)==70 && abs(shape.cursor_y-100)==30);
         constrain_shape_cursor(&shape,ShiftMask);
         assert(abs(shape.cursor_x-100)==70 && abs(shape.cursor_y-100)==70);
         assert(commit_current_tool(&shape)==0);
         struct action *a=&shape.actions.items[shape.actions.cursor-1];
         assert(abs(a->x1-a->x0)==abs(a->y1-a->y0));
     }
 }
 shape.tool=TOOL_RECT;shape.anchor_active=1;shape.cursor_x=390;shape.cursor_y=290;
 constrain_shape_cursor(&shape,ShiftMask);
 assert(shape.cursor_x==299 && shape.cursor_y==299);
 free_actions(&shape.actions);
 /* Resizing in local axes keeps the opposite corner fixed even after rotation. */
 struct editor_state transform={.scale=1,.selected_idx=0,.sel_len=1};
 struct action rectangle={.type=ACTION_RECT,.x0=100,.y0=100,.x1=200,.y1=150,.p0=2};
 assert(push_action(&transform.actions,&rectangle)==0);
 for(int angle=0;angle<360;angle+=45) {
     rectangle.rotation=angle;transform.actions.items[0]=rectangle;
     transform.resize_original=rectangle;transform.resize_corner=2;
     double rad=angle*M_PI/180.,c=cos(rad),sn=sin(rad);
     int before[8],after[8],handle[2];
     selection_geometry(&transform,&rectangle,before,handle);
     int px=lround(150+c*150-sn*75),py=lround(125+sn*150+c*75);
     resize_selected_to_pointer(&transform,px,py,1);
     struct action *resized=&transform.actions.items[0];
     assert(abs((resized->x1-resized->x0)-2*(resized->y1-resized->y0))<=2);
     selection_geometry(&transform,resized,after,handle);
     assert(abs(before[0]-after[0])<=2 && abs(before[1]-after[1])<=2);
     rotate_selected_to_pointer(&transform,300,300,1);
     assert(transform.actions.items[0].rotation%45==0);
 }
 transform.actions.items[0]=rectangle;
 transform.resize_original=rectangle;transform.resize_original.rotation=0;
 transform.resize_corner=2;
 resize_selected_to_pointer(&transform,300,175,0);
 assert(transform.actions.items[0].x1==300 && transform.actions.items[0].y1==175);
 free_actions(&transform.actions);
 /* Theme selection changes chrome only; picker uses configured row order. */
 for(size_t i=0;i<THEME_COUNT;i++) {
     set_theme(&number,i);assert(number.window_bg==themes[i].workspace && number.raster_dirty);
 }
 number.popup=3;menu=popup_rect(&number);
 popup_click(&number,menu.x+90,menu.y+44+26+12);
 assert(number.theme==1 && !number.popup);
 set_theme(&number,0);
 /* Pins have a color-derived gradient and smooth transparent edges. */
 struct image pin={.width=160,.height=160,.len=160*160*4};pin.pixels=calloc(1,pin.len);
 struct action badge={.type=ACTION_NUMBER,.x0=80,.y0=80,.x1=123,.y1=4,.p0=40,.color=0x5689ef};
 draw_number_pin(&pin,&badge);
 assert(pin.pixels[(50*160+65)*4+2]>pin.pixels[(110*160+65)*4+2]);
 int partial=0;for(size_t i=3;i<pin.len;i+=4)if(pin.pixels[i]>0 && pin.pixels[i]<255)partial++;
 assert(partial>50);free(pin.pixels);
 /* Fill toggles update the selected object and retain its previous stroke. */
 struct editor_state edit={.selected_idx=0,.sel_len=1,.sel=(int[]){0}};
 struct action fill_rect={.type=ACTION_RECT,.x0=10,.y0=10,.x1=70,.y1=70,.p0=5};
 assert(push_action(&edit.actions,&fill_rect)==0);
 toggle_fill_mode(&edit);assert(edit.actions.items[0].p0<0 && edit.dirty);
 toggle_fill_mode(&edit);assert(edit.actions.items[0].p0==5);
 set_fill_mode(&edit,1);assert(edit.actions.items[0].p0<0);
 free_actions(&edit.actions);
 struct action text_style={.type=ACTION_TEXT,.x0=20,.y0=20,.p0=4,.text="Styled text"};
 assert(push_action(&edit.actions,&text_style)==0);
 assert(style_selected_text(&edit,XK_B) && edit.actions.items[0].font_weight==500);
 assert(style_selected_text(&edit,XK_b) && edit.actions.items[0].font_weight==400);
 for(int i=0;i<20;i++)style_selected_text(&edit,XK_b);
 assert(edit.actions.items[0].font_weight==100);
 for(int i=0;i<20;i++)style_selected_text(&edit,XK_B);
 assert(edit.actions.items[0].font_weight==900);
 for(int i=0;i<2;i++) {
     style_selected_text(&edit,XK_i);style_selected_text(&edit,XK_u);style_selected_text(&edit,XK_s);
     assert(edit.actions.items[0].italic==!i && edit.actions.items[0].underline==!i && edit.actions.items[0].strikethrough==!i);
 }
 int padding=text_bg_padding_for_scale(4),radius=text_bg_radius_for_scale(4);
 style_selected_text(&edit,XK_P);assert(edit.actions.items[0].text_padding==padding+1);
 style_selected_text(&edit,XK_p);assert(edit.actions.items[0].text_padding==padding);
 style_selected_text(&edit,XK_C);assert(edit.actions.items[0].text_radius==radius+4);
 style_selected_text(&edit,XK_c);assert(edit.actions.items[0].text_radius==radius);
 assert(color_selected_palette(&edit,XK_3,1));
 assert(edit.actions.items[0].has_fill_color && edit.actions.items[0].fill_color==palette[2] && edit.actions.items[0].p0<0);
 assert(color_selected_palette(&edit,XK_1,0));
 assert(edit.actions.items[0].color==palette[0] && edit.actions.items[0].fill_color==palette[2]);
 edit.actions.items[0].font_weight=400;
 struct image regular={.width=300,.height=100,.len=300*100*4},styled_image=regular;
 regular.pixels=calloc(1,regular.len);styled_image.pixels=calloc(1,regular.len);
 apply_action(&edit,&regular,&edit.actions.items[0]);
 style_selected_text(&edit,XK_B);style_selected_text(&edit,XK_i);style_selected_text(&edit,XK_u);style_selected_text(&edit,XK_s);
 apply_action(&edit,&styled_image,&edit.actions.items[0]);
 assert(memcmp(regular.pixels,styled_image.pixels,regular.len));
 edit.actions.items[0].color=0xffffff;edit.actions.items[0].has_fill_color=0;
 assert(color_selected_palette(&edit,XK_1,0));
 assert(edit.actions.items[0].fill_color==0x000000 && edit.actions.items[0].has_fill_color);
 free(regular.pixels);free(styled_image.pixels);free_actions(&edit.actions);
 struct image filled={.width=80,.height=80,.len=80*80*4};filled.pixels=calloc(1,filled.len);
 draw_gradient_fill(&filled,10,10,60,60,0,0,0x5689ef,1.,1);
 assert(filled.pixels[(15*80+20)*4]>filled.pixels[(65*80+20)*4]);
 free(filled.pixels);
 struct image zoom_image={.width=1600,.height=1000};
 struct editor_state view={.img=&zoom_image,.win_w=1200,.win_h=900};
 recalc_layout(&view);
 double ix=(420-view.canvas_x)/(double)view.scale,iy=(360-view.canvas_y)/(double)view.scale;
 set_zoom_at(&view,view.scale*1.5f,420,360);
 assert(fabs(view.canvas_x+ix*view.scale-420)<=.6 && fabs(view.canvas_y+iy*view.scale-360)<=.6);
 int origin_x=view.canvas_x,origin_y=view.canvas_y;
 view.pan_pointer_x=420;view.pan_pointer_y=360;view.pan_view_x=view.view_x;view.pan_view_y=view.view_y;
 pan_to(&view,570,280);
 assert(view.canvas_x==origin_x+150 && view.canvas_y==origin_y-80);
 set_zoom(&view,0);assert(view.view_x==0 && view.view_y==0);
 /* The top-right edge is lighter than the bottom-left edge. */
 struct image diagonal={.width=80,.height=80,.len=80*80*4};diagonal.pixels=calloc(1,diagonal.len);
 draw_gradient_fill(&diagonal,0,0,80,80,0,0,0x5689ef,1.,1);
 assert(diagonal.pixels[(5*80+70)*4]>diagonal.pixels[(70*80+5)*4]);
 free(diagonal.pixels);
 struct editor_state typed={.tool=TOOL_TEXT,.text_mode=1,.text_scale=4,.text_edit_index=-1,.color=0x5689ef};
 append_pasted_text(&typed,"Original");commit_text_input(&typed);
 assert(typed.tool==TOOL_SELECT && typed.selected_idx==0 && typed.actions.cursor==1);
 assert(typed.actions.items[0].font_weight==S2_DEFAULT_TEXT_WEIGHT);
 int original_scale=abs(typed.actions.items[0].p0);
 adjust_tool_size(&typed,1);assert(abs(typed.actions.items[0].p0)==original_scale+1 && tool_size(&typed)==(original_scale+1)*TEXT_SCALE_PX);
 adjust_tool_size(&typed,-1);assert(abs(typed.actions.items[0].p0)==original_scale);
 set_tool_size(&typed,64);assert(abs(typed.actions.items[0].p0)==8);
 set_tool_size(&typed,32);assert(abs(typed.actions.items[0].p0)==4);
 typed.actions.items[0].font_weight=600;typed.actions.items[0].italic=1;
 typed.actions.items[0].text_box_custom=1;typed.actions.items[0].text_padding=12;typed.actions.items[0].text_radius=16;
 typed.actions.items[0].has_fill_color=1;typed.actions.items[0].fill_color=0xff0000;
 assert(edit_selected_text(&typed) && typed.text_mode && typed.text_edit_index==0);
 typed.text_anchor=0;typed.text_cursor=typed.text_len;append_pasted_text(&typed,"abc🙂d");
 assert(!strcmp(typed.text_buf,"abc🙂d"));
 typed.text_cursor=typed.text_anchor=3;append_pasted_text(&typed,"é");
 assert(!strcmp(typed.text_buf,"abcé🙂d") && g_utf8_validate(typed.text_buf,-1,NULL));
 typed.text_anchor=3;text_delete_selection(&typed);assert(!strcmp(typed.text_buf,"abc🙂d"));
 commit_text_input(&typed);
 assert(typed.tool==TOOL_SELECT && typed.selected_idx==0 && typed.actions.cursor==1);
 assert(!strcmp(typed.actions.items[0].text,"abc🙂d") && typed.actions.items[0].font_weight==600 && typed.actions.items[0].italic);
 assert(typed.actions.items[0].text_padding==12 && typed.actions.items[0].text_radius==16 && typed.actions.items[0].fill_color==0xff0000);
 assert(edit_selected_text(&typed));append_pasted_text(&typed," discarded");clear_text_mode(&typed);
 assert(!strcmp(typed.actions.items[0].text,"abc🙂d"));
 set_tool(&typed,TOOL_SELECT);sel_set_single(&typed,0);assert(edit_selected_text(&typed));
 typed.text_anchor=0;typed.text_cursor=typed.text_len;text_delete_selection(&typed);commit_text_input(&typed);
 assert(typed.actions.cursor==0 && typed.tool==TOOL_SELECT);
 free_actions(&typed.actions);free(typed.sel);
 assert(default_fill_mode==1 && text_bg_padding_for_scale(4)==12);
 struct editor_state coarse={.tool=TOOL_MARKER,.marker_height=24,.blur_radius=5,.highlight_strength=25};
 adjust_tool_size(&coarse,1);assert(coarse.marker_height==32);
 coarse.tool=TOOL_BLUR;adjust_tool_size(&coarse,1);assert(coarse.blur_radius==8);
 coarse.tool=TOOL_HIGHLIGHT;adjust_tool_size(&coarse,1);assert(coarse.highlight_strength==30);
 char timestamp_name[64];assert(build_timestamp_name(timestamp_name,sizeof timestamp_name)==0);
 assert(strlen(timestamp_name)==28 && timestamp_name[17]=='.' && !strcmp(timestamp_name+24,".png"));
 for(int i=18;i<24;i++)assert(isdigit((unsigned char)timestamp_name[i]));
 char directory[]="/tmp/s2-unique-save-XXXXXX";assert(mkdtemp(directory));
 char base_path[PATH_MAX],saved_path[PATH_MAX];snprintf(base_path,sizeof base_path,"%s/fixed.png",directory);
 FILE *existing=fopen(base_path,"wb");assert(existing);fputs("keep",existing);fclose(existing);
 unsigned char single_pixel[]={12,34,56,255};struct image single={.width=1,.height=1,.len=4,.pixels=single_pixel};
 assert(save_unique_png(&single,base_path,saved_path,sizeof saved_path)==0 && strstr(saved_path,"fixed-1.png"));
 unlink(saved_path);
 existing=fopen(base_path,"rb");char original[5]={0};assert(fread(original,1,4,existing)==4);fclose(existing);assert(!strcmp(original,"keep"));
 unlink(base_path);rmdir(directory);
    for(int width=400;width<=1200;width+=100) {
        struct editor_state responsive={.win_w=width,.win_h=700};
        float scale=chrome_scale(&responsive);
        int left=toolbar_left(&responsive);
        for(int tool=0;tool<tool_count();tool++)
            assert(chrome_hit(&responsive,left+(30+tool*38)*scale,16+32*scale)==tool);
        const int positions[]={640,690,740,776,850,1010};
        const int expected[]={13,14,21,23,15,16};
        for(int i=0;i<6;i++) {
            float x=positions[i]-108,y=32;
            if(toolbar_rows(&responsive)==2){x-=504;y+=64;}
            assert(chrome_hit(&responsive,left+x*scale,16+y*scale)==expected[i]);
        }
    }
    struct image connector_image={.width=400,.height=300,.len=400*300*4};
    connector_image.pixels=calloc(1,connector_image.len);
    struct editor_state connector={.img=&connector_image,.scale=1,.tool=TOOL_ARROW,.color=0x5689ef};
    connector.selected_idx=-1;
    assert(add_simple_action(&connector,ACTION_ARROW,60,120,340,120,5)==0);
    assert(connector.tool==TOOL_SELECT && connector.selected_idx==0);
    struct action *arrow=&connector.actions.items[0];
    assert(arrow->edges==2 && arrow->edge_style==S2_DEFAULT_ARROW_EDGE_STYLE);
    int hx,hy;assert(curve_handle_position(&connector,&hx,&hy) && hx==200 && hy==120);
    curve_to_pointer(&connector,200,190,0);
    assert(fabs(arrow->bend-.5)<.001);
    assert(action_hit_test(&connector,arrow,200,190));
    assert(!action_hit_test(&connector,arrow,200,120));
    double cx,cy;action_center(&connector,arrow,&cx,&cy);assert(cx==200 && cy==120);
    arrow->rotation=90;curve_handle_position(&connector,&hx,&hy);
    assert(hx==130 && hy==120 && action_hit_test(&connector,arrow,hx,hy));
    curve_to_pointer(&connector,110,120,0);assert(fabs(arrow->bend-90./140)<.001);
    curve_to_pointer(&connector,110,120,ShiftMask);assert(arrow->bend==0);
    arrow->rotation=0;arrow->bend=.5;
    for(size_t i=0;i<sizeof arrow_edge_cycle/sizeof *arrow_edge_cycle;i++) {
        assert(style_selected_arrow(&connector,XK_e));
        assert(arrow->edge_style==arrow_edge_cycle[(i+1)%(sizeof arrow_edge_cycle/sizeof *arrow_edge_cycle)]);
        memset(connector_image.pixels,0,connector_image.len);
        apply_action(&connector,&connector_image,arrow);
        assert(connector_image.pixels[(190*400+200)*4+3]>0);
    }
    assert(style_selected_arrow(&connector,XK_E) && arrow->edge_style==arrow_edge_cycle[(sizeof arrow_edge_cycle/sizeof *arrow_edge_cycle)-1]);
    assert(style_selected_arrow(&connector,XK_e) && arrow->edge_style==arrow_edge_cycle[0]);
    arrow->edge_style=99;assert(style_selected_arrow(&connector,XK_e) && arrow->edge_style==arrow_edge_cycle[0]);
    assert(style_selected_arrow(&connector,XK_less) && arrow->edges==3);
    assert(style_selected_arrow(&connector,XK_greater) && arrow->edges==1);
    assert(style_selected_arrow(&connector,XK_less) && arrow->edges==0);
    assert(size_selected_actions(&connector,1,1) && arrow->p0==6);
    assert(size_selected_actions(&connector,-1,1) && arrow->p0==5);
    assert(color_selected_palette(&connector,XK_2,0) && arrow->color==palette[1]);
    assert(color_selected_palette(&connector,XK_3,1) && !arrow->has_fill_color);
    arrow->type=ACTION_RECT;arrow->p0=5;
    assert(apply_selected_color(&connector,0xff8800,1));
    assert(arrow->p0<0 && arrow->outline_thickness==5 && arrow->fill_color==0xff8800);
    assert(apply_selected_color(&connector,0x5689ef,0));
    assert(arrow->color==0x5689ef && arrow->fill_color==0xff8800);
    assert(size_selected_actions(&connector,1,1) && arrow->outline_thickness==8 && arrow->p0<0);
    connector.color_target_fill=1;apply_picker_color(&connector,0x112233);
    assert(arrow->fill_color==0x112233 && arrow->color==0x5689ef);
    connector.color_target_fill=0;apply_picker_color(&connector,0x445566);
    assert(arrow->color==0x445566 && arrow->fill_color==0x112233);
    arrow->type=ACTION_NUMBER;arrow->y1=4;arrow->p0=number_radius_for_scale(4);
    int old_radius=arrow->p0;
    assert(size_selected_actions(&connector,1,1) && arrow->y1==5 && arrow->p0>old_radius);
    arrow->type=ACTION_MARKER;arrow->p0=24;size_selected_actions(&connector,1,1);assert(arrow->p0==32);
    arrow->type=ACTION_BLUR;arrow->p0=5;size_selected_actions(&connector,1,1);assert(arrow->p0==8);
    arrow->type=ACTION_HIGHLIGHT;arrow->p0=25;size_selected_actions(&connector,1,1);assert(arrow->p0==30);
    arrow->type=ACTION_RECT;arrow->x0=60;arrow->y0=60;arrow->x1=340;arrow->y1=240;
    arrow->p0=-1;arrow->no_outline=1;arrow->no_gradient=0;
    connector.gradient_mode=1;toggle_gradient_mode(&connector);
    assert(!connector.gradient_mode && arrow->no_gradient);
    memset(connector_image.pixels,0,connector_image.len);apply_action(&connector,&connector_image,arrow);
    assert(!memcmp(connector_image.pixels+(100*400+100)*4,connector_image.pixels+(200*400+300)*4,4));
    toggle_gradient_mode(&connector);assert(connector.gradient_mode && !arrow->no_gradient);
    memset(connector_image.pixels,0,connector_image.len);apply_action(&connector,&connector_image,arrow);
    assert(memcmp(connector_image.pixels+(100*400+100)*4,connector_image.pixels+(200*400+300)*4,4));
    connector.outline_mode=0;toggle_outline_mode(&connector);
    assert(connector.outline_mode && !arrow->no_outline && arrow->p0<0);
    arrow->outline_thickness=32;assert(action_draw_pad(arrow)>=19);
    toggle_outline_mode(&connector);assert(!connector.outline_mode && arrow->no_outline && arrow->p0<0);
    free_actions(&connector.actions);free(connector.sel);image_free(&connector_image);
    for(int width=400;width<=1200;width+=100) {
        struct editor_state about={.win_w=width,.win_h=320};
        cui_rect card=about_rect(&about);double scale=card.w/560;
        assert(card.x>=15 && card.y>=15 && card.x+card.w<=width-15 && card.y+card.h<=305);
        assert(about_hit(&about,card.x+280*scale,card.y+327*scale)==1);
        assert(about_hit(&about,card.x+510*scale,card.y+40*scale)==2);
        int fx=footer_left(&about),fw=footer_width(&about),fy=about.win_h-footer_height(&about)-16;
        assert(chrome_hit(&about,fx+fw-266,fy+22)==17);
        assert(chrome_hit(&about,fx+fw-64,fy+22)==20);
        assert(chrome_hit(&about,fx+fw-28,fy+22)==25);
    }
	free_actions(&number.actions);
	chrome_cleanup();
	puts("PASS: label centering survives cache churn; text alpha, UTF-8, "
	     "number pins, themes, transforms, selected fill, text styles and gradients are correct");
	return 0;
}
