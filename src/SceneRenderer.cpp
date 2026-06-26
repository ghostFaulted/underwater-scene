#include "SceneRenderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>

#include "SharkSkeletonAnimator.h"

namespace {
    SharkSkeletonAnimator& GetSwimAnimator() {
        static SharkSkeletonAnimator animator;
        return animator;
    }

    void ApplySharkSkinning(Shader& shader, Model& sharkModel, const SceneRenderContext& context) {
        auto& animator = GetSwimAnimator();
        static bool initialized = false;
        if (!initialized) {
            animator.InitializeFromModel(sharkModel);
            initialized = true;
        }

        animator.ApplyPose(sharkModel, context.animationTimeSeconds, context.signedTurnCurvature);
        sharkModel.UploadBoneTransforms(shader, true);
    }

    void DisableSkinning(Shader& shader) {
        shader.setBool("uUseBoneSkinning", false);
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
    AppState& appState,
    const SplinePath& sharkPath,
    const float currentFrameTime
) {
    SceneRenderContext context{};

    const float aspect = static_cast<float>(appState.windowWidth) / static_cast<float>(appState.windowHeight);
    context.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f);
    context.view = appState.camera.GetViewMatrix();
    context.cameraPosition = appState.camera.GetPosition();

    const float splineTime = std::fmod(appState.sharkVirtualSplineTime / 42.0f, 1.0f);
    context.animationTimeSeconds = currentFrameTime;
    context.globalTimeSeconds = currentFrameTime; 
    context.splineTime = splineTime;
    context.signedTurnCurvature = sharkPath.GetSignedCurvature(splineTime);
    context.sharkModelMatrix = sharkPath.GetTransform(splineTime);

    context.sharkModelMatrix = glm::rotate(context.sharkModelMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    context.sharkModelMatrix = glm::scale(context.sharkModelMatrix, glm::vec3(0.05f));

    appState.currentSharkModelMatrix = context.sharkModelMatrix;

    context.floorMatrix = glm::mat4(1.0f);
    context.floorMatrix = glm::translate(context.floorMatrix, glm::vec3(0.0f, -15.0f, 0.0f));
    context.floorMatrix = glm::scale(context.floorMatrix, glm::vec3(10.0f));

    glm::vec3 lightTarget(0.0f);
    glm::vec3 lightDir = glm::normalize(appState.dirLightDirection);
    glm::vec3 lightPos = lightTarget - lightDir * 30.0f;

    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, upVector)) > 0.999f) {
        upVector = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    const glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 100.0f);
    const glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, upVector);
    context.sunLightMatrix = lightProjection * lightView;

    context.spotDirection = appState.camera.GetForward();
    context.spotPosition = context.cameraPosition - 0.5f * appState.camera.GetForward() - 2.0f * appState.camera.GetUp();

    glm::vec3 up = appState.camera.GetUp();

    const float clampedOuterCutoff = std::clamp(appState.outerCutoff, 1.0f, 89.0f);
    const float spotFovDegrees = std::clamp(std::max(60.0f, clampedOuterCutoff * 2.0f + 18.0f), 45.0f, 120.0f);
    glm::mat4 spotProjection = glm::perspective(glm::radians(spotFovDegrees), 1.0f, 2.0f, 200.0f);
    glm::mat4 spotView = glm::lookAt(context.spotPosition, context.spotPosition + context.spotDirection, up);

    context.spotLightMatrix = spotProjection * spotView;
    context.submarineMatrix = appState.currentSubmarineMatrix;

    return context;
}

void RenderShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const ShadowMapResources& shadowMap
) {
    RenderSunShadowPass(context, shadowShader, appState, shark, seabed, submarine, shadowMap);
}

void RenderSunShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const ShadowMapResources& sunShadow
) {
    shadowShader.use();
    shadowShader.setMat4("lightSpaceMatrix", context.sunLightMatrix);

    glViewport(0, 0, static_cast<GLsizei>(sunShadow.width), static_cast<GLsizei>(sunShadow.height));
    glBindFramebuffer(GL_FRAMEBUFFER, sunShadow.framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.setMat4("model", context.sharkModelMatrix);
    ApplySharkSkinning(shadowShader, shark, context);
    shark.Draw(shadowShader, nullptr);

    shadowShader.setMat4("model", context.submarineMatrix);
    DisableSkinning(shadowShader);
    submarine.Draw(shadowShader, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderSpotShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    Model& submarine,
    const ShadowMapResources& spotShadow
) {
    shadowShader.use();
    shadowShader.setMat4("lightSpaceMatrix", context.spotLightMatrix);
    glViewport(0, 0, static_cast<GLsizei>(spotShadow.width), static_cast<GLsizei>(spotShadow.height));
    glBindFramebuffer(GL_FRAMEBUFFER, spotShadow.framebuffer);

    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.setMat4("model", context.sharkModelMatrix);
    ApplySharkSkinning(shadowShader, shark, context);
    shark.Draw(shadowShader, nullptr);

    shadowShader.setMat4("model", context.submarineMatrix);
    DisableSkinning(shadowShader);
    submarine.Draw(shadowShader, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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
) {
    glViewport(0, 0, appState.windowWidth, appState.windowHeight);
    glClearColor(appState.waterColor.r, appState.waterColor.g, appState.waterColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pbrShader.use();
    pbrShader.setMat4("projection", context.projection);
    pbrShader.setMat4("view", context.view);
    pbrShader.setVec3("camPos", context.cameraPosition);
    pbrShader.setMat4("sunLightMatrix", context.sunLightMatrix);
    pbrShader.setMat4("spotLightMatrix", context.spotLightMatrix);
    pbrShader.setBool("uDebugMaterialKindView", appState.debugMaterialKindView);
    pbrShader.setBool("uDebugRawAlbedoView", appState.debugRawAlbedoView);
    pbrShader.setBool("uDebugSpotOnlyView", appState.debugSpotOnlyView);

    pbrShader.setInt("uMeshMaterialKind", 0);
    pbrShader.setBool("useAlbedoMap", true);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textures.sharkAlbedo);
    pbrShader.setInt("albedoMap", 3);

    pbrShader.setBool("useNormalMap", true);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures.sharkNormal);
    pbrShader.setInt("normalMap", 2);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, textures.sharkEyeAlbedo);
    pbrShader.setInt("albedoMapEye", 5);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, textures.sharkEyeNormal);
    pbrShader.setInt("normalMapEye", 6);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, textures.sharkTeethAlbedo);
    pbrShader.setInt("albedoMapTeeth", 7);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, textures.sharkTeethNormal);
    pbrShader.setInt("normalMapTeeth", 8);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sunShadow.depthMap);
    pbrShader.setInt("sunShadowMap", 1);

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, spotShadow.depthMap);
    pbrShader.setInt("spotShadowMap", 9);

    pbrShader.setVec3("dirLightDirection", appState.dirLightDirection);
    pbrShader.setVec3("dirLightColor", appState.dirLightColor);
    pbrShader.setFloat("dirLightIntensity", appState.dirLightIntensity);
    pbrShader.setVec3("waterColor", appState.waterColor);
    pbrShader.setFloat("fogDensity", appState.fogDensity);

    pbrShader.setVec3("spotPosition", context.spotPosition);
    pbrShader.setVec3("spotDirection", context.spotDirection);
    pbrShader.setBool("spotEnabled", appState.spotlightEnabled);
    pbrShader.setVec3("spotColor", appState.spotColor);
    pbrShader.setFloat("spotIntensity", appState.spotIntensity);
    pbrShader.setBool("uDebugSpotShadowCompareView", appState.debugSpotShadowCompareView);
    pbrShader.setBool("uDebugSpotCenterProbeView", appState.debugSpotCenterProbeView);

    const float clampedInnerCutoff = std::clamp(appState.innerCutoff, 1.0f, 89.0f);
    const float clampedOuterCutoff = std::clamp(appState.outerCutoff, clampedInnerCutoff, 89.0f);
    pbrShader.setFloat("innerCutoff", glm::cos(glm::radians(clampedInnerCutoff)));
    pbrShader.setFloat("outerCutoff", glm::cos(glm::radians(clampedOuterCutoff)));
    pbrShader.setFloat("uTime", context.globalTimeSeconds);
    pbrShader.setFloat("flowMapIntensity", appState.flowMapIntensity);

    pbrShader.setBool("useDetailNormalMap", appState.sharkUseDetailNormal);
    pbrShader.setFloat("detailNormalStrength", appState.sharkDetailNormalStrength);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, textures.sharkDetailNormal);
    pbrShader.setInt("detailNormalMap", 4);

    pbrShader.setVec3("albedo", appState.albedoColor);
    pbrShader.setFloat("ao", appState.ambientOcclusion);
    pbrShader.setFloat("metallic", appState.metallic);
    pbrShader.setFloat("roughness", appState.roughness);

    pbrShader.setVec3("teethColor", appState.teethColor);
    pbrShader.setFloat("teethMetallic", appState.teethMetallic);
    pbrShader.setFloat("teethRoughness", appState.teethRoughness);

    pbrShader.setMat4("model", context.sharkModelMatrix);
    ApplySharkSkinning(pbrShader, shark, context);

    pbrShader.setBool("useFlowMap", false);
    shark.Draw(pbrShader, &appState);

    pbrShader.setMat4("model", context.floorMatrix);
    DisableSkinning(pbrShader);
    pbrShader.setBool("useDetailNormalMap", false);
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

    pbrShader.setBool("useFlowMap", true);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, textures.seabedFlowMap);
    pbrShader.setInt("flowMap", 10);

    seabed.Draw(pbrShader);

    pbrShader.setMat4("model", context.submarineMatrix);
    DisableSkinning(pbrShader);
    pbrShader.setBool("useDetailNormalMap", false);

    pbrShader.setFloat("metallic", appState.subMetallic);
    pbrShader.setFloat("roughness", appState.subRoughness);
    pbrShader.setVec3("albedo", appState.subAlbedoColor);
    pbrShader.setFloat("ao", 1.0f);

    pbrShader.setBool("useAlbedoMap", true);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textures.submarineAlbedo);
    pbrShader.setInt("albedoMap", 3);

    pbrShader.setBool("useNormalMap", false);
    pbrShader.setBool("useFlowMap", false);

    submarine.Draw(pbrShader, nullptr);
}

