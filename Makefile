# get arch name
ARCH = $(shell uname -m)
TARGET = dsktools
DEPS	= dsk.h
OBJS	= dsk.o
CFLAGS	= -I. -g -Wall
LIBNAME = libdsk.a
LFLAGS += -L. -ldsk -lm

all: $(LIBNAME) $(TARGET) dsk_new dsk_format dsk_add dsk_extract dsk_rename dsk_del
	
$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $(OBJS)

dsk_new: dsk_new.o $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

dsk_format: dsk_format.o $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

dsk_add: dsk_add.o $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

dsk_extract: dsk_extract.o $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

dsk_rename: dsk_rename.o $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

dsk_del: dsk_del.o $(LIBNAME)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TARGET): $(OBJS) main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm $(TARGET) $(LIBNAME) $(OBJS) *.o dsk_new dsk_format dsk_add dsk_extract dsk_rename dsk_del
