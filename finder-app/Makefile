SRC := writer.c 
TARGET = writer
OBJS := $(SRC:.c=.o)

CC_host = gcc
CC_xcompile = /home/fenson/gcc-arm-aarch64-none-linux-gnu/usr/bin/aarch64-none-linux-gnu-gcc

Sources = writer.c

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC_host) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

CROSS_COMPILE aarch64-none-linux-gnu- : $(OBJS)
	/home/fenson/gcc-arm-aarch64-none-linux-gnu/usr/bin/aarch64-none-linux-gnu-gcc writer.c -o writer

.PHONY : clean

clean:
	-rm -f *.o $(TARGET) 