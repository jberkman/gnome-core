#include "panel-widget.h"
#include "panel-types.h"

#ifndef PANEL_CONFIG_GLOBAL_H
#define PANEL_CONFIG_GLOBAL_H

typedef struct _GlobalConfig GlobalConfig;
struct _GlobalConfig {
	int hide_speed;
	int minimized_size;
	int hide_delay;
	int show_delay;
	gboolean tooltips_enabled;
	gboolean keep_menus_in_memory;
	gboolean disable_animations;
	gboolean autoraise;
	PanelLayer layer;
	gboolean drawer_auto_close;
	gboolean highlight_when_over;
	gboolean confirm_panel_remove;
	gboolean keys_enabled;
	char *menu_key;
	  guint menu_keysym;  /* these are not really properties but */
	  guint menu_state;   /* from the above */
	char *run_key;
	  guint run_keysym;   /* these are not really properties but */
	  guint run_state;    /* from the above */
	char *screenshot_key;
	  guint screenshot_keysym;   /* these are not really properties but */
	  guint screenshot_state;    /* from the above */
	char *window_screenshot_key;
	  guint window_screenshot_keysym; /* these are not really properties */
	  guint window_screenshot_state;  /* but from the above */
	int menu_flags;
	gboolean use_large_icons;
	gboolean merge_menus;
	gboolean avoid_collisions;
	gboolean menu_check;
};

#endif /* PANEL_CONFIG_GLOBAL_H */
