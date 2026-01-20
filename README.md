# Magazyn firmy spedycyjnej
### Autor: Adrian Waligóra
### Przedmiot: Systemy Operacyjne
### Kompilacja
```
make
```
Makefile:

```makefile
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
```

### Uruchomienie
```bash
./m

./dyspozytor
```

## Wprowadzenie
Celem projektu była symulacja działania magazynu firmy spedycyjnej z wykorzystaniem mechanizmów IPC (Inter-Process Communication) w środowisku Linux. Projekt realizuje model niescentralizowany, gdzie poszczególne elementy systemu są niezależnymi procesami. 

## Główne założenia

- ***Architektura:*** System oparty na procesach potomnych tworzonych przez funkcję fork() i uruchamianych przez exec(). 
- ***Komunikacja:*** 
  - **Pamięć dzielona:** Przechowuje stan taśmy (bufor cykliczny), stan ciężarówki oraz flagi sterujące symulacją.
  - **Semafory:** Służą do synchronizacji dostępu do sekcji krytycznych oraz do sygnalizowania stanów. 
    - SEM_MUTEX_TASMA daje dostęp do taśmy, można położyć na niej paczkę lub ściągnąc z niej paczkę 
    - SEM_MUTEX_CIEZAROWKA daje dostęp do modyfikacji wagi i objętości ładunku na ciężarówce 
    - SEM_EMPTY pokazuje, ile wolnych miejsc na taśmie 
    - SEM_FULL pokazuje, ile zajętych miejsc na taśmie 
    - SEM_RAMPA umożliwia wjazd, wyjazd ciężarówki na rampę 
    - SEM_PRACOWNIK4 umożliwia wprowadzenie paczek priorytetowych 
    - SEM_LOG umożliwia synchronizacje logów 
- ***Kolejki komunikatów:*** Umożliwiają asynchroniczną komunikację między Dyspozytorem a procesami (wymuszanie odjazdu, obsługa paczek ekspresowych, zakończenie pracy).

 - ***Konfiguracja:*** Parametry symulacji (pojemność taśmy K, maksymalna liczba przesyłek na taśmie M, ładowność W, objętość V, liczba ciężarówek N, czas trasy ciężarówki Ti ) są zdefiniowane w pliku nagłówkowym dane.h, co ułatwia modyfikację bez ingerencji w logikę.


## Analiza implementacji

### main.c 
- Inicjalizacja IPC
- Konfiguracja początkowa
- Tworzenie procesów
- Sprzątanie
### pracownicy.c
- Generowanie paczek
- Sekcja krytyczna (Taśma):
   1) ```sem_P(SEM_EMPTY)``` – sprawdza, czy jest miejsce w buforze (ilościowo).
   2) ```sem_P(SEM_MUTEX_TASMA)``` – blokuje dostęp do taśmy dla innych.
   3) Warunek wagowy: Sprawdza, czy dodanie paczki nie przekroczy limitu M.

### pracownik4.c
- Buforowanie: P4 generuje paczki do swojego lokalnego bufora (bufor[MAX_BUFOR]).
- Oczekiwanie na sygnał załadunku paczek priorytetowych
- Po otrzymaniu sygnału:
    1) Blokuje dostęp do ciężarówki: ```sem_P(SEM_MUTEX_CIEZAROWKA)```.
    2) Ładuje paczki bezpośrednio ze swojego bufora na pojazd.
    3) Jeśli ciężarówka się przepełni – wymusza jej odjazd ```(wspolna->ciezarowka.wymus_odjazd = 1)```.
- Nie pozwala zakończyć pracy bez załadowania swoich przesyłek.

### ciezarowka.c
- Dostęp do rampy: ```sem_P(SEM_RAMPA)``` zapewnia, że tylko jedna ciężarówka jest ładowana w danym momencie.
- Pętla załadunku:
    1) Sprawdza komunikaty (odjazd na żądanie)
    2) Pobiera paczkę z taśmy
    3) Jeśli paczka się mieści, jest ładowana. Jeśli nie – ciężarówka uznaje się za pełną i odjeżdża.
