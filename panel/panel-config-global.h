#include "panel-widget.h"
#include "panel-types.h"

#ifndef PANEL_CONFIG_GLOBAL_H
#define PANEL_CONFIG_GLOBAL_H

typedef struct _GlobalConfig GlobalConfig;
struct _GlobalConfig {
	int auto_hide_step_size;
	int explicit_hide_step_size;
	int drawer_step_size;
	int minimized_size;
	int minimize_delay;
	int tooltips_enabled;
	int show_small_icons;
	int prompt_for_logout;
	PanelMovementType movement_type;
	int disable_animations;
	int applet_padding;
	int tiles_enabled;
	char *tile_up[LAST_TILE];
	char *tile_down[LAST_TILE];
	int tile_border[LAST_TILE];
	int tile_depth[LAST_TILE];
};

void panel_config_global(void);

#endif /* PANEL_CONFIG_H */
