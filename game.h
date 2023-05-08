#pragma once

#include <stdbool.h>

#include "networking.h"

#include "geometry.h"

#include "wall.h" // -> wall_obj
#include "ball.h" // -> ball_obj

typedef enum { SIDE_LEFT = 0, SIDE_RIGHT = 1 } which_side;

typedef struct {
	network_info *network;
	which_side side;
	int ticks_per_sec;

	char *name[2];
	char *message;

	bool playing, error;
	bool in_this_field;
	int serves;
	int scores[2];

	rect2i field;

	ball_obj ball;

	wall_obj paddle[1];
	wall_obj wall[2];
} game_obj;

void game_handshake(game_obj *, network_info *);

void game_setup(game_obj *);
void game_input(game_obj *, int);
void game_event(game_obj *);
void game_update(game_obj *);
bool game_draw(game_obj *);

void game_destroy(game_obj *);
