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
    // Singleton dostarczajacy system animacji plywania rekina
    SharkSkeletonAnimator& GetSwimAnimator() {
        static SharkSkeletonAnimator animator;
        return animator;
    }

    // Funkcja nakladajaca animacje szkieletowa na model rekina i wysylajaca klatke kluczowa do shadera
    void ApplySharkSkinning(Shader& shader, Model& sharkModel, const SceneRenderContext& context) {
        auto& animator = GetSwimAnimator();
        static bool initialized = false;
        // Inicjalizacja kości rekina na podstawie hierarchii z pliku FBX
        if (!initialized) {
            animator.InitializeFromModel(sharkModel);
            initialized = true;
        }

        // Kalkulacja wychylenia kosci w zaleznosci od czasu animacji i krzywizny skretu na splajnie
        animator.ApplyPose(sharkModel, context.animationTimeSeconds, context.signedTurnCurvature);
        // Przeslanie macierzy transformacji dla kazdej kosci jako tablicy do shadera GPU
        sharkModel.UploadBoneTransforms(shader, true);
    }

    // Funkcja wylaczajaca sytem kosci (np. podczas rysowania obiektow statycznych jak dno lub lodz)
    void DisableSkinning(Shader& shader) {
        shader.setBool("uUseBoneSkinning", false);
    }
}

// Sprawdzenie nacisnietych klawiszy WASD i poruszenie kamera w trybie swobodnym
void UpdateCamera(AppState& appState, const float deltaTime) {
    // Zapobiegamy ruchowi, jesli uzytkownik obsluguje interfejs okienkowy UI (kursor myszy aktywny)
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

// Inicjacja nowych klatek ImGui na poczatku petli glownej
void BeginImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// Funkcja odpowiedzialna za przygotowanie wszystkich macierzy, zmiennych czasu 
// i parametrow dla obecnej klatki renderowania. Zgrupowanie ich w jeden obiekt Context.
SceneRenderContext BuildSceneRenderContext(
    AppState& appState,
    const SplinePath& sharkPath,
    const float currentFrameTime
) {
    SceneRenderContext context{};

    // Obliczenie proporcji ekranu i macierzy rzutowania perspektywicznego
    const float aspect = static_cast<float>(appState.windowWidth) / static_cast<float>(appState.windowHeight);
    context.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f);

    // Macierz widoku bazujaca na aktualnej pozycji i orientacji (kwaterniony) kamery
    context.view = appState.camera.GetViewMatrix();
    context.cameraPosition = appState.camera.GetPosition();

    // Obliczanie czasu na splajnie dla rekina 
    const float splineTime = std::fmod(appState.sharkVirtualSplineTime / 42.0f, 1.0f);
    context.animationTimeSeconds = appState.sharkVirtualAnimTime;
    context.globalTimeSeconds = currentFrameTime;
    context.splineTime = splineTime;

    // Obliczanie wektora krzywizny w danym punkcie splajnu, co pozwala np. zgiac kregoslup rekina na zakrecie
    context.signedTurnCurvature = sharkPath.GetSignedCurvature(splineTime);
    context.sharkModelMatrix = sharkPath.GetTransform(splineTime);

    // Obracanie i skalowanie modelu rekina, aby dopasowac go do sceny
    context.sharkModelMatrix = glm::rotate(context.sharkModelMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    context.sharkModelMatrix = glm::scale(context.sharkModelMatrix, glm::vec3(0.05f));

    appState.currentSharkModelMatrix = context.sharkModelMatrix;

    // Pozycjonowanie dna morskiego
    context.floorMatrix = glm::mat4(1.0f);
    context.floorMatrix = glm::translate(context.floorMatrix, glm::vec3(0.0f, -15.0f, 0.0f));
    context.floorMatrix = glm::scale(context.floorMatrix, glm::vec3(10.0f));

    // ==== OBLICZANIE MACIERZY CIENI DLA SWIATLA KIERUNKOWEGO (SLONCA) ====
    glm::vec3 lightTarget(0.0f);
    glm::vec3 lightDir = glm::normalize(appState.dirLightDirection);
    glm::vec3 lightPos = lightTarget - lightDir * 30.0f;

    // Przeciwdzialanie Gimbal Lock dla wektora Up swiatla w przestrzeni cieni
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, upVector)) > 0.999f) {
        upVector = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // Swiatlo sloneczne uzywa rzutu ortograficznego
    const glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 100.0f);
    const glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, upVector);
    context.sunLightMatrix = lightProjection * lightView;

    // ==== OBLICZANIE MACIERZY CIENI DLA SWIATLA PUNKTOWEGO/REFLEKTORA ====
    // Spotlight jest zawieszony lekko pod i za kamera
    context.spotDirection = appState.camera.GetForward();
    context.spotPosition = context.cameraPosition - 0.5f * appState.camera.GetForward() - 2.0f * appState.camera.GetUp();

    glm::vec3 up = appState.camera.GetUp();

    // Obliczenie pola widzenia (FOV) reflektora w zaleznosci od ostrosci zewnetrznego stozka oswietlenia
    const float clampedOuterCutoff = std::clamp(appState.outerCutoff, 1.0f, 89.0f);
    const float spotFovDegrees = std::clamp(std::max(60.0f, clampedOuterCutoff * 2.0f + 18.0f), 45.0f, 120.0f);

    // Reflektor uzywa rzutu perspektywicznego
    glm::mat4 spotProjection = glm::perspective(glm::radians(spotFovDegrees), 1.0f, 2.0f, 200.0f);
    glm::mat4 spotView = glm::lookAt(context.spotPosition, context.spotPosition + context.spotDirection, up);

    context.spotLightMatrix = spotProjection * spotView;
    context.submarineMatrix = appState.currentSubmarineMatrix;

    return context;
}

