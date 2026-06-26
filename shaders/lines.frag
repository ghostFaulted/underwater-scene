#version 330 core
// Prosty shader fragmentow nakladajacy plaski (solidny) kolor bez zadnego oswietlenia.
out vec4 FragColor;

// Zmienna wpuszczana ze srodowiska C++ pozwalajaca pokolorowac rozne linie na rozne kolory 
// (np. cyan dla sciezki, zielony dla Normalnych)
uniform vec3 lineColor;

void main()
{
    // Konwersja koloru RGB na format RGBA z wymuszonym kanalem Alpha na poziomie 100% (nieprzezroczysty)
    FragColor = vec4(lineColor, 1.0);
}