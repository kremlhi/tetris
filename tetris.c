#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <err.h>

#include "tetris.h"
#include "term.h"

static void drawpiece(struct piece *, int, int, enum colour_t);
static void drawboard(int *);
static void drawpreview(struct piece *, struct piece *);

static void rotate(struct piece *);
static void animate(int);
static int validmove(int *, struct piece *, int, int);
static void removefull(int *, int *, int *, int);
static int udelay(int, int);
static void keys(int *, struct piece *, int *, int *, int *);
static void drawscore(int, int, int);

static const struct piece
pieces[] = {
	/* [][]
       [][] */
	{{{0,0}, {1,0}, {0,1}, {1,1}}, GREY},

	/* [][][][] */
	{{{-1,0}, {0,0}, {1,0}, {2,0}}, CYAN},

	/* [][][]
           [] */
	{{{-1,0}, {0,0}, {1,0}, {1,1}}, BLUE},

	/* [][][]
       [] */
	{{{-1,0}, {0,0}, {1,0}, {-1,1}}, YELLOW},

	/* [][]
         [][] */
	{{{-1,0}, {0,0}, {0,1}, {1,1}}, RED},

	/* [][]
     [][] */
	{{{0,0}, {1,0}, {-1,1}, {0,1}}, GREEN},

	/* [][][]
         [] */
	{{{-1,0}, {0,0}, {1,0}, {0,1}}, MAGENTA}
};
#define NPIECES (sizeof(pieces)/sizeof(*pieces))

/*           []        []        []
   [][][] -> [][] -> [][][] -> [][]
     []      []                  [] */ 
static void
rotate(struct piece *p)
{
	int i, x;

	for(i = 0; i < NPARTS; i++){
		x = p->p[i].x;
		p->p[i].x = p->p[i].y;
		p->p[i].y = -x;
	}
}

static void
drawpiece(struct piece *p, int x, int y, enum colour_t colour)
{
	int i;

	for(i=0; i < NPARTS; i++)
		drawpixel(x + p->p[i].x, y + p->p[i].y,
			colour == BGCOLOUR ? p->colour : colour); 
	flush();
}

static void
newpiece(struct piece *p)
{
	int i, rots;

	*p = pieces[rand() % NPIECES];
	rots = rand() % NPARTS;
	for(i = 0; i < rots; i++)
		rotate(p);
}

static suseconds_t
usecs()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec % 1000)*1000000 + tv.tv_usec;
}

static int
validmove(int *board, struct piece *p, int x, int y)
{
	int i, x1, y1;

	for(i = 0; i < NPARTS; i++){
		x1 = x + p->p[i].x;
		y1 = p->p[i].y + y;
		if(x1 < 0 || x1 >= WIDTH || y1 < 0 || y1 >= HEIGHT
		|| board[y1] & (1 << x1))
			return 0;
	}
	return 1;
}

static int
udelay(int sec, int usec)
{
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	if(usec <= 0 && sec <= 0)
		return 0;
	tv.tv_sec = sec;
	tv.tv_usec = usec;
	return select(1, &rfds, NULL, NULL, &tv);
}

static void
drawscore(int score, int level, int secs)
{
	posxy(WIDTH + 2, 4);
	printf("score: %d level: %d %03d\n", score, level, secs);
}

static void
drawpreview(struct piece *old, struct piece *preview)
{
	int x, y;

	for(y = 0; y < NPARTS + 1; y++) 
		for(x = 0; x < NPARTS + 1; x++)
			drawpixel(WIDTH + 2 + x, 6 + y, NORMAL);
	*old = *preview;
	newpiece(preview);
	drawpiece(preview, WIDTH + 4, 8, INVERT);
}

#include <setjmp.h>
static jmp_buf jmpbuf;

void
jmpback(int sig)
{
/*
	longjmp(jmpbuf, sig);
*/
}

