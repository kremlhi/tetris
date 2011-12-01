#define NPARTS 4
#define NEXTLEVEL 240

#define WIDTH 10
#define HEIGHT (20 + NPARTS)

#define MAXTEXTLEN 80

enum colour_t {
	NORMAL = 0,
	INVERT = 7,
	BGCOLOUR = 40,
	RED = 41,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	GREY
};

struct pos {
	int x;
	int y;
}; 

struct piece {
	struct pos p[NPARTS];
	enum colour_t colour;
};


