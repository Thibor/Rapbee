#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <windows.h>
#include "main.h"

SearchInfo info;

U64 GetTimeMs() {
#ifdef WIN32
	return GetTickCount64();
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000 + t.tv_usec / 1000;
#endif
}

static void ReadLine(char* str, int mc){
	char* ptr;
	if (fgets(str, mc, stdin) == NULL)
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

void SetFen(const char* s)
{
	int i;
	int z;
	int a = 0;
	int sq = 0;
	int n = (int)strlen(s);

	for (i = 0; i < 64; ++i) {
		color[i] = EMPTY;
		piece[i] = EMPTY;
	}

	for (i = 0, z = 0; i < n && z == 0; ++i) {
		switch (s[i]) {
		case '1': sq += 1; break;
		case '2': sq += 2; break;
		case '3': sq += 3; break;
		case '4': sq += 4; break;
		case '5': sq += 5; break;
		case '6': sq += 6; break;
		case '7': sq += 7; break;
		case '8': sq += 8; break;
		case 'p': color[sq] = DARK; piece[sq] = PAWN;   ++sq; break;
		case 'n': color[sq] = DARK; piece[sq] = KNIGHT; ++sq; break;
		case 'b': color[sq] = DARK; piece[sq] = BISHOP; ++sq; break;
		case 'r': color[sq] = DARK; piece[sq] = ROOK;   ++sq; break;
		case 'q': color[sq] = DARK; piece[sq] = QUEEN;  ++sq; break;
		case 'k': color[sq] = DARK; piece[sq] = KING;   ++sq; break;
		case 'P': color[sq] = LIGHT; piece[sq] = PAWN;   ++sq; break;
		case 'N': color[sq] = LIGHT; piece[sq] = KNIGHT; ++sq; break;
		case 'B': color[sq] = LIGHT; piece[sq] = BISHOP; ++sq; break;
		case 'R': color[sq] = LIGHT; piece[sq] = ROOK;   ++sq; break;
		case 'Q': color[sq] = LIGHT; piece[sq] = QUEEN;  ++sq; break;
		case 'K': color[sq] = LIGHT; piece[sq] = KING;   ++sq; break;
		case '/': break;
		default: z = 1; break;
		}
		a = i;
	}

	side = -1;
	xside = -1;

	++a;

	for (i = a, z = 0; i < n && z == 0; ++i) {
		switch (s[i]) {
		case 'w': side = LIGHT; xside = DARK; break;
		case 'b': side = DARK; xside = LIGHT; break;
		default: z = 1; break;
		}
		a = i;
	}

	castle = 0;

	for (i = a + 1, z = 0; i < n && z == 0; ++i) {
		switch (s[i]) {
		case 'K': castle |= 1; break;
		case 'Q': castle |= 2; break;
		case 'k': castle |= 4; break;
		case 'q': castle |= 8; break;
		case '-': break;
		default: z = 1; break;
		}
		a = i;
	}

	ep = -1;

	for (i = a + 1, z = 0; i < n && z == 0; ++i) {
		switch (s[i]) {
		case '-': break;
		case 'a': ep = 0; break;
		case 'b': ep = 1; break;
		case 'c': ep = 2; break;
		case 'd': ep = 3; break;
		case 'e': ep = 4; break;
		case 'f': ep = 5; break;
		case 'g': ep = 6; break;
		case 'h': ep = 7; break;
		case '1': ep += 56; break;
		case '2': ep += 48; break;
		case '3': ep += 40; break;
		case '4': ep += 32; break;
		case '5': ep += 24; break;
		case '6': ep += 16; break;
		case '7': ep += 8; break;
		case '8': ep += 0; break;
		default: z = 1; break;
		}
	}
}

//parse the move s (in coordinate notation) and return the move's index in gen_dat, or -1 if the move is illegal or -2 if unknow command.
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

//Engine MOve TO Uci MOve
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


static void ParsePosition(char* ptr){
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
		InitBoard(fen);
	}
	else {
		ptr = ParseToken(ptr, token);
		InitBoard(DEFAULT_FEN);
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
	info.stop = FALSE;
	info.nodes = 0;
	info.depthLimit = 64;
	info.nodesLimit = 0;
	info.timeLimit = 0;
	info.timeStart = GetTimeMs();
	char token[80];
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movestogo = 32;
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
			info.timeLimit = atoi(token);
		}
		else if (strcmp(token, "depth") == 0) {
			ptr = ParseToken(ptr, token);
			info.depthLimit = atoi(token);
		}
		else if (strcmp(token, "nodes") == 0) {
			ptr = ParseToken(ptr, token);
			info.nodesLimit = atoi(token);
		}
	}
	int time = side ? btime : wtime;
	int inc = side ? binc : winc;
	if (time)
		info.timeLimit = min(time / movestogo + inc, time / 2);
	SearchIterate();
}

static void UciLoop() {
	char command[4000], token[80], * ptr;
	for (;;) {
		ReadLine(command, sizeof(command));
		ptr = ParseToken(command, token);
		if (strcmp(token, "ucinewgame") == 0) {}
		else if (strcmp(token, "uci") == 0) {
			printf("id name %s\n",NAME);
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

int main(){
	printf("%s %s\n", NAME, VERSION);
	InitHash();
	InitBoard(DEFAULT_FEN);
	UciLoop();
	return 0;
}
