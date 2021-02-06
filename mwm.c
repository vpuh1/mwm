#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LENGTH(X) (sizeof X / sizeof X[0])
#define BORDER_WIDTH 1
#define CLEANMASK(mask) (mask & (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))

/* linked list for clients */
typedef struct Client {
	Window win;	
	Window frame;	
	struct Client *next;
	int ws_num;
	int w, h;
} Client;

/* struct for tabs in monocle mode */
typedef struct Tab {
	int x;
	int active;
	int h;
	int	w; 
} Tab;

typedef struct Bar {
	GC gc;
	char text_len[9];
	char *text[9];
	int height;
	int space;
} Bar;

typedef struct Arg {
	int tag;
	const void *name;
} Arg;

typedef struct Key {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg args;
} Key;

static Bar bar;
static Client *chead = NULL;
static Tab tab;

static int wm_mode = 0; /* floating mod */
static int active_tag = 0;
static int screen;
static int display_width;
static int display_height;
static int tag_width[9];
static int tag_width_sum = 0;
static int tag_x[9];
static int tag_y;
static int tab_x = 0;
static int tab_width;
static int active_tab = 0;
static int tab_height;
static int ws_num = 0;
static int num_clients = 0;
static int prev_num_clients = 0;
static int prev_tag = -1;
static Display *dpy;
static Window win;
static Window focused_window[10];
static Window root;
static Window tabs;
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

/* function declaration */
static void print_list(Client *head);
static void push_back(Client *head, Window win, Window frame, int ws_num);
static void push_front(Client **head, Window win, Window frame, int ws_num);
static void push(Client *head, int index, Window win, Window frame, int ws_num);
static void pop_back(Client *head);
static void pop_front(Client **head);
static void pop(Client *head, int index);
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
static void change_ws(int tag);
static void move_to_ws(int tag);

#include "config.h"

/*function implementation */
void print_list(Client *head) {
	Client *current = head;
	while(current != NULL) {
		fprintf(stderr, "Window: %ld Frame: %ld Workspace: %d", current->win, current->frame, current->ws_num);
		current = current->next;
		if(current == NULL) {
			fprintf(stderr, "\n");
		}
	}
}

