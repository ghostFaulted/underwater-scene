#include "AppCallbacks.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "AppState.h"
#include "Raycaster.h"

namespace {
    AppState* GetAppState(GLFWwindow* window) {
        return static_cast<AppState*>(glfwGetWindowUserPointer(window));
    }
}

void cursor_position_callback(GLFWwindow* window, const double xpos, const double ypos) {
    AppState* appState = GetAppState(window);
    if (appState == nullptr) {
        return;
    }

    if (!appState->cursorDisabled) {
        appState->firstMouseSample = true;
        return;
    }

    if (appState->firstMouseSample) {
        appState->lastMouseX = xpos;
        appState->lastMouseY = ypos;
        appState->firstMouseSample = false;
    }

    const auto deltaX = static_cast<float>(xpos - appState->lastMouseX);
    const auto deltaY = static_cast<float>(ypos - appState->lastMouseY);
    appState->lastMouseX = xpos;
    appState->lastMouseY = ypos;
    appState->camera.ProcessMouse(deltaX, deltaY);
}

void key_callback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
    (void)scancode;
    (void)mods;

    AppState* appState = GetAppState(window);
    if (appState == nullptr) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        appState->cursorDisabled = !appState->cursorDisabled;
        glfwSetInputMode(window, GLFW_CURSOR, appState->cursorDisabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        appState->spotlightEnabled = !appState->spotlightEnabled;
    }


    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            appState->keyDown[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            appState->keyDown[key] = false;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, const int width, const int height) {
    glViewport(0, 0, width, height);

    AppState* appState = GetAppState(window);
    if (appState == nullptr) {
        return;
    }

    appState->windowWidth = width;
    appState->windowHeight = height;
}

void mouse_button_callback(GLFWwindow* window, const int button, const int action, const int mods) {
    (void)mods;
    AppState* appState = GetAppState(window);
    if (appState == nullptr) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !appState->cursorDisabled) {

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        const float aspect = static_cast<float>(appState->windowWidth) / static_cast<float>(appState->windowHeight);
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        const glm::mat4 view = appState->camera.GetViewMatrix();

        Ray worldRay = Raycaster::ScreenToWorldRay(mouseX, mouseY, appState->windowWidth, appState->windowHeight, view, projection);

        glm::mat4 invModel = glm::inverse(appState->currentSharkModelMatrix);

        Ray localRay;
        localRay.origin = glm::vec3(invModel * glm::vec4(worldRay.origin, 1.0f));
        localRay.direction = glm::normalize(glm::vec3(invModel * glm::vec4(worldRay.direction, 0.0f)));

        float hitDistance = 0.0f;
        if (Raycaster::IntersectAABB(localRay, appState->sharkLocalAABB, hitDistance)) {
            appState->sharkAngerTimer = 4.0f;
            std::cout << "[RAYCAST] HIT! Shark is now fleeing. Hit distance (local): " << hitDistance << std::endl;
        }
    }
}