- Odjazd: Symulowany przez funkcje difftime.
- Cieżarówka kończy pracę po rozwiezieniu wszystkich przesyłek.

### dyspozytor.c

- Komunikacja: Wykorzystuje kolejkę komunikatów (msgget) do wysyłania struktur moj_komunikat.
- Obsługa poleceń:
    1) [1]: Wysyła komunikat mtype=1. Odbiera go ciezarowka.c, co powoduje przerwanie pętli załadunku i natychmiastowy odjazd.
    2) [2]: Wysyła komunikat mtype=2. Odbiera go pracownik4.c, aktywując tryb wysyłania paczek ekspresowych.
    3) [3]: Ustawia flagę koniec_symulacji = 1 w pamięci dzielonej i wysyła puste komunikaty, aby "obudzić" procesy zablokowane na kolejkach.

### dane.c/dane.h

- Struktury
- Funkcje
- Klucze dostępu
- Biblioteki
- Stałe (K, M, W, V, N, TI, MAX_BUFOR)



## Udało się zrealizować wszystkie funkcjonalności wymagane w temacie projektu: 

- **Załadunek standardowy:** Pracownicy P1-P3 dodają paczki o różnych gabarytach na taśmę. Jeśli taśma jest pełna lub paczka przekracza dopuszczalną wagę taśmy, pracownik czeka (semafory SEM_EMPTY, SEM_MUTEX_TASMA).

- **Obsługa ciężarówek:** Ciężarówki podjeżdżają pod rampę. Pobierają paczki z taśmy zgodnie z kolejnością FIFO. Kontrolowane są limity wagi (W) i objętości (V). 

- **Priorytet P4 (Ekspres):** Na sygnał od Dyspozytora, Pracownik 4 przejmuje dostęp do ciężarówki (blokując zwykły załadunek) i ładuje paczki z własnego bufora. 

- **Interwencja Dyspozytora:** Zaimplementowano obsługę poleceń: 
    - Wymuszenie odjazdu niepełnej ciężarówki. 
    - Rozkaz załadunku ekspresowego. 
    - Bezpieczne zakończenie symulacji (zwolnienie zasobów). 

- **Dodanie kolorowania wyjścia terminala.**

## Przykładowe napotkane problemy

### Problem z losowością w procesach potomnych 
Po uruchomieniu symulacji zauważono, że wszyscy pracownicy (P1, P2, P3) generują identyczne sekwencje paczek (ten sam typ i waga) w tym samym czasie. Pomogło przeniesienie inicjalizacji generatora losowego (srand) do wnętrza kodu każdego pracownika po wykonaniu fork(). Dodatkowo, aby uniknąć identycznego ziarna przy uruchomieniu w tej samej sekundzie, użyto kombinacji czasu i ID procesu: srand(time(NULL) ^ getpid()) 

### Problem z synchronizacją zapisu do pliku 
Pomogło tu zastosowanie semafora SEM_LOG oraz bilioteka stdarg.

### Blokowanie się procesu (Pracownik 4) przy odbiorze komunikatów 
Początkowo proces Pracownika 4 zawieszał się (blokował) w oczekiwaniu na rozkaz od Dyspozytora. Działo się tak, ponieważ funkcja msgrcv domyślnie działa w trybie blokującym.  Pomogło zastosowanie flagi IPC_NOWAIT w funkcji odbierającej komunikaty.  

# Przeprowadzone testy 

## Test 1: Priorytetowy załadunek paczek ekspresowych (Sygnał 2)

**Opis:** Weryfikacja działania pracownika P4 oraz mechanizmu pierwszeństwa dla przesyłek ekspresowych.


**Działanie:** W panelu dyspozytora wybranie opcji [2].

**Oczekiwany rezultat:** Pracownik P4 (Ekspres) budzi się (komunikat "Otrzymalem rozkaz (SYGNAL 2)"), blokuje dostęp do ciężarówki dla taśmy i ładuje swoje paczki z bufora bezpośrednio na samochód.

