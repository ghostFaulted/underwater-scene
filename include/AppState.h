#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Camera.h"
#include "Raycaster.h"

struct AppState {
    Camera camera{ glm::vec3(0.0f, 5.0f, 25.0f) };
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

    glm::vec3 teethColor = glm::vec3(0.95f, 0.95f, 0.95f);
    float teethRoughness = 0.3f;
    float teethMetallic = 0.05f;

    float flowMapIntensity = 0.5f;
    bool spotlightEnabled = true;
    glm::vec3 spotColor = glm::vec3(1.0f);
    float spotIntensity = 1000.0f;
    float innerCutoff = 12.5f;
    float outerCutoff = 17.5f;
    bool cursorDisabled = true;

    bool debugMaterialKindView = false;
    bool debugRawAlbedoView = false;
    bool debugSpotOnlyView = false;
    bool debugSpotShadowMapView = false;
    bool debugSpotShadowCompareView = false;
    bool debugSpotCenterProbeView = false;

    glm::vec3 eyeMeshPositionOffset{ 0.0f };
    glm::vec3 eyeMeshRotationOffset{ 0.0f };
    glm::vec3 teethMeshPositionOffset{ 0.0f };
    glm::vec3 teethMeshRotationOffset{ 0.0f };

    glm::vec3 waterColor = glm::vec3(0.05f, 0.15f, 0.25f);
    float fogDensity = 0.02f;

    glm::vec3 dirLightDirection = glm::vec3(-0.5f, -1.0f, -0.5f);
    glm::vec3 dirLightColor = glm::vec3(1.0f, 0.95f, 0.8f);
    float dirLightIntensity = 10.0f;

    float sharkAngerTimer = 0.0f;
    float sharkVirtualSplineTime = 0.0f;
    float sharkVirtualAnimTime = 0.0f;
    bool showSharkSpline = false;

    glm::mat4 currentSharkModelMatrix{ 1.0f };

    AABB sharkLocalAABB = {
        glm::vec3(-150.0f, -80.0f, -300.0f), 
        glm::vec3(150.0f, 150.0f, 300.0f)    
    };

    bool isExcursionMode = true;
    float excursionVirtualTime = 0.0f;
    glm::mat4 currentSubmarineMatrix{ 1.0f };

    float subMetallic = 0.8f;     
    float subRoughness = 0.25f;    
    glm::vec3 subAlbedoColor = glm::vec3(1.0f, 1.0f, 1.0f);
    bool showSubmarineSpline = false;
};