#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "windows.h"
#include "main.h"

int GetTimeMs() {
#ifdef WIN32
	return GetTickCount64();
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000 + t.tv_usec / 1000;
#endif
}

static void PrintWelcome() {
	printf("%s %s\n", NAME, __DATE__);
}

static void ReadLine(char* str, int n)
{
	char* ptr;

	if (fgets(str, n, stdin) == NULL)
		exit(0);
	if ((ptr = strchr(str, '\n')) != NULL)
		*ptr = '\0';
}

static char* ParseToken(char* string, char* token)
{
	while (*string == ' ')
		string++;
	while (*string != ' ' && *string != '\0')
		*token++ = *string++;
	*token = '\0';
	return string;
}

static void ParsePosition(char* ptr)
{
	int m;
	char token[80], fen[80];

	ptr = ParseToken(ptr, token);
	if (strcmp(token, "fen") == 0) {
		fen[0] = '\0';
		for (;;) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0' || strcmp(token, "moves") == 0)
				break;
			strcat(fen, token);
			strcat(fen, " ");
		}
		init_board(fen);
	}
	else {
		ptr = ParseToken(ptr, token);
		init_board(spos);
	}
	ply = 0;
	gen();
	if (strcmp(token, "moves") == 0)
		for (;;) {
			ptr = ParseToken(ptr, token);
			if (*token == '\0')
				break;
			m = ParseMove(token);
			if (m < 0 || !makemove(gen_dat[m].m.b))
				printf("Illegal move (%s).\n", token);
			ply = 0;
			gen();
		}
}

static void ParseGo(char* ptr) {
	max_time = MAX_TIME;
	max_depth = MAX_DEPTH;
	char token[80], bestmove_str[6], ponder_str[6];
	int wtime, btime, winc, binc, movestogo, time, inc, pv[MAX_PLY];
	int movetime, movedepth;
	wtime = -1;
	btime = -1;
	winc = 0;
	binc = 0;
	movestogo = 40;
	movetime = 0;
	movedepth = 0;
	for (;;) {
		ptr = ParseToken(ptr, token);
		if (*token == '\0')
			break;
		else if (strcmp(token, "wtime") == 0) {
			ptr = ParseToken(ptr, token);
			wtime = atoi(token);
		}
		else if (strcmp(token, "btime") == 0) {
			ptr = ParseToken(ptr, token);
			btime = atoi(token);
		}
		else if (strcmp(token, "winc") == 0) {
			ptr = ParseToken(ptr, token);
			winc = atoi(token);
		}
		else if (strcmp(token, "binc") == 0) {
			ptr = ParseToken(ptr, token);
			binc = atoi(token);
		}
		else if (strcmp(token, "movestogo") == 0) {
			ptr = ParseToken(ptr, token);
			movestogo = atoi(token);
		}
		else if (strcmp(token, "movetime") == 0) {
			ptr = ParseToken(ptr, token);
			movetime = atoi(token);
		}
		else if (strcmp(token, "depth") == 0) {
			ptr = ParseToken(ptr, token);
			movedepth = atoi(token);
		}
	}
	if (movedepth > 0)
		max_depth = movedepth;
	if (movetime > 0)
		max_time = movetime;
	else {
		time = side == LIGHT ? wtime : btime;
		inc = side == LIGHT ? winc : binc;
		if (time >= 0) {
			if (movestogo == 1) time -= min(1000, time / 10);
			max_time = (time + inc * (movestogo - 1)) / movestogo;
			if (max_time > time)
				max_time = time;
			max_time -= 10;
			if (max_time < 0)
				max_time = 0;
		}
	}
	SearchIterate();
}


static void UciLoop() {
	char command[4096], token[80], * ptr;
	for (;;) {
		ReadLine(command, sizeof(command));
		ptr = ParseToken(command, token);
		if (strcmp(token, "uci") == 0) {
			printf("id name Rapbee\n");
			printf("uciok\n");
			fflush(stdout);
		}
		else if (strcmp(token, "isready") == 0) {
			printf("readyok\n");
			fflush(stdout);
		}
		else if (strcmp(token, "position") == 0) {
			ParsePosition(ptr);
		}
		else if (strcmp(token, "go") == 0) {
			ParseGo(ptr);
		}
		else if (strcmp(token, "quit") == 0) {
			exit(0);
		}
		else if (strcmp(token, "print") == 0)
			PrintBoard();
	}
}

/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int main()
{
	PrintWelcome();
	init_hash();
	init_board(spos);
	gen();
	UciLoop();
	return 0;
}


/* parse the move s (in coordinate notation) and return the move's
   index in gen_dat, or -1 if the move is illegal */

   /* parse the move s (in coordinate notation) and return the move's
	  index in gen_dat, or -1 if the move is illegal or -2 if unknow command. */

int ParseMove(char* s) {
	int from, to, i;

	/* make sure the string looks like a move */
	if (s[0] < 'a' || s[0] > 'h' ||
		s[1] < '0' || s[1] > '9' ||
		s[2] < 'a' || s[2] > 'h' ||
		s[3] < '0' || s[3] > '9')
		return -2;	// unknow command, before -1

	from = s[0] - 'a';
	from += 8 * (8 - (s[1] - '0'));
	to = s[2] - 'a';
	to += 8 * (8 - (s[3] - '0'));

	for (i = 0; i < first_move[1]; ++i)
		if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) {

			/* if the move is a promotion, handle the promotion piece; assume
			 * that the promotion moves occur consecutively in gen_dat. */
			if (gen_dat[i].m.b.bits & 32)
				switch (s[4]) {
				case 'N':
				case 'n':
					return i;
				case 'B':
				case 'b':
					return i + 1;
				case 'R':
				case 'r':
					return i + 2;
				default:        /* assume it's a queen */
					return i + 3;
				}
			return i;
		}
	/* didn't find the move, illegal move */
	return -1;
}



//engine move to uci move
char* EmoToUmo(SMove m)
{
	static char str[6];
	char c;
	if (m.bits & 32) {
		switch (m.promote) {
		case KNIGHT:
			c = 'n';
			break;
		case BISHOP:
			c = 'b';
			break;
		case ROOK:
			c = 'r';
			break;
		default:
			c = 'q';
			break;
		}
		sprintf(str, "%c%d%c%d%c",
			COL(m.from) + 'a',
			8 - ROW(m.from),
			COL(m.to) + 'a',
			8 - ROW(m.to),
			c);
	}
	else
		sprintf(str, "%c%d%c%d",
			COL(m.from) + 'a',
			8 - ROW(m.from),
			COL(m.to) + 'a',
			8 - ROW(m.to));
	return str;
}

//prints the board
void PrintBoard() {
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	printf(t);
	for (int r = 0; r < 8; r++) {
		printf(s);
		printf(" %d |", 8 - r);
		for (int f = 0; f < 8; f++) {
			int i = r * 8 + f;
			switch (color[i]) {
			case EMPTY:
				printf("   |");
				break;
			case LIGHT:
				printf(" %c |", piece_char[piece[i]]);
				break;
			case DARK:
				printf(" %c |", piece_char[piece[i]] + ('a' - 'A'));
				break;
			}
		}
		printf(" %d \n", 8 - r);
	}
	printf(s);
	printf(t);
}

