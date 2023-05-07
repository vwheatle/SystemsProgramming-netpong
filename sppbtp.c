#include <stdio.h>

#include <string.h>

#include <sys/types.h> // -> size_t, ssize_t
#include <sys/socket.h>

#include <unistd.h> // -> write, read

#include "sppbtp.h"

static char sppbtp_buff[SPPBTP_BUFMAX];

#define sizeofarray(arr) (sizeof(arr) / sizeof(*(arr)))

// write into the command buffer and send it.
// adds the CRLF line ending automatically
// -- even adding it if the regular output was truncated.
// that's the main reason why i didn't just use dprintf.
//
// snprintf returns the number of characters written, excluding the null byte.
// this is the same as the index of the null byte.
#define sendprintf(fd, ...) \
	do { \
		int len = snprintf(sppbtp_buff, SPPBTP_BUFMAX - 2, __VA_ARGS__); \
		if (len < 0) \
			len = snprintf(sppbtp_buff, SPPBTP_BUFMAX - 2, \
				"?ERR couldn't format command"); \
		if (len < 0) continue; \
		if (len >= (SPPBTP_BUFMAX - 2)) len = SPPBTP_BUFMAX - 3; \
		sppbtp_buff[len++] = '\r'; \
		sppbtp_buff[len++] = '\n'; \
		sppbtp_buff[len++] = '\0'; \
		write(fd, sppbtp_buff, len); \
	} while (0)


void sppbtp_send_helo(
	int fd, int ticks_per_sec, int net_height, char *player_name) {
	sendprintf(fd, "HELO " SPPBTP_ARG " %d %d " SPPBTP_ARG, SPPBTP_VERSION,
		ticks_per_sec, net_height, player_name);
}

void sppbtp_send_name(int fd, char *player_name) {
	sendprintf(fd, "NAME %s %s", SPPBTP_VERSION, player_name);
}

void sppbtp_send_serv(int fd, int serves) {
	sendprintf(fd, "SERV %d", serves);
	// (this comment included to expand this so formatter
	//  doesn't think this should be a one-liner)
}

void sppbtp_send_ball(
	int fd, int net_y, int x_ttm, int y_ttm, int y_dir, char ppb) {
	y_dir = y_dir > 0 ? 1 : y_dir < 0 ? -1 : 0;
	if (ppb != '\0') {
		sendprintf(fd, "BALL %d %d %d %d %c", net_y, x_ttm, y_ttm, y_dir, ppb);
	} else {
		sendprintf(fd, "BALL %d %d %d %d", net_y, x_ttm, y_ttm, y_dir);
	}
}

void sppbtp_send_miss(int fd, char *message) {
	sendprintf(fd, "MISS " SPPBTP_ARG, message);
}

void sppbtp_send_quit(int fd, char *message) {
	sendprintf(fd, "QUIT " SPPBTP_ARG, message);
}

void sppbtp_send_done(int fd, char *message) {
	sendprintf(fd, "DONE " SPPBTP_ARG, message);
}

void sppbtp_send_err(int fd, char *message) {
	sendprintf(fd, "?ERR " SPPBTP_ARG, message);
}

#undef sendprintf
