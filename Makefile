CC = gcc
TARGETS = cat grep ls echo pwd ln

all: $(TARGETS)

cat: cat.c
	$(CC) -o cat cat.c

grep: grep.c
	$(CC) -o grep grep.c

ls: ls.c
	$(CC) -o ls ls.c

echo: echo.c
	$(CC) -o echo echo.c

pwd: pwd.c
	$(CC) -o pwd pwd.c

ln: ln.c
	$(CC) -o ln ln.c

clean:
	rm -f $(TARGETS)

Phony: all clean
