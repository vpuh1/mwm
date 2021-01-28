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

static void open_display()
{
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf (stderr, "Could not open display\n");
        exit (1);
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
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
	init_bar();
	XFlush(dpy);
}

static void configure_window(XConfigureRequestEvent e) {
	XWindowChanges changes;
	changes.x = e.x;
	changes.y = bar.height;
	changes.width = e.width;
	changes.height = e.height;
	changes.border_width = 5;
	changes.sibling = e.above;
  	changes.stack_mode = e.detail;
	XConfigureWindow(dpy, e.window, e.value_mask, &changes);
}

static void frame(Window w, client *clients_head) {
	XColor border_color;
	Colormap colormap3;
	colormap3 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap3, active_bg, &border_color);
	XAllocColor(dpy, colormap3, &border_color);
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, w, &attr);
	Window frame = XCreateSimpleWindow(dpy, root, 0, bar.height, attr.width, attr.height, BORDER_WIDTH, border_color.pixel, BlackPixel(dpy, 0));
	push_back(clients_head, w, frame);
	XSelectInput(dpy, frame, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask);
	XReparentWindow(dpy, w, frame, 0, 0);
	XMapWindow(dpy, frame);
	XGrabButton(dpy, 1, Mod1Mask, w, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod1Mask, w, False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
}

static void map_request(XMapRequestEvent e, client *clients_head) {
	frame(e.window, clients_head);
	XMapWindow(dpy, e.window);
}

static void run(client *clients_head) {
	XGrabServer(dpy);
	Window returned_root, returned_parent;
	Window *top_level_windows;
	unsigned int num_top_level_windows = 0;
	XQueryTree(dpy, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);
	for(unsigned int i = 0; i < num_top_level_windows; i++) {
		if(top_level_windows[i] != win) {
			frame(top_level_windows[i], clients_head);
		}
	}
	XFree(top_level_windows);
	XUngrabServer(dpy);
	fprintf(stderr, "FUCKING TEST!!!!\n");
    while (1) {
		XDefineCursor(dpy, root, default_cursor);
        XEvent e;
        XNextEvent (dpy, &e);
		XButtonEvent start;
		if(e.type == KeyPress){
			if(e.xkey.state == 64){
				if(e.xkey.keycode >= 10 && e.xkey.keycode <= 18){
					/*if(e.xkey.subwindow != win){
						XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
					}*/
					prev_tag = active_tag;
					active_tag = e.xkey.keycode-10;
					draw_bar(prev_tag, active_tag);
				}
			}
		}
		if(e.type == ButtonPress && e.xbutton.subwindow != None){
			start = e.xbutton;
			XGetWindowAttributes(dpy, start.subwindow, &attr);
			XRaiseWindow(dpy, start.subwindow);
			client *current = clients_head;
			while(current->next != NULL) {
				if(current->frame == start.subwindow) {
					break;
				}
				current = current->next;
			}
			XSetInputFocus(dpy, current->win, RevertToPointerRoot, CurrentTime);
			//XSetInputFocus(dpy, start.subwindow, RevertToPointerRoot, CurrentTime);
		}
		if(e.type == MotionNotify && start.subwindow != None && start.subwindow != win) {
			int xdiff = e.xbutton.x_root - start.x_root;
			int ydiff = e.xbutton.y_root - start.y_root;
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

			client *current = clients_head;
			while(current->next != NULL) {
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
		else if(e.type == ButtonRelease) {
			client *current = clients_head;
			while(current->next != NULL) {
				if(current->frame == start.subwindow) {
					break;
				}
				current = current->next;
			}
			XUndefineCursor(dpy, current->win);
			start.subwindow = None;
		}
		if(e.type == ConfigureRequest) {
			configure_window(e.xconfigurerequest);
		}
		if(e.type == MapRequest) {
			map_request(e.xmaprequest, clients_head);
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
	XGrabButton(dpy, 1, Mod1Mask, DefaultRootWindow(dpy), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod1Mask, DefaultRootWindow(dpy), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    run(clients_head);
    return 0;
}
