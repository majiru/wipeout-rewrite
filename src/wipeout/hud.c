#include "../types.h"
#include "../mem.h"
#include "../utils.h"
#include "../system.h"

#include "object.h"
#include "track.h"
#include "ship.h"
#include "weapon.h"
#include "hud.h"
#include "droid.h"
#include "camera.h"
#include "image.h"
#include "ship_ai.h"
#include "game.h"
#include "ui.h"

static texture_list_t weapon_icon_textures;
static uint16_t target_reticle;

typedef struct {
	vec2i_t offset;
	uint16_t height;
	rgba_t color;
} speedo_bar_t;

struct {
	uint16_t width;
	uint16_t skew;
	speedo_bar_t bars[13];
} speedo = {
	.width = 121,
	.skew = 2,
};

static void
initspeedo(void)
{
	speedo.bars[0] = (speedo_bar_t){(vec2i_t){6, 12}, 10, rgba( 66,  16,  49, 255)};
	speedo.bars[1] = (speedo_bar_t){(vec2i_t){13, 12}, 10, rgba(115,  33,  90, 255)};
	speedo.bars[2] = (speedo_bar_t){(vec2i_t){20, 12}, 10, rgba(132,  58, 164, 255)};
	speedo.bars[3] = (speedo_bar_t){(vec2i_t){27, 12}, 10, rgba( 99,  90, 197, 255)};
	speedo.bars[4] = (speedo_bar_t){(vec2i_t){34, 12}, 10, rgba( 74, 148, 181, 255)};
	speedo.bars[5] = (speedo_bar_t){(vec2i_t){41, 12}, 10, rgba( 66, 173, 115, 255)};
	speedo.bars[6] = (speedo_bar_t){(vec2i_t){50, 10}, 12, rgba( 99, 206,  58, 255)};
	speedo.bars[7] = (speedo_bar_t){(vec2i_t){59, 8}, 12, rgba(189, 206,  41, 255)};
	speedo.bars[8] = (speedo_bar_t){(vec2i_t){ 69, 5}, 13, rgba(247, 140,  33, 255)};
	speedo.bars[9] = (speedo_bar_t){(vec2i_t){81, 2}, 15, rgba(255, 197,  49, 255)};
	speedo.bars[10] = (speedo_bar_t){(vec2i_t){95, 1}, 16, rgba(255, 222, 115, 255)};
	speedo.bars[11] = (speedo_bar_t){(vec2i_t){110, 1}, 16, rgba(255, 239, 181, 255)};
	speedo.bars[12] = (speedo_bar_t){(vec2i_t){126, 1}, 16, rgba(255, 255, 255, 255)};
}


static uint16_t speedo_facia_texture;

void hud_load() {
	initspeedo();
	speedo_facia_texture = image_get_texture("wipeout/textures/speedo.tim");
	target_reticle = image_get_texture_semi_trans("wipeout/textures/target2.tim");
	weapon_icon_textures = image_get_compressed_textures("wipeout/common/wicons.cmp");
}

