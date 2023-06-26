
TARGET = aesdsocket
CC= $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g -O0

.PHONY : all

all: aesdsocket.c
	$(CC) aesdsocket.c $(CFLAGS) -o aesdsocket


.PHONY : clean
clean:
	-rm -f *.o aesdsocket
