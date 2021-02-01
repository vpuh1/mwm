#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "config.h"
#include "mwm.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define BORDER_WIDTH 1
#define CLEANMASK(mask)         (mask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

char bg[] = "";

struct {
	GC gc;
	char text_len[9];
	char *text[9];
	int height;
	int space;
} bar;

static void open_display();
static void get_display_resolution();
static void load_font();
static void setup_gc();
static void setup_bar_tags();
static void create_bar();
static void draw_bar();
static void init();
static void run();
static void frame();
static void map_request();
static void destroy_frame();

static int active_tag = 0;
static Display *dpy;
static int screen;
static Window win;
static Window root;
static XWindowAttributes attr;
static int display_width;
static int display_height;
static int tag_width[9];
static int tag_width_sum = 0;
static int tag_x[9];
static int tag_y;
static XFontStruct *font;
static GC title_gc;
static GC bg_gc;
static int prev_tag = -1;
static Cursor default_cursor, resize_cursor, move_cursor;
static int num_clients = 0;
static XButtonEvent start;
static int ws_num = 0;
static Window focused_window[10];

static void init_focused_windows() {
	for(int i = 0; i < 10; i++){
		focused_window[i] = win;
	}
}

static void open_display()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf (stderr, "Could not open display\n");
		exit (1);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask);
}

static void get_display_resolution() {
	display_width = DisplayWidth(dpy, screen);
	display_height = DisplayHeight(dpy, screen);
	bar.height = 26;
	bar.space = 22;
}

static void setup_gc() {
	screen = DefaultScreen(dpy);
	bar.gc = XCreateGC(dpy, win, 0, 0);
	title_gc = XCreateGC(dpy, win, 0, 0);
	bg_gc = XCreateGC(dpy, win, 0, 0);
	XColor inactive_bg_color;
	Colormap colormap1;
	colormap1 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap1, inactive_bg, &inactive_bg_color);
	XAllocColor(dpy, colormap1, &inactive_bg_color);
	XSetBackground(dpy, bar.gc, inactive_bg_color.pixel); 
	XColor inactive_fg_color;
	Colormap colormap2;
	colormap2 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap2, inactive_fg, &inactive_fg_color);
	XAllocColor(dpy, colormap2, &inactive_fg_color);
	XSetForeground(dpy, bar.gc, inactive_fg_color.pixel); 
	XColor active_bg_color;
	Colormap colormap3;
	colormap3 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap3, active_bg, &active_bg_color);
	XAllocColor(dpy, colormap3, &active_bg_color);
	XSetForeground(dpy, title_gc, active_bg_color.pixel);
	XSetForeground(dpy, bg_gc, inactive_bg_color.pixel);
	resize_cursor =	XCreateFontCursor(dpy, XC_sizing);
	default_cursor = XCreateFontCursor(dpy, XC_left_ptr);
	move_cursor = XCreateFontCursor(dpy, XC_fleur);
	XDefineCursor(dpy, root, default_cursor);
}


static void create_bar() {
	win = XCreateSimpleWindow(dpy, root, 0, 0, display_width, bar.height, 0, BlackPixel(dpy, 0), BlackPixel(dpy, 0));
	XSelectInput(dpy, win, StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask );
	XMapWindow(dpy, win);
}

static void load_font() {
	font = XLoadQueryFont(dpy, font_name);
	if(!font) {
		fprintf(stderr, "Unable to load font %s\n", font_name);
		font = XLoadQueryFont (dpy, "fixed");
	}
	XSetFont(dpy, bar.gc, font->fid);
}


static void setup_bar_tags() {
	for(int i = 0; i < 9; i++) {
		bar.text[i] = bar_tags[i];
		bar.text_len[i] = 1;
	}
}

