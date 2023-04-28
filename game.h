#pragma once

#include <stdbool.h>

#include "wall.h" // -> wall_obj
#include "ball.h" // -> ball_obj

typedef struct {
	bool playing;
	int serves;

	ball_obj ball[1];

	wall_obj paddle[1];
	wall_obj wall[3];
} game_obj;

void game_setup(game_obj *);
void game_input(game_obj *, int);
void game_update(game_obj *);
bool game_draw(game_obj *);
