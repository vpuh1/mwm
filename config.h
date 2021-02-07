#define MODKEY Mod4Mask
/* tags for bar */
char *bar_tags[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
/* default font -- fixed */
static const char font_name[] = "-*-helvetica-*-r-*-*-14-*-*-*-*-*-*-*";
/* colors */
char accent_color[] = "#1E81B0";
char fg_color[] = "#FFFFFF";
char bg_color[] = "#21130D";

static const char *term_name[]  = { "st", NULL };

static Key keys[] = {
	{ MODKEY, XK_1, change_ws, {.tag = 0} },
	{ MODKEY, XK_2, change_ws, {.tag = 1} },
	{ MODKEY, XK_3, change_ws, {.tag = 2} },
	{ MODKEY, XK_4, change_ws, {.tag = 3} },
	{ MODKEY, XK_5, change_ws, {.tag = 4} },
	{ MODKEY, XK_6, change_ws, {.tag = 5} },
	{ MODKEY, XK_7, change_ws, {.tag = 6} },
	{ MODKEY, XK_8, change_ws, {.tag = 7} },
	{ MODKEY, XK_9, change_ws, {.tag = 8} },
	{ MODKEY|ShiftMask, XK_1, move_to_ws, {.tag = 0} },
	{ MODKEY|ShiftMask, XK_2, move_to_ws, {.tag = 1} },
	{ MODKEY|ShiftMask, XK_3, move_to_ws, {.tag = 2} },
	{ MODKEY|ShiftMask, XK_4, move_to_ws, {.tag = 3} },
	{ MODKEY|ShiftMask, XK_5, move_to_ws, {.tag = 4} },
	{ MODKEY|ShiftMask, XK_6, move_to_ws, {.tag = 5} },
	{ MODKEY|ShiftMask, XK_7, move_to_ws, {.tag = 6} },
	{ MODKEY|ShiftMask, XK_8, move_to_ws, {.tag = 7} },
	{ MODKEY|ShiftMask, XK_9, move_to_ws, {.tag = 8} },
	{ MODKEY, XK_Return, spawn, {.name = term_name} },
	{ MODKEY, XK_Tab, change_focus, {.i = 1} },
	{ MODKEY, XK_m, change_mode, {.i = 1} },
	{ MODKEY, XK_f, change_mode, {.i = 0} },
	{ MODKEY|ShiftMask, XK_c, destroy_window, {.i = 0} },
};
