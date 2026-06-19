#include "SceneRenderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <string>

#include "SplineSwimAnimator.h"

namespace {
    SplineSwimAnimator& GetSwimAnimator() {
        static SplineSwimAnimator animator;
        return animator;
    }

    void ApplySubmarineSkinning(Shader& shader, const SceneRenderContext& context) {
        auto& animator = GetSwimAnimator();
        animator.BuildPose(context.animationTimeSeconds, context.signedTurnCurvature);
        animator.UploadToShader(shader, true);
    }

    void DisableSubmarineSkinning(Shader& shader) {
        GetSwimAnimator().UploadToShader(shader, false);
    }
}

void UpdateCamera(AppState& appState, const float deltaTime) {
    if (!appState.cursorDisabled) {
        return;
    }

    appState.camera.ProcessKeyboard(
        appState.keyDown[GLFW_KEY_W],
        appState.keyDown[GLFW_KEY_S],
        appState.keyDown[GLFW_KEY_A],
        appState.keyDown[GLFW_KEY_D],
        deltaTime
    );
}

void BeginImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

SceneRenderContext BuildSceneRenderContext(
    const AppState& appState,
    const SplinePath& submarinePath,
    const glm::vec3& lightPosition,
    const float currentFrameTime
) {
    SceneRenderContext context{};

    const float aspect = static_cast<float>(appState.windowWidth) / static_cast<float>(appState.windowHeight);
    context.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    context.view = appState.camera.GetViewMatrix();
    context.cameraPosition = appState.camera.GetPosition();

    const float splineTime = std::fmod(currentFrameTime / 20.0f, 1.0f);
    context.animationTimeSeconds = currentFrameTime;
    context.splineTime = splineTime;
    context.signedTurnCurvature = submarinePath.GetSignedCurvature(splineTime);
    context.submarineModelMatrix = submarinePath.GetTransform(splineTime);
    context.submarineModelMatrix = glm::rotate(
        context.submarineModelMatrix,
        glm::radians(180.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    context.floorMatrix = glm::mat4(1.0f);
    context.floorMatrix = glm::translate(context.floorMatrix, glm::vec3(0.0f, -15.0f, 0.0f));
    context.floorMatrix = glm::scale(context.floorMatrix, glm::vec3(10.0f));

    const glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 100.0f);
    const glm::mat4 lightView = glm::lookAt(lightPosition, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    context.lightSpaceMatrix = lightProjection * lightView;

    return context;
}

void RenderShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    Model& submarine,
    Model& seabed,
    const ShadowMapResources& shadowMap
) {
    shadowShader.use();
    shadowShader.setMat4("lightSpaceMatrix", context.lightSpaceMatrix);

    glViewport(0, 0, static_cast<GLsizei>(shadowMap.width), static_cast<GLsizei>(shadowMap.height));
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.setMat4("model", context.submarineModelMatrix);
    ApplySubmarineSkinning(shadowShader, context);
    submarine.Draw(shadowShader);

    shadowShader.setMat4("model", context.floorMatrix);
    DisableSubmarineSkinning(shadowShader);
    seabed.Draw(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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
    const std::size_t lightCount
) {
    glViewport(0, 0, appState.windowWidth, appState.windowHeight);
    glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pbrShader.use();
    pbrShader.setMat4("projection", context.projection);
    pbrShader.setMat4("view", context.view);
    pbrShader.setVec3("camPos", context.cameraPosition);
    pbrShader.setMat4("lightSpaceMatrix", context.lightSpaceMatrix);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
    pbrShader.setInt("shadowMap", 1);

    for (std::size_t i = 0; i < lightCount; ++i) {
        pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
        const glm::vec3 currentLightColor = appState.submarineLights ? lightColors[i] : glm::vec3(0.0f);
        pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", currentLightColor);
    }

    pbrShader.setBool("useAlbedoMap", true);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textures.submarineAlbedo);
    pbrShader.setInt("albedoMap", 3);

    pbrShader.setBool("useNormalMap", false);
    pbrShader.setVec3("albedo", appState.albedoColor);
    pbrShader.setFloat("ao", appState.ambientOcclusion);
    pbrShader.setFloat("metallic", appState.metallic);
    pbrShader.setFloat("roughness", appState.roughness);
    pbrShader.setMat4("model", context.submarineModelMatrix);
    ApplySubmarineSkinning(pbrShader, context);
    submarine.Draw(pbrShader);

    pbrShader.setMat4("model", context.floorMatrix);
    DisableSubmarineSkinning(pbrShader);
    pbrShader.setFloat("metallic", 0.0f);
    pbrShader.setFloat("roughness", 0.9f);
    pbrShader.setFloat("ao", 1.0f);

    pbrShader.setBool("useAlbedoMap", true);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textures.seabedAlbedo);
    pbrShader.setInt("albedoMap", 3);

    pbrShader.setBool("useNormalMap", true);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures.seabedNormal);
    pbrShader.setInt("normalMap", 2);
    seabed.Draw(pbrShader);
}

