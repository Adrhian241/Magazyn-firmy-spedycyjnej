#include "dane.h"


int main(int argc, char *argv[])
{
    int msgid = msgget(KEY_MSG, 0600);
    if (msgid == -1) { 
        perror("[DYSPOZYTOR] Blad msgget (dyspozytor)"); 
	exit(EXIT_FAILURE);
    }
    int shmid = shmget(KEY_SHM, sizeof(MagazynShared), 0600);
    if (shmid == -1) {
        perror("[DYSPOZYTOR] Nie widze pamieci dzielonej. Czy uruchomiles ./main?");
        exit(EXIT_FAILURE);
    }
    MagazynShared *wspolna = (MagazynShared*)shmat(shmid, NULL, 0);

    struct moj_komunikat msg;
    int wybor = 0;
    printf("     PANEL STEROWANIA DYSPOZYTORA        \n");
    while (1) 
    {
        printf("\nDOSTEPNE ROZKAZY:\n");
        printf(" [1] Nakaz odjazdu ciezarowki (niepelnej)\n");
        printf(" [2] Zaladunek paczek ekspresowych (P4)\n");
        printf(" [3] KONIEC PRACY MAGAZYNU\n");
        printf("Twoj wybor > ");

        // Zabezpieczenie przed wpisaniem liter
        if (scanf("%d", &wybor) != 1) 
        {
            while(getchar() != '\n'); 
            continue;
        }

        if(wybor == 1)
        {
            msg.mtype = 1;
	    strcpy(msg.text, "0");
	    if (msgsnd(msgid, &msg, sizeof(int), 0) == -1) 
	    {
    		perror("[DYSPOZYTOR] Blad wyslania komunikatu (1)\n");
	    }
	    else logp("[DYSPOZYTOR] Wyslano rozklaz odjazdu niepelnej ciezarowki\n");
        }
        else if(wybor == 2)
        {
            msg.mtype = 2;
            strcpy(msg.text, "0");
	    if (msgsnd(msgid, &msg, sizeof(int), 0) == -1)
            {
                perror("[DYSPOZYTOR] Blad wyslania komunikatu (2)\n");
            }
            else logp("[DYSPOZYTOR] Wyslano rozklaz zaladunku paczek priorytetowych\n");
        }
        else if (wybor == 3)
        {
            logp("[DYSPOZYTOR] Koncze symulacje...\n");
            wspolna->koniec_symulacji = 1;

            msg.mtype = 1; msgsnd(msgid, &msg, sizeof(int), 0);
            msg.mtype = 2; msgsnd(msgid, &msg, sizeof(int), 0);
            
            break;
        }
        else 
        {
            printf("Nieznana opcja. Wybierz 1, 2 lub 3.\n");
        }
    }
    return 0;
}