static void init_bar(){
	int x;
	int y;
	int direction;
	int ascent;
	int descent;
	XCharStruct overall;
	for(int i = 0; i < 9; i++){
		XTextExtents (font, bar.text[i], bar.text_len[i], & direction, & ascent, & descent, & overall);
		tag_width[i] = overall.width;
		tag_x[i] = bar.space*i+tag_width_sum;
		tag_width_sum+=tag_width[i];
	}
	x = bar.space/2;
	y = bar.height / 2 + (ascent - descent) / 2;
	tag_y = y;
	XClearWindow(dpy, win);
	XFillRectangle(dpy, win, title_gc, 0, 0, display_width, bar.height);
	XFillRectangle(dpy, win, bg_gc, 0, 0, tag_width_sum + bar.space*9, bar.height);
	for(int i = 0; i < 9; i++){
		if(i != 0)
			x += overall.width+bar.space;
		XDrawString(dpy, win, bar.gc, x, y, bar.text[i], bar.text_len[i]);
	}
}

static void expose_bar() {
	XClearWindow(dpy, win);
	XFillRectangle(dpy, win, title_gc, 0, 0, display_width, bar.height);
	XFillRectangle(dpy, win, bg_gc, 0, 0, tag_width_sum + bar.space*9, bar.height);
	for(int i = 0; i < 9; i++){
		XDrawString(dpy, win, bar.gc, tag_x[i]+bar.space/2, tag_y, bar.text[i], bar.text_len[i]);
	}
}

static void draw_bar(int prev, int index) {
	if(prev != -1){
		XFillRectangle(dpy, win, bg_gc, tag_x[prev], 0, tag_width[prev]+bar.space, bar.height);
		XDrawString(dpy, win, bar.gc, tag_x[prev]+bar.space/2, tag_y, bar.text[prev], bar.text_len[prev]);
	}
	XFillRectangle(dpy, win, title_gc, tag_x[index], 0, tag_width[index]+bar.space, bar.height);
	XDrawString(dpy, win, bar.gc, tag_x[index]+bar.space/2, tag_y, bar.text[index], bar.text_len[index]);
}

static void init() {
	open_display();
	get_display_resolution();
	create_bar();
	setup_gc();
	load_font();
	setup_bar_tags();
	XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
	init_focused_windows();
	init_bar();
	XFlush(dpy);
}

static void frame(Window w, client *clients_head) {
	client *current = clients_head->next;
	while(current != NULL) {
		if(current->frame == w) {
			return;
		}
		current = current->next;
	}
	num_clients += 1;
	XColor border_color;
	Colormap colormap3;
	colormap3 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap3, active_bg, &border_color);
	XAllocColor(dpy, colormap3, &border_color);
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, w, &attr);
	Window frame = XCreateSimpleWindow(dpy, root, 0, bar.height, attr.width, attr.height, BORDER_WIDTH, border_color.pixel, BlackPixel(dpy, 0));
	push_back(clients_head, w, frame, ws_num);
	XSelectInput(dpy, frame, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask);
	XReparentWindow(dpy, w, frame, 0, 0);
	XMapWindow(dpy, frame);
	XGrabButton(dpy, 1, Mod1Mask, w, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod1Mask, w, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabKey(dpy, AnyKey, Mod4Mask, w, True, GrabModeAsync, GrabModeAsync);
}

static void map_request(XMapRequestEvent e, client *clients_head) {
	int need_to_map = 1;
	frame(e.window, clients_head);
	client *current = clients_head->next;
	while(current != NULL) {
		if(current->frame == e.window && current->ws_num != ws_num) {
			need_to_map = 0;
			break;
		}
		current = current->next;
	}
	if(need_to_map) {
		XMapWindow(dpy, e.window);
		XSetInputFocus(dpy, e.window, RevertToPointerRoot, CurrentTime);
	}
}

static void button_press(XButtonEvent e, client *clients_head) {
	int detected = 0;
	client *current = clients_head->next;
	while(current != NULL) { 
		if(current->frame == e.subwindow && current->ws_num == ws_num) {
			detected = 1;
			break;
		}
		current = current->next;
	}
	if(detected) {
		fprintf(stderr, "CLICKED WINDOW: %ld", e.subwindow);
		start = e;
		XGetWindowAttributes(dpy, start.subwindow, &attr);
		XRaiseWindow(dpy, start.subwindow);
		XSetInputFocus(dpy, current->win, RevertToNone, CurrentTime);
		focused_window[ws_num] = current->frame;
	}
	else
		return;
}

