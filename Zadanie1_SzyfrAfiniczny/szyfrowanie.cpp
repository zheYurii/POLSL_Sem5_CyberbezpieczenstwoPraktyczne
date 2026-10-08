#include <cstdio>
#include <iostream>
#include <fstream>

using namespace std;
int main(int argc, char* argv[]) { // ./program <sciezka_do_pliku>

    if (argc != 2) { //sprawdzamy czy podano sciezke do pliku
        cerr << "Nie podano sciezki do pliku.\n";
        cerr << "Prawidlowe wywolanie: " << argv[0] << " <sciezka_do_pliku>\n";
        return 1;
    }

    //zmienne potrzebne do szyfrowania
    long int t = static_cast<long int>(time(NULL)); //czas systemowy wedlug unix
    int n = 256; 
    //wyliczamy a i b na podstawie czasu systemowego
    int a = t % n;
    if (a % 2 == 0) a += 1; //upewniamy sie, ze liczba nie jest parzysta (co uniemozliwiloby odszyfrowanie)
    int b = (t / n) % n;

    printf("Obliczone wspolczynniki:\n\tt = %ld\n\ta = %d\n\tb = %d\n", t, a, b);

    //wczytujemy plik
    fstream plik(argv[1], ios::in | ios::binary);
    if (!plik) {
        cerr << "Blad przy otwieraniu pliku do zaszyfrowania.";
        return 1;
    }

    /*Szyfrowanie*/
    //bierzemy bajt po bajcie i szyfrujemy go
    char znak;
    string bufor;
    while (plik.get(znak)) {
        char zaszyfrowany_bajt = (a*znak + b + 256) % n; //wzor na szyfr afiniczny (dodajac +256 upewniamy sie, ze liczba nie jest ujemna)
        bufor = bufor + zaszyfrowany_bajt;
    }   
    plik.close(); 

    //obliczamy gdzie umiescic a i b
    int rozmiar = bufor.length();
    int pozycja_A = t % (rozmiar / 2 + 1); //upewniamy sie, ze pozycja wsp. A bedzie w pierwszej polowie pliku
    int pozycja_B = pozycja_A + (rozmiar / 4); //pozycja wsp. B po A (ale nie dalej niz koniec pliku)

    //dzielimy zaszyfrowany tekst na: START+A+MIDDLE+B+END
    string lewa_czesc = bufor.substr(0, pozycja_A);
    string srodek = bufor.substr(pozycja_A, pozycja_B - pozycja_A);
    string prawa_czesc = bufor.substr(pozycja_B);

    //zwiększamy pozycję B o 1, aby wskazywała na miejsce po wsp. B w pliku
    pozycja_B++;
    printf("Obliczona pozycja wspolczynnikow:\n\tpoz_A = %d\n\tpoz_B = %d\n", pozycja_A, pozycja_B);
    
    //zapisujemy po kolei naglowek, lewa czesc, pozycja a, srodek, pozycja b, prawa czesc
    //dodatkowo na stale wrzucimy na sam koniec dane o pozycji wspolczynnikow
    //w formacie: 
    /*
    A F F 1 (4 byte)
    ...ciąg zaszyfrowanego tekstu...
    E O F NULL (4 byte)
    Pozycja A (4 byte)
    W B (2 byte)
    Pozycja B (4 byte)
    */

    string nazwa_pliku_wyjsciowego = string(argv[1]) + ".aff1";
    fstream plik_wyjsciowy(nazwa_pliku_wyjsciowego, ios::out | ios::binary);

    if (!plik_wyjsciowy) {
        cerr << "Nie udalo sie otworzyc pliku do zapisu." << endl;
        return 1;
    }

    //zapisujemy naglowek pliku
    char ciag_AFF1[] = {'A', 'F', 'F', '1'};
    plik_wyjsciowy.write(ciag_AFF1, sizeof(ciag_AFF1));

    plik_wyjsciowy << lewa_czesc;
    plik_wyjsciowy << (char)a;
    plik_wyjsciowy << srodek;
    plik_wyjsciowy << (char)b;
    plik_wyjsciowy << prawa_czesc;
    //Zaszyfrowana czesc z wspolczynnikami zapisana, zapisujemy nasz koniec pliku

    char ciag_EOF[] =  {'E', 'O', 'F', '\0'};
    plik_wyjsciowy.write(ciag_EOF, sizeof(ciag_EOF));
    plik_wyjsciowy.write(reinterpret_cast<const char*>(&pozycja_A), sizeof(pozycja_A)); //zapisujemy surowe 4 bajty wspolczynnika A do pliku.
    char ciag_WB[] = {'W', 'B'};
    plik_wyjsciowy.write(ciag_WB, sizeof(ciag_WB));
    plik_wyjsciowy.write(reinterpret_cast<const char*>(&pozycja_B), sizeof(pozycja_B)); //zapisujemy surowe 4 bajty wspolczynnika B do pliku.

    if (plik_wyjsciowy.good()) {
        cout << "Zaszyfrowany plik zapisany poprawnie.\n";
    } else {
        cerr << "Blad z zapisem pliku.\n";
    }
}