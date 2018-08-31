CC=gcc
CFLAGS=-Wall
INCLUDES=-Iincludes
LIBS=-lpthread
PROGNAME=threadpool
SRCS=threadpool.c jobqueue.c thread.c simple_test.c
OBJS=$(SRCS:.c=.o)

# DEBUG
CFLAGS := -g -DDEBUG

all: $(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(PROGNAME) $(LIBS)
	
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

clean:
	rm -rf *.o $(PROGNAME)