```c
[CIEZAROWKA 1] Zaladowano B (16.1kg). Stan: 716.1/1500.0 kg ---- 3.2/15m3
[PRACOWNIK 3] + Dodal paczke B (14.5kg) o V = 0.0462080m3, Tasma: 30/30 szt, 359.6/420.0 kg

[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
paczka 0 waga = 1.5
paczka 1 waga = 16.5
paczka 2 waga = 8.4
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 2. Ciezarowka: 724.5/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 1. Ciezarowka: 741.0/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 0. Ciezarowka: 742.5/1500
[CIEZAROWKA 1] Zaladowano C (21.0kg). Stan: 763.5/1500.0 kg ---- 3.4/15m3
[PRACOWNIK 2] + Dodal paczke B (12.2kg) o V = 0.0462080m3, Tasma: 30/30 szt, 350.8/420.0 kg 
```

## Test 2: Obsługa przerwania systemowego (SIGINT / Ctrl+C)

**Opis:** Weryfikacja odporności programu na nagłe przerwanie działania przez użytkownika (wymaganie ogólne o usuwaniu zasobów).

**Działanie:** Wciśnięcie kombinacji Ctrl+C w terminalu głównym podczas trwania symulacji.

**Oczekiwany rezultat:** Program przechwytuje sygnał (handle_sigint), wysyła sygnały zakończenia do procesów potomnych, sprząta IPC i kończy działanie bez błędów.
```c
[CIEZAROWKA 1] Zaladowano C (22.5kg). Stan: 88.7/1500.0 kg ---- 0.4/15m3
[PRACOWNIK 3] + Dodal paczke B (11.8kg) o V = 0.0462080m3, Tasma: 10/30 szt, 130.6/420.0 kg
[PRACOWNIK 1] + Dodal paczke C (24.8kg) o V = 0.0997120m3, Tasma: 11/30 szt, 155.4/420.0 kg
^C

[MAIN] Otrzymano sygnal SIGINT (Ctrl+C). Przerywam symulacje...
[MAIN] Procesy potomne zakonczone. Usuwam IPC.
[MAIN] Pamiec dzielona usunieta.
[MAIN] Semafory usuniete.
[MAIN] Kolejka komunikatow usunieta.
```
```bash
$ ipcs | grep "waligora"
$
```
```bash
$ ps
    PID TTY          TIME CMD
 515858 pts/5    00:00:00 bash
 522819 pts/5    00:00:00 ps
 ```

## Test 3: Poprawne zakończenie symulacji (Sygnał 3)

**Opis:** Sprawdzenie procedury bezpiecznego wyłączenia systemu na polecenie.

**Działanie:** W panelu dyspozytora wybranie opcji [3].

**Oczekiwany rezultat:** Dyspozytor wysyła sygnał końca. Pracownik P4 ładuje paczki i kończy pracę, pracownicy 1-3 kończą pracę odrazu, ciężarówki kończą po rozwiezieniu wszystkich paczek. Program main raportuje zakończenie procesów potomnych i usuwa zasoby IPC (pamięć, semafory).