// Alias dla przebiegu generowania cieni dla Slonca
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

// Funkcja przygotowujaca mape glebokosci dla swiatla slonecznego (kierunkowego)
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
    // Podlaczenie macierzy swiatla, za pomoca ktorej wierzcholki sa transformowane by znalezc ich pozycje wzgledem swiatla
    shadowShader.setMat4("lightSpaceMatrix", context.sunLightMatrix);

    // Ustawienie rozdzielczosci portu na rozmiar mapy cieni
    glViewport(0, 0, static_cast<GLsizei>(sunShadow.width), static_cast<GLsizei>(sunShadow.height));
    // Podpiecie Framebuffer'a tekstury Shadow Map
    glBindFramebuffer(GL_FRAMEBUFFER, sunShadow.framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Rysowanie modeli do bufora z-depth
    shadowShader.setMat4("model", context.sharkModelMatrix);
    ApplySharkSkinning(shadowShader, shark, context);
    shark.Draw(shadowShader, nullptr);

    shadowShader.setMat4("model", context.submarineMatrix);
    DisableSkinning(shadowShader);
    submarine.Draw(shadowShader, nullptr);

    // Odpiecie Framebuffer'a (powrot do domyslnego okna renderowania)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Funkcja przygotowujaca mape glebokosci dla reflektora lodzi podwodnej
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

// Glowny Pass renderowania kolorow z technologia PBR (Physically Based Rendering)
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
    // Reset wymiarow okna na glowne rozdzielczosci aplikacji
    glViewport(0, 0, appState.windowWidth, appState.windowHeight);

    // Tlo dla sceny (kolor mgly/wody) ustawione jako tlo dla glebi czyszczenia
    glClearColor(appState.waterColor.r, appState.waterColor.g, appState.waterColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pbrShader.use();
    // Podpinanie macierzy kamery
    pbrShader.setMat4("projection", context.projection);
    pbrShader.setMat4("view", context.view);
    pbrShader.setVec3("camPos", context.cameraPosition);

    // Macierze swiatel do porownan cieni wewnatrz PBR shadera
    pbrShader.setMat4("sunLightMatrix", context.sunLightMatrix);
    pbrShader.setMat4("spotLightMatrix", context.spotLightMatrix);

    // Flagi debugowania przekazane do shadera z panela UI
    pbrShader.setBool("uDebugMaterialKindView", appState.debugMaterialKindView);
    pbrShader.setBool("uDebugRawAlbedoView", appState.debugRawAlbedoView);
    pbrShader.setBool("uDebugSpotOnlyView", appState.debugSpotOnlyView);

    // Podpinanie i aktywacja tekstur rekina do odpowiednich slotow na GPU
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

    // Przeslanie gotowych map cieni do shadera (dla testu ShadowCalculation zewnatrz shadera)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sunShadow.depthMap);
    pbrShader.setInt("sunShadowMap", 1);

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, spotShadow.depthMap);
    pbrShader.setInt("spotShadowMap", 9);

    // Przekazanie parametrow Swiatla Kierunkowego
    pbrShader.setVec3("dirLightDirection", appState.dirLightDirection);
    pbrShader.setVec3("dirLightColor", appState.dirLightColor);
    pbrShader.setFloat("dirLightIntensity", appState.dirLightIntensity);

    // Przekazanie parametrow atmosfery oceanu 
    pbrShader.setVec3("waterColor", appState.waterColor);
    pbrShader.setFloat("fogDensity", appState.fogDensity);

    // Przekazanie parametrow Swiatla Reflektora lodzi
    pbrShader.setVec3("spotPosition", context.spotPosition);
    pbrShader.setVec3("spotDirection", context.spotDirection);
    pbrShader.setBool("spotEnabled", appState.spotlightEnabled);
    pbrShader.setVec3("spotColor", appState.spotColor);
    pbrShader.setFloat("spotIntensity", appState.spotIntensity);
    pbrShader.setBool("uDebugSpotShadowCompareView", appState.debugSpotShadowCompareView);
    pbrShader.setBool("uDebugSpotCenterProbeView", appState.debugSpotCenterProbeView);

    // Przeliczenie katow odciecia stozka swiatla na wartosci trygonometryczne przydatne w glsl
    const float clampedInnerCutoff = std::clamp(appState.innerCutoff, 1.0f, 89.0f);
    const float clampedOuterCutoff = std::clamp(appState.outerCutoff, clampedInnerCutoff, 89.0f);
    pbrShader.setFloat("innerCutoff", glm::cos(glm::radians(clampedInnerCutoff)));
    pbrShader.setFloat("outerCutoff", glm::cos(glm::radians(clampedOuterCutoff)));

    // Przekazanie czasu globalnego i sily "flow" do symulacji animowanej wody na dnie (FlowMap)
    pbrShader.setFloat("uTime", context.globalTimeSeconds);
    pbrShader.setFloat("flowMapIntensity", appState.flowMapIntensity);

    // Konfiguracja mikro-detali skory rekina
    pbrShader.setBool("useDetailNormalMap", appState.sharkUseDetailNormal);
    pbrShader.setFloat("detailNormalStrength", appState.sharkDetailNormalStrength);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, textures.sharkDetailNormal);
    pbrShader.setInt("detailNormalMap", 4);

    // Domyslne wartosci Materialu PBR z panela dla ciala rekina
    pbrShader.setVec3("albedo", appState.albedoColor);
    pbrShader.setFloat("ao", appState.ambientOcclusion);
    pbrShader.setFloat("metallic", appState.metallic);
    pbrShader.setFloat("roughness", appState.roughness);

    pbrShader.setVec3("teethColor", appState.teethColor);
    pbrShader.setFloat("teethMetallic", appState.teethMetallic);
    pbrShader.setFloat("teethRoughness", appState.teethRoughness);

    // === 1. Renderowanie Rekina ===
    pbrShader.setMat4("model", context.sharkModelMatrix);
    ApplySharkSkinning(pbrShader, shark, context); // Wlacz sytem deformacji szkieletu

    pbrShader.setBool("useFlowMap", false);
    shark.Draw(pbrShader, &appState);

    // === 2. Renderowanie Podwodnego Dna (Seabed) ===
    pbrShader.setMat4("model", context.floorMatrix);
    DisableSkinning(pbrShader); // Ziemia nie posiada kosci
    pbrShader.setBool("useDetailNormalMap", false);

    // Nadpisanie wlasciwosci PBR pod piaszczyste i zarośniete podloze (Brak metalicznosci)
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

    // Aktywacja wczytanej FlowMappy odpowiedzialnej ze iluzje plywacego piasku
    pbrShader.setBool("useFlowMap", true);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, textures.seabedFlowMap);
    pbrShader.setInt("flowMap", 10);

    seabed.Draw(pbrShader);

    // === 3. Renderowanie Lodzi Podwodnej (Submarine) ===
    pbrShader.setMat4("model", context.submarineMatrix);
    DisableSkinning(pbrShader);
    pbrShader.setBool("useDetailNormalMap", false);

    // Zmiana wlasciwosci PBR uzywajac zmiennych dedykowanych z interfejsu 
    pbrShader.setFloat("metallic", appState.subMetallic);
    pbrShader.setFloat("roughness", appState.subRoughness);
    pbrShader.setVec3("albedo", appState.subAlbedoColor);
    pbrShader.setFloat("ao", 1.0f);

    pbrShader.setBool("useAlbedoMap", true);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textures.submarineAlbedo);
    pbrShader.setInt("albedoMap", 3);

    // Model lodzi nie posiada wlasnej tekstury NormalMap oraz nie podlega pod FlowMap
    pbrShader.setBool("useNormalMap", false);
    pbrShader.setBool("useFlowMap", false);

    submarine.Draw(pbrShader, nullptr);
}

