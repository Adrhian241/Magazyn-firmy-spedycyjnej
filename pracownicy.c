#include "dane.h"
double losuj_paczke()
{
    int r = rand()%3 + 1;
    if (r == 1) return 1;
    else if (r == 2) return 2;
    else return 3;
}
double losuj_wage(int typ_paczki) {
    int waga_int;
    if (typ_paczki == 1) waga_int = (rand() % 80) + 1;       // 0.1 - 8.0 kg
    else if (typ_paczki == 2) waga_int = (rand() % 100) + 80; // 8.0 - 18.0 kg
    else waga_int = (rand() % 100) + 150;                        // 15.0 - 25.0 kg
    return (double)waga_int / 10.0;
}
int main(int argc,char *argv[])
{
    int id = atoi(argv[1]);
    srand(time(NULL) ^ getpid());
    printf("[PRACOWNIK %d] zaczyna prace.\n", id);

    int shmid = shmget(KEY_SHM,sizeof(MagazynShared),0600);
    if (shmid == -1)
    {
        perror("[PRACOWNIK] Blad shmget");
        exit(1);
    }

    MagazynShared *wspolna = (MagazynShared*)shmat(shmid, NULL, 0);
    if (wspolna == (void*)-1) 
    {
        perror("[PRACOWNIK] Blad shmat");
        exit(1);
    }
    int semid = semget(KEY_SEM,0,0);
    if (semid == -1)
    {
        perror("[PRACOWNIK] blad semget");
        exit(1);
    }
    while(1)
    {
	if(wspolna->koniec_symulacji)
        {
            printf("[PRACOWNIK %d] koniec pracy\n",id);
            break;
        }
        useconds_t czas_snu = 200000 + (rand() % 500000);
        usleep(czas_snu);
        int czy_udalo_sie_polozyc = 0;
        char typ_paczki = losuj_paczke();
        double waga_paczki = losuj_wage(typ_paczki);
        char nazwa_paczki = (typ_paczki == 1) ? 'A' : (typ_paczki == 2) ? 'B' : 'C';
        double objetosc_paczki = (typ_paczki == 1) ? 19456 : (typ_paczki == 2) ? 46208 : 99712; //w cm3
        while(!czy_udalo_sie_polozyc)
        {
	    if(wspolna->koniec_symulacji)
            {
                break;
            }
            sem_P(semid, SEM_EMPTY);
	    sem_P(semid, SEM_MUTEX_TASMA);
            if(wspolna->tasma.masa_paczek + waga_paczki <= M)
            {
                int indeks = wspolna->tasma.tail;
                wspolna->tasma.bufor[indeks].id_pracownika = id;
                wspolna->tasma.bufor[indeks].waga = waga_paczki;
                wspolna->tasma.bufor[indeks].objetosc = objetosc_paczki;
                wspolna->tasma.bufor[indeks].typ = typ_paczki;
                wspolna->tasma.tail = (indeks + 1) % K;
                wspolna->tasma.masa_paczek += waga_paczki;
                wspolna->tasma.ilosc_paczek += 1;
                 printf("[PRACOWNIK %d] + Dodal paczke %c (%.1fkg) o V = %.1fcm3, Tasma: %d/%d szt, %.1f/%.1f kg\n"
                ,id,nazwa_paczki,waga_paczki,objetosc_paczki,wspolna->tasma.ilosc_paczek, K,wspolna->tasma.masa_paczek, M);

		czy_udalo_sie_polozyc = 1;
                sem_V(semid, SEM_MUTEX_TASMA);
                sem_V(semid, SEM_FULL);
            }
            else
            {
                printf("[PRACOWNIK %d] !cd Paczka %.1fkg za ciezka (Tasma: %.1fkg). Czekam z paczka...\n",
                id, waga_paczki, wspolna->tasma.masa_paczek);

		sem_V(semid, SEM_MUTEX_TASMA);
                sem_V(semid, SEM_EMPTY);
                sleep(3);

            }
        }
    }

}