static void move_resize_window(XButtonEvent e, client *clients_head) {
	fprintf(stderr, "MOTION NOTIFY FOR WINDOW %ld\n", start.subwindow);
	int xdiff = e.x_root - start.x_root;
	int ydiff = e.y_root - start.y_root;
	int check_width = MAX(1, attr.width + (start.button == 3 ? xdiff : 0));
	int check_height = MAX(1, attr.height + (start.button == 3 ? ydiff : 0));
	int tmp_width, tmp_height;

	if(attr.x+check_width <= display_width-2*BORDER_WIDTH)
		tmp_width = check_width;
	else 
		tmp_width = display_width-2*BORDER_WIDTH-attr.x;
	if(attr.y+check_height <= display_height-2*BORDER_WIDTH)
		tmp_height = check_height;
	else
		tmp_height = display_height-2*BORDER_WIDTH-attr.y;

	client *current = clients_head->next;
	while(current != NULL) {
		if(current->frame == start.subwindow) {
			break;
		}
		current = current->next;
	}

	if(start.button == 3) {
		XDefineCursor(dpy, current->win, resize_cursor);
	}
	else 
		XDefineCursor(dpy, current->win, move_cursor);

	XMoveWindow(dpy, start.subwindow,
			(attr.x + (start.button == 1 ? xdiff : 0) >= 0 
			 ? (attr.x + (start.button == 1 ? xdiff: 0) + attr.width <= display_width
				 ? (attr.x + (start.button == 1 ? xdiff : 0)) : display_width - attr.width)
			 : 0),
			(attr.y + (start.button == 1 ? ydiff : 0) >= bar.height 
			 ? (attr.y + (start.button == 1 ? ydiff: 0) + attr.height <= display_height
				 ? (attr.y + (start.button == 1 ? ydiff : 0)) : display_height - attr.height)
			 : bar.height));
	XResizeWindow(dpy, start.subwindow, tmp_width, tmp_height);
	XResizeWindow(dpy, current->win, tmp_width, tmp_height);
}

static void button_release(client *clients_head) {
	int detected = 0;
	client *current = clients_head->next;
	while(current != NULL) {
		if(current->frame == start.subwindow && current->ws_num == ws_num) {
			detected = 1;
			break;
		}
		current = current->next;
	}
	if(!detected)
		return;
	XUndefineCursor(dpy, current->win);
	start.subwindow = None;
}

static void destroy_frame(XDestroyWindowEvent e, client *clients_head) {
	client *current = clients_head->next;
	int detected = 0;
	int cnt = 0;
	while(current != NULL) {
		cnt++;
		if(current->win == e.window) {
			//fprintf(stderr, "%d WINDOW %ld FRAME %ld\n", cnt, current->win, current->frame);
			detected = 1;
			break;
		}
		current = current->next;
	}
	if(!detected)
		return;
	//XDestroyWindow(dpy, current->frame);
	XUnmapWindow(dpy, current->frame);
	XDestroyWindow(dpy, current->frame);
	num_clients--;
	if(cnt != num_clients && cnt != 0)
		pop(clients_head, cnt);
	else if(cnt != num_clients) 
		pop_back(clients_head);
}

static void change_workspace(client *clients_head) {
	client *current = clients_head->next;
	while(current != NULL) {
		if(current->win != root && current->win != win){
			if(current->ws_num == ws_num) {
				XMapWindow(dpy, current->win);
				XMapWindow(dpy, current->frame);
			}
			else {
				XUnmapWindow(dpy, current->win);
				XUnmapWindow(dpy, current->frame);
			}
		}
		current = current->next;
	}
	XSetInputFocus(dpy, focused_window[ws_num], RevertToPointerRoot, CurrentTime);
}

static void move_window_to_ws(int move_to, client *clients_head) {
	Window focused_win;
	int revert;
	XGetInputFocus(dpy, &focused_win, &revert);
	if(focused_win == root || focused_win == None || focused_win == win)
		return;
	client *current = clients_head->next;
	while(current != NULL) {
		if(current->win == focused_win || current->frame == focused_win) {
			XUnmapWindow(dpy, current->win);
			XUnmapWindow(dpy, current->frame);
			focused_window[current->ws_num] = root;
			current->ws_num = move_to;
			focused_window[move_to] = current->frame;
		}
		current = current->next;
	}
}

