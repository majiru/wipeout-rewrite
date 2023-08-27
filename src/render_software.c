#include "system.h"
#include "render.h"
#include "mem.h"
#include "utils.h"
#include "platform.h"

#define NEAR_PLANE 16.0
#define FAR_PLANE (RENDER_FADEOUT_FAR)
#define TEXTURES_MAX 1024


typedef struct {
	vec2i_t size;
	rgba_t *pixels;
} render_texture_t;

static void line(vec2i_t p0, vec2i_t p1, rgba_t color);

static rgba_t *screen_buffer;
static int32_t screen_pitch;
static int32_t screen_ppr;
static vec2i_t screen_size;

static mat4_t view_mat;
static mat4_t mvp_mat;
static mat4_t projection_mat;
static mat4_t sprite_mat;

static render_texture_t textures[TEXTURES_MAX];
static uint32_t textures_len;

uint16_t RENDER_NO_TEXTURE;


void render_init(vec2i_t screen_size) {
#ifdef __plan9__
	setfcr(0);
#endif
	render_set_screen_size(screen_size);
	textures_len = 0;

	view_mat = mat4_identity();
	mvp_mat = mat4_identity();
	projection_mat = mat4_identity();
	sprite_mat = mat4_identity();
	rgba_t white_pixels[4] = {
		rgba(128,128,128,255), rgba(128,128,128,255),
		rgba(128,128,128,255), rgba(128,128,128,255)
	};
	RENDER_NO_TEXTURE = render_texture_create(2, 2, white_pixels);
}

void render_cleanup() {}

void render_set_screen_size(vec2i_t size) {
	screen_size = size;

	float aspect = (float)size.x / (float)size.y;
	float fov = (73.75 / 180.0) * 3.14159265358;
	float f = 1.0 / tan(fov / 2);
	float nf = 1.0 / (NEAR_PLANE - FAR_PLANE);
	projection_mat = mat4(
		f / aspect, 0, 0, 0,
		0, f, 0, 0, 
		0, 0, (FAR_PLANE + NEAR_PLANE) * nf, -1, 
		0, 0, 2 * FAR_PLANE * NEAR_PLANE * nf, 0
	);
}

void render_set_resolution(render_resolution_t res) {}
void render_set_post_effect(render_post_effect_t post) {}

vec2i_t render_size() {
	return screen_size;
}


void render_frame_prepare() {
	screen_buffer = platform_get_screenbuffer(&screen_pitch);
	screen_ppr = screen_pitch / sizeof(rgba_t);

	rgba_t color = rgba(0, 0, 0, 255);
	for (uint32_t y = 0; y < screen_size.y; y++) {
		for (uint32_t x = 0; x < screen_size.x; x++) {
			screen_buffer[y * screen_ppr + x] = color;
		}
	}
}

void render_frame_end() {}

void render_set_view(vec3_t pos, vec3_t angles) {
	view_mat = mat4_identity();
	mat4_set_translation(&view_mat, vec3(0, 0, 0));
	mat4_set_roll_pitch_yaw(&view_mat, vec3(angles.x, -angles.y + M_PI, angles.z + M_PI));
	mat4_translate(&view_mat, vec3_inv(pos));
	mat4_set_yaw_pitch_roll(&sprite_mat, vec3(-angles.x, angles.y - M_PI, 0));

	mat4_t _mat = mat4_identity();
	render_set_model_mat(&_mat);
}

void render_set_view_2d() {
	float near = -1;
	float far = 1;
	float left = 0;
	float right = screen_size.x;
	float bottom = screen_size.y;
	float top = 0;
	float lr = 1 / (left - right);
	float bt = 1 / (bottom - top);
	float nf = 1 / (near - far);
	mvp_mat = mat4(
		-2 * lr,  0,  0,  0,
		0,  -2 * bt,  0,  0,
		0,        0,  2 * nf,    0, 
		(left + right) * lr, (top + bottom) * bt, (far + near) * nf, 1
	);
}

void render_set_model_mat(mat4_t *m) {
	mat4_t vm_mat;
	mat4_mul(&vm_mat, &view_mat, m);
	mat4_mul(&mvp_mat, &projection_mat, &vm_mat);
}

void render_set_depth_write(bool enabled) {}
void render_set_depth_test(bool enabled) {}
void render_set_depth_offset(float offset) {}
void render_set_screen_position(vec2_t pos) {}
void render_set_blend_mode(render_blend_mode_t mode) {}
void render_set_cull_backface(bool enabled) {}

vec3_t render_transform(vec3_t pos) {
	return vec3_transform(vec3_transform(pos, &view_mat), &projection_mat);
}

