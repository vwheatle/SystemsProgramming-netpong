#pragma once

#include <stdlib.h>

#include <string.h>
#include <ctype.h>

// get minimum/maximum value
// #define min(x, y) ((x) < (y)) ? (x) : (y)
// #define max(x, y) ((x) > (y)) ? (x) : (y)

// swap
#define swap(x, y) \
	do { \
		x = x ^ y; \
		y = y ^ x; \
		x = x ^ y; \
	} while (0)

// statically get length of statically-sized array
#define sizeofarr(arr) (sizeof(arr) / sizeof(*(arr)))

// destructively trims whitespace
char *trim_whitespace(char *s, size_t max_len);

#define stringify_eval(x) #x
#define stringify(x)      stringify_eval(x)
