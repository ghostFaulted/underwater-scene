#pragma once

#include <cstddef>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "AppState.h"
#include "Model.h"
#include "SceneResources.h"
#include "Skybox.h"

// Struktura pelniaca role paczki danych wysylanych z funkcji przygotowujacej klatke
// bezposrednio do funkcji wykonujacych przebiegi renderowania (Draw calls)
struct SceneRenderContext {
    glm::mat4 projection{ 1.0f };
    glm::mat4 view{ 1.0f };
    glm::vec3 cameraPosition{ 0.0f };

    glm::mat4 sharkModelMatrix{ 1.0f };
    float animationTimeSeconds = 0.0f;
    float globalTimeSeconds = 0.0f;
    float splineTime = 0.0f;
    float signedTurnCurvature = 0.0f; // Parametr decydujacy o zagieciu ciala na zakrecie

    glm::mat4 floorMatrix{ 1.0f };

    // Obliczone macierze cieni dla konkretnej klatki
    glm::mat4 sunLightMatrix{ 1.0f };
    glm::mat4 spotLightMatrix{ 1.0f };

    glm::vec3 spotPosition{ 0.0f };
    glm::vec3 spotDirection{ 0.0f };
    glm::mat4 submarineMatrix{ 1.0f };
};

// Aktualizuje swobodny lot kamery WASD
void UpdateCamera(AppState& appState, float deltaTime);

// Resetuje kontekst interfejsu uzytkownika przed renderowaniem
void BeginImGuiFrame();

// Glowna funkcja budujaca macierze projekcji, swiatla i obiektow na biezaca klatke
SceneRenderContext BuildSceneRenderContext(
    AppState& appState,
    const SplinePath& sharkPath,
    float currentFrameTime
);

// Generowanie map cieni (Depth maps) do Framebufferow
void RenderShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const ShadowMapResources& shadowMap
);
void RenderSunShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const ShadowMapResources& sunShadow
);
void RenderSpotShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const ShadowMapResources& spotShadow
);

// Glowny Pass renderingu gdzie liczone sa swiatla Cook-Torrance (PBR)
void RenderPbrScene(
    const SceneRenderContext& context,
    const AppState& appState,
    Shader& pbrShader,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const TextureSet& textures,
    const ShadowMapResources& sunShadow,
    const ShadowMapResources& spotShadow
);

// Rysowanie otoczenia tła
void RenderSkyboxPass(const SceneRenderContext& context, Shader& skyboxShader, Skybox& skybox);

// Funkcja testowa rysujaca poligonowe linie krzywych (Splajnow)
void RenderTrajectoryDebug(const SceneRenderContext& context, Shader& linesShader, const TrajectoryDebugBuffers& buffers);

// Budowa Menu bocznego ImGui do sterowania scena
void RenderControlPanel(
    AppState& appState,
    const Model& shark,
    const float signedCurvature,
    const ShadowMapResources& sunShadow,
    const ShadowMapResources& spotShadow
);