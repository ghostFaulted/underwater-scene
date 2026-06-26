#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    // Usuniecie informacji o przesunieciu fizycznym (translacji) z macierzy widoku kamery za pomoca rzutowania mat4(mat3(...)).
    // Dzieki temu wirtualne otoczenie srodowiskowe porusza sie i obraca razem z glowa obserwatora,
    // dajac nieskonczona iluzje, ze pudlo skyboxa znajduje sie w nieskonczonosci poza wymiarem.
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    
    // Magiczna optymalizacja OpenGL: ustawienie glebi (Z) wierzcholka na rowna jego wspolczynnikowi (W).
    // Dzieki temu podczas dzielenia Z / W na koncu pipeline'u, wartosc zawsze wyniesie 1.0
    // Po wlaczeniu GL_LEQUAL, skybox narysuje sie tylko na "niezarysowanym niczym" (pustym) tle, oszczedzajac wydajnosc karty.
    gl_Position = pos.xyww; 
}