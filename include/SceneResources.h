#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "Shader.h"
#include "SplinePath.h"

// Kontener na wczytane z dysku programy shaderowe
struct ShaderSet {
    Shader pbr;
    Shader skybox;
    Shader lines;
    Shader shadow;

    // Konstruktor wymuszajacy zaladowanie wszystkich potrzebnych shaderow jednoczesnie
    ShaderSet(
        const std::string& pbrVertPath,
        const std::string& pbrFragPath,
        const std::string& skyVertPath,
        const std::string& skyFragPath,
        const std::string& lineVertPath,
        const std::string& lineFragPath,
        const std::string& shadowVertPath,
        const std::string& shadowFragPath
    )
        : pbr(pbrVertPath.c_str(), pbrFragPath.c_str())
        , skybox(skyVertPath.c_str(), skyFragPath.c_str())
        , lines(lineVertPath.c_str(), lineFragPath.c_str())
        , shadow(shadowVertPath.c_str(), shadowFragPath.c_str()) {
    }
};

// Kontener przechowujacy identyfikatory (ID) zaladowanych tekstur z pamieci VRAM karty graficznej
struct TextureSet {
    unsigned int sharkAlbedo = 0;
    unsigned int sharkNormal = 0;
    unsigned int sharkDetailNormal = 0;
    unsigned int sharkEyeAlbedo = 0;
    unsigned int sharkEyeNormal = 0;
    unsigned int sharkTeethAlbedo = 0;
    unsigned int sharkTeethNormal = 0;
    unsigned int seabedAlbedo = 0;
    unsigned int seabedNormal = 0;
    unsigned int seabedFlowMap = 0;
    unsigned int submarineAlbedo = 0;
};

// Zestaw identyfikatorow OpenGL potrzebnych do narysowania linii w przestrzeni (wizualizacja splajnu)
struct TrajectoryDebugBuffers {
    unsigned int normalVAO = 0;
    unsigned int normalVBO = 0;
    unsigned int normalLineCount = 0;

    unsigned int binormalVAO = 0;
    unsigned int binormalVBO = 0;
    unsigned int binormalLineCount = 0;

    unsigned int pathVAO = 0;
    unsigned int pathVBO = 0;
    unsigned int pathLineCount = 0;

    bool closedLoop = false; // Definiuje czy petla ma sie laczyc (GL_LINE_LOOP vs GL_LINE_STRIP)
};

// Identyfikatory wewnetrzne tworzace Framebuffer Mapy Cieni (Shadow Map)
struct ShadowMapResources {
    unsigned int framebuffer = 0; // Bufor ramki bez podpietego wyjscia koloru (DrawBuffer = GL_NONE)
    unsigned int depthMap = 0;    // Tekstura przechowujaca jedynie glebokosc fragmentu
    unsigned int width = 2048;
    unsigned int height = 2048;
};

// Narzedzia do obslugi sciezek relatywnych do roota projektu i generatorow zasobow
std::string ResolveProjectPath(const std::string& relativePath);
ShaderSet LoadShaders();
TextureSet LoadTextures();
std::vector<std::string> GetSkyboxFaces();

std::vector<glm::vec3> GenerateControlPoints();
TrajectoryDebugBuffers CreateTrajectoryDebugBuffers(const SplinePath& splinePath, bool useClosedTrajectory);
void DestroyTrajectoryDebugBuffers(TrajectoryDebugBuffers& buffers);

ShadowMapResources CreateShadowMapResources(unsigned int width, unsigned int height);
void DestroyShadowMapResources(ShadowMapResources& resources);

std::vector<glm::vec3> GenerateSubmarinePath();
std::vector<glm::vec3> GenerateCameraPath();