```c
[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
paczka 0 waga = 21.2
paczka 1 waga = 15.1
paczka 2 waga = 21.9
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 2. Ciezarowka: 361.9/500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 1. Ciezarowka: 377.0/500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 0. Ciezarowka: 398.2/500
[PRACOWNIK 4] Koniec pracy.
[PRACOWNIK 2] koniec pracy
[CIEZAROWKA 1] Zaladowano C (23.5kg). Stan: 421.7/500.0 kg ---- 1.9/15m3
[PRACOWNIK 3] + Dodal paczke A (5.2kg) o V = 0.0194560m3, Tasma: 10/10 szt, 71.8/420.0 kg
[PRACOWNIK 3] koniec pracy
[PRACOWNIK 1] koniec pracy
[CIEZAROWKA 1] Zaladowano C (22.4kg). Stan: 444.1/500.0 kg ---- 2.0/15m3
[CIEZAROWKA 1] Zaladowano B (15.5kg). Stan: 459.6/500.0 kg ---- 2.1/15m3
[CIEZAROWKA 1] Zaladowano A (5.7kg). Stan: 465.3/500.0 kg ---- 2.1/15m3
[CIEZAROWKA 1] Zaladowano A (0.3kg). Stan: 465.6/500.0 kg ---- 2.1/15m3
[CIEZAROWKA 1] Zaladowano A (3.7kg). Stan: 469.3/500.0 kg ---- 2.1/15m3
[CIEZAROWKA 1] Zaladowano B (9.9kg). Stan: 479.2/500.0 kg ---- 2.2/15m3
[CIEZAROWKA 1] Zaladowano A (0.6kg). Stan: 479.8/500.0 kg ---- 2.2/15m3
[CIEZAROWKA 1] Zaladowano A (5.2kg). Stan: 485.0/500.0 kg ---- 2.2/15m3
[CIEZAROWKA 1] Zaladowano A (3.3kg). Stan: 488.3/500.0 kg ---- 2.2/15m3
[CIEZAROWKA 1] Zaladowano A (5.2kg). Stan: 493.5/500.0 kg ---- 2.2/15m3
[CIEZAROWKA 1] Koniec symulacji i pusta tasma. Koncze ladunek i odjezdzam w ostatnia trase.
[CIEZAROWKA 1] Odjezdzam w trase (3s)...
[CIEZAROWKA 2] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 3] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 4] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 5] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 6] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 7] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 8] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 9] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 10] Wjechalem na rampe ale brak paczek. Koncze prace
[CIEZAROWKA 1] Koniec mojej pracy.
[CIEZAROWKA 1] Koniec symulacji i brak paczek. Koncze prace

[MAIN] Wszyscy pracownicy oraz ciezarowki zakonczyly prace. Sprzatam system.
```
```bash
$ ipcs | grep "waligora"
$
```
```bash
$ ps
    PID TTY          TIME CMD
 515858 pts/5    00:00:00 bash
 522819 pts/5    00:00:00 ps
 ```

## Test 4: Rotacja ciężarówek (Cykliczność pracy)

**Opis:** Weryfikacja, czy po odjeździe jednej ciężarówki, na jej miejsce podstawiana jest inna z dostępnej puli N pojazdów.**

**Działanie:** Obserwacja dłuższego fragmentu symulacji obejmującego kilka odjazdów.

**Oczekiwany rezultat:** Po komunikacie "Odjezdzam w trase" jednej ciężarówki, wkrótce pojawia się komunikat innej ciężarówki (o ile jest dostępna) "Podjechalem pod rampe". Ciężarówki wracają po czasie TI.

```c
[CIEZAROWKA 1] Zaladowano B (10.8kg). Stan: 279.2/300.0 kg ---- 1.2/15m3
[PRACOWNIK 2] + Dodal paczke A (5.9kg) o V = 0.0194560m3, Tasma: 30/30 szt, 410.3/420.0 kg
[CIEZAROWKA 1] pelna! Paczka 21.5kg nie wejdzie. Odjazd.
[CIEZAROWKA 1] Odjezdzam w trase (3s)...

[CIEZAROWKA 2] --- Podjechalem pod rampe
[CIEZAROWKA 2] Zaladowano C (21.5kg). Stan: 21.5/300.0 kg ---- 0.1/15m3
[PRACOWNIK 3] + Dodal paczke A (0.1kg) o V = 0.0194560m3, Tasma: 30/30 szt, 388.9/420.0 kg
[CIEZAROWKA 2] Zaladowano B (8.9kg). Stan: 30.4/300.0 kg ---- 0.1/15m3
[PRACOWNIK 1] + Dodal paczke A (0.3kg) o V = 0.0194560m3, Tasma: 30/30 szt, 380.3/420.0 kg
[CIEZAROWKA 2] Zaladowano A (5.2kg). Stan: 35.6/300.0 kg ---- 0.2/15m3
[PRACOWNIK 2] + Dodal paczke A (0.7kg) o V = 0.0194560m3, Tasma: 30/30 szt, 375.8/420.0 kg
[CIEZAROWKA 2] Zaladowano B (12.8kg). Stan: 48.4/300.0 kg ---- 0.2/15m3
[PRACOWNIK 3] + Dodal paczke C (18.7kg) o V = 0.0997120m3, Tasma: 30/30 szt, 381.7/420.0 kg
[CIEZAROWKA 2] Zaladowano C (15.5kg). Stan: 63.9/300.0 kg ---- 0.3/15m3
[PRACOWNIK 2] + Dodal paczke A (8.0kg) o V = 0.0194560m3, Tasma: 30/30 szt, 374.2/420.0 kg
[CIEZAROWKA 2] Zaladowano C (16.4kg). Stan: 80.3/300.0 kg ---- 0.4/15m3
[PRACOWNIK 1] + Dodal paczke B (14.5kg) o V = 0.0462080m3, Tasma: 30/30 szt, 372.3/420.0 kg
[CIEZAROWKA 2] Zaladowano B (14.1kg). Stan: 94.4/300.0 kg ---- 0.5/15m3
[PRACOWNIK 3] + Dodal paczke C (16.2kg) o V = 0.0997120m3, Tasma: 30/30 szt, 374.4/420.0 kg
[CIEZAROWKA 2] Zaladowano B (12.4kg). Stan: 106.8/300.0 kg ---- 0.5/15m3
[PRACOWNIK 2] + Dodal paczke C (21.5kg) o V = 0.0997120m3, Tasma: 30/30 szt, 383.5/420.0 kg
[CIEZAROWKA 1] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 2] Zaladowano B (11.2kg). Stan: 118.0/300.0 kg ---- 0.5/15m3
```

