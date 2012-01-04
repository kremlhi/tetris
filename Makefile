CFLAGS += -ansi -pedantic -Wall -DUSE_SELECT
srcs := tetris.c term.c
objs := $(srcs:.c=.o)
incs := tetris.h term.h
bin = tetris

all: $(bin)
	
%.o: %.c $(incs)
	$(CC) $(CFLAGS) -c $<

$(bin): $(objs)
	$(CC) $(LDFLAGS) -o $(bin) $(objs)

clean:
	$(RM) $(bin) $(objs)