static void change_focus(Window w, client *clients_head) {
	client *current = clients_head->next;
	int detected = 0;
	while(current != NULL) {
		if(current->win == w || current->frame == w) {
			detected = 1;
		}
		else if(current->ws_num == ws_num && detected) {
			XSetInputFocus(dpy, current->win, RevertToParent, CurrentTime);
			XRaiseWindow(dpy, current->frame);
			return;
		}
		current = current->next;
	}
	if(!detected)
		return;
	current = clients_head->next;
	while(current != NULL) {
		if(current->ws_num == ws_num && current->win != w && current->frame != w) {
			XSetInputFocus(dpy, current->win, RevertToParent, CurrentTime);
			XRaiseWindow(dpy, current->frame);
			return;
		}
		current = current->next;
	}
}

static void run(client *clients_head) {
	XGrabServer(dpy);
	Window returned_root, returned_parent;
	Window *top_level_windows;
	unsigned int num_top_level_windows = 0;
	XQueryTree(dpy, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);
	for(unsigned int i = 0; i < num_top_level_windows; i++) {
		if(top_level_windows[i] != win && top_level_windows[i] != root) {
			frame(top_level_windows[i], clients_head);
		}
	}
	XFree(top_level_windows);
	XUngrabServer(dpy);
	fprintf(stderr, "FUCKING TEST!!!!\n");
	while (1) {
		print_list(clients_head);
		XDefineCursor(dpy, root, default_cursor);
		XEvent e;
		XNextEvent (dpy, &e);
		if(e.type == KeyPress){
			KeySym keysym = XKeycodeToKeysym(dpy, e.xkey.keycode, 0);
			if(CLEANMASK(e.xkey.state) == CLEANMASK((Mod4Mask|ShiftMask))) {
				for(int i = 0; i < 10; i++) { 
					if((int)e.xkey.keycode == i+10 && i != ws_num)
						move_window_to_ws(i, clients_head);
				}
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask)){
				if(e.xkey.keycode >= 10 && e.xkey.keycode <= 19) {
					prev_tag = active_tag;
					active_tag = e.xkey.keycode-10;
					ws_num = active_tag;
					change_workspace(clients_head);
					draw_bar(prev_tag, active_tag);
				}
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_Tab) {
				change_focus(e.xkey.window, clients_head);
			}
		}
		if(e.type == ButtonPress && e.xbutton.subwindow != None && e.xbutton.subwindow != root && e.xbutton.subwindow != win){
			button_press(e.xbutton, clients_head);
		}
		if(e.type == MotionNotify && start.subwindow != None && start.subwindow != win && start.subwindow != root) {
			move_resize_window(e.xbutton, clients_head);
		}
		else if(e.type == ButtonRelease && e.xbutton.subwindow != None && e.xbutton.subwindow != None && e.xbutton.subwindow != win)
			button_release(clients_head);
		if(e.type == MapRequest) {
			fprintf(stderr, "Map Request for window %ld\n", e.xmaprequest.window);
			map_request(e.xmaprequest, clients_head);
		}
		else if(e.type == DestroyNotify && e.xdestroywindow.window != root && e.xdestroywindow.window != win) {
			printf("Unmap notify!\n");
			destroy_frame(e.xdestroywindow, clients_head);
			//fprintf(stderr, "UnmapRequest: %ld\n", e.xunmap.window);
		}
		if(e.type == Expose){
			expose_bar();
			draw_bar(prev_tag, active_tag);
			XFlush(dpy);
		}
	}
}


int main(int argc, char ** argv){
	client *clients_head = NULL;
	clients_head = (client *)malloc(sizeof(client));
	init();
	//fprintf(stderr, "ROOT IS %ld\n", root);
	XGrabButton(dpy, 1, Mod4Mask, DefaultRootWindow(dpy), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod4Mask, DefaultRootWindow(dpy), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	run(clients_head);
	return 0;
}
