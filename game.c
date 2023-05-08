#include <stdlib.h>

#include <string.h>

#include <curses.h>

#include "networking.h" // -> network_info

#include "geometry.h" // -> vec2i, rect2i
#include "game.h"     // -> game_obj
#include "wall.h"     // -> wall_obj, PADDLE_*

#include "utility.h"
#include "sppbtp.h"

void game_handshake(game_obj *game, network_info *network) {
	game->network = network;

	fprintf(stdout, "Enter your name (%d chars max): ", SPPBTP_ARGMAX);
	fflush(stdout); // may need to flush stdout to display this prompt correctly

	char name[SPPBTP_ARGMAX];
	fgets(name, sizeofarr(name), stdin);
	trim_whitespace(name, SPPBTP_ARGMAX);
	game->name[game->network->role] = strndup(name, SPPBTP_ARGMAX);

	// Have to initialize the field size here because
	// it gets modified by HELO...
	game->field = rect_from_corners((vec2i) {FIELD_LEFT_EDGE, FIELD_TOP_ROW},
		(vec2i) {FIELD_RIGHT_EDGE, FIELD_BOT_ROW});

	if (game->network->role == ROLE_SERVER)
		sppbtp_send_helo(game->network->socket, TICKS_PER_SEC,
			game->field.size.height, name);

	sppbtp_command cmd = sppbtp_recv(game->network->socket);
	if (cmd.valid) {
		switch (cmd.which) {
		case SPPBTP_HELO:
			if (game->network->role == ROLE_SERVER) {
				sppbtp_send_err(game->network->socket,
					"servers shouldn't get HELO commands");
				return;
			} else if (strncmp(cmd.data.helo.version, SPPBTP_VERSION,
						   sizeofarr(SPPBTP_VERSION)) != 0) {
				sppbtp_send_err(game->network->socket, "versions don't match");
				return;
			}
			game->field.size.height = cmd.data.helo.net_height;
			game->name[ROLE_SERVER] =
				strndup(cmd.data.helo.player_name, SPPBTP_ARGMAX);
			sppbtp_send_name(game->network->socket, name);
			break;
		case SPPBTP_NAME:
			if (game->network->role == ROLE_CLIENT) {
				sppbtp_send_err(game->network->socket,
					"clients shouldn't get NAME commands");
				return;
			} else if (strncmp(cmd.data.helo.version, SPPBTP_VERSION,
						   sizeofarr(SPPBTP_VERSION)) != 0) {
				sppbtp_send_err(game->network->socket, "versions don't match");
				return;
			}
			game->name[ROLE_CLIENT] =
				strndup(cmd.data.name.player_name, SPPBTP_ARGMAX);
			break;
		default:
			fprintf(stderr, "they didn't even give me their name.\n");
			sppbtp_send_err(game->network->socket, "needed HELO/NAME first");
			return;
		}
	} else {
		sppbtp_send_err(game->network->socket, "couldn't understand");
		return;
	}

	fprintf(stderr, "they called themself '%s'.\n",
		game->name[other_role(game->network->role)]);
}

// set up the initial game state.
void game_setup(game_obj *game) {
	game->playing = true;

	// field
	game->field = rect_from_corners((vec2i) {FIELD_LEFT_EDGE, FIELD_TOP_ROW},
		(vec2i) {FIELD_RIGHT_EDGE, FIELD_BOT_ROW});

	// serves (lives)
	game->serves = 3;

	// paddle
	if (game->network->role == ROLE_SERVER) {
		// if server, appear on left.
		game->paddle[0].rect =
			(rect2i) {{game->field.pos.x, PADDLE_START_Y}, PADDLE_SIZE};
	} else {
		// if client, appear on right.
		game->paddle[0].rect = (rect2i) {
			{rect_bottom_right(game->field).x, PADDLE_START_Y}, PADDLE_SIZE};
	}

	// walls
	game->wall[0].rect =
		(rect2i) {game->field.pos, {game->field.size.width, 1}};
	game->wall[1].rect =
		(rect2i) {{game->field.pos.x, rect_bottom_right(game->field).y},
			{game->field.size.width, 1}};

	// additional wall setup
	// (paddles and walls are the same thing aside from being in different
	//  arrays and having a different draw function.)
	for (size_t i = 0; i < sizeofarr(game->paddle); i++)
		wall_setup(&game->paddle[i]);
	for (size_t i = 0; i < sizeofarr(game->wall); i++)
		wall_setup(&game->wall[i]);

	// balls
	// (game supports more than one ball in play, but they may overlap.)
	for (size_t i = 0; i < sizeofarr(game->ball); i++) {
		game->ball[i].paddles = &game->paddle[0];
		game->ball[i].paddles_len = sizeofarr(game->paddle);

		game->ball[i].walls = &game->wall[0];
		game->ball[i].walls_len = sizeofarr(game->wall);

		game->ball[i].field = &game->field;

		ball_setup(&game->ball[i]);
	}
}

void game_input(game_obj *game, int key) {
	if (key == 'Q') game->playing = false;

	for (size_t i = 0; i < sizeofarr(game->ball); i++) {
		if (key == 'f')
			game->ball[i].ticks_total.x--;
		else if (key == 's')
			game->ball[i].ticks_total.x++;
		else if (key == 'F')
			game->ball[i].ticks_total.y--;
		else if (key == 'S')
			game->ball[i].ticks_total.y++;
	}

	vec2i *paddle_pos = &game->paddle[0].rect.pos;
	if (key == 'j' &&
		paddle_pos->y <= (rect_bottom_right(game->field).y - 1 - PADDLE_HEIGHT))
		paddle_pos->y++;
	else if (key == 'k' && paddle_pos->y > (game->field.pos.y + 1))
		paddle_pos->y--;
}

void game_update(game_obj *game) {
	// this simply runs every individual object's update function.
	// these handle stuff like redrawing and movement.

	for (size_t i = 0; i < sizeofarr(game->paddle); i++)
		paddle_update(&game->paddle[i]);

	for (size_t i = 0; i < sizeofarr(game->wall); i++)
		wall_update(&game->wall[i]);

	for (size_t i = 0; i < sizeofarr(game->ball); i++) {
		// balls have a "lost" flag stored inside them, and it disables all
		// movement and drawing until it's unset by ball_serve.
		if (game->ball[i].lost) {
			ball_serve(&game->ball[i]);
			// oops! this         ^^^ was [0] in the printed version...

			// lose a life
			game->serves--;
		}

		ball_update(&game->ball[i]);
	}

	// if you've run out of serves, end the game.
	if (game->serves <= 0) game->playing = false;
}

// returns a bool indicating if the window was even drawn to at all,
// so i can selectively execute refresh() move()..
bool game_draw(game_obj *game) {
	bool drawn = false;

	// using OR assignment ( |= ) to have even one
	// true value overrule every false return value.

	// did any wall redraw?
	for (size_t i = 0; i < sizeofarr(game->wall); i++)
		drawn |= wall_draw(&game->wall[i]);

	// did any ball redraw?
	for (size_t i = 0; i < sizeofarr(game->ball); i++)
		drawn |= ball_draw(&game->ball[i]);

	// did any paddle redraw?
	for (size_t i = 0; i < sizeofarr(game->paddle); i++)
		drawn |= paddle_draw(&game->paddle[i]);

	return drawn;
}

void game_destroy(game_obj *game) {
	free(game->name[0]);
	free(game->name[1]);
	free(game->message);
}
