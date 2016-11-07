SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)
TARGET_SRC = ../int_bucket.c \
							../xlweighter.c \
						 ../string_indexer.c \
						 ../sparse_vector.c
TARGET_OBJS=$(TARGET_SRC:.c=.o) ../../THIRDPARTY-LIBS/GRAPES/src/net_helper-udp.o
LIBS=-lm -lgrapes
CFLAGS=-g -O0 -I../ -I../../THIRDPARTY-LIBS/GRAPES/include -L../../THIRDPARTY-LIBS/GRAPES/src

all: $(TARGET_SRC) $(TARGET_OBJS) $(OBJS)

%.test: %.c $(TARGET_OBJS) 
	$(CC) -o $@ $< $(CFLAGS) $(TARGET_OBJS) $(LIBS)

clean:
	rm -f *.test

.PHONY: all clean
