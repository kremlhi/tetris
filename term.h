void clear(void);
void posxy(int, int);
void drawpixel(int, int, enum colour_t);
void flush(void);
void init(void);
void cleanexit(int);
void redraw(int);
void drawtext(char *, ...);
char readch(void);

#define CSI "\033["
