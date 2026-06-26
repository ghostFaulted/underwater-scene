# Interaktywna scena podwodna

## Skład grupy
1. Yaroslav Zamorskyi s498784
2. Maksim Sysoyeu s498775
3. Kiryl Kavaliou s498809

## Metody do wyboru
* **Metoda A:** A14 (Flow-map current distortion) - zastosowana na dnie oceanicznym (symulacja prądów wodnych).
* **Metoda B:** B04 (Spline movement) - wykorzystana do ruchu rekina i łodzi podwodnej wzdłuż krzywych Catmull-Rom z użyciem Parallel Transport Frames (PTF).

## Metody obowiązkowe zaimplementowane w projekcie
1. **Normal mapping:** Użyty dla modelu rekina oraz dna oceanicznego wraz z transformacją przestrzeni stycznej (TBN).
2. **PBR lighting:** Pełny model Cook-Torrance (GGX, Schlick-GGX, Fresnel-Schlick) z uwzględnieniem tekstur albedo, normal, roughness i parametrów metallic.
3. **Quaternion camera control:** System kamery oparty na kwaternionach, eliminujący problem gimbal lock.
4. **Shadow mapping:** Zaawansowane cienie z kierunkowego źródła światła (Słońce) oraz ze światła punktowego (reflektor łodzi), z wykorzystaniem Percentage-Closer Filtering (PCF) i dynamicznego biasu.
5. **Parallel Transport Frames:** Wyliczanie lokalnego układu współrzędnych bez skrętów wzdłuż krzywych dla płynnej nawigacji i orientacji modeli.
6. **Underwater skybox/cubemap:** Renderowanie tła bez uwzględniania translacji kamery, z optymalizacją głębi (`xyww`).

## Instrukcja budowania i uruchomienia
Projekt wykorzystuje CMake z automatycznym pobieraniem zależności (FetchContent).
1. `mkdir build && cd build`
2. `cmake ..`
3. `cmake --build .`
4. `./UnderwaterScene` (lub uruchomienie wygenerowanego celu w Visual Studio)

## Sterowanie i Interakcje
* `W, A, S, D` - Ruch kamery po scenie (aktywne tylko w trybie zablokowanego kursora).
* `Mysz` - Obrót kamery (aktywne tylko w trybie zablokowanego kursora).
* `C` - Przełączanie trybu kursora (LOCKED - sterowanie kamerą / FREE - obsługa UI i kliknięć).
* `E` - Włączanie/wyłączanie trybu "Wycieczki" (kamera podąża za łodzią podwodną).
* **Interakcja 1 (Światło):** Klawisz `F` lub przycisk w UI przełącza reflektor (spotlight) zamontowany na kamerze/łodzi.
* **Interakcja 2 (Zaawansowany Panel UI):** Rozbudowany interfejs graficzny pozwala na dynamiczną modyfikację kluczowych parametrów sceny w czasie rzeczywistym:
  * **Środowisko i Światło:** Zmiana gęstości mgły (Depth Fog), koloru wody, siły prądów morskich (Flow-map), a także pozycji i jasności głównego światła (Słońca).
  * **Materiały (PBR):** Suwaki do regulacji właściwości fizycznych (Metallic, Roughness, Tint) niezależnie dla rekina oraz łodzi podwodnej. Możliwość włączenia mikro-detali skóry rekina (Detail Normal Map).
  * **Diagnostyka i Splajny:** Przełączniki pozwalające na wizualizację trójwymiarowych trajektorii obiektów (Splajnów) oraz podgląd buforów cieni (Shadow Maps) generowanych przez reflektor i słońce.
* **Interakcja 3 (Picking / Raycasting):** W trybie swobodnego kursora (klawisz `C`), kliknięcie Lewym Przyciskiem Myszy (LPM) na model rekina aktywuje stan ucieczki (rekin znacznie przyspiesza i intensywniej macha ogonem na 4 sekundy).

## Screenshots
![Screenshot 2026-06-26 164654.png](Screenshot%202026-06-26%20164654.png)
![Screenshot 2026-06-26 164737.png](Screenshot%202026-06-26%20164737.png)
