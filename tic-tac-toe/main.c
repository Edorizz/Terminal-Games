/* C library */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
/* ncurses */
#include <ncurses.h>

#define MAX_OPTIONS	 8

#define P1		 1
#define P2		 2
#define AI		 P2

#define RUNNING		 0
#define WIN		 1
#define LOSS		-1
#define TIE		 2

#define P1_CH		'o'
#define P2_CH		'x'
#define AI_CH		P2_CH

FILE *inf_log = NULL;

typedef struct {
	char *str;
	void (*f)(void);
} option;

typedef struct {
	option options[MAX_OPTIONS];
	WINDOW *win;
	int x, y;
	int width, height;
	int highlight, count;
} menu;

void print_menu(menu *menu)
{
	int i;
	for (i = 0; i != menu->count; ++i) {
		if (i == menu->highlight)
			wattron(menu->win, A_REVERSE);
		mvwprintw(menu->win, i + 1, 1, "%s", menu->options[i].str);
		wattroff(menu->win, A_REVERSE);
	}

	wrefresh(menu->win);
}

void print_board(char board[3][3], int y, int x)
{
	int i, j, row, col;

	clear();
	col = (COLS - 5) / 2;
	for (i = 0; i != 3; ++i) {
		row = (LINES - 5) / 2 + i * 2;
		move(row, col);
		for (j = 0; j != 3; ++j) {
			if (i == y && j == x)
				attron(A_REVERSE);
			addch(board[i][j]);
			attroff(A_REVERSE);
			if (j != 2)
				addch('|');
		}
		if (i != 2)
			mvprintw(row + 1, col, "-+-+-");
	}
}

int game_state(char state[3][3], char player)
{
	int x, y;

	/* Check for winner */
	if (state[0][0] != ' ' && ((state[0][0] == state[1][0] && state[1][0] == state[2][0]) ||
				   (state[0][0] == state[0][1] && state[0][1] == state[0][2])))
		return state[0][0] == player ? 1 : -1;
	else if (state[2][2] != ' ' && ((state[2][2] == state[2][1] && state[2][1] == state[2][0]) ||
					(state[2][2] == state[1][2] && state[1][2] == state[0][2])))
		return state[2][2] == player ? 1 : -1;
	else if (state[1][1] != ' ' && ((state[0][0] == state[1][1] && state[1][1] == state[2][2]) ||
					(state[2][0] == state[1][1] && state[1][1] == state[0][2]) ||
					(state[1][0] == state[1][1] && state[1][1] == state[1][2]) ||
					(state[0][1] == state[1][1] && state[1][1] == state[2][1])))
		return state[1][1] == player ? 1 : -1;

	/* Check for tie */
	for (y = 0; y != 3; ++y) {
		for (x = 0; x != 3; ++x)
			if (state[y][x] == ' ')
				return RUNNING;
	}

	return TIE;

}

int player_turn(char board[3][3], int *player, int *y, int *x)
{
	int ch;

	switch (ch = getch()) {
	case 'w':	case 'W':
	case 'k':	case 'K':
	case KEY_UP:
		if (--*y < 0)
			*y = 2;
		break;
	case 's':	case 'S':
	case 'j':	case 'J':
	case KEY_DOWN:
		if (++*y > 2)
			*y = 0;
		break;
	case 'a':	case 'A':
	case 'h':	case 'H':
	case KEY_LEFT:
		if (--*x < 0)
			*x = 2;
		break;
	case 'd':	case 'D':
	case 'l':	case 'L':
	case KEY_RIGHT:
		if (++*x > 2)
			*x = 0;
		break;
	case '\n':
		if (board[*y][*x] == ' ') {
			board[*y][*x] = *player == P1 ? P1_CH : P2_CH;
			*player = !(*player - 1) + 1;
		}
		break;
	}

	return ch;
}

