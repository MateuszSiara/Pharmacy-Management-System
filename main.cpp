#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <memory>
#include <ctime>

using namespace std;


using Waluta = double;
using Ilosc = int;


class Lek;


class Lek {
protected:
    string nazwa;
    Waluta cena;
    Ilosc ilosc;
    bool naRecepte;

public:
    Lek(string n, Waluta c, Ilosc i, bool recepta)
        : nazwa(n), cena(c), ilosc(i), naRecepte(recepta) {
    }

    virtual ~Lek() {
    }

    string getNazwa() const { return nazwa; }
    Waluta getCena() const { return cena; }
    Ilosc getIlosc() const { return ilosc; }
    bool czyNaRecepte() const { return naRecepte; }

    void zmienIlosc(Ilosc nowaIlosc) { ilosc = nowaIlosc; }

    virtual void wyswietlInfo() const {
        cout << "Lek: " << nazwa << " | Cena: " << cena << " PLN | Stan: " << ilosc
                << (naRecepte ? " [RX - Na recepte]" : " [Bez recepty]") << endl;
    }

    Lek &operator+=(Ilosc dodatek) {
        this->ilosc += dodatek;
        return *this;
    }

    friend ostream &operator<<(ostream &os, const Lek &l);
};

ostream &operator<<(ostream &os, const Lek &l) {
    os << l.nazwa << " (" << l.cena << " zl)";
    return os;
}


class Antybiotyk : public Lek {
public:
    Antybiotyk(string n, Waluta c, Ilosc i) : Lek(n, c, i, true) {
    }


    void wyswietlInfo() const override {
        cout << "[ANTYBIOTYK] ";
        Lek::wyswietlInfo();
        cout << "   -> UWAGA: Wymagana ścisła kontrola recepty!" << endl;
    }
};

class Suplement : public Lek {
public:
    Suplement(string n, Waluta c, Ilosc i) : Lek(n, c, i, false) {
    }

    void wyswietlInfo() const override {
        cout << "[SUPLEMENT] ";
        Lek::wyswietlInfo();
    }
};


template<typename T>
class Magazyn {
private:
    vector<shared_ptr<T> > asortyment;

public:
    void dodajProdukt(shared_ptr<T> produkt) {
        asortyment.push_back(produkt);
    }

    shared_ptr<T> znajdzProdukt(string nazwa) {
        for (auto &produkt: asortyment) {
            if (produkt->getNazwa() == nazwa) {
                return produkt;
            }
        }
        return nullptr;
    }

    void wyswietlStan() {
        cout << "\n--- STAN MAGAZYNU ---" << endl;
        if (asortyment.empty()) {
            cout << "(Magazyn pusty)" << endl;
            return;
        }
        for (auto &produkt: asortyment) {
            produkt->wyswietlInfo();
        }
        cout << "---------------------" << endl;
    }
};

class Uzytkownik {
protected:
    string login;

public:
    Uzytkownik(string l) : login(l) {
    }

    virtual ~Uzytkownik() {
    }

    virtual void pokazMenu() = 0;

    string getLogin() { return login; }
};

void archiwizujTransakcje(string kto, string co, double kwota) {
    ofstream plik("historia_transakcji.txt", ios::app);
    if (plik.good()) {
        time_t now = time(0);
        char *dt = ctime(&now);
        string czas = dt;
        czas.pop_back();

        plik << "[" << czas << "] Uzytkownik: " << kto
                << " | Sprzedano: " << co << " | Kwota: " << kwota << " PLN" << endl;
    }
    plik.close();
}

// Klasa Farmaceuta
class Farmaceuta : public Uzytkownik {
public:
    Farmaceuta(string l) : Uzytkownik(l) {
    }

    void sprzedajLek(Magazyn<Lek> &magazyn) {
        string nazwa;
        cout << "Podaj nazwe leku do sprzedazy: ";
        cin >> nazwa;

        auto lek = magazyn.znajdzProdukt(nazwa);
        if (lek) {
            if (lek->getIlosc() > 0) {
                if (lek->czyNaRecepte()) {
                    char odp;
                    cout << "Lek WYMAGA recepty. Czy klient okazal wazna recepte? (t/n): ";
                    cin >> odp;
                    if (odp != 't' && odp != 'T') {
                        cout << "ODMOWA SPRZEDAZY: Brak recepty!" << endl;
                        return;
                    }
                }

                lek->zmienIlosc(lek->getIlosc() - 1);
                cout << "Sprzedano: " << *lek << endl;
                archiwizujTransakcje(login, lek->getNazwa(), lek->getCena());
            } else {
                cout << "Brak towaru na stanie" << endl;
            }
        } else {
            cout << "Nie znaleziono leku." << endl;
        }
    }

