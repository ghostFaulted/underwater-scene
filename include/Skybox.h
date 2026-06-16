#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include "Shader.h"

class Skybox {
public:
    Skybox(const std::vector<std::string>& faces);

    void Draw(Shader& shader);

private:
    unsigned int skyboxVAO, skyboxVBO;
    unsigned int cubemapTexture;

    void setupMesh();
    unsigned int loadCubemap(const std::vector<std::string>& faces);
};