TARGET_FILE = aesdsocket
#CROSS_COMPILE = aarch64-none-linux-gnu-
#if CROSS_COMPILE SPECIFIED IN COMMAND LINE


CC	  :=$(CROSS_COMPILE)gcc
CFLAGS:=-Werror -Wall -g

#Else it will run with CC=gcc


all: aesdsocket.c
	$(CC) -o aesdsocket aesdsocket.c  $(CFLAGS)  

clean:
	rm -f $(TARGET_FILE) *.o
