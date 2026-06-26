#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include "Shader.h"

// Klasa odpowiedzialna za renderowanie 3-wymiarowego tla symulujacego bardzo oddalone obiekty w nieskonczonosci.
// W tym wypadku zastepuje niebo uzywajac tekstur symulujacych metna morską wode we wszystkich kierunkach
class Skybox {
public:
    // Wymaga 6 tekstur - odpowiednio prawą, lewą, górną, dolną, tył oraz przód
    Skybox(const std::vector<std::string>& faces);

    // Rysuje szescian skyboxa, pomijajac zapis w buforze glebi zeby dzialal jak najdalsze tlo
    void Draw(Shader& shader);

private:
    unsigned int skyboxVAO, skyboxVBO;
    unsigned int cubemapTexture; // ID wygenerowanej tekstury typu GL_TEXTURE_CUBE_MAP w OpenGL

    // Przygotowanie buforow geometrii idealnego szescianu od [-1 do 1]
    void setupMesh();

    // Uzywa zewnetrznego loadera do zlozenia 6 grafik w jedna wielowarstwowa teksture
    unsigned int loadCubemap(const std::vector<std::string>& faces);
};