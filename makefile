CC = gcc
CFLAGS = -Wall -g

BINS = ocr

all: $(BINS)

ocr:  lab3.c
	$(CC) -o ocr lab3.c $(CFLAGS)

clean:
	rm $(BINS)

cleanorig:
	rm *.orig
