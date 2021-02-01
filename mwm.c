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
#define CLEANMASK(mask) (mask & (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))

struct {
	GC gc;
	char text_len[9];
	char *text[9];
	int height; int space;
} bar;

static void open_display();
static void load_font();
static void setup_gc();
static void create_bar();
static void draw_bar();
static void init();
static void run();
static void frame();
static void map_request();
static void destroy_frame();

static int wm_mode = 0; /* floating mod */
static int active_tag = 0;
static int screen;
static int display_width;
static int display_height;
static int tag_width[9];
static int tag_width_sum = 0;
static int tag_x[9];
static int tag_y;
static int ws_num = 0;
static int num_clients = 0;
static int prev_tag = -1;
static Display *dpy;
static Window win;
static Window focused_window[10];
static Window root;
static GC title_gc;
static GC bg_gc;
static Colormap col;
static XColor accent;
static XColor bg;
static XColor fg;
static Cursor default_cursor;
static Cursor resize_cursor;
static Cursor move_cursor;
static XWindowAttributes attr;
static XFontStruct *font;
static XButtonEvent start;

static void open_display()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf (stderr, "mwm: could not open display\n");
		exit (1);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	for(int i = 0; i < 10; i++) {
		focused_window[i] = root;
	}
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask);
	display_width = DisplayWidth(dpy, screen);
	display_height = DisplayHeight(dpy, screen);
}

static void setup_gc() {
	screen = DefaultScreen(dpy);
	col = DefaultColormap(dpy, 0);
	bar.gc = XCreateGC(dpy, win, 0, 0);
	title_gc = XCreateGC(dpy, win, 0, 0);
	bg_gc = XCreateGC(dpy, win, 0, 0);

	XParseColor(dpy, col, bg_color, &bg);
	XAllocColor(dpy, col, &bg);
	XSetBackground(dpy, bar.gc, bg.pixel); 
	XParseColor(dpy, col, fg_color, &fg);
	XAllocColor(dpy, col, &fg);
	XSetForeground(dpy, bar.gc, fg.pixel); 
	XParseColor(dpy, col, accent_color, &accent);
	XAllocColor(dpy, col, &accent);
	XSetForeground(dpy, title_gc, accent.pixel);
	XSetForeground(dpy, bg_gc, bg.pixel);

	resize_cursor =	XCreateFontCursor(dpy, XC_sizing);
	default_cursor = XCreateFontCursor(dpy, XC_left_ptr);
	move_cursor = XCreateFontCursor(dpy, XC_fleur);
	XDefineCursor(dpy, root, default_cursor);
}


static void create_bar() {
	bar.height = 26;
	bar.space = 22;
	for(int i = 0; i < 9; i++) {
		bar.text[i] = bar_tags[i];
		bar.text_len[i] = 1;
	}
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

static void init_bar(){
	int x;
	int y;
	int direction;
	int ascent;
	int descent;
	XCharStruct overall;
	for(int i = 0; i < 9; i++){
		XTextExtents(font, bar.text[i], bar.text_len[i], &direction, &ascent, &descent, &overall);
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
	create_bar();
	setup_gc();
	load_font();
	XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
	init_bar();
	XFlush(dpy);
}

static void frame(Window w, client *clients_head) {
	for(client *cur = clients_head->next; cur != NULL; cur = cur->next) {
		if(cur->frame == w)
			return;
	}
	num_clients += 1;
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, w, &attr);
	Window frame;
	if(wm_mode == 0)
		frame = XCreateSimpleWindow(dpy, root, 0, bar.height, attr.width, attr.height, BORDER_WIDTH, accent.pixel, BlackPixel(dpy, 0));
	else {
		frame = XCreateSimpleWindow(dpy, root, 0, bar.height, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH, BORDER_WIDTH, accent.pixel, BlackPixel(dpy, 0));
		XResizeWindow(dpy, w, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH);
	}
	push_back(clients_head, w, frame, ws_num);
	XSelectInput(dpy, frame, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask);
	XReparentWindow(dpy, w, frame, 0, 0);
	XMapWindow(dpy, frame);
	XGrabButton(dpy, 1, Mod1Mask, w, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod1Mask, w, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabKey(dpy, AnyKey, Mod4Mask, w, True, GrabModeAsync, GrabModeAsync);
}

static void map_request(XMapRequestEvent e, client *clients_head) {
	frame(e.window, clients_head);
	for(client *cur = clients_head; cur != NULL; cur = cur->next) {
		if(cur->frame == e.window && cur->ws_num != ws_num) {
			return;
		}
	}
	XMapWindow(dpy, e.window);
	XSetInputFocus(dpy, e.window, RevertToPointerRoot, CurrentTime);
	focused_window[ws_num] = e.window;
}

static void button_press(XButtonEvent e, client *clients_head) {
	client *cur = clients_head->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->frame == e.subwindow && cur->ws_num == ws_num) {
			fprintf(stderr, "CLICKED WINDOW: %ld", e.subwindow);
			start = e;
			XGetWindowAttributes(dpy, start.subwindow, &attr);
			XRaiseWindow(dpy, start.subwindow);
			XSetInputFocus(dpy, cur->win, RevertToNone, CurrentTime);
			focused_window[ws_num] = cur->win;
			return;
		}
	}
}

