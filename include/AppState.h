#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Camera.h"
#include "Raycaster.h"

// Glowna struktura przetrzymujaca stan calej aplikacji. 
// Dzieki niej wszystkie parametry modyfikowane w ImGui (interfejsie) 
// sa dostepne z poziomu funkcji renderujacych sceny.
struct AppState {
    Camera camera{ glm::vec3(0.0f, 5.0f, 25.0f) };
    bool keyDown[1024] = {}; // Tablica wcisnietych klawiszy klawiatury
    bool firstMouseSample = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    int windowWidth = 1280;
    int windowHeight = 720;

    // Parametry materialu rekina (PBR)
    glm::vec3 albedoColor = glm::vec3(1.0f);
    float ambientOcclusion = 1.0f;
    float roughness = 0.62f;
    float metallic = 0.03f;
    float sharkDetailNormalStrength = 0.12f;
    bool sharkUseDetailNormal = false;

    // Parametry materialu zebow
    glm::vec3 teethColor = glm::vec3(0.95f, 0.95f, 0.95f);
    float teethRoughness = 0.3f;
    float teethMetallic = 0.05f;

    // Srodowisko - sily dzialajace na wode
    float flowMapIntensity = 0.25f;

    // Ustawienia reflektora (Spotlight) lodzi
    bool spotlightEnabled = true;
    glm::vec3 spotColor = glm::vec3(1.0f);
    float spotIntensity = 1000.0f;
    float innerCutoff = 12.5f;
    float outerCutoff = 17.5f;

    // Stan myszy (czy kursor lata po ekranie czy obraca kamera)
    bool cursorDisabled = true;

    // Flagi debugowania z panelu UI
    bool debugMaterialKindView = false;
    bool debugRawAlbedoView = false;
    bool debugSpotOnlyView = false;
    bool debugSpotShadowMapView = false;
    bool debugSpotShadowCompareView = false;
    bool debugSpotCenterProbeView = false;

    // Dynamiczne offsety nakladane na sub-meshe
    glm::vec3 eyeMeshPositionOffset{ 0.0f };
    glm::vec3 eyeMeshRotationOffset{ 0.0f };
    glm::vec3 teethMeshPositionOffset{ 0.0f };
    glm::vec3 teethMeshRotationOffset{ 0.0f };

    // Parametry oswietlenia kierunkowego (Slonce) i podwodnej mgly (Exponential Fog)
    glm::vec3 waterColor = glm::vec3(0.196f, 0.373f, 0.529f);
    float fogDensity = 0.0150f;
    glm::vec3 dirLightDirection = glm::vec3(-0.8f, -0.8f, -0.4f);
    glm::vec3 dirLightColor = glm::vec3(1.0f, 0.95f, 0.8f);
    float dirLightIntensity = 25.0f;

    // Parametry wirtualnego czasu rekina
    float sharkAngerTimer = 0.0f;
    float sharkVirtualSplineTime = 0.0f;
    float sharkVirtualAnimTime = 0.0f;
    bool showSharkSpline = false;

    // Macierz oraz Box kolizyjny wykorzystywane do Raycastingu z myszy
    glm::mat4 currentSharkModelMatrix{ 1.0f };
    AABB sharkLocalAABB = {
        glm::vec3(-150.0f, -80.0f, -300.0f),
        glm::vec3(150.0f, 150.0f, 300.0f)
    };

    // Tryb wycieczkowy (podazanie kamery za lodzia)
    bool isExcursionMode = true;
    float excursionVirtualTime = 0.0f;
    glm::mat4 currentSubmarineMatrix{ 1.0f };

    // Parametry PBR lodzi podwodnej
    float subMetallic = 0.8f;
    float subRoughness = 0.25f;
    glm::vec3 subAlbedoColor = glm::vec3(1.0f, 1.0f, 1.0f);

    bool showSubmarineSpline = false;
};