void render_push_tris(tris_t tris, uint16_t texture_index) {
	float w2 = screen_size.x * 0.5;
	float h2 = screen_size.y * 0.5;

	vec3_t p0 = vec3_transform(tris.vertices[0].pos, &mvp_mat);
	vec3_t p1 = vec3_transform(tris.vertices[1].pos, &mvp_mat);
	vec3_t p2 = vec3_transform(tris.vertices[2].pos, &mvp_mat);
	if (p0.z >= 1.0 || p1.z >= 1.0 || p2.z >= 1.0) {
		return;
	}

	vec2i_t sc0 = vec2i(p0.x * w2 + w2, h2 - p0.y * h2);
	vec2i_t sc1 = vec2i(p1.x * w2 + w2, h2 - p1.y * h2);
	vec2i_t sc2 = vec2i(p2.x * w2 + w2, h2 - p2.y * h2);

	rgba_t color = tris.vertices[0].color;
	color.as_rgba.r = min(color.as_rgba.r * 2, 255);
	color.as_rgba.g = min(color.as_rgba.g * 2, 255);
	color.as_rgba.b = min(color.as_rgba.b * 2, 255);

	float _v = color.as_rgba.a * (1.0-p0.z) * FAR_PLANE * (2.0/255.0);
	float _min = 0;
	float _max = 255;
	color.as_rgba.a = _v > _max ? _max : _v < _min ? _min : _v;

	line(sc0, sc1, color);
	line(sc1, sc2, color);
	line(sc2, sc0, color);
}

void render_push_sprite(vec3_t pos, vec2i_t size, rgba_t color, uint16_t texture_index) {
	tris_t _tris;
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	vec3_t p0 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5, -size.y * 0.5, 0), &sprite_mat));
	vec3_t p1 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5, -size.y * 0.5, 0), &sprite_mat));
	vec3_t p2 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5,  size.y * 0.5, 0), &sprite_mat));
	vec3_t p3 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5,  size.y * 0.5, 0), &sprite_mat));

	render_texture_t *t = &textures[texture_index];
	_tris.vertices[0] = (vertex_t){p0, (vec2_t){0, 0}, color};
	_tris.vertices[1] = (vertex_t){p1, (vec2_t){0 + t->size.x ,0}, color};
	_tris.vertices[2] = (vertex_t){p2, (vec2_t){0, 0 + t->size.y}, color};
	render_push_tris(_tris, texture_index);

	_tris.vertices[0] = (vertex_t){p2, (vec2_t){0, 0 + t->size.y}, color};
	_tris.vertices[1] = (vertex_t){p1, (vec2_t){0 + t->size.x, 0}, color};
	_tris.vertices[2] = (vertex_t){p3, (vec2_t){0 + t->size.x, 0 + t->size.y}, color};
	render_push_tris(_tris, texture_index);
}

void render_push_2d(vec2i_t pos, vec2i_t size, rgba_t color, uint16_t texture_index) {
	render_push_2d_tile(pos, vec2i(0, 0), render_texture_size(texture_index), size, color, texture_index);
}

void render_push_2d_tile(vec2i_t pos, vec2i_t uv_offset, vec2i_t uv_size, vec2i_t size, rgba_t color, uint16_t texture_index) {
	tris_t _tris;
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);

	_tris.vertices[0] = (vertex_t){(vec3_t){pos.x, pos.y + size.y, 0}, (vec2_t){uv_offset.x , uv_offset.y + uv_size.y}, color};
	_tris.vertices[1] = (vertex_t){(vec3_t){pos.x + size.x, pos.y, 0}, (vec2_t){uv_offset.x +  uv_size.x, uv_offset.y}, color};
	_tris.vertices[2] = (vertex_t){(vec3_t){pos.x, pos.y, 0}, (vec2_t){uv_offset.x , uv_offset.y}, color};
	render_push_tris(_tris, texture_index);

	_tris.vertices[0] = (vertex_t){(vec3_t){pos.x + size.x, pos.y + size.y, 0}, (vec2_t){uv_offset.x + uv_size.x, uv_offset.y + uv_size.y}, color};
	_tris.vertices[1] = (vertex_t){(vec3_t){pos.x + size.x, pos.y, 0}, (vec2_t){uv_offset.x + uv_size.x, uv_offset.y}, color};
	_tris.vertices[2] = (vertex_t){(vec3_t){pos.x, pos.y + size.y, 0}, (vec2_t){uv_offset.x , uv_offset.y + uv_size.y}, color};
	render_push_tris(_tris, texture_index);
}


uint16_t render_texture_create(uint32_t width, uint32_t height, rgba_t *pixels) {
	error_if(textures_len >= TEXTURES_MAX, "TEXTURES_MAX reached");

	uint32_t byte_size = width * height * sizeof(rgba_t);
	uint16_t texture_index = textures_len;
	
	textures[texture_index] = (render_texture_t){(vec2i_t){width, height}, NULL};
	// textures[texture_index] = (render_texture_t){{width, height}, mem_bump(byte_size)};
	// memcpy(textures[texture_index].pixels, pixels, byte_size);

	textures_len++;
	return texture_index;
}

vec2i_t render_texture_size(uint16_t texture_index) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);
	return textures[texture_index].size;
}

void render_texture_replace_pixels(int16_t texture_index, rgba_t *pixels) {
	error_if(texture_index >= textures_len, "Invalid texture %d", texture_index);
	render_texture_t *t = &textures[texture_index];
	// memcpy(t->pixels, pixels, t->size.x * t->size.y * sizeof(rgba_t));
}