void push_back(Client *head, Window win, Window frame, int ws_num) {
	Client *current = head;
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = malloc(sizeof(Client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->ws_num = ws_num;
	current->next->next = NULL;
}

void push_front(Client **head, Window win, Window frame, int ws_num) {
	Client *new_node;
	new_node = malloc(sizeof(Client));
	new_node->next = *head;
	new_node->win = win;
	new_node->frame = frame;
	new_node->ws_num = ws_num;
	*head = new_node;
}

void push(Client *head, int index, Window win, Window frame, int ws_num) {
	Client *current = head;
	for(int i = 0; i < index-1; i++) {
		if(current->next != NULL) {
			current = current->next;
		}
	}
	if(current->next == NULL || index == 0) {
		return;
	}
	Client *tmp = current->next;
	current->next = malloc(sizeof(Client));
	current->next->win = win;
	current->next->frame = frame;
	current->next->ws_num = ws_num;
	current->next->next = tmp;
}

void pop_back(Client *head) {
	if(head->next == NULL) {
		free(head);
		return;
	}
	Client *current = head;
	while(current->next->next != NULL) {
		current = current->next;
	}
	free(current->next);
	current->next = NULL;
}

void pop_front(Client **head) {
	Client *next_node = NULL;
	if(head == NULL){
		return;
	}
	next_node = (*head)->next;
	free(*head);
	*head = next_node;
}

void pop(Client *head, int index) {
	Client *current = head;
	for(int i = 0; i < index-1; i++){
		if(current->next != NULL){
			current = current->next;
		}
	}
	Client *tmp = current->next->next;
	free(current->next);
	current->next = tmp;
}


static void open_display() {
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf (stderr, "mwm: could not open display\n");
		exit(1);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XGrabButton(dpy, 1, Mod4Mask, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod4Mask, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	for(int i = 0; i < 10; i++) {
		focused_window[i] = root;
	}
	XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask);
	display_width = DisplayWidth(dpy, screen);
	display_height = DisplayHeight(dpy, screen);
}

static void setup_gc() {
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

static void create_tabs() {
	tab_width = display_width;
	tab_height = 18;
	tabs = XCreateSimpleWindow(dpy, root, 0, bar.height, display_width, tab_height, 0, BlackPixel(dpy, 0), BlackPixel(dpy, 0));
}

static void load_font() {
	font = XLoadQueryFont(dpy, font_name);
	if(!font) {
		fprintf(stderr, "mwm: unable to load font %s\n", font_name);
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

static void draw_tabs() {
	tab_x = 0;
	if(num_clients != 0)
		tab_width = display_width / num_clients;
	else tab_width = display_width;
	XFillRectangle(dpy, tabs, bg_gc, 0, 0, display_width, tab_height);
	for(int i = 0; i < num_clients - 1; i++) {
		if(i == active_tab)
			XFillRectangle(dpy, tabs, title_gc, tab_x, 0, tab_width, tab_height);
		tab_x += tab_width;
	}
	if(num_clients-1 == active_tab)
		XFillRectangle(dpy, tabs, title_gc, tab_x, 0, display_width-tab_x, tab_height);

}

static void init() {
	chead = (Client *)malloc(sizeof(Client));
	open_display();
	create_bar();
	create_tabs();
	setup_gc();
	load_font();
	XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
	init_bar();
}

static void frame(Window w) {
	if(w == tabs)
		return;
	for(Client *cur = chead->next; cur != NULL; cur = cur->next) {
		if(cur->frame == w)
			return;
	}
	num_clients++;
	XWindowAttributes attr;
	XGetWindowAttributes(dpy, w, &attr);
	Window frame;
	if(wm_mode == 0)
		frame = XCreateSimpleWindow(dpy, root, 0, bar.height, attr.width, attr.height, BORDER_WIDTH, accent.pixel, BlackPixel(dpy, 0));
	else {
		frame = XCreateSimpleWindow(dpy, root, 0, bar.height, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH, BORDER_WIDTH, accent.pixel, BlackPixel(dpy, 0));
		XMoveResizeWindow(dpy, w, 0, 0, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH-tab_height);
		XMoveResizeWindow(dpy, frame, 0, bar.height+tab_height, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH-tab_height);
	}
	push_back(chead, w, frame, ws_num);
	XSelectInput(dpy, frame, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask);
	XReparentWindow(dpy, w, frame, 0, 0);
	XMapWindow(dpy, frame);
	XGrabButton(dpy, 1, Mod1Mask, w, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod1Mask, w, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabKey(dpy, AnyKey, Mod4Mask, w, True, GrabModeAsync, GrabModeAsync);
}

static void map_request(XMapRequestEvent e) {
	frame(e.window);
	for(Client *cur = chead; cur != NULL; cur = cur->next) {
		if(cur->frame == e.window && cur->ws_num != ws_num) {
			return;
		}
	}
	XMapWindow(dpy, e.window);
	if(wm_mode == 1) {
		draw_tabs();
	}
	XSetInputFocus(dpy, e.window, RevertToPointerRoot, CurrentTime);
	focused_window[ws_num] = e.window;
}

static void button_press(XButtonEvent e) {
	Client *cur = chead->next;
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

static void move_resize_window(XButtonEvent e) {
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

	Client *cur = chead->next;
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

static void button_release() {
	Client *cur= chead->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->frame == start.subwindow && cur->ws_num == ws_num) {
			XUndefineCursor(dpy, cur->win);
			start.subwindow = None;
			return;
		}
	}
}


void change_focus(Window w) {
	Client *cur = chead->next;
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
	for(cur = chead->next; cur != NULL; cur = cur->next) {
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

static void destroy_frame(XDestroyWindowEvent e) {
	Client *cur = chead->next;
	int cnt = 0;
	for(; cur != NULL; cur = cur->next) {
		cnt++;
		if(cur->win == e.window) {
			change_focus(e.window);
			XUnmapWindow(dpy, cur->frame);
			XDestroyWindow(dpy, cur->frame);
			if(wm_mode == 1) {
				num_clients--;
				draw_tabs();
			}
			if(cnt != num_clients && cnt != 0) {
				pop(chead, cnt);
				return;
			}
			if(cnt == num_clients) {
				pop_back(chead);
				return;
			}
		}
	}
}

static void change_ws(int tag) {
	Client *cur = chead->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->win != root && cur->win != win){
			if(cur->ws_num == tag) {
				XMapWindow(dpy, cur->win);
				XMapWindow(dpy, cur->frame);
			}
			else {
				XUnmapWindow(dpy, cur->win);
				XUnmapWindow(dpy, cur->frame);
			}
		}
	}
	XSetInputFocus(dpy, focused_window[tag], RevertToNone, CurrentTime);
	for(cur = chead->next; cur != NULL; cur = cur->next) {
		if(cur->win == focused_window[tag] && focused_window[tag] != root) {
			XRaiseWindow(dpy, cur->frame);
			return;
		}
	}
}

static void move_to_ws(int tag) {
	Window focused_win;
	int revert;
	XGetInputFocus(dpy, &focused_win, &revert);
	if(focused_win == root || focused_win == None || focused_win == win)
		return;
	Client *cur = chead->next;
	for(; cur != NULL; cur = cur->next) {
		if(cur->win == focused_win || cur->frame == focused_win) {
			XUnmapWindow(dpy, cur->win);
			XUnmapWindow(dpy, cur->frame);
			change_focus(focused_win);
			cur->ws_num = tag;
			focused_window[tag] = cur->win;
		}
	}
}

static void change_wm_mode(int mode) {
	Client *cur = chead->next;
	if(mode == 1) {
		for(; cur != NULL; cur = cur->next) {
			if(cur->ws_num == ws_num && cur->win != root && cur->win != tabs) {
				XMoveResizeWindow(dpy, cur->win, 0, 0, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH-tab_height);
				XMoveResizeWindow(dpy, cur->frame, 0, bar.height+tab_height, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH-tab_height);
			}
		}
		XMapWindow(dpy, tabs);
		draw_tabs();
	}
	else if(mode == 0) {
		for(; cur != NULL; cur = cur->next) {
			if(cur->ws_num == ws_num && cur->win != root && cur->win != tabs) {
				XMoveResizeWindow(dpy, cur->win, 0, 0, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH);
				XMoveResizeWindow(dpy, cur->frame, 0, bar.height, display_width-2*BORDER_WIDTH, display_height-bar.height-2*BORDER_WIDTH);
			}
		}
	}
}

/*static void keypress(XKeyPressedEvent e) {
	KeySym keysym = XKeycodeToKeysym(dpy, e.keycode, 0);
	for(int i = 0; i < LENGTH(keys); i++) {
		if(keys[i].keysym == keysym && CLEANMASK(keys[i].mod) == CLEANMASK(e.state))
			keys[i].func(&keys[i].args);
	}
}*/

static void run() {
	XGrabServer(dpy);
	Window returned_root, returned_parent;
	Window *top_level_windows;
	unsigned int num_top_level_windows = 0;
	XQueryTree(dpy, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);
	for(unsigned int i = 0; i < num_top_level_windows; i++) {
		if(top_level_windows[i] != win && top_level_windows[i] != root) {
			frame(top_level_windows[i]);
		}
	}
	XFree(top_level_windows);
	XUngrabServer(dpy);
	while (1) {
		print_list(chead);
		fprintf(stderr, "%d\n", active_tab);
		XDefineCursor(dpy, root, default_cursor);
		XEvent e;
		XNextEvent (dpy, &e);
		if(num_clients == 0) {
			XUnmapWindow(dpy, tabs);
		}
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
					if((int)e.xkey.keycode == i + 10 && i != ws_num) {
						move_to_ws(i);
					}
				}
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask)){
				if(e.xkey.keycode >= 10 && e.xkey.keycode <= 19) {
					prev_tag = active_tag;
					active_tag = e.xkey.keycode - 10;
					ws_num = active_tag;
					change_ws(ws_num);
					draw_bar(prev_tag, active_tag);
				}
			}
			//keypress(e.xkey);
			if(CLEANMASK(e.xkey.state) == CLEANMASK((Mod4Mask | ShiftMask)) && keysym == XK_c && e.xkey.window != root && e.xkey.window != win) {
				int detected = 0;
				Client *cur = chead->next;
				int cnt = 0;
				for(; cur != NULL; cur = cur->next) {
					cnt++;
					if(cur->win == e.xkey.window) {
						detected = 1;
						Window tmp_frame = cur->frame;
						Window tmp_win = cur->win;
						XUnmapWindow(dpy, cur->win);
						XUnmapWindow(dpy, cur->frame);
						change_focus(tmp_win);
						if(cnt != num_clients && cnt != 0)
							pop(chead, cnt);
						if(cnt == num_clients) 
							pop_back(chead);
						XDestroyWindow(dpy, tmp_win);
						XDestroyWindow(dpy, tmp_frame);
						break;
					}
				}
				if(wm_mode == 1) {
					num_clients--;
					if(active_tab == num_clients)
						active_tab--;
					draw_tabs();
				}
				if(!detected)
					fprintf(stderr, "mwm: could not found frame window, killing window. %ld\n", e.xkey.window);
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_Tab) {
				if(wm_mode == 1) {
					active_tab++;
					active_tab %= num_clients;
					draw_tabs();
				}
				change_focus(e.xkey.window);
			}
			if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_m) {
				wm_mode = 1;
				change_wm_mode(1);
			}
			else if(CLEANMASK(e.xkey.state) == CLEANMASK(Mod4Mask) && keysym == XK_f) {
				wm_mode = 0;
				XUnmapWindow(dpy, tabs);
				change_wm_mode(0);
			}
		}
		if(e.type == ButtonPress && e.xbutton.subwindow != None && e.xbutton.subwindow != root && e.xbutton.subwindow != win && wm_mode != 1){
			button_press(e.xbutton);
		}
		if(e.type == MotionNotify && start.subwindow != None && start.subwindow != win && start.subwindow != root && wm_mode != 1) {
			move_resize_window(e.xbutton);
		}
		else if(e.type == ButtonRelease && e.xbutton.subwindow != None && e.xbutton.subwindow != None && e.xbutton.subwindow != win && wm_mode != 1)
			button_release(chead);
		if(e.type == MapRequest && e.xmaprequest.window != tabs) {
			if(wm_mode == 1) {
				XMapWindow(dpy, tabs);
				draw_tabs();
			}
			map_request(e.xmaprequest);
		}
		else if(e.type == DestroyNotify && e.xdestroywindow.window != root && e.xdestroywindow.window != win)
			destroy_frame(e.xdestroywindow);
		if(e.type == Expose){
			expose_bar();
			draw_bar(prev_tag, active_tag);
			XFlush(dpy);
		}
		prev_num_clients = num_clients;
	}
}


int main(int argc, char ** argv){
	XInitThreads();
	init();
	run();
	return 0;
}
