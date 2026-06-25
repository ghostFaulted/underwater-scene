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
	GLFWwindow* CreateWindow() {
		return glfwCreateWindow(1280, 720, "GRK: Shark + Spline + Skinning", nullptr, nullptr);
	}

	bool InitializeOpenGL(GLFWwindow* window, AppState& appState) {
		glfwMakeContextCurrent(window);
		glfwSetWindowUserPointer(window, &appState);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
		glfwSetCursorPosCallback(window, cursor_position_callback);
		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
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

	Model shark(ResolveProjectPath("assets/shark/shark.fbx"));
	Model seabed(ResolveProjectPath("assets/ocean_floor/ocean_floor.obj"));
	Model submarine(ResolveProjectPath("assets/submarine/sub.obj"));

	auto textures = LoadTextures();

	std::cout << "[DEBUG] Project root resolved from: " << ResolveProjectPath("") << std::endl;

	const bool useClosedTrajectory = true;
	auto sharkPath = SplinePath(GenerateControlPoints(), useClosedTrajectory);
	auto trajectoryDebugBuffers = CreateTrajectoryDebugBuffers(sharkPath, useClosedTrajectory);

	auto submarinePath = SplinePath(GenerateSubmarinePath(), true);

	auto lastFrameTime = static_cast<float>(glfwGetTime());
	auto sunShadow = CreateShadowMapResources(2048, 2048);
	auto spotShadow = CreateShadowMapResources(4096, 4096);

	while (!glfwWindowShouldClose(window)) {
		const auto currentFrameTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		float currentSpeedMultiplier = 1.0f;
		float currentAnimMultiplier = 1.0f;

		if (appState.sharkAngerTimer > 0.0f) {
			appState.sharkAngerTimer -= deltaTime;
			currentSpeedMultiplier = 3.5f;
			currentAnimMultiplier = 4.0f;
		}

		appState.sharkVirtualSplineTime += deltaTime * currentSpeedMultiplier;
		appState.sharkVirtualAnimTime += deltaTime * currentAnimMultiplier;

		appState.excursionVirtualTime += deltaTime * 0.5f;
		float subSplineTime = std::fmod(appState.excursionVirtualTime / 30.0f, 1.0f);

		glm::mat4 rawSubTransform = submarinePath.GetTransform(subSplineTime);

		glm::vec3 subPos = glm::vec3(rawSubTransform[3]); 
		glm::vec3 splineForward = glm::normalize(glm::vec3(rawSubTransform[2])); 

		glm::vec3 flatForward = glm::normalize(glm::vec3(splineForward.x, 0.0f, splineForward.z));

		glm::vec3 realisticForward = glm::normalize(glm::mix(flatForward, splineForward, 0.15f));

		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 subRight = glm::normalize(glm::cross(worldUp, realisticForward));
		glm::vec3 subUp = glm::cross(realisticForward, subRight);

		glm::mat4 subTransform = glm::mat4(1.0f);
		subTransform[0] = glm::vec4(subRight, 0.0f);        
		subTransform[1] = glm::vec4(subUp, 0.0f);           
		subTransform[2] = glm::vec4(realisticForward, 0.0f); 
		subTransform[3] = glm::vec4(subPos, 1.0f);        

		subTransform = glm::scale(subTransform, glm::vec3(5.0f));

		appState.currentSubmarineMatrix = subTransform;

		if (appState.isExcursionMode) {
			glm::vec3 cameraOffset = glm::vec3(0.0f, 6.0f, 0.0f);
			appState.camera.SetPosition(subPos + cameraOffset);
		}
		else {
			UpdateCamera(appState, deltaTime);
		}

		BeginImGuiFrame();

		const auto frameContext = BuildSceneRenderContext(appState, sharkPath, currentFrameTime);

		RenderSunShadowPass(frameContext, shaders.shadow, appState, shark, seabed, submarine, sunShadow);
		if (appState.spotlightEnabled || appState.debugSpotShadowMapView) {
			RenderSpotShadowPass(frameContext, shaders.shadow, appState, shark, seabed, submarine, spotShadow);
		}

		RenderPbrScene(frameContext, appState, shaders.pbr, shark, seabed, submarine, textures, sunShadow, spotShadow);
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