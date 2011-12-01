#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "tetris.h"
#include "term.h"

struct termios oldt;

void
clear(void)
{
	printf(CSI "2J");
}
 
void
posxy(int x, int y)
{
	if(y < 4)
		return;
	printf(CSI "%d;%dH", y - 3, x*2 + 27);
}

void
drawpixel(int x, int y, enum colour_t colour)
{
	if(y < 4)
		return;
	printf(CSI "%d;%dH" CSI "%dm  " CSI "0m", y - 3, x*2 + 27, colour);
}

void
flush(void)
{
	printf(CSI "24;1H");
	fflush(stdout);
}

void
cleanexit(int sig)
{
	tcsetattr(0, TCSANOW, &oldt);
	exit(0);
}

void
redraw(int sig)
{
}

void
init(void)
{
	static struct termios newt;

	if(tcgetattr(0, &oldt) == -1)
		err(1, "tcgetattr");
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	if(tcsetattr(0, TCSANOW, &newt) == -1)
		err(1, "tcsetattr");

}

char
readch(void)
{
	char c, c2[2];

	read(0, &c, 1);
	if(c == CSI[0]){
		read(0, c2, 2);
		if(c2[0] != CSI[1])
			return '\0';
		switch(c2[1]){
		case 'A': return 'k';
		case 'B': return ' ';
		case 'C': return 'l';
		case 'D': return 'j';
		}
		return '\0';
	}
	return c;
}

void
drawtext(char *fmt, ...)
{
	va_list ap;
	static char buf[MAXTEXTLEN];
	int len;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	posxy(WIDTH/2 - len/4, HEIGHT/2);
	printf("%s", buf);
	flush();
}

