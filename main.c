#include "dane.h"





int main()
{

    g_parent_pid = getpid();
    printf(KOLOR_CYAN "\n");
    printf("   ##################################################\n");
    printf("   #           SYMULACJA MAGAZYNU SPEDYCYJNEGO      #\n");
    printf("   ##################################################\n");
    printf("   #                                                #\n");
    printf("   #  KONFIGURACJA SYSTEMU:                         #\n");
    printf("   #  --------------------------------------------  #\n");
    printf("   #  [-] Liczba ciezarowek (N):      %-4d          #\n", N);
    printf("   #  [-] Czas trasy (TI):            %-4d sek      #\n", TI);
    printf("   #  [-] Zaloga:                     3 + 1 (P4)    #\n");
    printf("   #                                                #\n");
    printf("   #  PARAMETRY TASMY I LADUNKU:                    #\n");
    printf("   #  --------------------------------------------  #\n");
    printf("   #  [-] Pojemnosc tasmy (K):        %-4d szt      #\n", K);
    printf("   #  [-] Maks. masa na tasmie (M):   %-6.1f kg     #\n", M);
    printf("   #  [-] Ladownosc ciezarowki (W):   %-6.1f kg     #\n", W);
    printf("   #  [-] Pojemnosc ciezarowki (V):   %-6.1f m3     #\n", V);
    printf("   #                                                #\n");
    printf("   ##################################################\n");
    printf("\n" KOLOR_RESET);
    fflush(stdout);
    
    signal(SIGINT, handle_sigint);
    FILE *fp = fopen("raport.txt", "w");
    if (fp)
    {
    fclose(fp);
    }

    //stworzenie semaforow
    int semid = semget(KEY_SEM, 7, IPC_CREAT | 0600);
        if (semid==-1)
        {
                perror("[MAIN] Nie moglem utworzyc nowych semaforow.\n");
                exit(EXIT_FAILURE);
        }
        else
        {
            ustaw_semafor(semid, SEM_LOG, 1); //ustawienie w tym miejscu, żeby logi mogły działać od początku
            logp(KOLOR_CYAN,"[MAIN] Semafory zostal utworzone : %d\n",semid);
         	g_semid = semid;
	}
    

    logp(KOLOR_CYAN,"[MAIN] START SYMULACJI MAGAZYNU\n");

    //stworzenie pamieci dzielonej
    int shmid = shmget(KEY_SHM, sizeof(MagazynShared), IPC_CREAT | 0600);
    if (shmid == -1)
    {
        logp(KOLOR_CYAN,"[MAIN] problem z utworzeniem pamieci dzielonej. \n");
        exit(EXIT_FAILURE);
    }
    else
    {
	    logp(KOLOR_CYAN,"[MAIN] Pamiec dzielona zostala utworzona : %d\n",shmid);
	    g_shmid = shmid;
    }
    
    //tworzenie kolejki komunikatow
    int msgid = msgget(KEY_MSG, IPC_CREAT | 0600);
    if (msgid == -1)
    {
        perror("[MAIN] Nie moglem utworzyc kolejki komunikatow");
        exit(EXIT_FAILURE);
    }
    else
    {
	    logp(KOLOR_CYAN,"[MAIN] Kolejka komunikatow utworzona: %d\n", msgid);
	    g_msgid = msgid;
    }

    //polaczenie sie z pamieci dzielona
    MagazynShared *wspolna = (MagazynShared*)shmat(shmid, NULL, 0);
    if (wspolna == (void*)-1)
    {
        perror("[MAIN] Blad shmat");
	    handle_sigint(0);
    }

    //ustawianie semaforow
    ustaw_semafor(semid, SEM_MUTEX_TASMA, 1);
    ustaw_semafor(semid, SEM_MUTEX_CIEZAROWKA, 1);
    ustaw_semafor(semid, SEM_EMPTY, K);
    ustaw_semafor(semid, SEM_FULL, 0);
    ustaw_semafor(semid, SEM_RAMPA, 1);
    ustaw_semafor(semid, SEM_PRACOWNIK4, 1);

    //ustawienia poczatkowe tasmy i ciezarowek
    wspolna->tasma.head = 0;
    wspolna->tasma.tail = 0;
    wspolna->tasma.ilosc_paczek = 0;
    wspolna->tasma.masa_paczek = 0.0;
    wspolna->ciezarowka.czy_stoi = 0;
    wspolna->ciezarowka.id_ciezarowki = -1;
    wspolna->koniec_symulacji = 0;
    wspolna->ciezarowka.wymus_odjazd = 0;

    shmdt(wspolna);

    //tworzenie pracownikow 1,2,3
    char id_str[10]; // Bufor tekstowy na ID pracownika
        for (int i = 1; i <= 3; i++)
            {
            pid_t pid = fork();

            if (pid == 0)
            {
            signal(SIGINT, SIG_DFL); // Powrót do default, żeby nie czyściło IPC 
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
                g_pids_pracownicy[i-1] = pid;
		        logp(KOLOR_CYAN,"[MAIN] Uruchomiono pracownika P%d (PID: %d)\n", i, pid);
            }
        }

    //tworzenie pracownika od paczek ekspresowych p4
    pid_t pid4 = fork();
    if (pid4 == 0)
    {
        signal(SIGINT, SIG_DFL);
        execlp("./pracownik4", "pracownik4", NULL, NULL);
        perror("[MAIN] Blad execlp (uruchamianie pracownika4)");
        exit(EXIT_FAILURE);
    }
    else if (pid4 < 0)
    {
        perror("[MAIN] Blad fork");
    }
    else
    {
	    g_pid_p4 = pid4;
        logp(KOLOR_CYAN,"[MAIN] Uruchomiono pracownika P4 (PID: %d)\n",pid4);
    }

    //tworzenie N ciezarowek
    char id_c[10];
        for (int i = 1; i <= N; i++)
        {
            pid_t pid = fork();

            if (pid == 0)
            {
            signal(SIGINT, SIG_DFL);
            sprintf(id_c, "%d", i); // Zamiana int na string, np. 1 -> "1"
            execlp("./ciezarowka", "ciezarowka", id_c, NULL);
            perror("[MAIN] Blad execlp (uruchamianie ciezarowki)");
            exit(EXIT_FAILURE);
            }
            else if (pid < 0)
            {
                perror("[MAIN] Blad fork");
            }
                else
            {
		        g_pids_ciezarowki[i-1] = pid;
                logp(KOLOR_CYAN,"[MAIN] Uruchomiono ciezarowke C%d (PID: %d)\n", i, pid);
            }
        }
        int status;
    while (wait(&status) > 0);

    logp(KOLOR_CYAN,"\n[MAIN] Wszyscy pracownicy oraz ciezarowki zakonczyly prace. Sprzatam system.\n");

    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    msgctl(msgid, IPC_RMID, NULL);
}