static void move_resize_window(XButtonEvent e, client *clients_head) {
	//fprintf(stderr, "MOTION NOTIFY FOR WINDOW %ld\n", start.subwindow);
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

	client *cur = clients_head->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->frame == start.subwindow) {
			break;
		}
	}
	if(start.button == 3) {
		XDefineCursor(dpy, cur->win, resize_cursor);
	}
	else 
		XDefineCursor(dpy, cur->win, move_cursor);

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
	XResizeWindow(dpy, cur->win, tmp_width, tmp_height);
}

static void button_release(client *clients_head) {
	client *cur= clients_head->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->frame == start.subwindow && cur->ws_num == ws_num) {
			XUndefineCursor(dpy, cur->win);
			start.subwindow = None;
			return;
		}
	}
}


void change_focus(Window w, client *clients_head) {
	client *cur = clients_head->next;
	int detected = 0;
	for(; cur != NULL; cur = cur->next) {
		if((cur->win == w || cur->frame == w )&& cur->win != root) {
			detected = 1;
		}
		else if(cur->ws_num == ws_num && detected && cur->win != root) {
			XSetInputFocus(dpy, cur->win, RevertToParent, CurrentTime);
			focused_window[ws_num] = cur->win;
			XRaiseWindow(dpy, cur->frame);
			return;
		}
	}
	if(!detected) {
		XSetInputFocus(dpy, root, RevertToNone, CurrentTime);
		focused_window[ws_num] = root;
		return;
	}
	for(cur = clients_head->next; cur != NULL; cur = cur->next) {
		if(cur->ws_num == ws_num && cur->win != w && cur->frame != w && cur->win != root) {
			XSetInputFocus(dpy, cur->win, RevertToParent, CurrentTime);
			focused_window[ws_num] = cur->win;
			XRaiseWindow(dpy, cur->frame);
			return;
		}
	}
	XSetInputFocus(dpy, root, RevertToNone, CurrentTime);
	focused_window[ws_num] = root;
}

static void destroy_frame(XDestroyWindowEvent e, client *clients_head) {
	client *cur = clients_head->next;
	int cnt = 0;
	for(; cur != NULL; cur = cur->next) {
		cnt++;
		if(cur->win == e.window) {
			change_focus(e.window, clients_head);
			XUnmapWindow(dpy, cur->frame);
			XDestroyWindow(dpy, cur->frame);
			if(cnt != num_clients && cnt != 0) {
				pop(clients_head, cnt);
				return;
			}
			if(cnt == num_clients) {
				pop_back(clients_head);
				return;
			}
			num_clients--;
		}
	}
}

static void change_ws(client *clients_head) {
	client *cur = clients_head->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->win != root && cur->win != win){
			if(cur->ws_num == ws_num) {
				XMapWindow(dpy, cur->win);
				XMapWindow(dpy, cur->frame);
			}
			else {
				XUnmapWindow(dpy, cur->win);
				XUnmapWindow(dpy, cur->frame);
			}
		}
	}
	XSetInputFocus(dpy, focused_window[ws_num], RevertToNone, CurrentTime);
	for(cur = clients_head->next; cur != NULL; cur = cur->next) {
		if(cur->win == focused_window[ws_num] && focused_window[ws_num] != root) {
			XRaiseWindow(dpy, cur->frame);
			return;
		}
	}
}

