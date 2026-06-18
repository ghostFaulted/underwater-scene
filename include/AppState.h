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

    glm::vec3 albedoColor = glm::vec3(0.4f, 0.45f, 0.5f);
    float ambientOcclusion = 1.0f;
    float roughness = 0.25f;
    float metallic = 0.9f;

    float flowMapIntensity = 0.5f;
    bool submarineLights = true;
    bool cursorDisabled = true;
};