void RenderSkyboxPass(const SceneRenderContext& context, Shader& skyboxShader, Skybox& skybox) {
    skyboxShader.use();
    skyboxShader.setMat4("view", context.view);
    skyboxShader.setMat4("projection", context.projection);
    skybox.Draw(skyboxShader);
}

void RenderTrajectoryDebug(const SceneRenderContext& context, Shader& linesShader,
    const TrajectoryDebugBuffers& buffers) {
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

void RenderControlPanel(
    AppState& appState,
    const Model& shark,
    const float signedCurvature,
    const ShadowMapResources& sunShadow,
    const ShadowMapResources& spotShadow
) {
    ImGui::Begin("Interaktywna scena podwodna");
    ImGui::Separator();
    ImGui::Text("=== Sterowanie (Controls) ===");
    ImGui::Text("C    - Kursor (Zablokowany / Swobodny)");
    ImGui::Text("E    - Kamera (Z 3-go osoby / Swobodna)");
    ImGui::Text("F    - Reflektor lodzi (Wl / Wyl)");
    ImGui::Text("LPM  - Kliknij na rekina zeby go przestraszyc (Gdy kursor jest swobodny)");
    ImGui::Spacing();
    ImGui::Text("Tryb kamery: %s", appState.isExcursionMode ? "3RD PERSON (Submarine)" : "FREE (WASD)");
    ImGui::Text("Kursor: %s", appState.cursorDisabled ? "LOCKED" : "FREE (UI Active)");

    ImGui::Separator();
    ImGui::Text("=== Srodowisko (Environment) ===");
    ImGui::SliderFloat("Prady wodne (Flow Map)", &appState.flowMapIntensity, 0.0f, 1.5f);
    ImGui::SliderFloat("Gestosc mgly (Fog)", &appState.fogDensity, 0.0f, 0.05f, "%.4f");
    ImGui::ColorEdit3("Kolor wody", &appState.waterColor[0]);

    ImGui::Separator();
    ImGui::Text("=== Oswietlenie (Lighting) ===");
    ImGui::SliderFloat3("Kierunek slonca", &appState.dirLightDirection[0], -1.0f, 1.0f);
    ImGui::SliderFloat("Sila slonca", &appState.dirLightIntensity, 0.0f, 30.0f);
    ImGui::Checkbox("Reflektor lodzi (Spotlight)", &appState.spotlightEnabled);

    ImGui::Separator();
    ImGui::Text("=== Diagnostyka ===");
    ImGui::Checkbox("Pokaz trajektorie (Splines)", &appState.showSubmarineSpline);
    appState.showSharkSpline = appState.showSubmarineSpline;

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
}