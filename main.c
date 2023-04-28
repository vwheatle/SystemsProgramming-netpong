// CS4-53203: Systems Programming
// Name: V Wheatley
// Date: 2023-03-10
// pong_game

#include <stdlib.h>

#include <unistd.h>
#include <signal.h>

#include <curses.h>

#include "set_ticker.h" // -> set_ticker()

#include "game.h" // -> game_obj

void set_up();
void update(int signum);
void wrap_up();

static game_obj game;

int main() {
	set_up();

	// todo: see if async input could work
	// to put an input callback in update loop
	while (game.playing) game_input(&game, getchar());

	wrap_up();
	exit(EXIT_SUCCESS);
}

void set_up() {
	srand(getpid()); // seed random number generator

	initscr(); // give me a new screen buffer (a "window")
	noecho();  // don't echo characters as i type them
	crmode();  // don't process line breaks or delete characters

	game_setup(&game); // set up game state

	signal(SIGINT, SIG_IGN);          // ignore interrupt signals (Ctrl+C)
	set_ticker(1000 / TICKS_PER_SEC); // param is in millisecs per tick

	update(SIGALRM); // tail call into update (and pretend the call
	                 //  was from the ticker triggering a SIGALRM)
}

void update(__attribute__((unused)) int signum) {
	// don't want to risk signal calling update inside of previous update call
	signal(SIGALRM, SIG_IGN); // disarm alarm

	// update the state of every object in the game.
	// (some objects' update functions may actually draw when called in this
	//  update function -- but they do it with the promise of later, in their
	//  respective draw function, returning true.)
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
	set_ticker(0); // disable sending of SIGALRM at constant interval
	endwin();      // destroy my window

	printf("Thank you for playing Pong!\n");
}