static void move_to_ws(int move_to, client *clients_head) {
	Window focused_win;
	int revert;
	XGetInputFocus(dpy, &focused_win, &revert);
	if(focused_win == root || focused_win == None || focused_win == win)
		return;
	client *cur= clients_head->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->win == focused_win || cur->frame == focused_win) {
			XUnmapWindow(dpy, cur->win);
			XUnmapWindow(dpy, cur->frame);
			change_focus(focused_win, clients_head);
			cur->ws_num = move_to;
			focused_window[move_to] = cur->win;
		}
	}
}

static void change_wm_mode(Window w, int mode, client *clients_head) {
	if(mode == 1) {
		client *cur = clients_head->next;
		for(; cur != NULL; cur = cur->next) {
			if(cur->win == w) {
				XResizeWindow(dpy, w, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH);
				XResizeWindow(dpy, cur->frame, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH);
			}
		}
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
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask)) {
				if(keysym == XK_Return) {
					char *argvv[] = {"st", NULL};
					pid_t pid;
					pid  = fork();
					if(pid == 0) {
						execvp("st", argvv);
					}
				}
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK((Mod4Mask|ShiftMask))) {
				for(int i = 0; i < 10; i++) { 
					if((int)e.xkey.keycode == i + 10 && i != ws_num)
						move_to_ws(i, clients_head);
				}
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK((Mod4Mask | ShiftMask)) && keysym == XK_c && e.xkey.window != root && e.xkey.window != win) {
				int detected = 0;
				client *cur = clients_head->next;
				int cnt = 0;
				for(; cur != NULL; cur = cur->next) {
					cnt++;
					if(cur->win == e.xkey.window) {
						detected = 1;
						Window tmp_frame = cur->frame;
						Window tmp_win = cur->win;
						XUnmapWindow(dpy, cur->win);
						XUnmapWindow(dpy, cur->frame);
						change_focus(tmp_win, clients_head);
						if(cnt != num_clients && cnt != 0)
							pop(clients_head, cnt);
						if(cnt == num_clients) 
							pop_back(clients_head);
						num_clients--;
						XDestroyWindow(dpy, tmp_win);
						XDestroyWindow(dpy, tmp_frame);
						break;
					}
				}
				if(!detected)
					fprintf(stderr, "mwm: could not found frame window, killing window. %ld\n", e.xkey.window);
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask)){
				if(e.xkey.keycode >= 10 && e.xkey.keycode <= 19) {
					prev_tag = active_tag;
					active_tag = e.xkey.keycode - 10;
					ws_num = active_tag;
					change_ws(clients_head);
					draw_bar(prev_tag, active_tag);
				}
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_Tab) {
				change_focus(e.xkey.window, clients_head);
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_m) {
				wm_mode = 1;
				change_wm_mode(e.xkey.window, 1, clients_head);
			}
			else if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_f)
				wm_mode = 0;
		}
		if(e.type == ButtonPress && e.xbutton.subwindow != None && e.xbutton.subwindow != root && e.xbutton.subwindow != win){
			button_press(e.xbutton, clients_head);
		}
		if(e.type == MotionNotify && start.subwindow != None && start.subwindow != win && start.subwindow != root) {
			move_resize_window(e.xbutton, clients_head);
		}
		else if(e.type == ButtonRelease && e.xbutton.subwindow != None && e.xbutton.subwindow != None && e.xbutton.subwindow != win)
			button_release(clients_head);
		if(e.type == MapRequest)
			map_request(e.xmaprequest, clients_head);
		else if(e.type == DestroyNotify && e.xdestroywindow.window != root && e.xdestroywindow.window != win)
			destroy_frame(e.xdestroywindow, clients_head);
		if(e.type == Expose){
			expose_bar();
			draw_bar(prev_tag, active_tag);
			XFlush(dpy);
		}
	}
}


int main(int argc, char ** argv){
	XInitThreads();
	client *clients_head = NULL;
	clients_head = (client *)malloc(sizeof(client));
	init();
	XGrabButton(dpy, 1, Mod4Mask, DefaultRootWindow(dpy), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod4Mask, DefaultRootWindow(dpy), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	run(clients_head);
	return 0;
}
