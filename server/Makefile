
TARGET ?= aesdsocket
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g -O0 
LDFLAGS ?= -lpthread -lrt
RM ?= rm -f

.PHONY : all

all: aesdsocket.c
	$(CC) aesdsocket.c $(CFLAGS) -o aesdsocket $(LDFLAGS)


.PHONY : clean
clean:
	$(RM) aesdsocket
	$(RM) *.o 