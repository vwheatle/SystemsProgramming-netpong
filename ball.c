#include <stdlib.h>
#include <stdbool.h>

#include <curses.h>

#include "geometry.h"
#include "ball.h"

// game settings

#define X_INIT (BOARD_WIDTH / 2) + LEFT_EDGE // starting column
#define Y_INIT (BOARD_HEIGHT / 2) + TOP_ROW  // starting row

#define X_TTM 5
#define Y_TTM 8

#define BALL_BOUNCED 1
#define BALL_LOST    -1

// private functions

bool bounce_off_of_wall(ball_obj *, wall_obj *, vec2i);
int bounce_or_lose(ball_obj *, bool[2]);
void change_speed(ball_obj *);

// implementations

void ball_setup(ball_obj *ball) {
	ball->symbol = 'o';
	ball->draw_pos = ball->pos;
	ball_serve(ball);
}

void ball_serve(ball_obj *ball) {
	// pick a random direction on the Y axis to move in
	int random_y_dir = (rand() & 1) ? +1 : -1;

	ball->pos = (vec2i) {X_INIT, Y_INIT};
	ball->dir = (vec2i) {1, random_y_dir};

	ball->ticks_left = ball->ticks_total = (vec2i) {X_TTM, Y_TTM};

	ball->lost = false;
	ball->redraw = true;
}

void ball_update(ball_obj *ball) {
	if (ball->lost) return;

	// which axis will we increment if we moved?
	bool step[2] = {false, false};

	if (ball->ticks_total.y > 0 && ball->ticks_left.y-- == 1) {
		step[1] = true;                           // move y later
		ball->ticks_left.y = ball->ticks_total.y; // reset timer
		ball->redraw = true;                      // need to redraw!
	}

	if (ball->ticks_total.x > 0 && ball->ticks_left.x-- == 1) {
		step[0] = true;                           // move x later
		ball->ticks_left.x = ball->ticks_total.x; // reset timer
		ball->redraw = true;                      // need to redraw!
	}

	if (ball->redraw) {
		int result = bounce_or_lose(ball, step);
		// (mutates ball direction)

		if (result == BALL_LOST) {
			// if we lost the ball, don't even move.
			ball->lost = true;
		} else {
			if (step[0]) ball->pos.x += ball->dir.x;
			if (step[1]) ball->pos.y += ball->dir.y;
		}

		// we're *gonna* draw later in the loop now -- as proven by the redraw
		// flag being true -- so we can definitely do some cleanup in the
		// update function.
		// (all this assuming we didn't just lose the ball, lol)
		mvaddch(ball->draw_pos.y, ball->draw_pos.x, ' ');
	}
}

bool ball_draw(ball_obj *ball) {
	bool drawn;
	if ((drawn = ball->redraw)) {
		if (!ball->lost) mvaddch(ball->pos.y, ball->pos.x, ball->symbol);

		// keep track of last drawn position, so we can freely change
		// ball->pos without having to undraw the old ball first.
		ball->draw_pos = ball->pos;
		ball->redraw = false;
	}
	return drawn;
}

bool bounce_off_of_wall(ball_obj *ball, wall_obj *wall, vec2i pos_next) {
	bool bounced;
	if ((bounced = point_in_rect(pos_next, wall->rect))) {
		vec2i top_left = wall->rect.pos;
		vec2i btm_rght = rect_bottom_right(wall->rect);

		if (ball->pos.x < top_left.x) ball->dir.x = -1;
		if (ball->pos.x > btm_rght.x) ball->dir.x = +1;

		if (ball->pos.y < top_left.y) ball->dir.y = -1;
		if (ball->pos.y > btm_rght.y) ball->dir.y = +1;

		// even after bouncing, in some cases (such as where the wall has moved)
		// the ball may be inside the wall. we'll need to redraw the wall then.
		wall->redraw = true;

		// if ball inside a wall, this doesn't work.  ops
	}
	return bounced;
}

int bounce_or_lose(ball_obj *ball, bool step[2]) {
	bool bounced = false;
	bool lost = false;

	// ball's next position if it continues in this direction
	// FIXME: ball could phase through a wall if walls in unlucky order:
	//  - wall A changes ball's direction such that it's going to immediately
	//    hit wall B if it steps forward -- but since pos_next is not updated
	//    with new direction, wall B check goes by and ball goes inside wall B
	//  - if ball ever changes direction, maybe scan against every wall again?
	//    this may cause an infinite loop, so need to have iteration limit.
	vec2i pos_next = (vec2i) {
		ball->pos.x + (ball->dir.x * step[0]),
		ball->pos.y + (ball->dir.y * step[1]),
	};

	// bounce off of paddles, and adjust speed if bounce happened
	for (size_t i = 0; i < ball->paddles_len; i++) {
		if (bounce_off_of_wall(ball, &ball->paddles[i], pos_next)) {
			change_speed(ball);
			bounced = true;
		}
	}

	// bounce off of walls
	for (size_t i = 0; i < ball->walls_len; i++) {
		bounced |= bounce_off_of_wall(ball, &ball->walls[i], pos_next);
	}

	// bounce off of edges of window

	// top/bottom edges
	if (ball->pos.y <= TOP_ROW) {
		ball->dir.y = 1;
		bounced = true;
	} else if (ball->pos.y >= BOT_ROW) {
		ball->dir.y = -1;
		bounced = true;
	}

	// left/right edges
	if (ball->pos.x <= LEFT_EDGE) {
		ball->dir.x = 1;
		bounced = true;
	} else if (ball->pos.x >= RIGHT_EDGE) {
		ball->dir.x = -1;
		lost = true;
	}

	if (lost) return BALL_LOST;
	if (bounced) return BALL_BOUNCED;
	return 0;
}

void change_speed(ball_obj *ball) {
	// select an axis to change, and keep a pointer to it.
	bool change_x = rand() & 1;
	int *to_change = change_x ? &(ball->ticks_total.x) : &(ball->ticks_total.y);

	// randomly decide to either subtract from it or add to it.
	int neg_or_pos = rand() ? -1 : +1;

	// keep the game fair; keep the speeds within a certain range.
	if (*to_change <= 1) {
		*to_change = 1;
		neg_or_pos = 1;
	} else if (*to_change >= 30) {
		*to_change = 30;
		neg_or_pos = -1;
	}

	// finally, change the speed of the randomly-selected axis!
	*to_change += neg_or_pos;
}
