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
Paczka generuj_ekspres() 
{
    Paczka p;
    char typ_paczki = losuj_paczke();
    p.typ = (typ_paczki == 1) ? 'A' : (typ_paczki == 2) ? 'B' : 'C';
    p.waga = losuj_wage(typ_paczki);
    p.objetosc = (typ_paczki == 1) ? 19456 : (typ_paczki == 2) ? 46208 : 99712; //w cm3
    p.id_pracownika = 4;
    return p;
}
int main(int argc,char *argv[])
{
    
    srand(time(NULL));

    int shmid = shmget(KEY_SHM,sizeof(MagazynShared),0600);
    if (shmid == -1)
    {
        perror("[PRACOWNIK 4] Blad shmget");
	exit(EXIT_FAILURE);
    }

    MagazynShared *wspolna = (MagazynShared*)shmat(shmid, NULL, 0);
    if (wspolna == (void*)-1)
    {
        perror("[PRACOWNIK] Blad shmat");
	exit(EXIT_FAILURE);
    }
    
    int semid = semget(KEY_SEM,0,0);
    if (semid == -1)
    {
        perror("[PRACOWNIK] blad semget");
        exit(EXIT_FAILURE);
    }

    int msgid = msgget(KEY_MSG, 0600);
    if (msgid == -1) 
    { 
        perror("[PRACOWNIK4 ] Blad msgget"); 
        exit(1); 
    }
    
    logp("[PRACOWNIK 4] Pracownik 4 zaczyna prace.\n");
    sleep(1);
    struct moj_komunikat msg;
    Paczka bufor[MAX_BUFOR];
    int ilosc_w_buforze = 0;
    int tryb_wysylania = 0; //0 generuj 1 laduj
    int licznik_sekund = 0;
    while(1)
    {
        if(wspolna->koniec_symulacji) 
        {
            logp("[PRACOWNIK 4] Koniec pracy. W buforze: %d paczek\n", ilosc_w_buforze);
            break;
        }
        usleep(250000);
        if(licznik_sekund>=25)
	{
	    if (ilosc_w_buforze < MAX_BUFOR) 
            {
                bufor[ilosc_w_buforze] = generuj_ekspres();
                logp("[PRACOWNIK P4] + Dodal paczke %c (%.1fkg) o V = %.1fcm3\n"
                ,bufor[ilosc_w_buforze].typ,bufor[ilosc_w_buforze].waga,bufor[ilosc_w_buforze].objetosc);
                ilosc_w_buforze++;
	    licznik_sekund = 0;
	    }
	}
	licznik_sekund++;
        if (msgrcv(msgid, &msg, sizeof(int), 2, IPC_NOWAIT) != -1) {
            logp("\n[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe\n");
            tryb_wysylania = 1;
            for(int i=0;i<ilosc_w_buforze;i++)
            {
                logp("paczka %d waga = %.1f\n",i,bufor[i].waga);
            }
	}
        while (tryb_wysylania == 1 && ilosc_w_buforze > 0)    
        {
            sem_P(semid, SEM_PRACOWNIK4);
            sem_P(semid, SEM_MUTEX_CIEZAROWKA);
	    Paczka p = bufor[ilosc_w_buforze - 1];
	    int udalo_sie_zaladowac = 0;

            if (wspolna->ciezarowka.czy_stoi == 1)
            {
                if ( (wspolna->ciezarowka.zaladowana_waga + p.waga <= W) &&
                     (wspolna->ciezarowka.zaladowana_objetosc + p.objetosc <= (double)V) )
                {
                    wspolna->ciezarowka.zaladowana_waga += p.waga;
                    wspolna->ciezarowka.zaladowana_objetosc += p.objetosc;
                    ilosc_w_buforze--;
                    logp("[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: %d. Ciezarowka: %.1f/%.0f\n", 
                           ilosc_w_buforze, wspolna->ciezarowka.zaladowana_waga, W);
                    udalo_sie_zaladowac=1;
		}
                else
                {
                    logp("[PRACOWNIK4] Ciezarowka pelna! Paczka o wadze %.1f nie wejdzie. Czekam na nowa.\n",p.waga);
                    wspolna->ciezarowka.wymus_odjazd = 1;
		    udalo_sie_zaladowac = 0;
                }
            }
            else {
                 logp("[PRACOWNIK 4] Brak ciezarowki! Czekam...\n");
                 udalo_sie_zaladowac = 0;
            }

            sem_V(semid, SEM_MUTEX_CIEZAROWKA);
            sem_V(semid, SEM_PRACOWNIK4);

            if (udalo_sie_zaladowac == 0) {
		usleep(100000);
            }
            if(udalo_sie_zaladowac == 1 && ilosc_w_buforze == 0)
	    {
		tryb_wysylania = 0;
	    }
	}    
    }
    return 0;
}
