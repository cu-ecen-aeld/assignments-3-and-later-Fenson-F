SRC := writer.c 
TARGET = writer
#OBJS := $(SRC:.c=.o)

#CC_host = gcc
CC= $(CROSS_COMPILE)gcc

#Sources = writer.c

all: $(TARGET)

#$(TARGET) : $(OBJS)
$(TARGET) :
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -c $(TARGET).c -o $(TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET).c -o $(TARGET).o
	
#CROSS_COMPILE aarch64-none-linux-gnu- : $(OBJS)
#	aarch64-none-linux-gnu- writer.c -o writer

.PHONY : clean

clean:
	-rm -f *.o $(TARGET) 