void RenderSkyboxPass(const SceneRenderContext& context, Shader& skyboxShader, Skybox& skybox) {
    skyboxShader.use();
    skyboxShader.setMat4("view", context.view);
    skyboxShader.setMat4("projection", context.projection);
    skybox.Draw(skyboxShader);
}

void RenderTrajectoryDebug(const SceneRenderContext& context, Shader& linesShader, const TrajectoryDebugBuffers& buffers) {
    if (buffers.normalLineCount == 0 && buffers.binormalLineCount == 0 && buffers.pathLineCount == 0) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glLineWidth(3.0f);

    linesShader.use();
    linesShader.setMat4("projection", context.projection);
    linesShader.setMat4("view", context.view);

    if (buffers.normalLineCount > 0) {
        glBindVertexArray(buffers.normalVAO);
        linesShader.setVec3("lineColor", glm::vec3(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(buffers.normalLineCount));
    }

    if (buffers.binormalLineCount > 0) {
        glBindVertexArray(buffers.binormalVAO);
        linesShader.setVec3("lineColor", glm::vec3(1.0f, 0.0f, 1.0f));
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(buffers.binormalLineCount));
    }

    if (buffers.pathLineCount > 0) {
        glBindVertexArray(buffers.pathVAO);
        linesShader.setVec3("lineColor", glm::vec3(0.0f, 1.0f, 1.0f));
        const GLenum pathDrawMode = buffers.closedLoop ? GL_LINE_LOOP : GL_LINE_STRIP;
        glDrawArrays(pathDrawMode, 0, static_cast<GLsizei>(buffers.pathLineCount));
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void RenderControlPanel(AppState& appState) {
    ImGui::Begin("GRK: Interactive U-Boat Diorama");
    ImGui::Separator();
    ImGui::Text("System Controls");
    ImGui::Text("Press 'C' to toggle Cursor lock.");
    ImGui::Text("Cursor state: %s", appState.cursorDisabled ? "LOCKED (Camera)" : "FREE (UI Active)");
    ImGui::Separator();
    ImGui::Text("Submarine PBR Materials");
    ImGui::ColorEdit3("Steel Albedo", &appState.albedoColor[0]);
    ImGui::SliderFloat("Metallic (Metalness)", &appState.metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &appState.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Ambient Occlusion", &appState.ambientOcclusion, 0.0f, 1.0f);
    ImGui::Separator();
    ImGui::Text("Environment (A14 / Fog)");
    ImGui::SliderFloat("Current Intensity", &appState.flowMapIntensity, 0.0f, 2.0f);
    ImGui::Separator();
    ImGui::Text("Submarine controls");
    ImGui::Checkbox("Toggle Lights (F)", &appState.submarineLights);
    ImGui::Separator();
    ImGui::Text("Diagnostics");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Use Mouse to look around, WASD to move.");
    ImGui::End();
}