int minimax(char board[3][3], char player, int *y, int *x)
{
	int state, i, j, scores[3][3], hi, hi_i, hi_j;
	char board_cpy[3][3], other_player;

	switch (state = game_state(board, player)) {
	case WIN:	return -10;
	case LOSS:	return  10;
	case TIE:	return   0;
	}

	other_player = player == P1_CH ? AI_CH : P1_CH;
	memset(scores, -1, sizeof(int) * 9);
	for (i = 0; i != 3; ++i) {
		for (j = 0; j != 3; ++j) {
			if (board[i][j] == ' ') {
				fprintf(inf_log, "~x = %d, y = %d\n", j, i);
				memcpy(board_cpy, board, 9);
				board_cpy[i][j] = player;
				scores[i][j] = minimax(board_cpy, other_player, NULL, NULL);
			}
		}
	}

	hi = -100;
	for (i = 0; i != 3; ++i) {
		for (j = 0; j != 3; ++j) {
			fprintf(inf_log, "|x = %d, y - %d\n", j, i);
			if (scores[i][j] != -1 && scores[i][j] > hi) {
				hi = scores[i][j];
				hi_i = i;
				hi_j = j;
			}
		}
	}

	if (x != NULL) {
		*y = hi_i;
		*x = hi_j;
	}

	return hi * -1;
}

void ai_turn(char board[3][3])
{
	int x, y;
	
	minimax(board, AI_CH, &y, &x);
	fprintf(inf_log, "x = %d, y = %d\n", x, y);
	board[y][x] = AI_CH;
}

void game_1p(void)
{
	char board[3][3];
	int x, y, ch, player, state;

	clear();
	memset(board, ' ', 9);
	x = y = 0;
	player = rand() % 2 + P1;
	do {
		print_board(board, y, x);

		if (player == P1) {
			ch = player_turn(board, &player, &y, &x);
		} else {
			ai_turn(board);
			player = !(player - 1) + 1;
		}
	} while (ch != 'q' && (state = game_state(board, P1_CH)) == RUNNING);
	print_board(board, y, x);

	if (ch != 'q') {
		move(0, 0);
		switch (state) {
		case TIE:	printw("Tie :/");		break;
		case WIN:	printw("You win! :)");		break;
		case LOSS:	printw("You lose :(");		break;
		}

		refresh();
		getch();
	}
}

void game_2p(void)
{
	char board[3][3];
	int x, y, ch, player, state;

	clear();
	memset(board, ' ', 9);
	x = y = 0;
	player = rand() % 2 + P1;
	do {
		print_board(board, y, x);

		ch = player_turn(board, &player, &y, &x);
	} while (ch != 'q' && (state = game_state(board, P1_CH)) == RUNNING);
	print_board(board, y, x);

	if (ch != 'q') {
		move(0, 0);
		switch (state) {
		case TIE:	printw("Tie :/");		break;
		case WIN:	printw("Player 1 won!");	break;
		case LOSS:	printw("Player 2 won!");	break;
		}

		refresh();
		getch();
	}
}

int main(void)
{
	menu main_menu = { { { "Singleplayer", game_1p }, { "2-Player", game_2p } } };
	int ch;

	srand(time(NULL));

	/* Setup ncurses */
	initscr();
	noecho();
	cbreak();
	curs_set(0);
	keypad(stdscr, TRUE);

	main_menu.width = COLS / 2;
	main_menu.height = LINES / 2;
	main_menu.x = (COLS - main_menu.width) / 2;
	main_menu.y = (LINES - main_menu.height) / 2;
	main_menu.win = newwin(main_menu.height, main_menu.width, main_menu.y, main_menu.x);

	main_menu.count = 2;

	box(main_menu.win, 0, 0);
	mvprintw(LINES - 1, 0, "PRESS F1 TO QUIT");
	refresh();
	wrefresh(main_menu.win);

	inf_log = fopen("log.txt", "w");

	do {
		print_menu(&main_menu);

		switch (ch = getch()) {
		case 'w':	case 'W':
		case 'k':	case 'K':
		case KEY_UP:
			if (--main_menu.highlight < 0)
				main_menu.highlight = main_menu.count - 1;
			break;
		case 's':	case 'S':
		case 'j':	case 'J':
		case KEY_DOWN:
			if (++main_menu.highlight > main_menu.count - 1)
				main_menu.highlight = 0;
			break;
		case '\n':
			main_menu.options[main_menu.highlight].f();
			clear();
			box(main_menu.win, 0, 0);
			mvprintw(LINES - 1, 0, "PRESS F1 TO QUIT");
			refresh();
			wrefresh(main_menu.win);
			break;
		}
	} while (ch != KEY_F(1));

	fclose(inf_log);
	endwin();
	return 0;
}
