CC 	= gcc

CFLAGS  = -Wall -g -I ../include

test : testfile.o lwp.o
	$(CC) $(CFLAGS) -o testfile

lwp.o : lwp.c lwp.h
	$(CC) $(CFLAGS) -c lwp.c

testfile.o : testfile.c
	$(CC) $(CFLAGS) -c testfile.c

