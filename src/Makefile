CC 	= gcc

CFLAGS  = -Wall -g -fPIC -I ../include

OBJS = testfile.o lwp.o rr.o magic64.o

TARGETS = liblwp.so

all: liblwp.so

liblwp.so: lwp.o rr.o magic64.o
	$(CC) $(CFLAGS) -shared -o $@ lwp.o rr.o magic64.o

clean :
	rm -f $(OBJS) $(TARGETS) testfile

test: $(OBJS)
	$(CC) $(CFLAGS) -o testfile $(OBJS)

lwp.o : lwp.c ../include/lwp.h ../include/rr.h
	$(CC) $(CFLAGS) -c lwp.c

rr.o : rr.c ../include/rr.h
	$(CC) $(CFLAGS) -fPIC -c rr.c

testfile.o : testfile.c
	$(CC) $(CFLAGS) -c testfile.c

magic64.o : magic64.S
	$(CC) $(CFLAGS) -c magic64.S
