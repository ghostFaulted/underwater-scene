#pragma once

#include <glm/vec3.hpp>

#include "Camera.h"

struct AppState {
    Camera camera{glm::vec3(0.0f, 5.0f, 25.0f)};
    bool keyDown[1024] = {};
    bool firstMouseSample = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    int windowWidth = 1280;
    int windowHeight = 720;

    glm::vec3 albedoColor = glm::vec3(1.0f);
    float ambientOcclusion = 1.0f;
    float roughness = 0.62f;
    float metallic = 0.03f;
    float sharkDetailNormalStrength = 0.12f;
    bool sharkUseDetailNormal = false;

    // Контроль материалов специальных частей акулы
    glm::vec3 teethColor = glm::vec3(0.95f, 0.95f, 0.95f);  // Почти белый для зубов
    float teethRoughness = 0.3f;
    float teethMetallic = 0.05f;

    float flowMapIntensity = 0.5f;
    bool sharkLights = true;
    bool cursorDisabled = true;

    // Debug visualization for material and texture verification.
    bool debugMaterialKindView = false;
    bool debugRawAlbedoView = false;

    // Manual mesh offset adjustment (for eye/teeth positioning).
    glm::vec3 eyeMeshPositionOffset{0.0f};
    glm::vec3 eyeMeshRotationOffset{0.0f};
    glm::vec3 teethMeshPositionOffset{0.0f};
    glm::vec3 teethMeshRotationOffset{0.0f};
};

