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
		return glfwCreateWindow(1280, 720, "GRK: Shark + Spline + Skinning", nullptr, nullptr);
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

	Model shark("assets/shark/shark.fbx");
	Model seabed("assets/ocean_floor/ocean_floor.obj");

	auto textures = LoadTextures();

	std::cout << std::filesystem::current_path() << std::endl;

	const bool useClosedTrajectory = true;
	auto sharkPath = SplinePath(GenerateControlPoints(), useClosedTrajectory);
	auto trajectoryDebugBuffers = CreateTrajectoryDebugBuffers(sharkPath, useClosedTrajectory);

	auto lastFrameTime = static_cast<float>(glfwGetTime());
	// Separate shadow maps for sun (directional) and spotlight
	auto sunShadow = CreateShadowMapResources(2048, 2048);
	auto spotShadow = CreateShadowMapResources(4096, 4096);

	while (!glfwWindowShouldClose(window)) {
		const auto currentFrameTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		UpdateCamera(appState, deltaTime);
		BeginImGuiFrame();

		const auto frameContext = BuildSceneRenderContext(appState, sharkPath, currentFrameTime);

		// Render shadow maps: sun (directional) and camera spotlight
		RenderSunShadowPass(frameContext, shaders.shadow, appState, shark, seabed, sunShadow);
		if (appState.spotlightEnabled || appState.debugSpotShadowMapView) {
			RenderSpotShadowPass(frameContext, shaders.shadow, appState, shark, seabed, spotShadow);
		}

		// Render PBR scene with both shadow maps
		RenderPbrScene(frameContext, appState, shaders.pbr, shark, seabed, textures, sunShadow, spotShadow);
		RenderSkyboxPass(frameContext, shaders.skybox, skybox);
		RenderTrajectoryDebug(frameContext, shaders.lines, trajectoryDebugBuffers);
		RenderControlPanel(appState, shark, frameContext.signedTurnCurvature, sunShadow, spotShadow);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ShutdownImGui();
	DestroyTrajectoryDebugBuffers(trajectoryDebugBuffers);
	DestroyShadowMapResources(sunShadow);
	DestroyShadowMapResources(spotShadow);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}