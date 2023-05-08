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

	if (game->network->role == ROLE_SERVER) {
		sppbtp_send_helo(game->network->socket, TICKS_PER_SEC,
			game->field.size.height, name);
		game->ticks_per_sec = TICKS_PER_SEC;
	}

	sppbtp_command cmd = sppbtp_recv(game->network->socket);
	if (!cmd.valid) {
		sppbtp_send_err(game->network->socket, "couldn't understand");
		game->error = true;
		return;
	}
	switch (cmd.which) {
	case SPPBTP_HELO:
		if (game->network->role == ROLE_SERVER) {
			sppbtp_send_err(
				game->network->socket, "servers shouldn't get HELO commands");
			game->error = true;
			return;
		} else if (strncmp(cmd.data.helo.version, SPPBTP_VERSION,
					   sizeofarr(SPPBTP_VERSION)) != 0) {
			sppbtp_send_err(game->network->socket, "versions don't match");
			game->error = true;
			return;
		}
		game->field.size.height = cmd.data.helo.net_height;
		game->ticks_per_sec = cmd.data.helo.ticks_per_sec;
		game->name[ROLE_SERVER] =
			strndup(cmd.data.helo.player_name, SPPBTP_ARGMAX);
		sppbtp_send_name(game->network->socket, name);
		break;
	case SPPBTP_NAME:
		if (game->network->role == ROLE_CLIENT) {
			sppbtp_send_err(
				game->network->socket, "clients shouldn't get NAME commands");
			game->error = true;
			return;
		} else if (strncmp(cmd.data.helo.version, SPPBTP_VERSION,
					   sizeofarr(SPPBTP_VERSION)) != 0) {
			sppbtp_send_err(game->network->socket, "versions don't match");
			game->error = true;
			return;
		}
		game->name[ROLE_CLIENT] =
			strndup(cmd.data.name.player_name, SPPBTP_ARGMAX);
		break;
	default:
		fprintf(stderr, "they didn't even give me their name.\n");
		sppbtp_send_err(game->network->socket, "needed HELO/NAME first");
		game->error = true;
		return;
	}

	fprintf(stderr, "they called themself '%s'.\n",
		game->name[other_role(game->network->role)]);

	game->side = game->network->role == ROLE_SERVER ? SIDE_LEFT : SIDE_RIGHT;


	// serves (lives)
	if (game->network->role == ROLE_SERVER) {
		game->serves = 3;
		game->in_this_field = false;
		sppbtp_send_serv(game->network->socket, game->serves);
	} else {
		sppbtp_command cmd = sppbtp_recv(game->network->socket);
		if (!cmd.valid) {
			sppbtp_send_err(game->network->socket, "couldn't understand");
			game->error = true;
			return;
		}
		if (cmd.which == SPPBTP_SERV) {
			game->serves = cmd.data.serv.serves;
			game->in_this_field = true;
		} else {
			sppbtp_send_err(game->network->socket, "expected SERV");
			game->error = true;
			return;
		}
	}
}

