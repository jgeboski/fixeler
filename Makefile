PRGNAM = fixeler
PRGCNF = glib-2.0 gtk+-2.0

CFLAGS  = -Wall -O0 -g
LDFLAGS = -Wl,-O0

PRGCFLAGS  = $(CFLAGS)  $(shell pkg-config --cflags $(PRGCNF))
PRGLDFLAGS = $(LDFLAGS) $(shell pkg-config --libs   $(PRGCNF))

PRGSRCS = fixeler.c
PRGOBJS = $(PRGSRCS:.c=.o)

all: $(PRGNAM)

$(PRGNAM): $(PRGOBJS)
	$(CC) $(PRGLDFLAGS) -o $(PRGNAM) $(PRGOBJS)

%.o: %.c
	$(CC) $(PRGCFLAGS) -o $@ -c $<

clean:
	$(RM) $(PRGNAM) $(PRGOBJS)
