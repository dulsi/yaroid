CC	= /proj/yaroze/bin/psx-gcc
CFLAGS	= -O1 -g
LINKER	= -Xlinker -Ttext -Xlinker 80100000
RM	= rm

PROG	= yaroid
OBJS	= asteroid.o images.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LINKER) -o $@ $(OBJS)

asteroid.o: asteroid.c
images.o: images.c

clean:
	$(RM) $(PROG) 
	$(RM) $(OBJS)