## Test 5: Ochrona przed przeciążeniem taśmy (Masa M)

**Opis:** Sprawdzenie, czy pracownicy (P1-P3) przestrzegają limitu udźwigu taśmy transportowej (M).

**Działanie:** Obserwacja logów w momencie dużego obciążenia taśmy.

**Oczekiwany rezultat:** Pojawienie się komunikatu pracownika: "Paczka X kg za ciezka (Tasma: Y kg). Czekam z paczka...". Pracownik powinien wstrzymać się z dołożeniem paczki do momentu, aż ciężarówka zdejmie inne paczki z taśmy.
```c
[CIEZAROWKA 1] Zaladowano B (10.0kg). Stan: 142.9/1500.0 kg ---- 0.6/15m3
[PRACOWNIK 1] + Dodal paczke C (19.0kg) o V = 0.0997120m3, Tasma: 22/30 szt, 319.3/350.0 kg
[PRACOWNIK 3] + Dodal paczke B (17.2kg) o V = 0.0462080m3, Tasma: 23/30 szt, 336.5/350.0 kg
[PRACOWNIK 2] !cd Paczka 16.2kg za ciezka (Tasma: 336.5kg). Czekam z paczka...
[PRACOWNIK 3] !cd Paczka 14.7kg za ciezka (Tasma: 336.5kg). Czekam z paczka...
[PRACOWNIK 1] !cd Paczka 22.7kg za ciezka (Tasma: 336.5kg). Czekam z paczka...
[CIEZAROWKA 1] Zaladowano B (14.7kg). Stan: 157.6/1500.0 kg ---- 0.7/15m3
[CIEZAROWKA 1] Zaladowano A (7.5kg). Stan: 165.1/1500.0 kg ---- 0.7/15m3
[CIEZAROWKA 1] Zaladowano B (9.9kg). Stan: 175.0/1500.0 kg ---- 0.8/15m3
[CIEZAROWKA 1] Zaladowano C (22.1kg). Stan: 197.1/1500.0 kg ---- 0.9/15m3
[CIEZAROWKA 1] Zaladowano C (17.0kg). Stan: 214.1/1500.0 kg ---- 1.0/15m3
[PRACOWNIK P4] + Dodal paczke A (2.7kg) o V = 0.0194560m3
[CIEZAROWKA 1] Zaladowano C (15.6kg). Stan: 229.7/1500.0 kg ---- 1.1/15m3
[CIEZAROWKA 1] Zaladowano C (24.9kg). Stan: 254.6/1500.0 kg ---- 1.2/15m3
[CIEZAROWKA 1] Zaladowano B (16.1kg). Stan: 270.7/1500.0 kg ---- 1.2/15m3
[PRACOWNIK 2] + Dodal paczke B (16.2kg) o V = 0.0462080m3, Tasma: 16/30 szt, 224.9/350.0 kg
[CIEZAROWKA 1] Zaladowano C (19.0kg). Stan: 289.7/1500.0 kg ---- 1.3/15m3
```

