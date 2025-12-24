#include "dane.h"
#include <errno.h>

int main(int argc, char *argv[])
{
    int id =  atoi(argv[1]);
    srand(time(NULL));
    int shmid = shmget(KEY_SHM, sizeof(MagazynShared), 0600);
    if (shmid == -1) 
    {
        perror("[CIEZAROWKA] Blad shmget"); exit(1); 
    }
    MagazynShared *wspolna = (MagazynShared*)shmat(shmid, NULL, 0);

    int semid = semget(KEY_SEM, 0, 0);
    if (semid == -1) 
    { 
        perror("[CIEZAROWKA] blad semget"); exit(1); 
    }
    int msgid = msgget(KEY_MSG, 0666);
    if (msgid == -1) 
    { 
        perror("[CIEZAROWKA ] Blad msgget"); exit(1); 
    }

    printf("[CIEZAROWKA %d] zaczyna prace.\n",id);
    while(1)
    {
        if (wspolna->koniec_symulacji && wspolna->tasma.ilosc_paczek == 0) 
        {
            printf("[CIEZAROWKA %d] Koniec symulacji i brak paczek. Koncze prace\n", id);
            break; 
        }
        sem_P(semid, SEM_RAMPA);
        if (wspolna->koniec_symulacji && wspolna->tasma.ilosc_paczek == 0) 
        {
            printf("[CIEZAROWKA %d] Wjechalem na rampe ale brak paczek. Koncze prace\n", id);
            sem_V(semid, SEM_RAMPA);
            break; 
        }
        sem_P(semid, SEM_MUTEX_CIEZAROWKA);

        wspolna->ciezarowka.id_ciezarowki = id;
        wspolna->ciezarowka.zaladowana_waga = 0.0;
        wspolna->ciezarowka.zaladowana_objetosc = 0;
        wspolna->ciezarowka.czy_stoi = 1; //tak
	wspolna->ciezarowka.wymus_odjazd = 0;

        sem_V(semid, SEM_MUTEX_CIEZAROWKA);
        printf("\n [CIEZAROWKA %d] --- Podjechalem pod rampe\n", id);

        int czy_pelna = 0;
        struct moj_komunikat msg;

        while(czy_pelna == 0)
        {
	    if (wspolna->ciezarowka.wymus_odjazd == 1) 
	    {
                printf("[CIEZAROWKA %d] P4 wymusil odjazd (brak miejsca). Odjezdzam.\n", id);
                czy_pelna = 1;
                continue;
            }
            if (msgrcv(msgid, &msg, sizeof(int), 1, IPC_NOWAIT) != -1)
            {
		if(wspolna->koniec_symulacji)
		{
                    continue;
		}
		printf("\nDYSPOZYTOR KAZE ODJECHAC (Sygnal 1)!\n");
                czy_pelna = 1;
                continue;
            }
            
            struct sembuf check_full = {SEM_FULL, -1, IPC_NOWAIT};
            if (semop(semid, &check_full, 1) == -1) {
                if(wspolna->koniec_symulacji) 
                {
                    printf("[CIEZAROWKA %d] Koniec symulacji i pusta tasma. Koncze ladunek i odjezdzam w ostatnia trase.\n", id);
                    czy_pelna = 1; 
                    continue;
                }
                usleep(350000); 
                continue;
            }
            usleep(350000);
	    sem_P(semid, SEM_PRACOWNIK4); 
            sem_V(semid, SEM_PRACOWNIK4);
            sem_P(semid, SEM_MUTEX_TASMA);

            int idx = wspolna->tasma.head;
            Paczka p = wspolna->tasma.bufor[idx];
            int vol = p.objetosc;
            
	        sem_P(semid, SEM_MUTEX_CIEZAROWKA);
            if ( (wspolna->ciezarowka.zaladowana_waga + p.waga <= W) &&
                 (wspolna->ciezarowka.zaladowana_objetosc + vol <= V) )
            {
                wspolna->ciezarowka.zaladowana_waga += p.waga;
                wspolna->ciezarowka.zaladowana_objetosc += p.objetosc;
                wspolna->tasma.head = (idx + 1) % K;
                wspolna->tasma.ilosc_paczek--;
                wspolna->tasma.masa_paczek -= p.waga;

                printf("[CIEZAROWKA %d] Zaladowano %c (%.1fkg). Stan: %.1f/%.1f kg ---- %.1f/%.0fcm3\n",
                       id, p.typ, p.waga, wspolna->ciezarowka.zaladowana_waga, W, wspolna->ciezarowka.zaladowana_objetosc,V);

                sem_V(semid, SEM_MUTEX_CIEZAROWKA);
                sem_V(semid, SEM_MUTEX_TASMA);
                sem_V(semid, SEM_EMPTY);    
            }
            else
            {
                printf("[CIEZAROWKA %d] pelna! Paczka %.1fkg nie wejdzie. Odjazd.\n", id, p.waga);
                czy_pelna=1;
                sem_V(semid, SEM_MUTEX_CIEZAROWKA);
                sem_V(semid, SEM_MUTEX_TASMA);
                sem_V(semid, SEM_FULL);
            }
        }

        sem_P(semid, SEM_MUTEX_CIEZAROWKA);
        wspolna->ciezarowka.czy_stoi = 0; // Zwalniamy logicznie (dla P4)
        sem_V(semid, SEM_MUTEX_CIEZAROWKA);
        if(wspolna->koniec_symulacji && wspolna->ciezarowka.zaladowana_waga == 0) 
        {
             printf("[CIEZAROWKA %d] Pusta i koniec pracy. Zwalniam rampe i wychodze.\n", id);
             sem_V(semid, SEM_RAMPA); 
             break;
        }
        printf("[CIEZAROWKA %d] Odjezdzam w trase (%ds)...\n", id, TI);
        sem_V(semid, SEM_RAMPA);

        sleep(TI);
	if(wspolna->koniec_symulacji && wspolna->tasma.ilosc_paczek == 0)
        {
            printf("[CIEZAROWKA %d] Koniec mojej pracy.\n", id);
        }

        else
        {
            printf("[CIEZAROWKA %d] Wrocilem z trasy. Ustawiam sie w kolejce.\n", id);
        }
    }

}
