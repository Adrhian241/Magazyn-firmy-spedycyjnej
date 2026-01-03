CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

DEPS = dane.h
TARGETS = m pracownicy pracownik4 ciezarowka dyspozytor

all: $(TARGETS)


m: main.c $(DEPS)
	$(CC) $(CFLAGS) -o m main.c

pracownicy: pracownicy.c $(DEPS)
	$(CC) $(CFLAGS) -o pracownicy pracownicy.c

pracownik4: pracownik4.c $(DEPS)
	$(CC) $(CFLAGS) -o pracownik4 pracownik4.c

ciezarowka: ciezarowka.c $(DEPS)
	$(CC) $(CFLAGS) -o ciezarowka ciezarowka.c

dyspozytor: dyspozytor.c $(DEPS)
	$(CC) $(CFLAGS) -o dyspozytor dyspozytor.c

clean:
	rm -f $(TARGETS) raport.txt

.PHONY: all clean
