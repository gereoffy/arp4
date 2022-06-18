
# SRCS = $(wildcard *.c)
# BINS = $(patsubst %.c,%,$(SRCS))
BINS = traffmon2 traffer2 traffdata pinger pingstat3 pingdata

CC = gcc
CFLAGS = -Os -Wno-unused-result
# -Wall -Wno-unused-variable -Wno-pointer-sign
LDFLAGS = -lpng -lm

all: $(BINS)

$(BINS): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(BINS)

.PHONY: all clean
