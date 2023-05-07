#pragma once

#define SPPBTP_VERSION "1.0"
#define SPPBTP_BUFMAX  128
#define SPPBTP_ARGMAX  40

#define as_a_string_eval(x) #x
#define as_a_string(x)      as_a_string_eval(x)

// include in printf-s to limit strings to the maximum length.
#define SPPBTP_ARG "%" as_a_string(SPPBTP_ARGMAX) "s"

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
	int ticks_per_sec;
	int net_height;
	char *player_name;
};
struct sppbtp_name_data {
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
	SPPBTP_ERR
} sppbtp_which;
typedef struct {
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

sppbtp_command sppbtp_parse(char *data);
void sppbtp_free_command(sppbtp_command command);
