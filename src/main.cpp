#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Camera.h"

#include <iostream>

namespace {
Camera g_camera;
bool g_keyDown[1024] = {};
bool g_firstMouseSample = true;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    if (g_firstMouseSample) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouseSample = false;
    }

    const auto deltaX = static_cast<float>(xpos - g_lastMouseX);
    const auto deltaY = static_cast<float>(ypos - g_lastMouseY);
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    g_camera.ProcessMouse(deltaX, deltaY);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            g_keyDown[key] = true;
        } else if (action == GLFW_RELEASE) {
            g_keyDown[key] = false;
        }
    }
}
}

// Callback function for handling window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW for OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS
#endif

    // 2. Create Window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "GRK: Underwater Scene", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 3. Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable Depth Testing for 3D rendering
    glEnable(GL_DEPTH_TEST);

    // 4. Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends for ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Clear color set to a deep underwater blue
    glClearColor(0.05f, 0.15f, 0.25f, 1.0f);

    auto lastFrameTime = static_cast<float>(glfwGetTime());
    float debugPrintAccumulator = 0.0f;

    // 5. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        const auto currentFrameTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // --- Input Processing ---
        g_camera.ProcessKeyboard(
            g_keyDown[GLFW_KEY_W],
            g_keyDown[GLFW_KEY_S],
            g_keyDown[GLFW_KEY_A],
            g_keyDown[GLFW_KEY_D],
            deltaTime
        );

        // Force view matrix evaluation so camera path is exercised each frame.
        (void)g_camera.GetViewMatrix();

        debugPrintAccumulator += deltaTime;
        if (debugPrintAccumulator >= 0.5f) {
            debugPrintAccumulator = 0.0f;
            const glm::vec3 position = g_camera.GetPosition();
            const glm::vec3 forward = g_camera.GetForward();
            const glm::vec3 up = g_camera.GetUp();
            const glm::vec3 right = g_camera.GetRight();

            std::cout
                << "Pos: (" << position.x << ", " << position.y << ", " << position.z << ")"
                << " | Forward: (" << forward.x << ", " << forward.y << ", " << forward.z << ")"
                << " | Up: (" << up.x << ", " << up.y << ", " << up.z << ")"
                << " | Right: (" << right.x << ", " << right.y << ", " << right.z << ")"
                << std::endl;
        }

        // --- Logic & Rendering ---
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Start ImGui Frame ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // UI Definition
        ImGui::Begin("Environment Setup");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, 
                    ImGui::GetIO().Framerate);
        ImGui::End();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // --- Swap buffers and poll IO events ---
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 6. Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}