#include "AppCallbacks.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "AppState.h"
#include "Raycaster.h"

namespace {
    // Funkcja pomocnicza do pobierania glownego stanu aplikacji z uchwytu okna GLFW.
    // Zostalo to ustawione wczesniej za pomoca glfwSetWindowUserPointer.
    AppState* GetAppState(GLFWwindow* window) {
        return static_cast<AppState*>(glfwGetWindowUserPointer(window));
    }
}

// Obsluga ruchu myszy - sterowanie obrotem kamery.
void cursor_position_callback(GLFWwindow* window, const double xpos, const double ypos) {
    AppState* appState = GetAppState(window);
    if (appState == nullptr) {
        return;
    }

    // Jesli kursor jest odblokowany (tryb UI), nie obracamy kamery.
    if (!appState->cursorDisabled) {
        appState->firstMouseSample = true;
        return;
    }

    // Zapobieganie gwaltownemu skokowi kamery przy pierwszym przejeciu kursora.
    if (appState->firstMouseSample) {
        appState->lastMouseX = xpos;
        appState->lastMouseY = ypos;
        appState->firstMouseSample = false;
    }

    // Obliczanie przesuniecia (delty) myszy wzgledem poprzedniej klatki
    const auto deltaX = static_cast<float>(xpos - appState->lastMouseX);
    const auto deltaY = static_cast<float>(ypos - appState->lastMouseY);
    appState->lastMouseX = xpos;
    appState->lastMouseY = ypos;

    // Przekazanie przesuniecia do klasy kamery
    appState->camera.ProcessMouse(deltaX, deltaY);
}

// Obsluga klawiatury - pojedyncze wcisniecia klawiszy (toggle) oraz zapisywanie stanu ciaglego.
void key_callback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
    (void)scancode;
    (void)mods;

    AppState* appState = GetAppState(window);
    if (appState == nullptr) {
        return;
    }

    // Wyjscie z programu po wcisnieciu ESCAPE
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Przelaczanie trybu kursora klawiszem 'C' (blokada do latania / odblokowanie dla interfejsu)
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        appState->cursorDisabled = !appState->cursorDisabled;
        glfwSetInputMode(window, GLFW_CURSOR, appState->cursorDisabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    // Wlaczanie/wylaczanie reflektora (spotlight) lodzi podwodnej klawiszem 'F'
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        appState->spotlightEnabled = !appState->spotlightEnabled;
    }

    // Przelaczanie trybu kamery (wycieczka/swobodna) klawiszem 'E'
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        appState->isExcursionMode = !appState->isExcursionMode;
    }

    // Zapisywanie stanu wcisnietych klawiszy do tablicy (potrzebne do plynnego ruchu WASD)
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            appState->keyDown[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            appState->keyDown[key] = false;
        }
    }
}

// Aktualizacja wymiarow Viewportu w OpenGL podczas zmiany wielkosci okna Windowsa
void framebuffer_size_callback(GLFWwindow* window, const int width, const int height) {
    glViewport(0, 0, width, height);

    AppState* appState = GetAppState(window);
    if (appState == nullptr) {
        return;
    }

    appState->windowWidth = width;
    appState->windowHeight = height;
}

// Obsluga klikniec myszki - Raycasting do wykyrywania trafien w obiekty (rekin)
void mouse_button_callback(GLFWwindow* window, const int button, const int action, const int mods) {
    (void)mods;
    AppState* appState = GetAppState(window);
    if (appState == nullptr) return;

    // Detekcja klikniecia Lewego Przycisku Myszy w trybie odblokowanego kursora
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !appState->cursorDisabled) {

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Rekonstrukcja macierzy widoku i projekcji do rzutowania promienia
        const float aspect = static_cast<float>(appState->windowWidth) / static_cast<float>(appState->windowHeight);
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f);
        const glm::mat4 view = appState->camera.GetViewMatrix();

        // Wygenerowanie promienia (Ray) startujacego z kamery i przechodzacego przez klikniety piksel ekranu
        Ray worldRay = Raycaster::ScreenToWorldRay(mouseX, mouseY, appState->windowWidth, appState->windowHeight, view, projection);

        // Macierz odwrotna modelu rekina do transformacji promienia ze swiata do przestrzeni lokalnej rekina
        glm::mat4 invModel = glm::inverse(appState->currentSharkModelMatrix);

        Ray localRay;
        localRay.origin = glm::vec3(invModel * glm::vec4(worldRay.origin, 1.0f));
        localRay.direction = glm::normalize(glm::vec3(invModel * glm::vec4(worldRay.direction, 0.0f)));

        // Sprawdzenie przeciecia promienia z obwiednia AABB rekina w jego lokalnym ukladzie wspolrzednych
        float hitDistance = 0.0f;
        if (Raycaster::IntersectAABB(localRay, appState->sharkLocalAABB, hitDistance)) {
            // Reakcja na klikniecie - rekin zaczyna szybciej plynac przez 4 sekundy
            appState->sharkAngerTimer = 4.0f;
            std::cout << "[RAYCAST] HIT! Shark is now fleeing. Hit distance (local): " << hitDistance << std::endl;
        }
    }
}