static void hud_draw_speedo_bar(vec2i_t *pos, const speedo_bar_t *a, const speedo_bar_t *b, float f, rgba_t color_override) {
	rgba_t left_color, right_color;
	tris_t _tris;
	if (color_override.as_uint32 > 0) {
		left_color = color_override;
		right_color = color_override;
	}
	else {
		left_color = a->color;
		float _za, _zb, _zc, _zd;
		float _a;
		_a = a->color.as_rgba.r;
		_za = _a + (b->color.as_rgba.r - _a) * f;

		_a = a->color.as_rgba.g;
		_zb = _a + (b->color.as_rgba.g - _a) * f;

		_a = a->color.as_rgba.b;
		_zc = _a + (b->color.as_rgba.b - _a) * f;

		_a = a->color.as_rgba.a;
		_zd = _a + (b->color.as_rgba.a - _a) * f;
		right_color = rgba(
			_za,
			_zb,
			_zc,
			_zd
		);
	}

	float right_h;
	{
		float _a;
		float _za;
		_a = a->height;
		_za = _a + (b->height - _a) * f;
		right_h = _za;
	}

	vec2i_t top_left     = vec2i(a->offset.x + 1, a->offset.y);
	vec2i_t bottom_left  = vec2i(a->offset.x + 1 - a->height / speedo.skew, a->offset.y + a->height);
	vec2i_t top_right, bottom_right;
	{
		float _a;
		float _za, _zb;
		_a = a->offset.x + 1;
		_za = _a + (b->offset.x - _a) * f;

		_a = a->offset.y;
		_zb = _a + (b->offset.y - _a) * f;
		top_right    = vec2i(_za, _zb);
		bottom_right = vec2i(top_right.x - right_h / speedo.skew, top_right.y + right_h);
	}

	top_left     = ui_scaled(top_left);
	bottom_left  = ui_scaled(bottom_left);
	top_right    = ui_scaled(top_right);
	bottom_right = ui_scaled(bottom_right);

	_tris.vertices[0] = (vertex_t) {
		(vec3_t){pos->x + bottom_left.x, pos->y + bottom_left.y, 0},
		(vec2_t){0, 0},
		left_color,
	};
	_tris.vertices[1] = (vertex_t) {
		(vec3_t){pos->x + top_right.x, pos->y + top_right.y, 0},
		(vec2_t){0, 0},
		right_color,
	};
	_tris.vertices[2] = (vertex_t) {
		(vec3_t){pos->x + top_left.x, pos->y + top_left.y, 0},
		(vec2_t){0, 0},
		left_color,
	};
	render_push_tris(_tris, RENDER_NO_TEXTURE);

	_tris.vertices[0] = (vertex_t) {
		(vec3_t){pos->x + bottom_right.x, pos->y + bottom_right.y, 0},
		(vec2_t){0, 0},
		right_color
	};
	_tris.vertices[1] = (vertex_t) {
		(vec3_t){pos->x + top_right.x, pos->y + top_right.y, 0},
		(vec2_t){0, 0},
		right_color
	};
	_tris.vertices[2] = (vertex_t) {
		(vec3_t){pos->x + bottom_left.x, pos->y + bottom_left.y, 0},
		(vec2_t){0, 0},
		left_color
	};
	render_push_tris(_tris, RENDER_NO_TEXTURE);
}

static void hud_draw_speedo_bars(vec2i_t *pos, float f, rgba_t color_override) {
	if (f <= 0) {
		return;
	}

	if (f - floor(f) > 0.9) {
		f = ceil(f);
	}
	if (f > 13) {
		f = 13;
	}

	int bars = f;
	for (int i = 1; i < bars; i++) {
		hud_draw_speedo_bar(pos, &speedo.bars[i - 1], &speedo.bars[i], 1, color_override);
	}

	if (bars > 12) {
		return;
	}

	float last_bar_fraction = f - bars + 0.1;
	if (last_bar_fraction <= 0) {
		return;
	}

	if (last_bar_fraction > 1) {
		last_bar_fraction = 1;
	}
	int last_bar = bars == 0 ? 1 : bars;
	hud_draw_speedo_bar(pos, &speedo.bars[last_bar - 1], &speedo.bars[last_bar], last_bar_fraction, color_override);
}

static void hud_draw_speedo(int speed, int thrust) {
	vec2i_t facia_pos = ui_scaled_pos(UI_POS_BOTTOM | UI_POS_RIGHT, vec2i(-141, -45));
	vec2i_t bar_pos = ui_scaled_pos(UI_POS_BOTTOM | UI_POS_RIGHT, vec2i(-141, -40));
	hud_draw_speedo_bars(&bar_pos, thrust / 65.0, rgba(255, 0, 0, 128));
	hud_draw_speedo_bars(&bar_pos, speed / 2166.0, rgba(0, 0, 0, 0));
	render_push_2d(facia_pos, ui_scaled(render_texture_size(speedo_facia_texture)), rgba(128, 128, 128, 255), speedo_facia_texture);
}

static void hud_draw_target_icon(vec3_t position) {
	vec2i_t screen_size = render_size();
	vec2i_t size = ui_scaled(render_texture_size(target_reticle));
	vec3_t projected = render_transform(position);

	vec2i_t pos = vec2i(
		(( projected.x + 1.0) / 2.0) * screen_size.x - size.x / 2,
		((-projected.y + 1.0) / 2.0) * screen_size.y - size.y / 2
	);
	render_push_2d(pos, size, rgba(128, 128, 128, 128), target_reticle);
}