// Renderowanie tla - szescianu mapowanego na sferyczne widoki glebin oceanu
void RenderSkyboxPass(const SceneRenderContext& context, Shader& skyboxShader, Skybox& skybox) {
    skyboxShader.use();
    // Odfiltrowanie translacji w macierzy widoku, dzieki czemu skybox na zawsze wydaje sie byc 'w nieskonczonosci'
    skyboxShader.setMat4("view", context.view);
    skyboxShader.setMat4("projection", context.projection);
    skybox.Draw(skyboxShader);
}

// Kod debugowy - Renderuje sciezke punktow splajnu jako linie proste, co pomaga sprawdzic jak wezly wygladaja z kamery
void RenderTrajectoryDebug(const SceneRenderContext& context, Shader& linesShader,
    const TrajectoryDebugBuffers& buffers) {
    if (buffers.normalLineCount == 0 && buffers.binormalLineCount == 0 && buffers.pathLineCount == 0) {
        return;
    }

    // Wylaczenie bufora testu glebi, aby upewnic sie, ze linie sa widoczne zawsze (przenikaja modele)
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glLineWidth(3.0f);

    linesShader.use();
    linesShader.setMat4("projection", context.projection);
    linesShader.setMat4("view", context.view);

    // Rysowanie Normalnych
    if (buffers.normalLineCount > 0) {
        glBindVertexArray(buffers.normalVAO);
        linesShader.setVec3("lineColor", glm::vec3(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(buffers.normalLineCount));
    }

    // Rysowanie Binormalnych (Bi-Tangensow) do orientacji Splajnu
    if (buffers.binormalLineCount > 0) {
        glBindVertexArray(buffers.binormalVAO);
        linesShader.setVec3("lineColor", glm::vec3(1.0f, 0.0f, 1.0f));
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(buffers.binormalLineCount));
    }

    // Rysowanie calej glownej sciezki Splajnu (laczenie wezlow)
    if (buffers.pathLineCount > 0) {
        glBindVertexArray(buffers.pathVAO);
        linesShader.setVec3("lineColor", glm::vec3(0.0f, 1.0f, 1.0f));
        const GLenum pathDrawMode = buffers.closedLoop ? GL_LINE_LOOP : GL_LINE_STRIP;
        glDrawArrays(pathDrawMode, 0, static_cast<GLsizei>(buffers.pathLineCount));
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);

    // Przywrocenie stanu rysowania glebi przed powrotem do glownych procesow
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

// Glowny UI panel do sterowania wartosciami w calym ekosystemie programu
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
    ImGui::Text("Tryb kamery: %s", appState.isExcursionMode ? "EXCURSION (Submarine)" : "FREE (WASD)");
    ImGui::Text("Kursor: %s", appState.cursorDisabled ? "LOCKED" : "FREE (UI Active)");

    ImGui::Separator();
    ImGui::Text("=== Srodowisko (Environment) ===");
    ImGui::SliderFloat("Prady wodne (Flow Map)", &appState.flowMapIntensity, 0.0f, 1.5f);
    ImGui::SliderFloat("Gestosc mgly (Fog)", &appState.fogDensity, 0.0f, 0.05f, "%.4f");
    ImGui::ColorEdit3("Kolor wody", &appState.waterColor[0]);
    ImGui::Spacing();
    ImGui::SliderFloat3("Kierunek slonca", &appState.dirLightDirection[0], -1.0f, 1.0f);
    ImGui::SliderFloat("Sila slonca", &appState.dirLightIntensity, 0.0f, 30.0f);

    ImGui::Separator();
    ImGui::Text("=== Rekin (Shark) ===");
    ImGui::Checkbox("Pokaz trajektorie rekina (Shark Spline)", &appState.showSharkSpline);
    ImGui::ColorEdit3("Body Tint", &appState.albedoColor[0]);
    ImGui::SliderFloat("Metallic (Metalness)", &appState.metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &appState.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Ambient Occlusion", &appState.ambientOcclusion, 0.0f, 1.0f);
    ImGui::Checkbox("Use Detail Normal", &appState.sharkUseDetailNormal);
    ImGui::SliderFloat("Detail Normal Strength", &appState.sharkDetailNormalStrength, 0.0f, 0.5f);

    ImGui::Separator();
    ImGui::Text("=== Lodz podwodna (Submarine) ===");
    ImGui::Checkbox("Pokaz trajektorie lodzi (Sub Spline)", &appState.showSubmarineSpline);
    ImGui::ColorEdit3("Sub Tint", &appState.subAlbedoColor[0]);
    ImGui::SliderFloat("Sub Metallic", &appState.subMetallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Sub Roughness", &appState.subRoughness, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::Text("Reflektor (Spotlight):");
    ImGui::Checkbox("Wlacz reflektor (F)", &appState.spotlightEnabled);
    ImGui::ColorEdit3("Spot Color", &appState.spotColor[0]);
    ImGui::SliderFloat("Spot Intensity", &appState.spotIntensity, 0.0f, 50000.0f);
    ImGui::SliderFloat("Inner Cutoff (deg)", &appState.innerCutoff, 1.0f, 89.0f);
    ImGui::SliderFloat("Outer Cutoff (deg)", &appState.outerCutoff, 1.0f, 89.0f);

    ImGui::Separator();
    ImGui::Text("=== Diagnostyka (Diagnostics) ===");
    ImGui::Checkbox("Debug Material Kind Colors", &appState.debugMaterialKindView);
    ImGui::Checkbox("Debug Raw Albedo", &appState.debugRawAlbedoView);
    ImGui::Checkbox("Debug Spotlight Only", &appState.debugSpotOnlyView);
    ImGui::Checkbox("Debug Spot Shadow Map", &appState.debugSpotShadowMapView);
    ImGui::Checkbox("Debug Spot Shadow Compare", &appState.debugSpotShadowCompareView);
    ImGui::Checkbox("Debug Spot Center Probe", &appState.debugSpotCenterProbeView);

    // Wizualizacja samej tekstury cieni zbuforowanej podczas fazy renderu Spotlight
    if (appState.debugSpotShadowMapView) {
        ImGui::Text("Spot shadow depth texture:");
        ImGui::Image(
            reinterpret_cast<ImTextureID>(static_cast<intptr_t>(spotShadow.depthMap)),
            ImVec2(256.0f, 256.0f),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f)
        );
        ImGui::Text("If this looks almost white, the depth map is empty.");
    }

    // Wizualizacja obu map cieni obok siebie (Slonce vs Reflektor)
    if (appState.debugSpotShadowCompareView) {
        ImGui::Separator();
        ImGui::Text("Spot shadow compare:");
        ImGui::Text("R = sun visibility, G = spot visibility, B = spot cone factor");
        ImGui::Image(
            reinterpret_cast<ImTextureID>(static_cast<intptr_t>(sunShadow.depthMap)),
            ImVec2(180.0f, 180.0f),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f)
        );
        ImGui::SameLine();
        ImGui::Image(
            reinterpret_cast<ImTextureID>(static_cast<intptr_t>(spotShadow.depthMap)),
            ImVec2(180.0f, 180.0f),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f)
        );
    }

    if (appState.debugSpotCenterProbeView) {
        ImGui::Separator();
        ImGui::Text("Spot center probe:");
        ImGui::Text("R = spot shadow amount, G = visibility, B = cone factor");
        ImGui::Text("This is the center-fragment diagnostic for the spotlight shadow term.");
    }

    ImGui::Separator();
    // Odczytywanie aktualnego FPS (Frames Per Second) wprost z obiektu IO we framereightcie
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
}