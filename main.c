#include "dane.h"
void ustaw_semafor(int semid, int numer_semafora, int wartosc)
{
    if (semctl(semid,numer_semafora,SETVAL,wartosc)==-1)
    {
	    perror("[MAIN] Nie mozna ustawic semafora");
	    exit(EXIT_FAILURE);
    }
    else
    {
        printf("[MAIN] semafor %d zostal ustawiony na %d.\n",numer_semafora,wartosc);
    }
}
int main(){
    
    //stworzenie pamieci dzielonej
    printf("[MAIN] START SYMULACJI MAGAZYNU");
    int shmid = shmget(KEY_SHM, sizeof(MagazynShared), IPC_CREAT | 0600);
    if (shmid == -1)
    {
	printf("[MAIN] problem z utworzeniem pamieci dzielonej. \n");
	exit(EXIT_FAILURE);
    }
    else printf("[MAIN] Pamiec dzielona zostala utworzona : %d\n",shmid);
    
    //stworzenie semaforow
    int semid = semget(KEY_SEM, 5, IPC_CREAT | 0600);
        if (semid==-1)
        {
                printf("[MAIN] Nie moglem utworzyc nowych semaforow.\n");
                exit(EXIT_FAILURE);
        }
        else
        {
                printf("[MAIN] Semafory zostal utworzone : %d\n",semid);
        }

    //tworzenie kolejki komunikatow
    int msgid = msgget(KEY_MSG, IPC_CREAT | 0600);
    if (msgid == -1) 
    {
        perror("[MAIN] Nie moglem utworzyc kolejki komunikatow");
        exit(1);
    }
    printf("[MAIN] Kolejka komunikatow utworzona: %d\n", msgid);

    //polaczenie sie z pamieci dzielona
    MagazynShared *wspolna = (MagazynShared*)shmat(shmid, NULL, 0);
    if (wspolna == (void*)-1)
    {
        perror("[MAIN] Blad shmat");
        exit(1);
    }
    //ustawianie semaforow
    ustaw_semafor(semid, SEM_MUTEX_TASMA, 1);
    ustaw_semafor(semid, SEM_MUTEX_CIEZAROWKA, 1);
    ustaw_semafor(semid, SEM_EMPTY, K);
    ustaw_semafor(semid, SEM_FULL, 0);
    ustaw_semafor(semid, SEM_RAMPA, 1);

    //ustawienia poczatkowe tasmy i ciezarowek
    wspolna->tasma.head = 0;
    wspolna->tasma.tail = 0;
    wspolna->tasma.ilosc_paczek = 0;
    wspolna->tasma.masa_paczek = 0.0;
    wspolna->ciezarowka.czy_stoi = 0;
    wspolna->ciezarowka.id_ciezarowki = -1;
    wspolna->koniec_symulacji = 0;
    shmdt(wspolna);

    //tworzenie pracownikow 1,2,3
    char id_str[10]; // Bufor tekstowy na ID pracownika
        for (int i = 1; i <= 3; i++) 
	    {
            pid_t pid = fork();

            if (pid == 0)
	    {
            sprintf(id_str, "%d", i); // Zamiana int na string, np. 1 -> "1"
            execlp("./pracownicy", "pracownicy", id_str, NULL);
            perror("[MAIN] Blad execlp (uruchamianie pracownika)");
            exit(1);
            }
            else if (pid < 0) 
	    {
                perror("[MAIN] Blad fork");
            }
            else
            {
                printf("[MAIN] Uruchomiono pracownika P%d (PID: %d)\n", i, pid);
            }
        }

    //tworzenie N ciezarowek
     char id_c[10];
        for (int i = 1; i <= N; i++) 
        {
            pid_t pid = fork();

            if (pid == 0) 
            {
            sprintf(id_c, "%d", i); // Zamiana int na string, np. 1 -> "1"
            execlp("./ciezarowka", "ciezarowka", id_c, NULL);
            perror("[MAIN] Blad execlp (uruchamianie ciezarowki)");
            exit(1);
            }
            else if (pid < 0) 
	    {
                perror("[MAIN] Blad fork");
            }
                else 
	    {
                printf("[MAIN] Uruchomiono ciezarowke C%d (PID: %d)\n", i, pid);
            }
        }
	int status;
    while (wait(&status) > 0);

    printf("\n[MAIN] Wszyscy pracownicy oraz ciezarowki zakonczyly prace. Sprzatam system.\n");

    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    msgctl(msgid, IPC_RMID, NULL);
}
