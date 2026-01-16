#include <signal.h>
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
#define M 420.0 //Maksymalna masa przesylek na tasmie ladunkowej [kg]
#define W 1500.0 //Ladownosc ciezarowki [kg]
#define V 15.0 //Objetosc ciezarowki [m3]
#define N 10 //Liczba ciezarowek
#define TI 3 // czas po jakim wraca ciezarowka w sekundach
#define MAX_BUFOR 100 // ilosc paczek jakie moga byc wytworzone przez pracownika 4 w jednym momencie
#define KEY_SHM 1111
#define KEY_SEM 2222
#define KEY_MSG 3333

// ID semaforow w tablicy semaforow
#define SEM_MUTEX_TASMA 0    //dostęp do taśmy
#define SEM_MUTEX_CIEZAROWKA 1   //dostęp do wagi ciężarówki
#define SEM_EMPTY 2   // Ile miejsc wolnych na taśmie
#define SEM_FULL 3   //ile miejsc zajętych na taśmie
#define SEM_RAMPA 4   //umożliwia wjazd, wyjazd z rampy ciężarówce
#define SEM_PRACOWNIK4 5   //wprowadzenie paczek ekspresowych
#define SEM_LOG 6   //synchronizacja logów



void sem_P(int semid, int numer_semafora)
{
    struct sembuf operacja;
    operacja.sem_num = numer_semafora;
    operacja.sem_op = -1;

    if (numer_semafora == SEM_EMPTY || numer_semafora == SEM_FULL) //semafory do obsługi ilości na taśmie
        operacja.sem_flg = 0; 
    else 
        operacja.sem_flg = SEM_UNDO; //gdy podczas sem_P(SEM_LOG) zakończy się program, żeby proces zwrócił semafor

    while (semop(semid, &operacja, 1) == -1) 
    {
        if (errno == EINTR) continue;
        perror("Blad semafor_p");
        exit(EXIT_FAILURE);
    }
}

void sem_V(int semid, int numer_semafora)
{
    struct sembuf operacja;
    operacja.sem_num = numer_semafora;
    operacja.sem_op = 1;

    if (numer_semafora == SEM_EMPTY || numer_semafora == SEM_FULL) 
        operacja.sem_flg = 0;
    else 
        operacja.sem_flg = SEM_UNDO;

    if (semop(semid, &operacja, 1) == -1)
    {
        perror("Blad semafor_v");
        exit(EXIT_FAILURE);
    }
}

void logp(const char *kolor, const char *format, ...)  //przyjmuje dowolną ilość argumentów
{

    int semid = semget(KEY_SEM, 0, 0); 
    if (semid != -1) sem_P(semid, SEM_LOG);

    va_list args;  //usatawia wskaznik za *format
    printf("%s", kolor); 
    va_start(args, format);   //przypisuje args
    vprintf(format, args);
    va_end(args);  //zakoncz
    printf("%s", KOLOR_RESET);
    fflush(stdout);

    FILE *fp = fopen("raport.txt", "a");
    if (fp) {

        va_start(args, format);
        vfprintf(fp, format, args);
        va_end(args);

        fclose(fp);

        if (semid != -1) sem_V(semid, SEM_LOG);
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