uint16_t render_textures_len() {
	return textures_len;
}

void render_textures_reset(uint16_t len) {
	error_if(len > textures_len, "Invalid texture reset len %d >= %d", len, textures_len);
	textures_len = len;
}

void render_textures_dump(const char *path) {}

static uint8_t
lerp8(uint8_t a, uint8_t b, float t)
{
	return a + (b - a) * t;
}

// -----------------------------------------------------------------------------

static inline rgba_t color_mix(rgba_t in, rgba_t out) {
	return rgba(
		lerp8(in.as_rgba.r, out.as_rgba.r, out.as_rgba.a/255.0),
		lerp8(in.as_rgba.g, out.as_rgba.g, out.as_rgba.a/255.0),
		lerp8(in.as_rgba.b, out.as_rgba.b, out.as_rgba.a/255.0),
		1
	);
}

typedef enum {
	CLIP_INSIDE = 0,
	CLIP_LEFT   = (1<<0),
	CLIP_RIGHT  = (1<<1),
	CLIP_BOTTOM = (1<<2),
	CLIP_TOP    = (1<<3),
} clip_code_t;

static inline clip_code_t clip_code(vec2i_t p) {
	clip_code_t cc = CLIP_INSIDE;
	if (p.x < 0) {
		flags_add(cc, CLIP_LEFT);
	}
	else if (p.x >= screen_size.x) {
		flags_add(cc, CLIP_RIGHT);	
	}
	if (p.y < 0) {
		flags_add(cc, CLIP_BOTTOM);
	}
	else if (p.y >= screen_size.y) {
		flags_add(cc, CLIP_TOP);	
	}
	return cc;
}

static void line(vec2i_t p0, vec2i_t p1, rgba_t color) {
	// Cohen Sutherland Line Clipping
	clip_code_t cc0 = clip_code(p0);
	clip_code_t cc1 = clip_code(p1);
	bool accept = false;

	vec2i_t ss = vec2i(screen_size.x-1, screen_size.y-1);
	while (true) {
		if (!(cc0 | cc1)) {
			accept = true;
			break;
		}
		else if (cc0 & cc1) {
			break;
		}
		else {
			vec2i_t r = p0;
			clip_code_t cc_out = cc0 ? cc0 : cc1;

			if (flags_is(cc_out, CLIP_TOP)) {
				r.x = p0.x + (p1.x - p0.x) * (ss.y - p0.y) / (p1.y - p0.y);
				r.y = ss.y;
			}
			else if (flags_is(cc_out, CLIP_BOTTOM)) {
				r.x = p0.x + (p1.x - p0.x) * (-p0.y) / (p1.y - p0.y);
				r.y = 0;
			}
			else if (flags_is(cc_out, CLIP_RIGHT)) {
				r.y = p0.y + (p1.y - p0.y) * (ss.x - p0.x) / (p1.x - p0.x);
				r.x = ss.x;
			}
			else if (flags_is(cc_out, CLIP_LEFT)) {
				r.y = p0.y + (p1.y - p0.y) * (-p0.x) / (p1.x - p0.x);
				r.x = 0;
			}

			if (cc_out == cc0) {
				p0.x = r.x;
				p0.y = r.y;
				cc0 = clip_code(p0);
			}
			else {
				p1.x = r.x;
				p1.y = r.y;
				cc1 = clip_code(p1);
			}
		}
	}
	if (!accept) {
		return;
	}

	// Bresenham's line algorithm
	bool steep = false; 
	if (abs(p0.x - p1.x) < abs(p0.y - p1.y)) {
		int32_t tmp = p0.x;
		p0.x = p0.y;
		p0.y = tmp;

		tmp = p1.x;
		p1.x = p1.y;
		p1.y = tmp;
		steep = true;
	} 
	if (p0.x > p1.x) { 

		int32_t tmp = p0.x;
		p0.x = p1.x;
		p1.x = tmp;

		tmp = p0.y;
		p0.y = p1.y;
		p1.y = tmp; 
	} 
	int32_t dx = p1.x - p0.x; 
	int32_t dy = p1.y - p0.y; 
	int32_t derror2 = abs(dy) * 2; 
	int32_t error2 = 0; 
	int32_t y = p0.y;
	int32_t ydir = (p1.y > p0.y ? 1 : -1);

	if (steep) {
		for (int32_t x = p0.x; x <= p1.x; x++) {
			screen_buffer[x * screen_ppr + y] = color_mix(screen_buffer[x * screen_ppr + y], color);
			error2 += derror2; 
			if (error2 > dx) { 
				y += ydir;
				error2 -= dx * 2; 
			} 
		}
	}
	else {
		for (int32_t x = p0.x; x <= p1.x; x++) {
			screen_buffer[y * screen_ppr + x] = color_mix(screen_buffer[y * screen_ppr + x], color);
			error2 += derror2; 
			if (error2 > dx) { 
				y += ydir;
				error2 -= dx * 2; 
			} 
		}
	}
}
