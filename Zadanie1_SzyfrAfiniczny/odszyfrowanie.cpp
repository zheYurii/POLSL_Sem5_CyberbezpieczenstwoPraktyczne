#include <cstdio>
#include <iostream>
#include <fstream>

using namespace std;
int main(int argc, char* argv[]) {
    //otwieramy zaszyfrowany plik do odczytu
    if (argc != 2) { //sprawdzamy czy podano sciezke do pliku
        cerr << "Nie podano sciezki do pliku.\n";
        cerr << "Prawidlowe wywolanie: " << argv[0] << " <sciezka_do_pliku>\n";
        return 1;
    }

    ifstream plik(argv[1], std::ios::in | std::ios::binary); //otwieramy plik
    if (!plik) {
        cerr << "Blad przy otwieraniu zaszyfrowanego pliku.\n";
    }

    //sprawdzamy czy plik zawiera poprawny naglowek
    //w formacie: 
    /*
    A F F 1 (4 byte)
    ...ciąg zaszyfrowanego tekstu...
    */
    plik.seekg(0, ios::beg);
    char read_byte[4];
    plik.read(read_byte, sizeof(read_byte)); //wczytujemy 4 pierwsze bajty pliku
    char oczekiwany_ciag[] = {'A', 'F', 'F', '1'}; //"AFF1" by dodalo null na koniec czego chcemy uniknac.
    if (memcmp(read_byte, oczekiwany_ciag, 4) == 0) { //sprawdzamy czy wynosza one AFF1
        cout << "Poprawny naglowek pliku!\n";       
    }
    else {
        cerr << "Podano niepoprawny plik.\n";
        return 1;
    }

    //Wiemy ze plik ma dobry naglowek. Przechodzimy do odczytywania danych z konca pliku.
    plik.seekg(0, ios::end); //przesuwamy wskaznik na sam koniec pliku
    long int rozmiarPliku = plik.tellg();

    //Idziemy do tyłu az znajdziemy znacznik EOF\0
    bool znalezionoEOF = false;
    int obecnyOffset = -4;
    long int pozycjaEOF = 0;
    while (!znalezionoEOF) {
        plik.seekg(obecnyOffset, ios::end); //przesuwamy wskaznik na obecnyOffset
        char byte;
        plik.read(&byte, sizeof(byte)); //wczytujemy 1 bajt
        if (byte == 'E'){
            plik.read(&byte, sizeof(byte)); //jezeli to E to czytamy nastepny
            if(byte == 'O'){
                plik.read(&byte, sizeof(byte)); //jezeli to O to czytamy nastepny
                if(byte == 'F'){
                    plik.read(&byte, sizeof(byte)); //jezeli to F to czytamy nastepny
                    if(byte == '\0'){
                        cout << "Znaleziono znacznik EOF\n"; //jezeli to NULL to wiemy, ze znalezlismy znacznik.
                        pozycjaEOF = rozmiarPliku + obecnyOffset;
                        znalezionoEOF = true;
                    }
                    obecnyOffset--;
                }
                obecnyOffset--;
            }
            obecnyOffset--;
        } else {
            obecnyOffset--;
        }
        if (obecnyOffset <= -15 && znalezionoEOF == false) {
            cout << "Niepoprawny format pliku.\n";
            return 1;
        }
    }

    //znaleziony znacznik EOF\0, odczytujemy pozycje wartosci A i B.
    //Format poprawnego pliku:
    /*
    ...ciąg zaszyfrowanego tekstu...
    E O F NULL (4 byte)
    Pozycja A (4 byte)
    W B (2 byte)
    Pozycja B (4 byte)
    */

    int pozycjaZnacznika = plik.tellg();
    printf("Obecna pozycja znacznika odczytu pliku: %d\n", pozycjaZnacznika); //wiemy ze po EOF\0 mamy 4 bajty wartosci A.
    int odczytana_pozycja_A = 0;
    plik.read(reinterpret_cast<char*>(&odczytana_pozycja_A), sizeof(odczytana_pozycja_A)); //wczytujemy 4 bajty wartosci A
    printf("Oczytana pozycja parametru A: %d\n", odczytana_pozycja_A);
    char ciagWB[2]; 
    plik.read(ciagWB, sizeof(ciagWB)); //wczytujemy znacznik WB
    char oczekiwany_ciag_wb[] = {'W', 'B'}; //strcmp(ciagWB, "WB") by dodalo null na koniec czego chcemy uniknac.
    if (memcmp(ciagWB, oczekiwany_ciag_wb, 2) == 0) { //sprawdzamy czy to WB
        cout << "Poprawny ciag WB\n";
    } else {
        cout << "Uszkodzony format pliku.\n";
        return 1;
    }
    int odczytana_pozycja_B = 0;
    plik.read(reinterpret_cast<char*>(&odczytana_pozycja_B), sizeof(odczytana_pozycja_B)); //po znaczniku WB sa 4 bajty pozycji B
    printf("Oczytana pozycja parametru B: %d\n", odczytana_pozycja_B);
    odczytana_pozycja_A = odczytana_pozycja_A + 4; //dodajemy 4 zeby pominac naglowek (ktory przy szyfrowaniu dodalismy po obliczeniu pozycji)
    odczytana_pozycja_B = odczytana_pozycja_B + 4; //analogicznie

    //Odczytywanie wartości parametrów A i B na podstawie odczytanych pozycji
    plik.seekg(odczytana_pozycja_A, ios::beg); //przechodzimy na pozycje A
    char wspA = '\0';
    plik.read(&wspA, sizeof(wspA)); //wczytujemy bajt A
    plik.seekg(odczytana_pozycja_B, ios::beg); //przechodzimy na pozycje B
    char wspB = '\0'; 
    plik.read(&wspB, sizeof(wspB)); //wczytujemy bajt B

    //zmieniamy na int
    int a = static_cast<unsigned char>(wspA);
    int b = static_cast<unsigned char>(wspB);

    printf("Odczytane z pliku wartosci:\n\ta = %d\n\tb = %d\n", a, b);

    //potrzebujemy odwrotnosci A do deszyfrowania
    int a_odwrotne = 0;
    for (int i = 1; i < 256; i++) {
        if ((a * i) % 256 == 1) {
            a_odwrotne = i;
            break;
        }
    }
    //deszyfrowanie pliku
    plik.seekg(4, ios::beg); //przesuwamy wskaznik na pozycje po naglowku.
    char znak;
    string bufor;
    int n = 256;
    while (plik.get(znak) && plik.tellg() <= pozycjaEOF) { //dopoki nie dojdziemy do EOF\0 pobieramy jeden znak
        if (plik.tellg() == odczytana_pozycja_A + 1  || plik.tellg() == odczytana_pozycja_B + 1) { //pomijamy pozycje A i B
            continue; 
        }
        char odszyfrowany_znak = (a_odwrotne * (znak - b + n))  % n; //odwrotna funkcja do funkcji szyfrujacej
        bufor = bufor + odszyfrowany_znak;
    }   
    plik.close(); 

    string nazwa_pliku = string(argv[1]);
    string pierwotna_nazwa_pliku = nazwa_pliku.substr(0, nazwa_pliku.find_last_of('.'));
    string nazwa_pliku_wyjsciowego = pierwotna_nazwa_pliku.substr(0, pierwotna_nazwa_pliku.find_last_of('.')) + "_odszyfrowany" + pierwotna_nazwa_pliku.substr(pierwotna_nazwa_pliku.find_last_of('.'));
    fstream plik_wyjsciowy(nazwa_pliku_wyjsciowego, ios::out | ios::binary);

    if (!plik_wyjsciowy) {
        cerr << "Nie udalo sie otworzyc pliku do zapisu." << endl;
        return 1;
    }

    plik_wyjsciowy << bufor;

    if (plik_wyjsciowy.good()) {
        cout << "Zaszyfrowany plik zapisany poprawnie.\n";
    } else {
        cerr << "Blad z zapisem pliku.\n";
    }
}