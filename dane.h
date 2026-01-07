#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>

#define KOLOR_RED     "\x1b[31m"
#define KOLOR_GREEN   "\x1b[32m"
#define KOLOR_YELLOW  "\x1b[33m"
#define KOLOR_BLUE    "\x1b[34m"
#define KOLOR_CYAN    "\x1b[36m"
#define KOLOR_RESET   "\x1b[0m"

#define K 30 //Pojemnosc tasmy ladunkowej
#define M 400.0 //Maksymalna masa przesylek na tasmie ladunkowej [kg]
#define W 1500.0 //Ladownosc ciezarowki [kg]
#define V 15.0 //Objetosc ciezarowki [m3]
#define N 4 //Liczba ciezarowek
#define TI 3 // czas po jakim wraca ciezarowka w sekundach
#define MAX_BUFOR 100 // ilosc paczek jakie moga byc wytworzone przez pracownika 4 w jednym momencie
#define KEY_SHM 1111
#define KEY_SEM 2222
#define KEY_MSG 3333

// ID semaforow w tablicy semaforow
#define SEM_MUTEX_TASMA 0    //dotep do tasmy
#define SEM_MUTEX_CIEZAROWKA 1   //dostep do wagi ciezarowki
#define SEM_EMPTY 2   // Ile miejsc wolnych na tasmie
#define SEM_FULL 3   //ile miejsc zajetych na tasmie
#define SEM_RAMPA 4   //umozliwia wjazd, wyjazd z rampy ciezarowce
#define SEM_PRACOWNIK4 5   //wprowadzenie paczek ekspresowych
void logp(const char *kolor, const char *format, ...) {
    va_list args;

    printf("%s", kolor); // Włącz kolor
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("%s", KOLOR_RESET); // Wyłącz kolor

    FILE *fp = fopen("raport.txt", "a");
    if (fp) {

        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);

        fclose(fp);
    }
}

void sem_P(int semid,int numer_semafora)
{
    struct sembuf operacja;
    operacja.sem_num = numer_semafora;
    operacja.sem_op = -1;
    operacja.sem_flg = 0;
    while (semop(semid, &operacja, 1) == -1) {
        if (errno == EINTR) continue;
        perror("Blad semafor_p");
        exit(EXIT_FAILURE);
    }
}
void sem_V(int semid,int numer_semafora)
{
    struct sembuf operacja;
    operacja.sem_num = numer_semafora;
    operacja.sem_op = 1;
    operacja.sem_flg = 0;
    if (semop(semid, &operacja, 1) == -1) {
        perror("Blad semafor_v");
        exit(EXIT_FAILURE);
    }
}
typedef struct{
        char typ; //A B C
        double waga;
        double objetosc;
        int id_pracownika; //1 2 3 4(ekspres)
}Paczka;

typedef struct{
        Paczka bufor[K];
        int head;
        int tail;
        int ilosc_paczek;
        double masa_paczek;
}Tasma;

typedef struct{
        double zaladowana_waga;
        double zaladowana_objetosc;
        int id_ciezarowki;
        int czy_stoi;
        int wymus_odjazd;
}Ciezarowka;

typedef struct {
    Tasma tasma;
    Ciezarowka ciezarowka;

    int koniec_symulacji;  // Flaga: 1 oznacza koniec pracy (sygnal 3)
} MagazynShared;

struct moj_komunikat {
    long int mtype; //typ komunikatu, 1 2 3
    char text[20]; // przekazane dane
};
