# Intelligent-traffic-lights

## Jak Uruchomić Projekt na Ubuntu
1. **Kompilacja**: Upewnij się, że masz zainstalowany kompilator C (np. gcc), bibliotekę glib oraz pozostałe wymagane narzędzia.
   sudo apt update
   sudo apt install build-essential
   sudo apt install pkg-config
   sudo apt install libglib2.0-dev
2. **Uruchomienie programu**: Aby uruchomić program, należy wybrać nazwę pliku wejściowego w zmiennej INPUT i wykonać następujące polecenia:
   make
   ./program INPUT=input.json OUTPUT=output.json
## Opis algorytmu
*coś tam napiszę*
![Skrzyżowanie](img/still.png)
![Wszystkie strony mogą skręcić w prawo](img/rightAll.png)
![Jedna strona skręca w prawo podczas gdy przeciwna jedzie do przodu](img/rightStraight.png)
![Jedna strona skręca w lewo, gdy ta lewa skręca na prawo](img/left.png)
![Przeciwne strony mogą jechać bezkolizyjnie](img/straight.png)

## Testy
1) input1.json:
Testowanie jak sobie radzi w przypadku dużej ilości samochodów w liniach east-west i north-south.
Algorytm tak jak i powinien przepuszcza po kolei jeden z west-east, a potem dwa south-north i north-south naraz tworząc bezkolizyjny ruch.
2) input2.json
Testowanie jak sobie radzi w przypadku dużej ilości samochodów (30) w linii east-west i nie tak dużej ilości (3) w south-north.
Algorytm tak jak i powinien przepuszcza odpowiedni procent samochodów w linii gdzie jest największy korek, a potem daje przejechać samochodowi z mniejszym korkiem
4) inpit3.json
Testowanie bezkolizyjnego przejechanie samochodów w różnych przypadkach tzn.
- cztery na raz jadą w prawo
- prawo + prosto
- lewo + prawo

## Potencjalne przeniesienie układu na platformę embedded
