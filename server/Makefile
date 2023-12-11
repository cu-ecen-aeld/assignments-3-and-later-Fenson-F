
TARGET ?= aesdsocket
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt 
RM = rm -f
SRC := aesdsocket.c
OBJS := $(SRC:.c=.o)

all:$(TARGET)

default: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(INCLUDES) $(LDFLAGS)

clean:
	$(RM) *.o $(TARGET)