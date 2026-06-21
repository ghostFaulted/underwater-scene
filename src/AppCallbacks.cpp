#include "AppCallbacks.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "AppState.h"

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
        appState->sharkLights = !appState->sharkLights;
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

