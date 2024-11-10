CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lreadline

SRCS = shell.c server.c client.c multi_server.c main.c
OBJS = $(SRCS:.c=.o)

all: main

main: main.o shell.o server.o client.o multi_server.o
	$(CC) $(CFLAGS) -o main main.o shell.o server.o client.o multi_server.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) main