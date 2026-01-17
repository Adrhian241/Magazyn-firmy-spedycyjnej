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
#include "dane.h"


int g_shmid = -1;
int g_semid = -1;
int g_msgid = -1;

pid_t g_parent_pid;
pid_t g_pids_pracownicy[3];
pid_t g_pid_p4;
pid_t g_pids_ciezarowki[N];

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

void handle_sigint(int sig) 
{
    if (getpid() != g_parent_pid) //sprawdzam czy main wywołuje funkcję
    {
        exit(0); 
    }
    
    signal(SIGTERM, SIG_IGN); 

    fprintf(stderr, "\n[MAIN] SIGINT! Zabijam procesy (kill 0)...\n");

    kill(0, SIGTERM);  //używam zwykłego SIGTERMA 

    int status;
    pid_t wpid;
   
    while ((wpid = wait(&status)) > 0);   //czekam aż wszystkie procesy się zakończą i sprzątam

    fprintf(stderr, "[MAIN] Sprzatam IPC...\n");
    
    if (g_shmid != -1) 
    {
        shmctl(g_shmid, IPC_RMID, NULL);
    }
    if (g_semid != -1) 
    {
        semctl(g_semid, 0, IPC_RMID);
    }
    if (g_msgid != -1) 
    {
        msgctl(g_msgid, IPC_RMID, NULL);
    }

    fprintf(stderr, "[MAIN] KONIEC.\n");
    exit(0);
}

void ustaw_semafor(int semid, int numer_semafora, int wartosc)
{
    if (semctl(semid,numer_semafora,SETVAL,wartosc)==-1)
    {
            perror("[MAIN] Nie mozna ustawic semafora");
            exit(EXIT_FAILURE);
    }
    else
    {
        logp(KOLOR_CYAN,"[MAIN] semafor %d zostal ustawiony na %d.\n",numer_semafora,wartosc);
    }
}

void handle_sigint_dyspozytor(int sig) 
{
    printf(KOLOR_RED "\n[DYSPOZYTOR] Zamykanie panelu sterowania...\n" KOLOR_RESET);
    exit(0);
}

double losuj_paczke()
{
    int r = rand()%3 + 1;
    if (r == 1) return 1;
    else if (r == 2) return 2;
    else return 3;
}

double losuj_wage(int typ_paczki) 
{
    int waga_int;
    if (typ_paczki == 1) waga_int = (rand() % 80) + 1;       // 0.1 - 8.0 kg
    else if (typ_paczki == 2) waga_int = (rand() % 100) + 80; // 8.0 - 18.0 kg
    else waga_int = (rand() % 100) + 150;                        // 15.0 - 25.0 kg
    return (double)waga_int / 10.0;
}