TARGET_FILE = writer
#CROSS_COMPILE = aarch64-none-linux-gnu-
#if CROSS_COMPILE SPECIFIED IN COMMAND LINE


CC	:=	$(CROSS_COMPILE)gcc


#Else it will run with CC=gcc

all: writer.c	
	$(CC) -o writer writer.c $(CFLAGS)  

clean:
	rm -f *.O $(TARGET_FILE)
