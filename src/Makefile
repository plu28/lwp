CC 	= gcc

CFLAGS  = -Wall -g -I ../include

OBJS = testfile.o lwp.o rr.o magic64.o

TARGETS = testfile

.PHONY = clean

clean :
	rm -f $(OBJS) $(TARGETS)

test : $(OBJS)
	$(CC) $(CFLAGS) -o testfile $(OBJS)

lwp.o : lwp.c ../include/lwp.h ../include/rr.h
	$(CC) $(CFLAGS) -c lwp.c

rr.o : rr.c ../include/rr.h
	$(CC) $(CFLAGS) -c rr.c

testfile.o : testfile.c
	$(CC) $(CFLAGS) -c testfile.c

magic64.o : magic64.S
	$(CC) $(CFLAGS) -c magic64.S