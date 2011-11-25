PRGNAM=fixeler

CFLAGS=-Wall -O2 $(shell pkg-config --cflags glib-2.0 gtk+-2.0)
LDFLAGS=$(shell pkg-config --libs glib-2.0 gtk+-2.0)

SRC=fixeler.c
OBJ=$(SRC:.c=.o)

all: fixiler

fixiler: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(PRGNAM)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(OBJ) $(PRGNAM)
