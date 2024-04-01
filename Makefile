TARGET = dsk
DEPS	= 
OBJS	= dsk.o
CFLAGS	= -I. -g -Wall
LIBS	= -lm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

#as09.o:	as09.tab.c
#	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm $(TARGET) $(OBJS) *.o