## Test 6: Małe ciężarówki vs wielkie paczki

**Opis:** Sprawdzenie, czy ciężarówki przestrzegają limitu  swojego udźwigu (W).

**Działanie:** Obserwacja logów w momencie ustalenia ładowności ciężarowki na 1 kg.

**Oczekiwany rezultat:** Pojawienie się komunikatu ciężarówki: "pelna! Paczka X kg nie wejdzie. Odjazd.". Ciężarówka powinna odjechać w trasę, wrócić po czasie TI i od nowa wyświetlić taki komunikat.
```c
[PRACOWNIK 3] + Dodal paczke C (20.7kg) o V = 0.0997120m3, Tasma: 25/30 szt, 341.9/350.0 kg
[PRACOWNIK 1] !cd Paczka 20.9kg za ciezka (Tasma: 341.9kg). Czekam z paczka...
[CIEZAROWKA 1] pelna! Paczka 12.7kg nie wejdzie. Odjazd.
[CIEZAROWKA 1] Odjezdzam w trase (3s)...
[PRACOWNIK 2] !cd Paczka 16.2kg za ciezka (Tasma: 341.9kg). Czekam z paczka...
[PRACOWNIK 3] !cd Paczka 21.3kg za ciezka (Tasma: 341.9kg). Czekam z paczka...
[CIEZAROWKA 4] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 2] Wrocilem z trasy. Ustawiam sie w kolejce.

[CIEZAROWKA 4] --- Podjechalem pod rampe
[CIEZAROWKA 3] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 4] pelna! Paczka 12.7kg nie wejdzie. Odjazd.
[CIEZAROWKA 4] Odjezdzam w trase (3s)...

[CIEZAROWKA 2] --- Podjechalem pod rampe
[CIEZAROWKA 2] pelna! Paczka 12.7kg nie wejdzie. Odjazd.
[CIEZAROWKA 2] Odjezdzam w trase (3s)...

[CIEZAROWKA 3] --- Podjechalem pod rampe
[CIEZAROWKA 3] pelna! Paczka 12.7kg nie wejdzie. Odjazd.
[CIEZAROWKA 3] Odjezdzam w trase (3s)...
[CIEZAROWKA 1] Wrocilem z trasy. Ustawiam sie w kolejce.

[CIEZAROWKA 1] --- Podjechalem pod rampe
[PRACOWNIK 1] !cd Paczka 20.9kg za ciezka (Tasma: 341.9kg). Czekam z paczka...
[CIEZAROWKA 1] pelna! Paczka 12.7kg nie wejdzie. Odjazd.
[CIEZAROWKA 1] Odjezdzam w trase (3s)...
```
![alt text](<Zrzut ekranu 2026-01-13 230232.png>)

## Test 7: Spamowanie Dyspozytora
**Opis:** Test wytrzymałościowy sprawdzający stabilność kolejki komunikatów i obsługi sygnałów. Weryfikuje, czy system nie ulega awarii przy gwałtownym napływie sprzecznych lub nakładających się poleceń.

**Działanie:** W panelu dyspozytora bardzo szybko, naprzemiennie wybierano opcje `[1]` (odjazd) i `[2]` (załadunek ekspresowy).

**Oczekiwany rezultat:** System zachowuje stabilność. Ciężarówka i pracownik P4 obsługują polecenia sekwencyjnie lub ignorują nadmiarowe komunikaty, nie doprowadzając do zakleszczenia ani przerwania działania programu.

