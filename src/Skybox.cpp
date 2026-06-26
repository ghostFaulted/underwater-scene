#include "Skybox.h"
#include <stb_image.h>
#include <iostream>

Skybox::Skybox(const std::vector<std::string>& faces) {
    setupMesh();
    cubemapTexture = loadCubemap(faces);
}

void Skybox::Draw(Shader& shader) {
    // Trik OpenGL: Poniewaz w Vertex Shaderze przypisujemy 'gl_Position = pos.xyww',
    // glebokosc skyboxa dla z-buffera zawsze wyjdzie '1.0' (Z=W w homogenizacji da Z/W=1).
    // Musimy wiec zmienic testowanie glebokosci z 'Less' na 'Less Equal', zeby skybox zrobil renderowanie.
    glDepthFunc(GL_LEQUAL);

    shader.use();
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36); // Rysowanie 36 wierzcholkow (12 trojkatow) tworzacych obwoduke szescianu
    glBindVertexArray(0);

    // Po ukonczeniu renderowania skyboxa koniecznie oddajemy domyslny, precyzyjny test glebi z-bufferowi.
    glDepthFunc(GL_LESS);
}

void Skybox::setupMesh() {
    // Bardzo sztywne wymiary lokalnego wnetrza siatki w formacie trojkatnym dla klasycznego pudla do tła (bez Normalnych i bez UV)
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    // Pojedynczy strumien danych atrybutu "Position" bez posrednictwa
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

unsigned int Skybox::loadCubemap(const std::vector<std::string>& faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    // Bind jako obiekt CUBE MAP zamiast zwyklego 2D
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    // Awaryjny kolor w przypadku braku lub usterki jednej z bitmap tła
    unsigned char dummyColor[] = { 5, 25, 45, 255 };

    // Iteracja po wszystkich wektorach tekstur (Zazwyczaj w kolejnosci Right, Left, Top, Bottom, Back, Front)
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            // Enumerator "GL_TEXTURE_CUBE_MAP_POSITIVE_X" narasta sekwencyjnie (+i)
            // co pozwala nam zautomatyzowac wgrywanie 6 roznych map do pamieci w jednym for'ze
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "ERROR::SKYBOX::Failed to load texture at: " << faces[i] << std::endl;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummyColor);
        }
    }

    // Parametry zapobiegajace widocznym rozdarciom liniowym na laczeniach naroznikow miedzy scianami skyboxa
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}