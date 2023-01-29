TARGET_FILE = writer
#CROSS_COMPILE = aarch64-none-linux-gnu-
#if CROSS_COMPILE SPECIFIED IN COMMAND LINE

ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
endif

#Else it will run with CC=gcc

$(TARGET_FILE) : writer.c	
	$(CC) -o writer writer.c $(CFLAGS)  

clean:
	rm -f *.O $(TARGET_FILE)
