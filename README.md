# Interaktywna scena podwodna

## Skład grupy
1. Yaroslav Zamorskyi s498784
2. Maksim Sysoyeu s498775
3. Kiryl Kavaliou s

## Metody do wyboru
* **Metoda A:** A14 (Flow-map current distortion)
* **Metoda B:** B04 (Spline movement)

## Metody obowiązkowe
1. Normal mapping 
2. PBR lighting 
3. Quaternion camera control 
4. Shadow mapping 
5. Parallel Transport Frames 
6. Underwater skybox/cubemap

## Instrukcja budowania i uruchomienia
1. `mkdir build && cd build`
2. `cmake ..`
3. `cmake --build .`
4. `./UnderwaterScene`

## Sterowanie i Interakcje
* `W, A, S, D` - Ruch kamery 
* `Mysz` - Obrót kamery 
* `C` - Przełączanie trybu kursora (Zablokowany / Swobodny)
* `Interakcja 1 (Światło):` Klawisz `F` włącza/wyłącza oświetlenie rekina.
* `Interakcja 2 (Środowisko):` Panel UI pozwala na zmianę gęstości mgły (Depth Fog) oraz siły prądów morskich (Flow-map).
* `Interakcja 3 (Picking):` 