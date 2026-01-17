CC = gcc
CFLAGS = -Wall -g
LIBS = 

all: m pracownicy pracownik4 ciezarowka dyspozytor

dane.o: dane.c dane.h
	$(CC) $(CFLAGS) -c dane.c

m: main.c dane.o dane.h
	$(CC) $(CFLAGS) -o m main.c dane.o $(LIBS)

pracownicy: pracownicy.c dane.o dane.h
	$(CC) $(CFLAGS) -o pracownicy pracownicy.c dane.o $(LIBS)

pracownik4: pracownik4.c dane.o dane.h
	$(CC) $(CFLAGS) -o pracownik4 pracownik4.c dane.o $(LIBS)

ciezarowka: ciezarowka.c dane.o dane.h
	$(CC) $(CFLAGS) -o ciezarowka ciezarowka.c dane.o $(LIBS)

dyspozytor: dyspozytor.c dane.o dane.h
	$(CC) $(CFLAGS) -o dyspozytor dyspozytor.c dane.o $(LIBS)

clean:
	rm -f *.o m pracownicy pracownik4 ciezarowka dyspozytor raport.txt