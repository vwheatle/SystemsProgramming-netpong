#include <stdlib.h>

#include <string.h>
#include <ctype.h>

#include "utility.h"

// destructively trims whitespace
char *trim_whitespace(char *s, size_t max_len) {
	size_t len = strnlen(s, max_len);
	while (len > 0 && isspace(s[0])) {
		(s++)[0] = '\0';
		len--;
	}
	while (len > 0 && isspace(s[len - 1])) {
		s[len - 1] = '\0';
		len--;
	}
	return s;
}
