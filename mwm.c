#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include "config.h"

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
static void open_display()
{
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf (stderr, "Could not open display\n");
        exit (1);
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
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
	XColor inactive_ws_bg_color;
	Colormap colormap1;
	colormap1 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap1, inactive_ws_bg, &inactive_ws_bg_color);
	XAllocColor(dpy, colormap1, &inactive_ws_bg_color);
	XSetBackground(dpy, bar.gc, inactive_ws_bg_color.pixel); 
	XColor inactive_ws_fg_color;
	Colormap colormap2;
	colormap2 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap2, inactive_ws_fg, &inactive_ws_fg_color);
	XAllocColor(dpy, colormap2, &inactive_ws_fg_color);
	XSetForeground(dpy, bar.gc, inactive_ws_fg_color.pixel); 
	XColor title_bg_color;
	Colormap colormap3;
	colormap3 = DefaultColormap(dpy, 0);
	XParseColor(dpy, colormap3, title_bg, &title_bg_color);
	XAllocColor(dpy, colormap3, &title_bg_color);
	XSetForeground(dpy, title_gc, title_bg_color.pixel);
	XSetForeground(dpy, bg_gc, inactive_ws_bg_color.pixel);
}


static void create_bar() {
    win = XCreateSimpleWindow (dpy, root, 0, 0, display_width, bar.height, 0, BlackPixel(dpy, 0), BlackPixel(dpy, 0));
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
		if(i == active_tag){
			strcpy(bg, active_ws_bg);
		}
		else{
			strcpy(bg, inactive_ws_bg);
		}
		XDrawString(dpy, win, bar.gc, x, y, bar.text[i], bar.text_len[i]);
	}
}

void expose_bar() {
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

static void run() {
	XSetLocaleModifiers("");

    XIM xim = XOpenIM(dpy, 0, 0, 0);
    if(!xim){
        // fallback to internal input method
        XSetLocaleModifiers("@im=none");
        xim = XOpenIM(dpy, 0, 0, 0);
    }

    // X input context, you can have multiple for text boxes etc, but having a
    // single one is the easiest.

    XIC xic = XCreateIC(xim,
                        XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
                        XNClientWindow, win,
                        XNFocusWindow,  win,
                        NULL);

    XSetICFocus(xic);

    // we want key presses

    XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask);
    while (1) {
        XEvent e;
        XNextEvent (dpy, & e);
		if(e.type == KeyPress){
			/*Status status;
            KeySym keysym = NoSymbol;
			char text[32] = {};

			e.xkey.state &= ~ControlMask;
			Xutf8LookupString(xic, &e.xkey, text, sizeof(text) - 1, &keysym, &status);

			if(status == XBufferOverflow){
				// an IME was probably used, and wants to commit more than 32 chars.
				// ignore this fairly unlikely case for now
			}

			if(status == XLookupChars){
				// some characters were returned without an associated key,
				// again probably the result of an IME
				printf("Got chars: (%s)\n", text);
			}

			if(status == XLookupBoth){
				// we got one or more characters with an associated keysym
				// (all the keysyms are listed in /usr/include/X11/keysymdef.h)

				char* sym_name = XKeysymToString(keysym);
				printf("Got both: (%s), (%s)\n", text, sym_name);
			}

			if(status == XLookupKeySym){
				// a key without text on it
				char* sym_name = XKeysymToString(keysym);
				printf("Got keysym: (%s)\n", sym_name);
			}
			if(keysym == XK_Return) */
			if(e.xkey.state){
				if(e.xkey.keycode >= 10 && e.xkey.keycode <= 18){
					prev_tag = active_tag;
					active_tag = e.xkey.keycode-10;
					draw_bar(prev_tag, active_tag);
				}
			}
		}
		else if(e.type == Expose){
			expose_bar();
			XFlush(dpy);
		}
    }
}


int main(int argc, char ** argv){
	init();
    run();
    return 0;
}

