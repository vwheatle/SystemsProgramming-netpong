#pragma once

#include "geometry.h"

#define PADDLE_HEIGHT   6
#define PADDLE_START_Y  ((LINES - PADDLE_HEIGHT) / 2)
#define PADDLE_SIZE     ((size2i) {1, PADDLE_HEIGHT})

typedef struct {
	rect2i rect;

	rect2i draw_rect;
	bool redraw;
} wall_obj;

// ouugh walls are just paddles that don't move

void wall_setup(wall_obj *);
void wall_update(wall_obj *);
bool wall_draw(wall_obj *);

void paddle_update(wall_obj *);
bool paddle_draw(wall_obj *);