// set up the initial game state.
void game_setup(game_obj *game) {
	game->scores[0] = game->scores[1] = 0;

	game->playing = true;

	// field
	game->field = rect_from_corners((vec2i) {FIELD_LEFT_EDGE, FIELD_TOP_ROW},
		(vec2i) {FIELD_RIGHT_EDGE, FIELD_BOT_ROW});

	// paddle
	if (game->side == SIDE_LEFT) {
		game->paddle[0].rect =
			(rect2i) {{game->field.pos.x, game->field.pos.y + 2}, PADDLE_SIZE};
	} else {
		game->paddle[0].rect =
			(rect2i) {{rect_bottom_right(game->field).x, game->field.pos.y + 2},
				PADDLE_SIZE};
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

	// ball

	game->ball.paddles = &game->paddle[0];
	game->ball.paddles_len = sizeofarr(game->paddle);

	game->ball.walls = &game->wall[0];
	game->ball.walls_len = sizeofarr(game->wall);

	game->ball.field = &game->field;

	ball_setup(&game->ball);

	// hack to hide ball. great
	if (game->network->role == ROLE_SERVER) game->ball.lost = true;
}

void game_input(game_obj *game, int key) {
	if (key == 'Q') game->playing = false;

	if (key == 'f')
		game->ball.ticks_total.x--;
	else if (key == 's')
		game->ball.ticks_total.x++;
	else if (key == 'F')
		game->ball.ticks_total.y--;
	else if (key == 'S')
		game->ball.ticks_total.y++;

	vec2i *paddle_pos = &game->paddle[0].rect.pos;
	vec2i field_br = rect_bottom_right(game->field);
	if (key == 'j' && paddle_pos->y <= (field_br.y - 1 - PADDLE_HEIGHT))
		paddle_pos->y++;
	else if (key == 'k' && paddle_pos->y > (game->field.pos.y + 1))
		paddle_pos->y--;
}

void game_event(game_obj *game) {
	sppbtp_command cmd = sppbtp_recv(game->network->socket);
	if (!cmd.valid) {
		sppbtp_send_err(game->network->socket, "couldn't understand");
		game->playing = false;
		game->error = true;
		return;
	}
	switch (cmd.which) {
	case SPPBTP_HELO:
	case SPPBTP_NAME:
	case SPPBTP_SERV:
		sppbtp_send_err(game->network->socket, "not valid in PLAYBALL state");
		game->playing = false;
		game->error = true;
		break;
	case SPPBTP_QUIT:
		fprintf(stderr, "%s\n", cmd.data.quit.message);
		game->playing = false;
		break;
	case SPPBTP_BALL:
		if (game->in_this_field) {
			sppbtp_send_err(game->network->socket, "hey!!! it's my ball");
			game->playing = false;
			game->error = true;
			break;
		}

		game->in_this_field = true;
		game->ball.lost = false;

		if (game->side == SIDE_LEFT)
			game->ball.pos.x = rect_bottom_right(game->field).x - 1;
		if (game->side == SIDE_RIGHT) game->ball.pos.x = game->field.pos.x + 1;

		game->ball.pos.y = cmd.data.ball.net_y + game->field.pos.y;

		// clamp position to inside field
		game->ball.pos.y = max(game->ball.pos.y, game->field.pos.y + 1);
		game->ball.pos.y =
			min(game->ball.pos.y, rect_bottom_right(game->field).y - 1);

		vec2i ticks = (vec2i) {cmd.data.ball.x_ttm, cmd.data.ball.y_ttm};
		game->ball.ticks_left = game->ball.ticks_total = ticks;

		game->ball.dir.x = game->side == SIDE_LEFT ? -1 : 1;
		game->ball.dir.y = cmd.data.ball.y_dir;

		// yes, '\0' is technically the only invalid character.
		// but i don't want evil implementations making the ball invisible!
		if (cmd.data.ball.symbol > ' ')
			game->ball.symbol = cmd.data.ball.symbol;

		break;
	case SPPBTP_MISS:
		if (game->in_this_field) {
			sppbtp_send_err(game->network->socket, "it's still my ball");
			game->playing = false;
			game->error = true;
			break;
		}

		game->in_this_field = true;
		game->serves--;
		game->scores[game->network->role]++;

		if (game->serves <= 0) {
			sppbtp_send_done(game->network->socket, "good game!");
			game->playing = false;
		} else {
			ball_serve(&game->ball);
		}
		break;
	case SPPBTP_DONE:
		fprintf(stderr, "%s\n", cmd.data.done.message);
		game->playing = false;
		break;
	default:
		fprintf(stderr, "ERR: %s\n", cmd.data.err.message);
		game->playing = false;
		game->error = true;
		break;
	}
}

void game_update(game_obj *game) {
	// this simply runs every individual object's update function.
	// these handle stuff like redrawing and movement.

	for (size_t i = 0; i < sizeofarr(game->paddle); i++)
		paddle_update(&game->paddle[i]);

	for (size_t i = 0; i < sizeofarr(game->wall); i++)
		wall_update(&game->wall[i]);

	// balls have a "lost" flag stored inside them, and it disables all
	// movement and drawing until it's unset by ball_serve.
	if (game->in_this_field) {
		if (game->ball.lost) {
			bool portal = false;
			if (game->side == SIDE_LEFT)
				portal = game->ball.pos.x >= rect_bottom_right(game->field).x;
			if (game->side == SIDE_RIGHT)
				portal = game->ball.pos.x <= game->field.pos.x;

			game->in_this_field = false;

			if (portal) {
				sppbtp_send_ball(game->network->socket,
					game->ball.pos.y - game->field.pos.y,
					game->ball.ticks_total.x, game->ball.ticks_total.y,
					game->ball.dir.y, 0);
			} else {
				sppbtp_send_miss(game->network->socket, "oh no");
				game->serves--;
				game->scores[other_role(game->network->role)]++;
			}
		} else {
			ball_update(&game->ball);
		}
	}

	// if you've run out of serves, try to end the game.
	if (game->serves <= 0 && !game->in_this_field) {
		game->playing = false;

		// update function takes over control of socket, waits...
		sppbtp_command cmd = sppbtp_recv(game->network->socket);
		if (!cmd.valid) {
			if (cmd.which != SPPBTP_NO_MORE)
				sppbtp_send_err(game->network->socket, "couldn't understand");
			game->error = true;
			return;
		}
		if (cmd.which == SPPBTP_DONE) {
			fprintf(stderr, "%s\n", cmd.data.done.message);
		} else {
			sppbtp_send_err(
				game->network->socket, "didn't we just run out of balls?");
			game->error = true;
			return;
		}
	}
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

	// did the ball redraw?
	drawn |= ball_draw(&game->ball);

	// did any paddle redraw?
	for (size_t i = 0; i < sizeofarr(game->paddle); i++)
		drawn |= paddle_draw(&game->paddle[i]);

	if (drawn) {
		// lazy impl of displaying score and other name
		char buff[128];
		snprintf(buff, sizeofarr(buff), "BALLS %d; %s: %02d\t%s: %02d",
			game->serves, game->name[0], game->scores[0], game->name[1],
			game->scores[1]);
		mvaddstr(1, 1, buff);
	}

	return drawn;
}

void game_destroy(game_obj *game) {
	free(game->name[0]);
	free(game->name[1]);
	free(game->message);
}