```c
[CIEZAROWKA 3] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 1] --- Podjechalem pod rampe
DYSPOZYTOR KAZE ODJECHAC (Sygnal 1)!
[CIEZAROWKA 1] Odjezdzam w trase (3s)...
[CIEZAROWKA 3] --- Podjechalem pod rampe
DYSPOZYTOR KAZE ODJECHAC (Sygnal 1)!
[CIEZAROWKA 3] Odjezdzam w trase (3s)...
[CIEZAROWKA 2] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 2] --- Podjechalem pod rampe
DYSPOZYTOR KAZE ODJECHAC (Sygnal 1)!
[CIEZAROWKA 2] Odjezdzam w trase (3s)...
[CIEZAROWKA 4] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 4] --- Podjechalem pod rampe
DYSPOZYTOR KAZE ODJECHAC (Sygnal 1)!
[CIEZAROWKA 4] Odjezdzam w trase (3s)...
[CIEZAROWKA 2] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 2] --- Podjechalem pod rampe
[CIEZAROWKA 1] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 3] Wrocilem z trasy. Ustawiam sie w kolejce.
[CIEZAROWKA 2] Zaladowano C (20.6kg). Stan: 20.6/1500.0 kg ---- 0.1/15m3
[PRACOWNIK 3] + Dodal paczke C (23.7kg) o V = 0.0997120m3, Tasma: 30/30 szt, 347.5/420.0 kg
[CIEZAROWKA 2] Zaladowano C (16.8kg). Stan: 37.4/1500.0 kg ---- 0.2/15m3
[PRACOWNIK 1] + Dodal paczke A (6.4kg) o V = 0.0194560m3, Tasma: 30/30 szt, 337.1/420.0 kg
```
```c
[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
paczka 0 waga = 6.6
paczka 1 waga = 22.2
paczka 2 waga = 20.4
paczka 3 waga = 15.8
paczka 4 waga = 24.2
paczka 5 waga = 19.5
paczka 6 waga = 15.6
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 6. Ciezarowka: 1246.4/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 5. Ciezarowka: 1265.9/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 4. Ciezarowka: 1290.1/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 3. Ciezarowka: 1305.9/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 2. Ciezarowka: 1326.3/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 1. Ciezarowka: 1348.5/1500
[PRACOWNIK 4] -> Zaladowano EKSPRES! Zostalo: 0. Ciezarowka: 1355.1/1500
[CIEZAROWKA 2] Zaladowano B (13.5kg). Stan: 1368.6/1500.0 kg ---- 6.3/15m3
[PRACOWNIK 1] + Dodal paczke A (6.5kg) o V = 0.0194560m3, Tasma: 30/30 szt, 343.7/420.0 kg
[CIEZAROWKA 2] Zaladowano B (11.7kg). Stan: 1380.3/1500.0 kg ---- 6.3/15m3
[PRACOWNIK 2] + Dodal paczke B (9.6kg) o V = 0.0462080m3, Tasma: 30/30 szt, 341.6/420.0 kg
[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
[CIEZAROWKA 2] Zaladowano A (2.0kg). Stan: 1382.3/1500.0 kg ---- 6.3/15m3
[PRACOWNIK 3] + Dodal paczke A (5.2kg) o V = 0.0194560m3, Tasma: 30/30 szt, 344.8/420.0 kg
[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
[CIEZAROWKA 2] Zaladowano B (8.3kg). Stan: 1390.6/1500.0 kg ---- 6.4/15m3
[PRACOWNIK 1] + Dodal paczke A (2.6kg) o V = 0.0194560m3, Tasma: 30/30 szt, 339.1/420.0 kg
[CIEZAROWKA 2] Zaladowano A (3.7kg). Stan: 1394.3/1500.0 kg ---- 6.4/15m3
[PRACOWNIK 2] + Dodal paczke C (23.9kg) o V = 0.0997120m3, Tasma: 30/30 szt, 359.3/420.0 kg
[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
[CIEZAROWKA 2] Zaladowano A (1.0kg). Stan: 1395.3/1500.0 kg ---- 6.4/15m3
[PRACOWNIK 3] + Dodal paczke C (24.0kg) o V = 0.0997120m3, Tasma: 30/30 szt, 382.3/420.0 kg
[DYSPOZYTOR] >>> [PRACOWNIK 4] Otrzymalem rozkaz (SYGNAL 2), laduje paczki ekspresowe
[CIEZAROWKA 2] Zaladowano B (16.4kg). Stan: 1411.7/1500.0 kg ---- 6.5/15m3
[PRACOWNIK 1] + Dodal paczke B (9.2kg) o V = 0.0462080m3, Tasma: 30/30 szt, 375.1/420.0 kg
```

