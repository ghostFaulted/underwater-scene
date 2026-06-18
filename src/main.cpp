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

#include <filesystem>
#include <iostream>

namespace {
	GLFWwindow* CreateWindow() {
		return glfwCreateWindow(1280, 720, "GRK: PBR + Camera", nullptr, nullptr);
	}

	bool InitializeOpenGL(GLFWwindow* window, AppState& appState) {
		glfwMakeContextCurrent(window);
		glfwSetWindowUserPointer(window, &appState);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
		glfwSetCursorPosCallback(window, cursor_position_callback);
		glfwSetKeyCallback(window, key_callback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			return false;
		}

		glEnable(GL_DEPTH_TEST);
		return true;
	}

	void InitializeImGui(GLFWwindow* window) {
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
	}

	void ShutdownImGui() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

int main() {
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	auto shaders = LoadShaders();
	Skybox skybox(GetSkyboxFaces());

	Model submarine("assets/submarine/sub.obj");
	Model seabed("assets/ocean_floor/ocean_floor.obj");

	auto textures = LoadTextures();

	glm::vec3 lightPositions[] = {
		glm::vec3(-20.0f,  20.0f, 20.0f), glm::vec3(20.0f,  20.0f, 20.0f),
		glm::vec3(-20.0f, -20.0f, 20.0f), glm::vec3(20.0f, -20.0f, 20.0f)
	};
	glm::vec3 lightColors[] = {
		glm::vec3(500.0f), glm::vec3(500.0f), glm::vec3(500.0f), glm::vec3(500.0f)
	};

	std::cout << std::filesystem::current_path() << std::endl;

	const bool useClosedTrajectory = true;
	auto submarinePath = SplinePath(GenerateControlPoints(), useClosedTrajectory);
	auto trajectoryDebugBuffers = CreateTrajectoryDebugBuffers(submarinePath, useClosedTrajectory);

	auto lastFrameTime = static_cast<float>(glfwGetTime());
	auto shadowMap = CreateShadowMapResources(2048, 2048);

	while (!glfwWindowShouldClose(window)) {
		const auto currentFrameTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		UpdateCamera(appState, deltaTime);
		BeginImGuiFrame();

		const auto frameContext = BuildSceneRenderContext(appState, submarinePath, lightPositions[0], currentFrameTime);

		RenderShadowPass(frameContext, shaders.shadow, submarine, seabed, shadowMap);
		RenderPbrScene(frameContext, appState, shaders.pbr, submarine, seabed, textures, shadowMap, lightPositions, lightColors, 4);
		RenderSkyboxPass(frameContext, shaders.skybox, skybox);
		RenderTrajectoryDebug(frameContext, shaders.lines, trajectoryDebugBuffers);
		RenderControlPanel(appState);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ShutdownImGui();
	DestroyTrajectoryDebugBuffers(trajectoryDebugBuffers);
	DestroyShadowMapResources(shadowMap);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}