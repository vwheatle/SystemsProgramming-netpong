#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <string.h>

#include <sys/types.h> // -> size_t, ssize_t
#include <sys/socket.h>

#include <unistd.h> // -> write, read

#include "sppbtp.h"
#include "utility.h"

static char sppbtp_buff[SPPBTP_BUFMAX];
static char sppbtp_args[SPPBTP_ARGSTRMAX][SPPBTP_ARGMAX + 1];

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
	sendprintf(fd, "HELO " SPPBTP_ARG_PRINT " %d %d " SPPBTP_ARG_PRINT,
		SPPBTP_VERSION, ticks_per_sec, net_height, player_name);
}

void sppbtp_send_name(int fd, char *player_name) {
	sendprintf(fd, "NAME " SPPBTP_ARG_PRINT " " SPPBTP_ARG_PRINT,
		SPPBTP_VERSION, player_name);
}

void sppbtp_send_serv(int fd, int serves) {
	sendprintf(fd, "SERV %d", serves);
	// (this comment included to waste space to convince
	//  formatter that this should not be a one-liner)
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
	sendprintf(fd, "MISS " SPPBTP_ARG_PRINT, message);
}

void sppbtp_send_quit(int fd, char *message) {
	sendprintf(fd, "QUIT " SPPBTP_ARG_PRINT, message);
}

void sppbtp_send_done(int fd, char *message) {
	sendprintf(fd, "DONE " SPPBTP_ARG_PRINT, message);
}

void sppbtp_send_err(int fd, char *message) {
	fprintf(stderr, "sent error message to other:\n\t%s\n", message);
	sendprintf(fd, "?ERR " SPPBTP_ARG_PRINT, message);
}

#undef sendprintf

sppbtp_which sppbtp_parse_name(char data[4]) {
	static struct {
		char *name;
		sppbtp_which num;
	} name_map[] = {// annoying comment
		{"HELO", SPPBTP_HELO}, {"NAME", SPPBTP_NAME}, {"SERV", SPPBTP_SERV},
		{"BALL", SPPBTP_BALL}, {"MISS", SPPBTP_MISS}, {"QUIT", SPPBTP_QUIT},
		{"DONE", SPPBTP_DONE}};

	for (size_t i = 0; i < sizeofarr(name_map); i++)
		if (strncmp(data, name_map[i].name, 4) == 0) return name_map[i].num;

	return SPPBTP_ERR;
}

sppbtp_command sppbtp_parse(char *data) {
	sppbtp_command cmd;
	memset(&cmd, 0, sizeof(cmd));
	memset(&sppbtp_args, 0, sizeof(sppbtp_args));
	cmd.valid = false;

	int got;
	switch ((cmd.which = sppbtp_parse_name(data))) {
	case SPPBTP_HELO:
		got = sscanf(data, "HELO " SPPBTP_ARG_SCAN " %d %d " SPPBTP_ARG_SCAN,
			(char *)sppbtp_args[0], &cmd.data.helo.ticks_per_sec,
			&cmd.data.helo.net_height, (char *)sppbtp_args[1]);
		cmd.data.helo.version = sppbtp_args[0];
		cmd.data.helo.player_name = sppbtp_args[1];
		cmd.valid = got == 4;
		break;
	case SPPBTP_NAME:
		got = sscanf(data, "NAME " SPPBTP_ARG_SCAN " " SPPBTP_ARG_SCAN,
			(char *)sppbtp_args[0], (char *)sppbtp_args[1]);
		cmd.data.name.version = sppbtp_args[0];
		cmd.data.name.player_name = sppbtp_args[1];
		cmd.valid = got == 2;
		break;
	case SPPBTP_SERV:
		got = sscanf(data, "SERV %d", &cmd.data.serv.serves);
		cmd.valid = got == 1;
		break;
	case SPPBTP_BALL:
		got = sscanf(data, "BALL %d %d %d %d %c", &cmd.data.ball.net_y,
			&cmd.data.ball.x_ttm, &cmd.data.ball.y_ttm, &cmd.data.ball.y_dir,
			&cmd.data.ball.symbol);
		cmd.valid = got == 4 || got == 5;
		break;
	case SPPBTP_MISS:
		got = sscanf(data, "MISS " SPPBTP_ARG_RESTOF, (char *)sppbtp_args[0]);
		cmd.data.miss.message = sppbtp_args[0];
		cmd.valid = got == 1;
		break;
	case SPPBTP_QUIT:
		got = sscanf(data, "QUIT " SPPBTP_ARG_RESTOF, (char *)sppbtp_args[0]);
		cmd.data.quit.message = sppbtp_args[0];
		cmd.valid = got == 1;
		break;
	case SPPBTP_DONE:
		got = sscanf(data, "DONE " SPPBTP_ARG_RESTOF, (char *)sppbtp_args[0]);
		cmd.data.done.message = sppbtp_args[0];
		cmd.valid = got == 1;
		break;
	default:
		got = sscanf(data, "%*5c" SPPBTP_ARG_RESTOF, (char *)sppbtp_args[0]);
		cmd.data.err.message = sppbtp_args[0];
		cmd.valid = got == 1;
		break;
	}

	return cmd;
}

sppbtp_command sppbtp_recv(int fd) {
	sppbtp_command cmd;
	memset(&cmd, 0, sizeof(cmd));
	memset(&sppbtp_buff, 0, sizeof(sppbtp_buff));
	cmd.valid = false;

	ssize_t len = read(fd, sppbtp_buff, sizeof(sppbtp_buff));
	if (len < 0) cmd.which = SPPBTP_ERR;
	if (len <= 0) return cmd;

	return sppbtp_parse((char *)sppbtp_buff);
}
