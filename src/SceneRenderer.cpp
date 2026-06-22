#include "SceneRenderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <algorithm>
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
    const AppState& appState,
    const SplinePath& sharkPath,
    const glm::vec3& lightPosition,
    const float currentFrameTime
) {
    SceneRenderContext context{};

    const float aspect = static_cast<float>(appState.windowWidth) / static_cast<float>(appState.windowHeight);
    context.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    context.view = appState.camera.GetViewMatrix();
    context.cameraPosition = appState.camera.GetPosition();

    const float splineTime = std::fmod(currentFrameTime / 42.0f, 1.0f);
    context.animationTimeSeconds = currentFrameTime;
    context.splineTime = splineTime;
    context.signedTurnCurvature = sharkPath.GetSignedCurvature(splineTime);
    context.sharkModelMatrix = sharkPath.GetTransform(splineTime);

    // Корректирующий поворот на 180 градусов вокруг X оси для правильной ориентации
    // (акула плывет "вверх ногами" без этого поворота)
    context.sharkModelMatrix = glm::rotate(context.sharkModelMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Scale shark model (FBX from 3ds Max is ~20x too large)
    context.sharkModelMatrix = glm::scale(context.sharkModelMatrix, glm::vec3(0.05f));

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
    context.lightSpaceMatrix = lightProjection * lightView;

    return context;
}

void RenderShadowPass(
    const SceneRenderContext& context,
    Shader& shadowShader,
    const AppState& appState,
    Model& shark,
    Model& seabed,
    const ShadowMapResources& shadowMap
) {
    shadowShader.use();
    shadowShader.setMat4("lightSpaceMatrix", context.lightSpaceMatrix);

    glViewport(0, 0, static_cast<GLsizei>(shadowMap.width), static_cast<GLsizei>(shadowMap.height));
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.setMat4("model", context.sharkModelMatrix);
    ApplySharkSkinning(shadowShader, shark, context);
    shark.Draw(shadowShader, nullptr);

    shadowShader.setMat4("model", context.floorMatrix);
    DisableSkinning(shadowShader);
    seabed.Draw(shadowShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPbrScene(
    const SceneRenderContext& context,
    const AppState& appState,
    Shader& pbrShader,
    Model& shark,
    Model& seabed,
    const TextureSet& textures,
    const ShadowMapResources& shadowMap,
    const glm::vec3* lightPositions,
    const glm::vec3* lightColors,
    const std::size_t lightCount
) {
    glViewport(0, 0, appState.windowWidth, appState.windowHeight);
    glClearColor(appState.waterColor.r, appState.waterColor.g, appState.waterColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pbrShader.use();
    pbrShader.setMat4("projection", context.projection);
    pbrShader.setMat4("view", context.view);
    pbrShader.setVec3("camPos", context.cameraPosition);
    pbrShader.setMat4("lightSpaceMatrix", context.lightSpaceMatrix);
    pbrShader.setBool("uDebugMaterialKindView", appState.debugMaterialKindView);
    pbrShader.setBool("uDebugRawAlbedoView", appState.debugRawAlbedoView);

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
    glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
    pbrShader.setInt("shadowMap", 1);

    pbrShader.setVec3("dirLightDirection", appState.dirLightDirection);
    pbrShader.setVec3("dirLightColor", appState.dirLightColor);
    pbrShader.setFloat("dirLightIntensity", appState.dirLightIntensity);
    pbrShader.setVec3("waterColor", appState.waterColor);
    pbrShader.setFloat("fogDensity", appState.fogDensity);

    pbrShader.setFloat("uTime", context.animationTimeSeconds);
    pbrShader.setFloat("flowMapIntensity", appState.flowMapIntensity);

    for (std::size_t i = 0; i < lightCount; ++i) {
        pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
        const glm::vec3 currentLightColor = appState.sharkLights ? lightColors[i] : glm::vec3(0.0f);
        pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", currentLightColor);
    }

    pbrShader.setBool("useDetailNormalMap", appState.sharkUseDetailNormal);
    pbrShader.setFloat("detailNormalStrength", appState.sharkDetailNormalStrength);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, textures.sharkDetailNormal);
    pbrShader.setInt("detailNormalMap", 4);

    pbrShader.setVec3("albedo", appState.albedoColor);
    pbrShader.setFloat("ao", appState.ambientOcclusion);
    pbrShader.setFloat("metallic", appState.metallic);
    pbrShader.setFloat("roughness", appState.roughness);

    // Параметры для зубов
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
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, textures.seabedFlowMap);
    pbrShader.setInt("flowMap", 9);

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

void RenderControlPanel(AppState& appState, const Model& shark, const float signedCurvature) {
    ImGui::Begin("GRK: Interactive U-Boat Diorama");
    ImGui::Separator();
    ImGui::Text("System Controls");
    ImGui::Text("Press 'C' to toggle Cursor lock.");
    ImGui::Text("Cursor state: %s", appState.cursorDisabled ? "LOCKED (Camera)" : "FREE (UI Active)");
    ImGui::Separator();
    ImGui::Text("Shark PBR Materials");
    ImGui::ColorEdit3("Body Tint", &appState.albedoColor[0]);
    ImGui::SliderFloat("Metallic (Metalness)", &appState.metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &appState.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Ambient Occlusion", &appState.ambientOcclusion, 0.0f, 1.0f);
    ImGui::Checkbox("Use Detail Normal", &appState.sharkUseDetailNormal);
    ImGui::SliderFloat("Detail Normal Strength", &appState.sharkDetailNormalStrength, 0.0f, 0.5f);

    ImGui::Separator();
    ImGui::Text("Teeth Materials");
    ImGui::ColorEdit3("Teeth Color", &appState.teethColor[0]);
    ImGui::SliderFloat("Teeth Metallic", &appState.teethMetallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Teeth Roughness", &appState.teethRoughness, 0.0f, 1.0f);

    ImGui::Separator();

    ImGui::Text("Environment (Fog & Skybox)");
    ImGui::ColorEdit3("Water Color", &appState.waterColor[0]);
    ImGui::SliderFloat("Fog Density", &appState.fogDensity, 0.0f, 0.1f, "%.4f");
    ImGui::Separator();

    ImGui::Text("Directional Light (Sun)");
    ImGui::SliderFloat3("Direction", &appState.dirLightDirection[0], -1.0f, 1.0f);
    ImGui::ColorEdit3("Light Color", &appState.dirLightColor[0]);
    ImGui::SliderFloat("Intensity", &appState.dirLightIntensity, 0.0f, 50.0f);
    ImGui::Separator();

    ImGui::Text("Shark controls");
    ImGui::Checkbox("Toggle Lights (F)", &appState.sharkLights);
    ImGui::Checkbox("Debug Material Kind Colors", &appState.debugMaterialKindView);
    ImGui::Checkbox("Debug Raw Albedo", &appState.debugRawAlbedoView);

    ImGui::Separator();
    ImGui::Text("Environment (A14)");
    ImGui::SliderFloat("Flow Map Intensity", &appState.flowMapIntensity, 0.0f, 2.0f);

    ImGui::Separator();
    ImGui::Text("Eye/Teeth Position & Rotation Offsets");
    ImGui::SliderFloat("Eye X offset", &appState.eyeMeshPositionOffset.x, -5.0f, 5.0f);
    ImGui::SliderFloat("Eye Y offset", &appState.eyeMeshPositionOffset.y, -5.0f, 5.0f);
    ImGui::SliderFloat("Eye Z offset", &appState.eyeMeshPositionOffset.z, -5.0f, 5.0f);
    ImGui::SliderFloat("Eye Rot X (deg)", &appState.eyeMeshRotationOffset.x, -180.0f, 180.0f);
    ImGui::SliderFloat("Eye Rot Y (deg)", &appState.eyeMeshRotationOffset.y, -180.0f, 180.0f);
    ImGui::SliderFloat("Eye Rot Z (deg)", &appState.eyeMeshRotationOffset.z, -180.0f, 180.0f);

    ImGui::Separator();
    ImGui::SliderFloat("Teeth X offset", &appState.teethMeshPositionOffset.x, -200.0f, 200.0f);
    ImGui::SliderFloat("Teeth Y offset", &appState.teethMeshPositionOffset.y, -200.0f, 200.0f);
    ImGui::SliderFloat("Teeth Z offset", &appState.teethMeshPositionOffset.z, -200.0f, 200.0f);
    ImGui::SliderFloat("Teeth Rot X (deg)", &appState.teethMeshRotationOffset.x, -180.0f, 180.0f);
    ImGui::SliderFloat("Teeth Rot Y (deg)", &appState.teethMeshRotationOffset.y, -180.0f, 180.0f);
    ImGui::SliderFloat("Teeth Rot Z (deg)", &appState.teethMeshRotationOffset.z, -180.0f, 180.0f);
    ImGui::Separator();

    ImGui::Text("Diagnostics");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Use Mouse to look around, WASD to move.");

    const auto& animator = GetSwimAnimator();
    const auto& rigLines = animator.GetDebugLines();
    const auto& rigBones = animator.GetAnimatedBones();
    const auto& rigAngles = animator.GetLastAppliedAnglesDegrees();

    ImGui::Separator();
    ImGui::Text("Shark Rig Overlay");
    ImGui::Text("Signed Turn Curvature: %.4f", signedCurvature);
    const int staticLineCount = std::min<int>(static_cast<int>(rigLines.size()), 12);
    for (int i = 0; i < staticLineCount; ++i) {
        ImGui::TextUnformatted(rigLines[static_cast<std::size_t>(i)].c_str());
    }

    const int liveLineCount = static_cast<int>(std::min(rigBones.size(), rigAngles.size()));
    for (int i = 0; i < liveLineCount; ++i) {
        ImGui::Text("%s : %.2f deg", rigBones[static_cast<std::size_t>(i)].c_str(), rigAngles[static_cast<std::size_t>(i)]);
    }

    ImGui::Separator();
    ImGui::Text("Bone debug (name | totalWeight | center xyz)");
    const auto boneDebug = shark.GetBoneDebugInfo();
    const int boneShow = std::min<int>(static_cast<int>(boneDebug.size()), 24);
    for (int i = 0; i < boneShow; ++i) {
        const auto& b = boneDebug[static_cast<std::size_t>(i)];
        ImGui::Text(
            "%s | w=%.3f | c=(%.3f %.3f %.3f)",
            b.name.c_str(),
            b.totalWeight,
            b.weightedCenter.x,
            b.weightedCenter.y,
            b.weightedCenter.z
        );
    }

    ImGui::Separator();
    ImGui::Text("Mesh debug (name | kind | attach | pivot xyz | centroid xyz | extent xyz)");
    const auto meshDebug = shark.GetMeshDebugInfo();
    const int meshShow = std::min<int>(static_cast<int>(meshDebug.size()), 48);
    for (int i = 0; i < meshShow; ++i) {
        const auto& m = meshDebug[static_cast<std::size_t>(i)];
        const char* kindStr = m.kind == MeshMaterialKind::Body ? "Body" : (m.kind == MeshMaterialKind::Eye ? "Eye" : (m.kind == MeshMaterialKind::Teeth ? "Teeth" : "Unknown"));
        const char* attachName = m.attachedBoneName.empty() ? "-" : m.attachedBoneName.c_str();
        ImGui::Text(
            "%s | %s | a %s | p %.3f %.3f %.3f | c %.3f %.3f %.3f | e %.3f %.3f %.3f",
            m.name.c_str(),
            kindStr,
            attachName,
            m.pivotTranslation.x,
            m.pivotTranslation.y,
            m.pivotTranslation.z,
            m.centroid.x,
            m.centroid.y,
            m.centroid.z,
            m.boundsExtent.x,
            m.boundsExtent.y,
            m.boundsExtent.z
        );
    }

    ImGui::Separator();
    if (ImGui::Button("Fix eye orientation (+90 X)")) {
        const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const_cast<Model&>(shark).ApplyLocalRotationToMeshes("eye", rot);
        const_cast<Model&>(shark).ApplyLocalRotationToMeshes("teeth", rot);
    }
    ImGui::End();
}