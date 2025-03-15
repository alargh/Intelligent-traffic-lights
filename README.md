Oczywiście! Oto poprawnie sformatowany tekst:

```markdown
# Intelligent-traffic-lights

## Jak Uruchomić Projekt na Ubuntu

1. **Kompilacja**: Upewnij się, że masz zainstalowany kompilator C (np. gcc), bibliotekę glib oraz pozostałe wymagane narzędzia.
   ```bash
   sudo apt update
   sudo apt install build-essential
   sudo apt install pkg-config
   sudo apt install libglib2.0-dev
   ```
   
2. **Uruchomienie programu**: Aby uruchomić program, należy wybrać nazwę pliku wejściowego w zmiennej `INPUT` i wykonać następujące polecenia:
   ```bash
   make
   ./program INPUT=input.json OUTPUT=output.json
   ```

## Opis algorytmu

Algorytm działa z uwzględnieniem 2 rzeczy:
- ilości samochodów po 4 różnych stronach świata
- długości oczekiwania przez różne strony świata

Program idzie po kolei przez polecenia w pliku `input.json` i działa według algorytmu.

### Algorytm:

#### Polecenie "addVehicle":
1) Dodaje do kolejki `south`, `north`, `east`, `west` element typu `Vehicle`, zawierający potrzebne informacje o samochodzie (id, skąd wyrusza, gdzie wyrusza).

#### Polecenie "step":
Polecenie `step` wymusza nam wybór drogi, która będzie miała zielone światło. Algorytm wyboru drogi krok po kroku:
1) Najpierw sprawdzamy ilość samochodów po każdej stronie, zapisując w zmiennych `overall_north`, `overall_south`, itd.
2) Potem patrzymy na zmienne zawierające ilość samochodów, które opuściły daną drogę **POD RZĄD** (czyli jeżeli wybieramy jakąś drogę, dla pozostałych ta zmienna się zeruje). Są to zmienne `out_north`, itd.
3) Znajdujemy 4 ilorazy: 
   - \((out_{dir} + 1) / (overall_{dir} + out_{dir} + 1)\)
4) Wybieramy tę stronę świata, która ma ten iloraz najniższy.
5) Dodatkowym warunkiem jest, jeżeli jakaś strona świata nie była wybrana ponad 10 razy, to jest wybierana ona, aby za duże korki w dwóch stronach nie spowodowały stania w trzeciej stronie w nieskończoność.
6) Jak wybraliśmy stronę, z której chcemy dać zielone światło, również damy zielone światło tej stronie, w której ruch byłby bezkolizyjny z naszą wybraną stroną.
7) Bezkolizyjne ruchy są podane poniżej na rysunkach:

   ![Skrzyżowanie](img/still.png)
   
   ![Wszystkie strony mogą skręcić w prawo](img/rightAll.png)
   
   ![Jedna strona skręca w prawo podczas gdy przeciwna jedzie do przodu](img/rightStraight.png)
   
   ![Jedna strona skręca w lewo, gdy ta lewa skręca na prawo](img/left.png)
   
   ![Przeciwne strony mogą jechać bezkolizyjnie](img/straight.png)

## Testy

0) **input.json**  
Test wzięty z polecenia.

1) **input1.json**  
Testowanie jak sobie radzi w przypadku dużej ilości samochodów w liniach `east-west` i `north-south`.  
Algorytm tak, jak powinien, przepuszcza po kolei jeden z `west-east`, a potem dwa `south-north` i `north-south` naraz, tworząc bezkolizyjny ruch.

2) **input2.json**  
Testowanie jak sobie radzi w przypadku dużej ilości samochodów (30) w linii `east-west` i nie tak dużej ilości (3) w `south-north`.  
Algorytm tak, jak powinien, przepuszcza odpowiedni procent samochodów w linii, gdzie jest największy korek, a potem daje przejechać samochodowi z mniejszym korkiem.

3) **input3.json**  
Testowanie bezkolizyjnego przejechania samochodów w różnych przypadkach, tzn.:
- Cztery na raz jadą w prawo.
- Prawo + prosto.
- Lewo + prawo.

## Potencjalne przeniesienie układu na platformę embedded
```

Ten format zapewnia czytelność, a także poprawne renderowanie na platformach takich jak GitHub. Jeśli potrzebujesz dodatkowych poprawek, daj znać!