static void
animate(int level)
{
	int x, y, delay, stop, score, tetrises, pieces, elapsed;
	static int board[HEIGHT];
	struct piece p, preview;

	tetrises = 0;
	pieces = 0;
	score = 0;
	delay = 200*1000;
	delay -= level*10*1000;
	elapsed = time(NULL) + NEXTLEVEL;

	srand(time(NULL));
	clear();
	newpiece(&preview);
	drawpreview(&p, &preview);
	drawboard(board);

	drawscore(score, level, elapsed - time(NULL));
	for(x = WIDTH/2, y = NPARTS;; y++){
		drawpiece(&p, x, y, BGCOLOUR);
		stop = usecs() + delay;
#ifdef USE_SELECT
		for(;;){
			if(udelay(0, stop - usecs()) < 1)
				break;
			keys(board, &p, &x, &y, &elapsed);
		}
#else
		{
		static struct itimerval tval;
		tval.it_value.tv_usec = stop - usecs();
		setitimer(ITIMER_REAL, &tval, NULL);
/* setjmp(jmpbuf); */
		for(;stop - usecs() > 0;)
			keys(board, &p, &x, &y, &elapsed);
		}
#endif
			
		if(!validmove(board, &p, x, y + 1)){
			drawpiece(&p, x, y, INVERT);
			if(y < NPARTS + 1)
				break;
			board[y + p.p[0].y] |= (1 << (x + p.p[0].x));
			board[y + p.p[1].y] |= (1 << (x + p.p[1].x));
			board[y + p.p[2].y] |= (1 << (x + p.p[2].x));
			board[y + p.p[3].y] |= (1 << (x + p.p[3].x));
			removefull(board, &score, &tetrises, level);
			drawpreview(&p, &preview);
			x = WIDTH/2;
			y = 0;
		}
		drawscore(score, level, elapsed - time(NULL));
		if(elapsed - time(NULL) <= 0){
			pieces = 0;
			level++;
			delay -= 10*1000;
			memset(board, 0, sizeof board);
			drawboard(board);
			drawtext("Level %d\n", level);
			sleep(2);
			drawboard(board);
			elapsed = time(NULL) + NEXTLEVEL;
			score += 2*level*level;
			y = 0;
		}
		drawpiece(&p, x, y, 0);
	}
}

static void
removefull(int *board, int *score, int *tetrises, int level)
{
	int y, y2;

	for(y = y2 = HEIGHT - 1; y >= 0; y2--, y--){
		while(board[y] == 0x3ff)
			y--;
		board[y2] = board[y];
	}
	y2++;
	if(y2 > 0 && y2 < 4)
		*tetrises = 0;
	if(y2 == 4)
		*score += (++*tetrises)*(*tetrises)*y2*y2*level;
	else
		*score += y2*y2*level;
	if(y2 > 0){
		memset(&board[WIDTH - y2 - 1], 0, y2);
		drawboard(board);
	}
}

static void
drawboard(int *board)
{
	int x, y;

	for(y = 0; y < HEIGHT + 1; y++)
		for(x = -1; x < WIDTH + 1; x++)
			if(x < 0 || x == WIDTH || y == HEIGHT || board[y] & (1 << x))
				drawpixel(x, y, INVERT);
			else
				drawpixel(x, y, NORMAL);
	flush();
}

static void
keys(int *board, struct piece *p, int *x, int *y, int *elapsed)
{
	struct piece np;
	int y1, t;

	switch(readch()){
	case ' ':
		for(y1 = 4; validmove(board, p, *x, y1); y1++)
			;
		drawpiece(p, *x, *y, NORMAL);
		*y = y1 - 1;
		drawpiece(p, *x, *y, BGCOLOUR);
		break;
	case '\t':
	case 'p':
		t = time(NULL);
		readch();
		*elapsed += time(NULL) - t;
		break;
	case 'q':
		cleanexit(SIGTERM);
		/* NOTREACHED */
	case 'k':
		np = *p;
		rotate(&np);
		if(validmove(board, &np, *x, *y)){
			drawpiece(p, *x, *y, NORMAL);
			*p = np;
			drawpiece(p, *x, *y, BGCOLOUR);
		}
		break;
	case 'j':
		if(validmove(board, p, *x - 1, *y)){
			drawpiece(p, *x, *y, NORMAL);
			drawpiece(p, --*x, *y, BGCOLOUR);
		}
		break;
	case 'l':
		if(validmove(board, p, *x + 1, *y)){
			drawpiece(p, *x, *y, NORMAL);
			drawpiece(p, ++*x, *y, BGCOLOUR);
		}
		break;
	}
}

void nop(int s){}

int
main(int argc, char *argv[])
{
	int level;

	signal(SIGINT, cleanexit);
	signal(SIGTERM, cleanexit);
	signal(SIGWINCH, redraw);
	signal(SIGALRM, nop);

	level = 1;
	if(argc > 1)
		level = atoi(argv[1]);
	if(level < 1)
		errx(1, "usage: %s [level]", argv[0]);

	init();
	animate(level);
	cleanexit(SIGTERM);
	return 0;
}

