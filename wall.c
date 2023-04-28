#include <stdbool.h>

#include <curses.h>

#include "geometry.h"
#include "wall.h"

const struct {
	char empty;              // empty space
	char unit;               // entire rect fits in one char
	char vert_wall;          // vertical wall
	char horiz_wall;         // horizontal wall
	char top_left, top_rght; // top left/right corners
	char btm_left, btm_rght; // bottom left/right corners
} rect_chars = {' ', '+', '|', '-', ',', ',', '\'', '\''};

// questionable function (but it works.)
void draw_rect(rect2i rect, bool visible) {
	vec2i tl = rect.pos;
	vec2i br = rect_bottom_right(rect);

	if (tl.x > br.x || tl.y > br.y) return;
	if (!rect.size.width || !rect.size.height) return;

	if (rect.size.width == 1 && rect.size.height == 1) {
		mvaddch(tl.y, tl.x, visible ? rect_chars.unit : rect_chars.empty);
		return;
	}

	if (rect.size.width == 1) tl.y--, br.y++;
	if (rect.size.height == 1) tl.x--, br.x++;

	if (rect.size.width > 1 && rect.size.height > 1) {
		mvaddch(tl.y, tl.x, visible ? rect_chars.top_left : rect_chars.empty);
		mvaddch(tl.y, br.x, visible ? rect_chars.top_rght : rect_chars.empty);
		mvaddch(br.y, tl.x, visible ? rect_chars.btm_left : rect_chars.empty);
		mvaddch(br.y, br.x, visible ? rect_chars.btm_rght : rect_chars.empty);
	}

	for (int x = tl.x + 1; x < br.x; x++) {
		char ch = visible ? rect_chars.horiz_wall : rect_chars.empty;
		mvaddch(tl.y, x, ch);
		if (tl.y != br.y) mvaddch(br.y, x, ch);
	}
	for (int y = tl.y + 1; y < br.y; y++) {
		char ch = visible ? rect_chars.vert_wall : rect_chars.empty;
		mvaddch(y, tl.x, ch);
		if (tl.x != br.x) mvaddch(y, br.x, ch);
	}
}

void draw_solid_rect(rect2i rect, char fill) {
	vec2i tl = rect.pos;
	vec2i br = rect_bottom_right(rect);

	for (int y = tl.y; y <= br.y; y++)
		for (int x = tl.x; x <= br.x; x++) mvaddch(y, x, fill);
}

void wall_setup(wall_obj *wall) {
	wall->draw_rect = wall->rect;
	wall->redraw = true;
}

void wall_update(wall_obj *wall) {
	if (!rect2i_eq(wall->rect, wall->draw_rect)) {
		wall->redraw = true;
		draw_rect(wall->draw_rect, false);
	}
}

bool wall_draw(wall_obj *wall) {
	bool drawn;
	if ((drawn = wall->redraw)) {
		draw_rect(wall->rect, true);

		wall->draw_rect = wall->rect;
		wall->redraw = false;
	}
	return drawn;
}

void paddle_update(wall_obj *wall) {
	if (!rect2i_eq(wall->rect, wall->draw_rect)) {
		wall->redraw = true;
		draw_solid_rect(wall->draw_rect, ' ');
	}
}

bool paddle_draw(wall_obj *wall) {
	bool drawn;
	if ((drawn = wall->redraw)) {
		draw_solid_rect(wall->rect, '#');

		wall->draw_rect = wall->rect;
		wall->redraw = false;
	}
	return drawn;
}
