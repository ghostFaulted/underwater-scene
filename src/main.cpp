#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vector>
#include "Camera.h"
#include "Model.h"
#include "SplinePath.h"
#include "Shader.h"
#include "Skybox.h"

#include <filesystem>
#include <glm/gtx/quaternion.hpp>

namespace {
    Camera g_camera(glm::vec3(0.0f, 0.0f, 18.0f));
    bool g_keyDown[1024] = {};
    bool g_firstMouseSample = true;
    double g_lastMouseX = 0.0;
    double g_lastMouseY = 0.0;

    // Window dimensions state for dynamic projection matrix calculation
    int g_windowWidth = 1280;
    int g_windowHeight = 720;

    // UI and interaction states
    glm::vec3 ui_albedoColor = glm::vec3(0.5f, 0.0f, 0.0f);
    float ui_ambientOcclusion = 1.0f;
    float ui_flowMapIntensity = 0.5f;
    bool ui_submarineLights = true;
    bool g_cursorDisabled = true; // Tracks whether the cursor is locked for camera movement

    void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
        (void) window;
        // Only rotate camera if the cursor is captured
        if (!g_cursorDisabled) {
            g_firstMouseSample = true;
            return;
        }

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

    void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        (void) scancode;
        (void) mods;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Toggle cursor mode (Capture / Free) via 'C' key
        if (key == GLFW_KEY_C && action == GLFW_PRESS) {
            g_cursorDisabled = !g_cursorDisabled;
            if (g_cursorDisabled) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        // Toggle lights via 'F' key (Interaction 2)
        if (key == GLFW_KEY_F && action == GLFW_PRESS) {
            ui_submarineLights = !ui_submarineLights;
        }

        if (key >= 0 && key < 1024) {
            if (action == GLFW_PRESS) g_keyDown[key] = true;
            else if (action == GLFW_RELEASE) g_keyDown[key] = false;
        }
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    g_windowWidth = width;
    g_windowHeight = height;
}

unsigned int sphereVAO = 0;
unsigned int indexCount;

void renderSphere() {
    if (sphereVAO == 0) {
        glGenVertexArrays(1, &sphereVAO);
        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
                float xSegment = (float) x / (float) X_SEGMENTS;
                float ySegment = (float) y / (float) Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
            if (!oddRow) {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            } else {
                for (int x = X_SEGMENTS; x >= 0; --x) {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i) {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0) {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (normals.size() > 0) {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void *) (5 * sizeof(float)));
    }
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

std::vector<glm::vec3> GenerateControlPoints() {
    std::vector<glm::vec3> controlPoints = {
        {0.0f, -2.0f, 0.0f},

        {5.0f, -1.0f, -5.0f},
        {10.0f, -4.0f, -10.0f},
        {15.0f, -8.0f, -15.0f},

        {20.0f, -12.0f, -10.0f},
        {25.0f, -15.0f, 0.0f},
        {20.0f, -12.0f, 10.0f},

        {10.0f, -6.0f, 20.0f},
        {0.0f, -2.0f, 25.0f},
        {-10.0f, -5.0f, 20.0f},

        {-20.0f, -10.0f, 10.0f},
        {-25.0f, -14.0f, 0.0f},
        {-20.0f, -10.0f, -10.0f},

        {-10.0f, -4.0f, -20.0f},
        {0.0f, 0.0f, -25.0f},

        {10.0f, 3.0f, -15.0f},
        {15.0f, 6.0f, -5.0f},
        {10.0f, 4.0f, 5.0f},

        {5.0f, 1.0f, 8.0f},
        {2.0f, -1.0f, 4.0f}
    };
    return controlPoints;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "GRK: PBR + Camera", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    // Initial cursor capture setup
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    const auto resolveShaderPath = [](const std::string &relPath) {
        if (std::filesystem::exists(relPath)) return relPath;
        const std::string parentPath = "../" + relPath;
        if (std::filesystem::exists(parentPath)) return parentPath;
        return relPath;
    };

    const std::string pbrVertPath = resolveShaderPath("shaders/pbr.vert");
    const std::string pbrFragPath = resolveShaderPath("shaders/pbr.frag");
    const std::string skyVertPath = resolveShaderPath("shaders/skybox.vert");
    const std::string skyFragPath = resolveShaderPath("shaders/skybox.frag");
    const std::string lineVertPath = resolveShaderPath("shaders/lines.vert");
    const std::string lineFragPath = resolveShaderPath("shaders/lines.frag");

    Shader pbrShader(pbrVertPath.c_str(), pbrFragPath.c_str());
    Shader skyboxShader(skyVertPath.c_str(), skyFragPath.c_str());
    Shader linesShader(lineVertPath.c_str(), lineFragPath.c_str());

    std::cout << "[DEBUG] Shaders loaded successfully" << std::endl;

    std::vector<std::string> faces = {
        "right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"
    };
    Skybox skybox(faces);

    glm::vec3 lightPositions[] = {
        glm::vec3(-10.0f, 10.0f, 10.0f), glm::vec3(10.0f, 10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f), glm::vec3(10.0f, -10.0f, 10.0f)
    };
    glm::vec3 lightColors[] = {
        glm::vec3(300.0f), glm::vec3(300.0f), glm::vec3(300.0f), glm::vec3(300.0f)
    };

    std::cout << std::filesystem::current_path() << std::endl;
    Model submarine("../models/submarine.obj");

    const bool useClosedTrajectory = true;
    auto submarinePath = SplinePath(GenerateControlPoints(), useClosedTrajectory);

    unsigned int normalVAO = 0, normalVBO = 0;
    unsigned int normalLineCount = 0;
    unsigned int binormalVAO = 0, binormalVBO = 0;
    unsigned int binormalLineCount = 0;
    {
        constexpr int splinePointsNumber = 1000;
        std::vector<glm::vec3> binormalLineVertices;
        binormalLineVertices.reserve(splinePointsNumber * 2);
        std::vector<glm::vec3> normalLineVertices;
        normalLineVertices.reserve(splinePointsNumber * 2);


        for (float t = 0.0f; t <= 1.0f; t += 1.0f / splinePointsNumber) {
            auto transform = submarinePath.GetTransform(t);
            glm::vec3 pos = glm::vec3(transform[3]);

            glm::vec3 binormal = glm::vec3(transform[0]);
            binormalLineVertices.push_back(pos);
            binormalLineVertices.push_back(pos + binormal * 0.5f);

            glm::vec3 normal = glm::vec3(transform[1]);
            normalLineVertices.push_back(pos);
            normalLineVertices.push_back(pos + normal * 0.5f);
        }

        normalLineCount = static_cast<unsigned int>(normalLineVertices.size());
        binormalLineCount = static_cast<unsigned int>(binormalLineVertices.size());

        const auto uploadLineBuffer = [](const std::vector<glm::vec3> &vertices, unsigned int &vao, unsigned int &vbo) {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *) 0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        };

        uploadLineBuffer(normalLineVertices, normalVAO, normalVBO);
        uploadLineBuffer(binormalLineVertices, binormalVAO, binormalVBO);
    }

    std::cout << "[NORMAL] VAO=" << normalVAO << " vertices=" << normalLineCount << std::endl;
    std::cout << "[BINORMAL] VAO=" << binormalVAO << " vertices=" << binormalLineCount << std::endl;

    // ======================================================================================
    // Create VAO/VBO for path line (connection points)
    // ======================================================================================
    unsigned int pathVAO = 0, pathVBO = 0;
    unsigned int pathLineCount = 0;

    {
        constexpr int splinePointsNumber = 500;
        std::vector<glm::vec3> pathLineVertices;
        pathLineVertices.reserve(splinePointsNumber);

        for (float t = 0.0f; t <= 1.0f; t += 1.0f / splinePointsNumber) {
            auto transform = submarinePath.GetTransform(t);
            pathLineVertices.push_back(glm::vec3(transform[3]));
        }

        pathLineCount = pathLineVertices.size();

        glGenVertexArrays(1, &pathVAO);
        glGenBuffers(1, &pathVBO);

        glBindVertexArray(pathVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
        glBufferData(GL_ARRAY_BUFFER, pathLineVertices.size() * sizeof(glm::vec3), pathLineVertices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *) 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    std::cout << "[PATH] VAO=" << pathVAO << " vertices=" << pathLineCount << std::endl;

    auto lastFrameTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        const auto currentFrameTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // Process movement only if the cursor is captured
        if (g_cursorDisabled) {
            g_camera.ProcessKeyboard(g_keyDown[GLFW_KEY_W], g_keyDown[GLFW_KEY_S], g_keyDown[GLFW_KEY_A],
                                     g_keyDown[GLFW_KEY_D], deltaTime);
        }

        glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Calculate dynamic projection based on current window dimensions
        float aspect = static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = g_camera.GetViewMatrix();
        glm::vec3 cameraPos = g_camera.GetPosition();

        // ======================================================================================
        // Render scene
        // ======================================================================================
        pbrShader.use();
        pbrShader.setMat4("projection", projection);
        pbrShader.setMat4("view", view);
        pbrShader.setVec3("camPos", cameraPos);

        // Upload live UI PBR variables to shader on every frame
        pbrShader.setVec3("albedo", ui_albedoColor);
        pbrShader.setFloat("ao", ui_ambientOcclusion);

        for (unsigned int i = 0; i < 4; ++i) {
            pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
            // If submarine lights are toggled off, dim the light colors to zero
            glm::vec3 currentLightColor = ui_submarineLights ? lightColors[i] : glm::vec3(0.0f);
            pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", currentLightColor);
        }

        int nrRows = 5, nrColumns = 5;
        float spacing = 2.5;
        for (int row = 0; row < nrRows; ++row) {
            pbrShader.setFloat("metallic", (float) row / (float) nrRows);
            for (int col = 0; col < nrColumns; ++col) {
                pbrShader.setFloat("roughness", glm::clamp((float) col / (float) nrColumns, 0.05f, 1.0f));
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3((col - (nrColumns / 2)) * spacing,
                                                        (row - (nrRows / 2)) * spacing, 0.0f));
                pbrShader.setMat4("model", model);
                renderSphere();
            }
        }

        // Calculate normalized time for submarine movement along the spline [0, 1]
        float splineTime = fmodf(lastFrameTime / 20.0f, 1.0f);

        glm::mat4 model = glm::mat4(1.0f);

        // Get transform from spline (position + PTF basis)
        model *= submarinePath.GetTransform(splineTime);

        // Rotate submarine model to align with spline tangent
        // If torpedo model looks along -Z, rotate -90 degrees around Y
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Scale submarine
        model = glm::scale(model, glm::vec3(1.0f));

        pbrShader.setFloat("metallic", 0.3f);
        pbrShader.setFloat("roughness", 0.7f);
        pbrShader.setMat4("model", model);

        submarine.Draw(pbrShader);

        skyboxShader.use();
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        skybox.Draw(skyboxShader);

        // ======================================================================================
        // Render trajectory as overlay (always visible)
        // ======================================================================================
        if (normalLineCount > 0 || binormalLineCount > 0 || pathLineCount > 0) {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glLineWidth(3.0f);

            linesShader.use();
            linesShader.setMat4("projection", projection);
            linesShader.setMat4("view", view);

            if (normalLineCount > 0) {
                glBindVertexArray(normalVAO);
                linesShader.setVec3("lineColor", glm::vec3(0.0f, 1.0f, 0.0f));
                glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(normalLineCount));
            }

            if (binormalLineCount > 0) {
                glBindVertexArray(binormalVAO);
                linesShader.setVec3("lineColor", glm::vec3(1.0f, 0.0f, 1.0f));
                glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(binormalLineCount));
            }

            if (pathLineCount > 0) {
                glBindVertexArray(pathVAO);
                linesShader.setVec3("lineColor", glm::vec3(0.0f, 1.0f, 1.0f));
                const GLenum pathDrawMode = useClosedTrajectory ? GL_LINE_LOOP : GL_LINE_STRIP;
                glDrawArrays(pathDrawMode, 0, static_cast<GLsizei>(pathLineCount));
            }

            glBindVertexArray(0);
            glLineWidth(1.0f);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
        }

        // ImGui Window rendering
        ImGui::Begin("Combined Test");

        ImGui::Separator();
        ImGui::Text("System Controls");
        ImGui::Text("Press 'C' to toggle Cursor lock.");
        ImGui::Text("Cursor state: %s", g_cursorDisabled ? "LOCKED (Camera)" : "FREE (UI Active)");

        ImGui::Separator();
        ImGui::Text("PBR Materials");
        ImGui::ColorEdit3("Albedo Base", &ui_albedoColor[0]);
        ImGui::SliderFloat("Ambient Occlusion", &ui_ambientOcclusion, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Environment (A14 / Fog)");
        ImGui::SliderFloat("Current Intensity", &ui_flowMapIntensity, 0.0f, 2.0f);

        ImGui::Separator();
        ImGui::Text("Submarine controls");
        ImGui::Checkbox("Toggle Lights (F)", &ui_submarineLights);

        ImGui::Separator();
        ImGui::Text("Trajectory Visualization");
        ImGui::Text("Mode: %s", useClosedTrajectory ? "Closed" : "Open");
        ImGui::Text("Cyan line: Spline path");
        ImGui::Text("Green lines: Normal vectors");
        ImGui::Text("Magenta lines: Binormal vectors");

        ImGui::Separator();
        ImGui::Text("Diagnostics");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Spline Time: %.3f", splineTime);
        ImGui::Text("Use Mouse to look around, WASD to move.");
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup trajectory buffers
    glDeleteBuffers(1, &normalVBO);
    glDeleteVertexArrays(1, &normalVAO);
    glDeleteBuffers(1, &binormalVBO);
    glDeleteVertexArrays(1, &binormalVAO);
    glDeleteBuffers(1, &pathVBO);
    glDeleteVertexArrays(1, &pathVAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
