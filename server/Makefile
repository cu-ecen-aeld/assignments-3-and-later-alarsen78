CFLAGS += -Wall -Werror -g -O0

CC=$(CROSS_COMPILE)gcc
all:
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:
	rm -f aesdsocket
