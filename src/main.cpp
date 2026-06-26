#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppCallbacks.h"
#include "AppState.h"
#include "Model.h"
#include "SceneRenderer.h"
#include "SceneResources.h"
#include "SplinePath.h"
#include "Skybox.h"

#include <iostream>

namespace {
    // Funkcja tworzaca glowne okno aplikacji za pomoca GLFW
    GLFWwindow* CreateWindow() {
        return glfwCreateWindow(1280, 720, "GRK: Underwater scene", nullptr, nullptr);
    }

    // Inicjalizacja kontekstu OpenGL, ustawienie callbackow wejscia (klawiatura, mysz)
    bool InitializeOpenGL(GLFWwindow* window, AppState& appState) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Wlaczenie V-Sync

        // Przypisanie struktury stanu aplikacji (AppState) do okna, 
        // aby moc z niej korzystac wewnatrz funkcji callback
        glfwSetWindowUserPointer(window, &appState);

        // Rejestracja funkcji obslugujacych zdarzenia z okna i wejscia
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);

        // Ukrycie kursora na starcie programu (kamera swobodna)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Ladowanie wskaznikow na funkcje OpenGL za pomoca GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            return false;
        }

        // Wlaczenie testowania glebokosci (Z-buffer), niezbedne w 3D
        glEnable(GL_DEPTH_TEST);
        return true;
    }

    // Inicjalizacja biblioteki ImGui uzywanej do interfejsu uzytkownika
    void InitializeImGui(GLFWwindow* window) {
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    // Sprzatanie zasobow po ImGui przed zamknieciem programu
    void ShutdownImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

int main() {
    // Inicjalizacja GLFW
    if (!glfwInit()) return -1;
    // Wymuszenie wersji OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Glowny stan aplikacji przechowujacy zmienne sterujace renderingiem i logika
    AppState appState;
    GLFWwindow* window = CreateWindow();
    if (!window) {
        glfwTerminate();
        return -1;
    }

    if (!InitializeOpenGL(window, appState)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    InitializeImGui(window);

    // Wczytanie shaderow z dysku i kompilacja (PBR, skybox, rysowanie linii, cienie)
    auto shaders = LoadShaders();

    // Utworzenie skyboxa (podwodnego tla) na podstawie 6 tekstur scian kubemapy
    Skybox skybox(GetSkyboxFaces());

    // Wczytanie modeli 3D uzywajac Assimp
    Model shark(ResolveProjectPath("assets/shark/shark.fbx"));
    Model seabed(ResolveProjectPath("assets/ocean_floor/ocean_floor.obj"));
    Model submarine(ResolveProjectPath("assets/submarine/sub.obj"));

    // Wczytanie wszystkich tekstur potrzebnych dla modeli
    auto textures = LoadTextures();

    // Utworzenie sciezki splajnowej dla rekina (Catmull-Rom)
    auto sharkPath = SplinePath(GenerateControlPoints(), true);
    // Utworzenie buforow z wierzcholkami do rysowania linii pomocniczych (debug)
    auto trajectoryDebugBuffers = CreateTrajectoryDebugBuffers(sharkPath, true);

    // Utworzenie sciezki splajnowej dla lodzi podwodnej
    auto submarinePath = SplinePath(GenerateSubmarinePath(), true);
    auto subTrajectoryDebugBuffers = CreateTrajectoryDebugBuffers(submarinePath, true);

    auto lastFrameTime = static_cast<float>(glfwGetTime());

    // Utworzenie buforow ramki (FBO) i tekstur glebokosci dla map cieni (Shadow Maps)
    auto sunShadow = CreateShadowMapResources(1024, 1024);
    auto spotShadow = CreateShadowMapResources(1024, 1024);

    // Glowna petla programu renderujaca klatki
    while (!glfwWindowShouldClose(window)) {
        // Obliczanie czasu trwania ostatniej klatki (Delta Time)
        const auto currentFrameTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // Mnozniki predkosci ruchu i animacji rekina (normalne tempo to 1.0)
        float currentSpeedMultiplier = 1.0f;
        float currentAnimMultiplier = 1.0f;

        // Jesli rekin zostal klikniety (Raycast) i jest "zly" / przestraszony
        if (appState.sharkAngerTimer > 0.0f) {
            appState.sharkAngerTimer -= deltaTime;
            // Przyspieszenie ruchu wzdluz splajnu i predkosci machania ogonem
            currentSpeedMultiplier = 3.5f;
            currentAnimMultiplier = 4.0f;
        }

        // Aktualizacja wirtualnego czasu dla ruchu i animacji
        appState.sharkVirtualSplineTime += deltaTime * currentSpeedMultiplier;
        appState.sharkVirtualAnimTime += deltaTime * currentAnimMultiplier;

        // Obliczenie wlasciwego momentu na splajnie rekina w przedziale [0.0, 1.0]
        float sharkSplineTime = std::fmod(appState.sharkVirtualSplineTime / 42.0f, 1.0f);
        // Pobranie pozycji rekina ze splajnu (dla ewentualnych testow/debugu)
        glm::vec3 sharkPos = glm::vec3(sharkPath.GetTransform(sharkSplineTime)[3]);

        // Obliczenia ruchu lodzi podwodnej wzdluz jej splajnu
        appState.excursionVirtualTime += deltaTime * 0.5f;
        float subSplineTime = std::fmod(appState.excursionVirtualTime / 30.0f, 1.0f);
        glm::mat4 rawSubTransform = submarinePath.GetTransform(subSplineTime);

        // Ekstrakcja danych z macierzy splajnu (pozycja, orientacja przodu)
        glm::vec3 subPos = glm::vec3(rawSubTransform[3]);
        glm::vec3 splineForward = glm::normalize(glm::vec3(rawSubTransform[2]));

        // Ustabilizowanie orientacji lodzi, by uniknac nienaturalnych przechylow (pitch/roll)
        glm::vec3 flatForward = glm::normalize(glm::vec3(splineForward.x, 0.0f, splineForward.z));
        glm::vec3 realisticForward = glm::normalize(glm::mix(flatForward, splineForward, 0.15f));

        // Rekonstrukcja wektorow bazowych (Up i Right) dla lodzi
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 subRight = glm::normalize(glm::cross(worldUp, realisticForward));
        glm::vec3 subUp = glm::cross(realisticForward, subRight);

        // Tworzenie koncowej macierzy transformacji dla modelu lodzi podwodnej
        glm::mat4 subTransform = glm::mat4(1.0f);
        subTransform[0] = glm::vec4(subRight, 0.0f);
        subTransform[1] = glm::vec4(subUp, 0.0f);
        subTransform[2] = glm::vec4(realisticForward, 0.0f);
        subTransform[3] = glm::vec4(subPos, 1.0f);

        // Zwiekszenie skali lodzi podwodnej
        subTransform = glm::scale(subTransform, glm::vec3(5.0f));
        appState.currentSubmarineMatrix = subTransform;

        // Ustawianie zachowania kamery zaleznie od trybu
        if (appState.isExcursionMode) {
            // W trybie "Wycieczki" kamera sztywno podaza nad lodzia podwodna
            glm::vec3 cameraOffset = glm::vec3(0.0f, 6.0f, 0.0f);
            appState.camera.SetPosition(subPos + cameraOffset);
        }
        else {
            // W trybie "Free" uzytkownik lata po scenie uzywajac klawiatury
            UpdateCamera(appState, deltaTime);
        }

        // Rozpoczecie budowania klatki interfejsu uzytkownika ImGui
        BeginImGuiFrame();

        // Budowanie struktury SceneRenderContext z niezbednymi macierzami i parametrami 
        const auto frameContext = BuildSceneRenderContext(appState, sharkPath, currentFrameTime);

        // Przebieg pierwszy: generowanie mapy cieni (Shadow Map) z kierunkowego swiatla (slonca)
        RenderSunShadowPass(frameContext, shaders.shadow, appState, shark, seabed, submarine, sunShadow);

        // Przebieg dodatkowy: generowanie mapy cieni z punktowego swiatla lodzi (jesli wlaczone)
        if (appState.spotlightEnabled || appState.debugSpotShadowMapView) {
            RenderSpotShadowPass(frameContext, shaders.shadow, appState, shark, seabed, submarine, spotShadow);
        }

        // Przebieg glowny: renderowanie sceny ze wszystkimi efektami materialow PBR i oswietlenia
        RenderPbrScene(frameContext, appState, shaders.pbr, shark, seabed, submarine, textures, sunShadow, spotShadow);

        // Renderowanie tła (nieba/wody) na koncu, dla poprawy wydajnosci (optymalizacja depth buffer)
        RenderSkyboxPass(frameContext, shaders.skybox, skybox);

        // Ewentualne renderowanie linii pomocniczych splajnow (wlaczone w UI)
        if (appState.showSharkSpline) RenderTrajectoryDebug(frameContext, shaders.lines, trajectoryDebugBuffers);
        if (appState.showSubmarineSpline) RenderTrajectoryDebug(frameContext, shaders.lines, subTrajectoryDebugBuffers);

        // Budowanie samego panelu sterujacego ImGui
        RenderControlPanel(appState, shark, frameContext.signedTurnCurvature, sunShadow, spotShadow);

        // Rysowanie poskladanego interfejsu na wierzchu sceny OpenGL
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Zamiana buforow (Double Buffering) i pobieranie zdarzen wejscia
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Sprzatanie pamieci i niszczenie struktur przed wylaczeniem programu
    ShutdownImGui();
    DestroyTrajectoryDebugBuffers(trajectoryDebugBuffers);
    DestroyTrajectoryDebugBuffers(subTrajectoryDebugBuffers);
    DestroyShadowMapResources(sunShadow);
    DestroyShadowMapResources(spotShadow);

    // Zamykanie okna i zamykanie procesow podsystemu GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}