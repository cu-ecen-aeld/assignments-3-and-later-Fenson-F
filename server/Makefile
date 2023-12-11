
TARGET ?= aesdsocket
CC ?= $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g3 
LDFLAGS = -pthread 
RM = rm -f

.PHONY : all

all: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c $(LDFLAGS)


.PHONY : clean
clean:
	$(RM) aesdsocket
	$(RM) *.o 