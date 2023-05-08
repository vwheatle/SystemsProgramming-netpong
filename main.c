// CS4-53203: Systems Programming
// Name: V Wheatley
// Date: 2023-03-10
// pong_game

#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <poll.h>   // -> poll
#include <signal.h> // -> signal

#include <curses.h>

#include "set_ticker.h" // -> set_ticker()

#include "networking.h"
#include "sppbtp.h"

#include "game.h" // -> game_obj

void set_up();
void update(int signum);
void wrap_up();

static game_obj game;
static network_info network;

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		char *name = argc > 0 ? argv[0] : "netpong";
		fprintf(stderr, "usage: %s [server_addr] port\n", name);
		fprintf(stderr,
			"(if server_addr not specified, "
			"you become a server.)\n");
		exit(EXIT_FAILURE);
	}

	char *server = argc == 3 ? argv[1] : NULL;
	int port = atoi(argc == 3 ? argv[2] : argv[1]);
	network = create_network_info(server, port);

	set_up();

	struct pollfd poll_data[2];
	poll_data[0].fd = STDIN_FILENO;
	poll_data[0].events = POLLIN;
	poll_data[1].fd = network.socket;
	poll_data[1].events = POLLIN;

	while (game.playing) {
		poll(poll_data, sizeofarr(poll_data), -1);
		if (poll_data[0].revents & POLLIN) game_input(&game, getchar());
		if (poll_data[1].revents & POLLIN) game_event(&game);
	}

	wrap_up();
	exit(EXIT_SUCCESS);
}

void set_up() {
	if (!connect_with_network_info(&network)) exit(EXIT_FAILURE);

	game_handshake(&game, &network);
	if (game.error) {
		wrap_up();
		exit(EXIT_FAILURE);
	}

	srand(getpid()); // seed random number generator

	initscr();       // give me a new screen buffer (a "window")
	noecho();        // don't echo characters as i type them
	crmode();        // don't process line breaks or delete characters

	game_setup(&game);                // set up game state

	signal(SIGINT, SIG_IGN);          // ignore interrupt signals (Ctrl+C)
	set_ticker(1000 / game.ticks_per_sec); // millisecs per tick

	update(SIGALRM); // tail call into update (and pretend the call
	                 //  was from the ticker triggering a SIGALRM)
}

void update(__attribute__((unused)) int signum) {
	// don't want to risk signal calling update inside of previous update call
	signal(SIGALRM, SIG_IGN); // disarm alarm

	// update the state of every object in the game.
	// (some objects' update functions may actually draw when called in this
	//  update function -- but they do it with the promise of later, in
	//  their respective draw function, returning true.)
	game_update(&game);

	// if the game actually changed anything
	// about its screen, update the screen.
	if (game_draw(&game)) {
		// move the cursor to the bottom right of the window
		move(LINES - 1, COLS - 1);

		// flush all changes of the window to the terminal
		refresh();
	}

	signal(SIGALRM, &update); // arm alarm
}

void wrap_up() {
	game_destroy(&game);
	disconnect_with_network_info(&network);

	set_ticker(0); // disable sending of SIGALRM at constant interval
	endwin();      // destroy my window

	if (!game.error) printf("Thank you for playing Pong!\n");
}
