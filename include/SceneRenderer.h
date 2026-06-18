#pragma once

#include <cstddef>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "AppState.h"
#include "Model.h"
#include "SceneResources.h"
#include "Skybox.h"

struct SceneRenderContext {
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec3 cameraPosition{0.0f};
    glm::mat4 submarineModelMatrix{1.0f};
    glm::mat4 floorMatrix{1.0f};
    glm::mat4 lightSpaceMatrix{1.0f};
};

void UpdateCamera(AppState& appState, float deltaTime);
void BeginImGuiFrame();
SceneRenderContext BuildSceneRenderContext(
    const AppState& appState,
    const SplinePath& submarinePath,
    const glm::vec3& lightPosition,
    float currentFrameTime
);
void RenderShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    Model& submarine,
    Model& seabed,
    const ShadowMapResources& shadowMap
);
void RenderPbrScene(
    const SceneRenderContext& context,
    const AppState& appState,
    Shader& pbrShader,
    Model& submarine,
    Model& seabed,
    const TextureSet& textures,
    const ShadowMapResources& shadowMap,
    const glm::vec3* lightPositions,
    const glm::vec3* lightColors,
    std::size_t lightCount
);
void RenderSkyboxPass(const SceneRenderContext& context, Shader& skyboxShader, Skybox& skybox);
void RenderTrajectoryDebug(const SceneRenderContext& context, Shader& linesShader, const TrajectoryDebugBuffers& buffers);
void RenderControlPanel(AppState& appState);

