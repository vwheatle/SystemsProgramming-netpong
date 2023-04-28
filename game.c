#include <curses.h>

#include "geometry.h" // -> vec2i, rect2i
#include "game.h"     // -> game_obj
#include "wall.h"     // -> wall_obj, PADDLE_*

// statically get length of statically-sized array
#define sizeofarr(arr) (sizeof(arr) / sizeof(*arr))

// set up the initial game state.
void game_setup(game_obj *game) {
	game->playing = true;

	// serves (lives)
	game->serves = 3;

	// paddle
	game->paddle[0].rect = (rect2i) {{RIGHT_EDGE, PADDLE_START_Y}, PADDLE_SIZE};

	// walls
	game->wall[0].rect =
		(rect2i) {{LEFT_EDGE, TOP_ROW + 1}, {1, BOARD_HEIGHT - 2}};
	game->wall[1].rect = (rect2i) {{LEFT_EDGE, TOP_ROW}, {BOARD_WIDTH, 1}};
	game->wall[2].rect = (rect2i) {{LEFT_EDGE, BOT_ROW}, {BOARD_WIDTH, 1}};

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
		ball_setup(&game->ball[i]);

		game->ball[i].paddles = &game->paddle[0];
		game->ball[i].paddles_len = sizeofarr(game->paddle);

		game->ball[i].walls = &game->wall[0];
		game->ball[i].walls_len = sizeofarr(game->wall);
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
	if (key == 'j' && paddle_pos->y <= (BOT_ROW - 1 - PADDLE_HEIGHT))
		paddle_pos->y++;
	else if (key == 'k' && paddle_pos->y > (TOP_ROW + 1))
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
