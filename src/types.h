#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma pack on
typedef union rgba_t { 
	struct { 
		uint8_t r, g, b, a;
	} as_rgba;
	uint8_t as_components[4];
	uint32_t as_uint32;
} rgba_t;
#pragma pack off

typedef struct {
	float x, y;
} vec2_t;


typedef struct {
	int32_t x, y;
} vec2i_t;


typedef struct {
	float x, y, z;
} vec3_t;

typedef union {
	float m[16];
	float cols[4][4];
} mat4_t;

typedef struct {
	vec3_t pos;
	vec2_t uv;
	rgba_t color;
} vertex_t;

typedef struct {
	vertex_t vertices[3];
} tris_t;

#define vec2(X, Y) ((vec2_t){X, Y})
#define vec3(X, Y, Z) ((vec3_t){X, Y, Z})
#define vec2i(X, Y) ((vec2i_t){X, Y})

#ifdef __plan9__

static rgba_t
rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	rgba_t x;

	x.as_rgba.r = r;
	x.as_rgba.g = g;
	x.as_rgba.b = b;
	x.as_rgba.a = a;
	return x;
}

static mat4_t
mat4(float m0,float m1,float m2,float m3,float m4,float m5,float m6,float m7,float m8,float m9,float m10,float m11,float m12,float m13,float m14, float m15)
{
	mat4_t r;

	r.m[0] = m0;
	r.m[1] = m1;
	r.m[2] = m2;
	r.m[3] = m3;
	r.m[4] = m4;
	r.m[5] = m5;
	r.m[6] = m6;
	r.m[7] = m7;
	r.m[8] = m8;
	r.m[9] = m9;
	r.m[10] = m10;
	r.m[11] = m11;
	r.m[12] = m12;
	r.m[13] = m13;
	r.m[14] = m14;
	r.m[15] = m15;
	return r;
}

static mat4_t
mat4_identity(void)
{
	return mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}

#else

#define rgba(R, G, B, A) ((rgba_t){{R, G, B, A}})

#define mat4(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15) \
	(mat4_t){{ \
		m0,   m1,  m2,  m3, \
		m4,   m5,  m6,  m7, \
		m8,   m9, m10, m11, \
		m12, m13, m14, m15  \
	}}

#define mat4_identity() mat4( \
		1, 0, 0, 0, \
		0, 1, 0, 0, \
		0, 0, 1, 0, \
		0, 0, 0, 1 \
	)

#endif

static inline vec2_t vec2_mulf(vec2_t a, float f) {
	return vec2(
		a.x * f,
		a.y * f
	);
}

static inline vec2i_t vec2i_mulf(vec2i_t a, float f) {
	return vec2i(
		a.x * f,
		a.y * f
	);
}


static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
	return vec3(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	);
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
	return vec3(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	);
}

static inline vec3_t vec3_mul(vec3_t a, vec3_t b) {
	return vec3(
		a.x * b.x,
		a.y * b.y,
		a.z * b.z
	);
}

static inline vec3_t vec3_mulf(vec3_t a, float f) {
	return vec3(
		a.x * f,
		a.y * f,
		a.z * f
	);
}

static inline vec3_t vec3_inv(vec3_t a) {
	return vec3(-a.x, -a.y, -a.z);
}

static inline vec3_t vec3_divf(vec3_t a, float f) {
	return vec3(
		a.x / f,
		a.y / f,
		a.z / f
	);
}

static inline float vec3_len(vec3_t a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
	return vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

static inline float vec3_dot(vec3_t a, vec3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
	return vec3(
		a.x + t * (b.x - a.x),
		a.y + t * (b.y - a.y),
		a.z + t * (b.z - a.z)
	);
}

static inline vec3_t vec3_normalize(vec3_t a) {
	float length = vec3_len(a);
	return vec3(
		a.x / length,
		a.y / length,
		a.z / length
	);
}

static inline float wrap_angle(float a) {
	a = fmod(a + M_PI, M_PI * 2);
	if (a < 0) {
		a += M_PI * 2;
	}
	return a - M_PI;
}

float vec3_angle(vec3_t a, vec3_t b);
vec3_t vec3_wrap_angle(vec3_t a);
static vec3_t vec3_normalize(vec3_t a);
vec3_t vec3_project_to_ray(vec3_t p, vec3_t r0, vec3_t r1);
float vec3_distance_to_plane(vec3_t p, vec3_t plane_pos, vec3_t plane_normal);
vec3_t vec3_reflect(vec3_t incidence, vec3_t normal, float f);

static float wrap_angle(float a);

vec3_t vec3_transform(vec3_t a, mat4_t *mat);
void mat4_set_translation(mat4_t *mat, vec3_t pos);
void mat4_set_yaw_pitch_roll(mat4_t *m, vec3_t rot);
void mat4_set_roll_pitch_yaw(mat4_t *mat, vec3_t rot);
void mat4_translate(mat4_t *mat, vec3_t translation);
void mat4_mul(mat4_t *res, mat4_t *a, mat4_t *b);

#endif
