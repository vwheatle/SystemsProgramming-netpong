#include "utility.h"
#include "geometry.h"

// utility functions to check equality of vectors, sizes, and rectangles

bool vec2i_eq(vec2i a, vec2i b) { return a.x == b.x && a.y == b.y; }
bool size2i_eq(size2i a, size2i b) {
	return a.width == b.width && a.height == b.height;
}

bool rect2i_eq(rect2i a, rect2i b) {
	return vec2i_eq(a.pos, b.pos) && size2i_eq(a.size, b.size);
}

// check if a point is inside a rectangle
bool point_in_rect(vec2i point, rect2i rect) {
	return point.x >= rect.pos.x && point.y >= rect.pos.y &&
		point.x < (rect.pos.x + rect.size.width) &&
		point.y < (rect.pos.y + rect.size.height);
}

// make a rectangle from a top left point and a bottom right point
rect2i rect_from_corners(vec2i top_left, vec2i bottom_right) {
	if (bottom_right.x < top_left.x) swap(top_left.x, bottom_right.x);
	if (bottom_right.y < top_left.y) swap(top_left.y, bottom_right.y);

	size2i size = (size2i) {
		bottom_right.x - top_left.x + 1,
		bottom_right.y - top_left.y + 1,
	};

	return (rect2i) {top_left, size};
}

// calculate the bottom right point of a rectangle
// (as they are stored as "top left point + rectangle size")
vec2i rect_bottom_right(rect2i rect) {
	return (vec2i) {
		rect.pos.x + rect.size.width - 1,
		rect.pos.y + rect.size.height - 1,
	};
}
