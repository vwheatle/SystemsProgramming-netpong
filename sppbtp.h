#pragma once

#include <stdbool.h>

#include "utility.h" // -> sizeofarr, stringify

#define SPPBTP_VERSION   "1.0"
#define SPPBTP_BUFMAX    128
#define SPPBTP_ARGMAX    40
#define SPPBTP_ARGSTRMAX 2

// include in printf-s to limit strings to the maximum length.
#define SPPBTP_ARG_PRINT "%-." stringify(SPPBTP_ARGMAX) "s"

// include in scanf-s to read only up to the maximum length.
#define SPPBTP_ARG_SCAN   "%" stringify(SPPBTP_ARGMAX) "s"
#define SPPBTP_ARG_RESTOF "%" stringify(SPPBTP_ARGMAX) "c"
// (the latter consumes the "rest of" the messaage)

// send commands

void sppbtp_send_helo(
	int fd, int ticks_per_sec, int net_height, char *player_name);
void sppbtp_send_name(int fd, char *player_name);
void sppbtp_send_serv(int fd, int serves);
void sppbtp_send_ball(
	int fd, int net_y, int x_ttm, int y_ttm, int y_dir, char ppb);
void sppbtp_send_miss(int fd, char *message);
void sppbtp_send_quit(int fd, char *message);
void sppbtp_send_done(int fd, char *message);
void sppbtp_send_err(int fd, char *message);

// parse commands

struct sppbtp_helo_data {
	char *version;
	int ticks_per_sec;
	int net_height;
	char *player_name;
};
struct sppbtp_name_data {
	char *version;
	char *player_name;
};
struct sppbtp_serv_data {
	int serves;
};
struct sppbtp_ball_data {
	int net_y;
	int x_ttm, y_ttm; // (same as ticks_total)
	int y_dir;
	char symbol;      // ('\0' to leave unchanged)
};
struct sppbtp_miss_data {
	char *message;
};
struct sppbtp_quit_data {
	char *message;
};
struct sppbtp_done_data {
	char *message;
};
struct sppbtp_err_data {
	char *message;
};

typedef enum {
	SPPBTP_HELO,
	SPPBTP_NAME,
	SPPBTP_SERV,
	SPPBTP_BALL,
	SPPBTP_MISS,
	SPPBTP_QUIT,
	SPPBTP_DONE,
	SPPBTP_ERR,
	SPPBTP_NO_MORE
} sppbtp_which;
typedef struct {
	bool valid;
	sppbtp_which which;
	union {
		struct sppbtp_helo_data helo;
		struct sppbtp_name_data name;
		struct sppbtp_serv_data serv;
		struct sppbtp_ball_data ball;
		struct sppbtp_miss_data miss;
		struct sppbtp_quit_data quit;
		struct sppbtp_done_data done;
		struct sppbtp_err_data err;
	} data;
} sppbtp_command;

sppbtp_which sppbtp_parse_name(char data[4]);
sppbtp_command sppbtp_parse(char *data);

// recv commands

sppbtp_command sppbtp_recv(int fd);