    void pokazMenu() override {
        cout << "\n[Panel Farmaceuty: " << login << "]" << endl;
        cout << "1. Sprzedaj lek" << endl;
        cout << "2. Sprawdz dostepnosc" << endl;
        cout << "3. Wyloguj" << endl;
    }
};


class Administrator : public Uzytkownik {
public:
    Administrator(string l) : Uzytkownik(l) {
    }

    void zamowTowar(Magazyn<Lek> &magazyn) {
        string nazwa;
        int ilosc;
        cout << "Podaj nazwe leku do zamowienia: ";
        cin >> nazwa;

        auto lek = magazyn.znajdzProdukt(nazwa);
        if (lek) {
            cout << "Podaj ilosc do domowienia: ";
            cin >> ilosc;
            *lek += ilosc;
            cout << "Zaktualizowano stan. Nowa ilosc: " << lek->getIlosc() << endl;
            archiwizujTransakcje(login, "ZAMOWIENIE: " + nazwa, 0);
        } else {
            cout << "Leku nie ma w bazie. (Funkcja dodawania nowego leku - TODO)" << endl;
        }
    }

    void pokazMenu() override {
        cout << "\n[Panel Administratora: " << login << "]" << endl;
        cout << "1. Zamow towar (Dostawa)" << endl;
        cout << "2. Wyswietl caly magazyn" << endl;
        cout << "3. Wyloguj" << endl;
    }
};


class SystemApteki {
private:
    Magazyn<Lek> magazyn;
    vector<shared_ptr<Uzytkownik> > uzytkownicy;

public:
    SystemApteki() {
        magazyn.dodajProdukt(make_shared<Lek>("Apap", 15.50, 10, false));
        magazyn.dodajProdukt(make_shared<Antybiotyk>("Augmentin", 45.00, 5));
        magazyn.dodajProdukt(make_shared<Suplement>("WitaminaC", 9.99, 50));
        magazyn.dodajProdukt(make_shared<Lek>("Ibuprom", 18.20, 0, false)); // Brak stanu

        uzytkownicy.push_back(make_shared<Farmaceuta>("Farmaceuta"));
        uzytkownicy.push_back(make_shared<Administrator>("Administrator"));
    }

    void uruchom() {
        cout << "Witaj w Systemie Aptecznym v1.0" << endl;

        while (true) {
            cout << "\nwybierz uzytkownika:" << endl;
            cout << "1. Farmaceuta" << endl;
            cout << "2. Administrator" << endl;
            cout << "0. Wyjscie" << endl;
            cout << "Wybor: ";

            int wybor;
            cin >> wybor;

            if (wybor == 0) break;

            shared_ptr<Uzytkownik> obecnyUzytkownik = nullptr;

            if (wybor == 1) obecnyUzytkownik = uzytkownicy[0];
            else if (wybor == 2) obecnyUzytkownik = uzytkownicy[1];
            else {
                cout << "Niepoprawny wybor." << endl;
                continue;
            }

            bool zalogowany = true;
            while (zalogowany) {
                obecnyUzytkownik->pokazMenu();
                cout << "Opcja: ";
                int opcja;
                cin >> opcja;

                auto farmaceuta = dynamic_pointer_cast<Farmaceuta>(obecnyUzytkownik);
                auto admin = dynamic_pointer_cast<Administrator>(obecnyUzytkownik);

                if (farmaceuta) {
                    switch (opcja) {
                        case 1: farmaceuta->sprzedajLek(magazyn);
                            break;
                        case 2: magazyn.wyswietlStan();
                            break;
                        case 3: zalogowany = false;
                            break;
                        default: cout << "Nieznana opcja." << endl;
                    }
                } else if (admin) {
                    switch (opcja) {
                        case 1: admin->zamowTowar(magazyn);
                            break;
                        case 2: magazyn.wyswietlStan();
                            break;
                        case 3: zalogowany = false;
                            break;
                        default: cout << "Nieznana opcja." << endl;
                    }
                }
            }
        }
    }
};

int main() {
    SystemApteki apteka;
    apteka.uruchom();
    return 0;
}
