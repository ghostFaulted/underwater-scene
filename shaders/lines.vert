#version 330 core
// Prosty shader wierzcholkow do rysowania linii pomocniczych dla splajnow (Trajektorie).
// Przyjmuje wylacznie jeden atrybut: pozycje x,y,z wierzcholka na lokalizatorze = 0.
layout(location = 0) in vec3 position;

// Macierze kamery sluzace do umiejscowienia tych wierzcholkow na ekranie monitora
uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Rysowanie odbywa sie wprost w obrebie World Space, dlatego pomijamy tu macierz 'model'.
    // Od razu mnozymy przez przestrzen kamery (view) i obiektyw perspektywiczny (projection).
    gl_Position = projection * view * vec4(position, 1.0);
}