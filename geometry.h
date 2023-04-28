#pragma once

#include <stdbool.h>

#define TOP_ROW    4
#define BOT_ROW    21
#define LEFT_EDGE  9
#define RIGHT_EDGE 70

#define BOARD_WIDTH  ((RIGHT_EDGE - LEFT_EDGE) + 1)
#define BOARD_HEIGHT ((BOT_ROW - TOP_ROW) + 1)

#define TICKS_PER_SEC 50 // affects speed

typedef struct {
	int x, y;
} vec2i;
typedef struct {
	int width, height;
} size2i;

typedef struct {
	vec2i pos; // (top left point of rectangle)
	size2i size;
} rect2i;

bool vec2i_eq(vec2i a, vec2i b);
bool size2i_eq(size2i a, size2i b);

bool rect2i_eq(rect2i a, rect2i b);

bool point_in_rect(vec2i point, rect2i rect);

vec2i rect_bottom_right(rect2i rect);