**Paczki nie ładowały się na ciężarówkę, ponieważ są tworzone co 5 sekund.**

## Test 8: Automatyczny odjazd ciężarówki po zapełnieniu

**Opis:** Weryfikacja, czy ciężarówka odjeżdża samodzielnie po osiągnięciu limitu wagi (W) lub objętości (V) bez ingerencji dyspozytora.

**Działanie:** Uruchomienie symulacji i oczekiwanie, aż pracownicy załadują wystarczającą liczbę paczek.

**Oczekiwany rezultat:** W logach pojawia się komunikat o braku miejsca na paczkę, zwolnienie semafora rampy i komunikat o odjeździe w trasę. Następnie podjeżdża kolejna ciężarówka.

```c
[PRACOWNIK 3] + Dodal paczke C (15.7kg) o V = 0.0997120m3, Tasma: 30/30 szt, 302.1/420.0 kg
[CIEZAROWKA 1] Zaladowano C (21.5kg). Stan: 1489.0/1500.0 kg ---- 6.7/15m3
[PRACOWNIK 2] + Dodal paczke C (21.5kg) o V = 0.0997120m3, Tasma: 30/30 szt, 302.1/420.0 kg
[CIEZAROWKA 1] pelna! Paczka 15.2kg nie wejdzie. Odjazd.
[CIEZAROWKA 1] Odjezdzam w trase (3s)...

[CIEZAROWKA 2] --- Podjechalem pod rampe
[PRACOWNIK P4] + Dodal paczke C (16.6kg) o V = 0.0997120m3
[CIEZAROWKA 2] Zaladowano B (15.2kg). Stan: 15.2/1500.0 kg ---- 0.0/15m3
[PRACOWNIK 1] + Dodal paczke B (16.8kg) o V = 0.0462080m3, Tasma: 30/30 szt, 303.7/420.0 kg
[CIEZAROWKA 2] Zaladowano A (1.6kg). Stan: 16.8/1500.0 kg ---- 0.1/15m3
```

## Linki do istotnych fragmentow kodu


### a. Tworzenie i obsluga plikow
- **fopen()**: [dane.c:78](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L78)  
- **vfprintf()**: [dane.c:82](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L82) 
- **fclose()**: [dane.c:85](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L85)  


### b. Tworzenie procesow
- **fork()**: [main.c:115](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L115)
- **execlp()**:[main.c:121](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L121)
- **wait()**: [main.c:183](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L183)


### c. Obsluga sygnalow
- **kill()**: [dane.c:102](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L102)
- **signal()**: [main.c:33](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L33)


### d. Semafory
- **ustaw_semafor()**: [dane.c:128](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L128)
- **semget()**: [main.c:41](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L41)
- **semctl()**: [mian.c:188](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L188)
- **semop()**: [dane.c:38](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L38)
- **sem_P()**: [dane.c:27](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L27)
- **sem_V()**: [dane.c:46](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dane.c#L46)


### e. Segmenty pamieci dzielonej
- **shmget()**: [main.c:58](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L58)
- **shmat()**: [main.c:84](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L84)
- **shmdt()**: [main.c:109](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L109)
- **shmctl()**: [main.c:187](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L187)

### f. Kolejki komunikatow
- **msgget()**: [main.c:71](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L71)
- **msgsnd()**: [dyspozytor.c:44](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/dyspozytor.c#L44)
- **msgrcv()**: [ciezarowka.c:70](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/ciezarowka.c#L70)
- **msgctl()**: [main.c:189](https://github.com/Adrhian241/Magazyn-firmy-spedycyjnej/blob/46ed385f5ca70142412d2d47e2b3615cd51ec4aa/main.c#L189)
