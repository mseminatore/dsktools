# get arch name
ARCH = $(shell uname -m)
TARGET = dsk_test
DEPS	= dsk.h
OBJS	= dsk.o
CFLAGS	= -I. -g -Wall
LIBNAME = libdsk.a
LFLAGS += -L. -ldsk -lm

all: $(LIBNAME) $(TARGET)
	
$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $(OBJS)

$(TARGET): $(OBJS) main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm $(TARGET) $(LIBNAME) $(OBJS) *.o