void hud_draw(ship_t *ship) {
	// Current lap time
	if (ship->lap >= 0) {
		ui_draw_time(ship->lap_time, ui_scaled_pos(UI_POS_BOTTOM | UI_POS_LEFT, vec2i(16, -30)), UI_SIZE_16, UI_COLOR_DEFAULT);
	
		for (int i = 0; i < ship->lap && i < NUM_LAPS-1; i++) {
			ui_draw_time(g.lap_times[ship->pilot][i], ui_scaled_pos(UI_POS_BOTTOM | UI_POS_LEFT, vec2i(16, -45 - (10 * i))), UI_SIZE_8, UI_COLOR_ACCENT);
		}
	}

	// Current Lap
	int display_lap = maxint(0, ship->lap + 1);
	ui_draw_text("LAP", ui_scaled(vec2i(15, 8)), UI_SIZE_8, UI_COLOR_ACCENT); 
	ui_draw_number(display_lap, ui_scaled(vec2i(10, 19)), UI_SIZE_16, UI_COLOR_DEFAULT); 
	int width = ui_char_width('0' + display_lap, UI_SIZE_16);
	ui_draw_text("OF", ui_scaled(vec2i((10 + width), 27)), UI_SIZE_8, UI_COLOR_ACCENT);
	ui_draw_number(NUM_LAPS, ui_scaled(vec2i((32 + width), 19)), UI_SIZE_16, UI_COLOR_DEFAULT);

	// Race Position
	if (g.race_type != RACE_TYPE_TIME_TRIAL) {
		ui_draw_text("POSITION", ui_scaled_pos(UI_POS_TOP | UI_POS_RIGHT, vec2i(-90, 8)), UI_SIZE_8, UI_COLOR_ACCENT);
		ui_draw_number(ship->position_rank, ui_scaled_pos(UI_POS_TOP | UI_POS_RIGHT, vec2i(-60, 19)), UI_SIZE_16, UI_COLOR_DEFAULT);
	}

	// Framerate
	if (save.show_fps) {
		ui_draw_text("FPS", ui_scaled(vec2i(16, 78)), UI_SIZE_8, UI_COLOR_ACCENT);
		ui_draw_number((int)(g.frame_rate), ui_scaled(vec2i(16, 90)), UI_SIZE_8, UI_COLOR_DEFAULT);
	}

	// Lap Record
	ui_draw_text("LAP RECORD", ui_scaled(vec2i(15, 43)), UI_SIZE_8, UI_COLOR_ACCENT);
	ui_draw_time(save.highscores[g.race_class][g.circut][g.highscore_tab].lap_record, ui_scaled(vec2i(15, 55)), UI_SIZE_8, UI_COLOR_DEFAULT);

	// Wrong way
	if (flags_not(ship->flags, SHIP_DIRECTION_FORWARD)) {
		ui_draw_text_centered("WRONG WAY", ui_scaled_pos(UI_POS_MIDDLE | UI_POS_CENTER, vec2i(-20, 0)), UI_SIZE_16, UI_COLOR_ACCENT);
	}

	// Speedo
	int speedo_speed = (g.camera.update_func == camera_update_attract_internal)
		? ship->speed * 7
		: ship->speed;
	hud_draw_speedo(speedo_speed, ship->thrust_mag);

	// Weapon icon
	if (ship->weapon_type != WEAPON_TYPE_NONE) {
		vec2i_t pos = ui_scaled_pos(UI_POS_TOP | UI_POS_CENTER, vec2i(-16, 20));
		vec2i_t size = ui_scaled(vec2i(32, 32));
		uint16_t icon = texture_from_list(weapon_icon_textures, ship->weapon_type-1);
		render_push_2d(pos, size, rgba(128,128,128,255), icon);
	}

	// Lives
	if (g.race_type == RACE_TYPE_CHAMPIONSHIP) {
		for (int i = 0; i < g.lives; i++) {
			ui_draw_icon(UI_ICON_STAR, ui_scaled_pos(UI_POS_BOTTOM | UI_POS_RIGHT, vec2i(-26 - 13 * i, -50)), UI_COLOR_DEFAULT);
		}
	}

	// Weapon target reticle
	if (ship->weapon_target) {
		hud_draw_target_icon(ship->weapon_target->position);
	}
}
