CFLAGS += -ansi -pedantic -Wall
srcs := tetris.c term.c
objs := $(srcs:.c=.o)
incs := tetris.h term.h
bin = tetris

%.o: %.c $(incs)
	$(CC) $(CFLAGS) -c $<

$(bin): $(objs)
	$(CC) $(LDFLAGS) -o $(bin) $(objs)

all: $(bin)

clean:
	$(RM) $(bin) $(objs)
