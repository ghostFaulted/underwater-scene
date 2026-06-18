#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vector>
#include <stb_image.h>

#include "Camera.h"
#include "Model.h"
#include "SplinePath.h"
#include "Shader.h"
#include "Skybox.h"

#include <filesystem>
#include <glm/gtx/quaternion.hpp>

namespace {
	Camera g_camera(glm::vec3(0.0f, 5.0f, 25.0f));
	bool g_keyDown[1024] = {};
	bool g_firstMouseSample = true;
	double g_lastMouseX = 0.0;
	double g_lastMouseY = 0.0;

	int g_windowWidth = 1280;
	int g_windowHeight = 720;

	glm::vec3 ui_albedoColor = glm::vec3(0.4f, 0.45f, 0.5f);
	float ui_ambientOcclusion = 1.0f;
	float ui_roughness = 0.25f;
	float ui_metallic = 0.9f;

	float ui_flowMapIntensity = 0.5f;
	bool ui_submarineLights = true;
	bool g_cursorDisabled = true;

	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
		(void)window;
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

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		(void)scancode;
		(void)mods;
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

        if (key == GLFW_KEY_C && action == GLFW_PRESS) {
            g_cursorDisabled = !g_cursorDisabled;
            if (g_cursorDisabled) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

		if (key == GLFW_KEY_F && action == GLFW_PRESS) {
			ui_submarineLights = !ui_submarineLights;
		}

		if (key >= 0 && key < 1024) {
			if (action == GLFW_PRESS) g_keyDown[key] = true;
			else if (action == GLFW_RELEASE) g_keyDown[key] = false;
		}
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	g_windowWidth = width;
	g_windowHeight = height;
}

unsigned int loadTexture(const char* path) {
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 4);
	if (data) {
		GLenum format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else {
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
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

	GLFWwindow* window = glfwCreateWindow(1280, 720, "GRK: PBR + Camera", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
	glEnable(GL_DEPTH_TEST);

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	const auto resolveShaderPath = [](const std::string& relPath) {
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

	const std::string shadowVertPath = resolveShaderPath("shaders/shadow.vert");
	const std::string shadowFragPath = resolveShaderPath("shaders/shadow.frag");
	Shader shadowShader(shadowVertPath.c_str(), shadowFragPath.c_str());
	std::cout << "[DEBUG] Shaders loaded successfully" << std::endl;

	std::vector<std::string> faces = {
		"assets/skybox/uw_rt.jpg",
		"assets/skybox/uw_lf.jpg",
		"assets/skybox/uw_up.jpg",
		"assets/skybox/uw_dn.jpg",
		"assets/skybox/uw_bk.jpg",
		"assets/skybox/uw_ft.jpg"
	};
	Skybox skybox(faces);

	Model submarine("assets/submarine/sub.obj");
	Model seabed("assets/ocean_floor/ocean_floor.obj");

	unsigned int subAlbedoTex = loadTexture("assets/submarine/ger_sub_diffuse.png");
	unsigned int seabedAlbedoTex = loadTexture("assets/ocean_floor/model.jpg");
	unsigned int seabedNormalMap = loadTexture("assets/ocean_floor/seabed_normal.png");

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
	  auto pos = glm::vec3(transform[3]);

	  auto binormal = glm::vec3(transform[0]);
	  binormalLineVertices.push_back(pos);
	  binormalLineVertices.push_back(pos + binormal * 0.5f);

	  auto normal = glm::vec3(transform[1]);
	  normalLineVertices.push_back(pos);
	  normalLineVertices.push_back(pos + normal * 0.5f);
	}

	normalLineCount = static_cast<unsigned int>(normalLineVertices.size());
	binormalLineCount = static_cast<unsigned int>(binormalLineVertices.size());

	const auto uploadLineBuffer = [](const std::vector<glm::vec3>& vertices, unsigned int& vao, unsigned int& vbo) {
	  glGenVertexArrays(1, &vao);
	  glGenBuffers(1, &vbo);

	  glBindVertexArray(vao);
	  glBindBuffer(GL_ARRAY_BUFFER, vbo);
	  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	  glEnableVertexAttribArray(0);
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	  glBindVertexArray(0);
	};

	uploadLineBuffer(normalLineVertices, normalVAO, normalVBO);
	uploadLineBuffer(binormalLineVertices, binormalVAO, binormalVBO);
  }

  std::cout << "[NORMAL] VAO=" << normalVAO << " vertices=" << normalLineCount << std::endl;
  std::cout << "[BINORMAL] VAO=" << binormalVAO << " vertices=" << binormalLineCount << std::endl;

	unsigned int pathVAO = 0, pathVBO = 0;
	unsigned int pathLineCount = 0;

	{
		constexpr int splinePointsNumber = 500;
		std::vector<glm::vec3> pathLineVertices;
		pathLineVertices.reserve(splinePointsNumber);

		const int sampleCount = useClosedTrajectory ? splinePointsNumber : splinePointsNumber + 1;
		for (int i = 0; i < sampleCount; ++i) {
			const float t = static_cast<float>(i) / static_cast<float>(splinePointsNumber);
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	std::cout << "[PATH] VAO=" << pathVAO << " vertices=" << pathLineCount << std::endl;

	auto lastFrameTime = static_cast<float>(glfwGetTime());

	const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	while (!glfwWindowShouldClose(window)) {
		const auto currentFrameTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		if (g_cursorDisabled) {
			g_camera.ProcessKeyboard(g_keyDown[GLFW_KEY_W], g_keyDown[GLFW_KEY_S], g_keyDown[GLFW_KEY_A], g_keyDown[GLFW_KEY_D], deltaTime);
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float aspect = static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
		glm::mat4 view = g_camera.GetViewMatrix();
		glm::vec3 cameraPos = g_camera.GetPosition();

		float splineTime = fmodf(currentFrameTime / 20.0f, 1.0f);
		glm::mat4 subModelMatrix = submarinePath.GetTransform(splineTime);
		subModelMatrix = glm::rotate(subModelMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 floorMatrix(1.0f);
		floorMatrix = glm::translate(floorMatrix, glm::vec3(0.0f, -15.0f, 0.0f));
		floorMatrix = glm::scale(floorMatrix, glm::vec3(10.0f));

		glm::vec3 lightPos = lightPositions[0];
		glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 100.0f);
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		shadowShader.use();
		shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		shadowShader.setMat4("model", subModelMatrix);
		submarine.Draw(shadowShader);

		shadowShader.setMat4("model", floorMatrix);
		seabed.Draw(shadowShader);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, g_windowWidth, g_windowHeight);
		glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pbrShader.use();
		pbrShader.setMat4("projection", projection);
		pbrShader.setMat4("view", view);
		pbrShader.setVec3("camPos", cameraPos);

		pbrShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		pbrShader.setInt("shadowMap", 1);

		for (unsigned int i = 0; i < 4; ++i) {
			pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
			glm::vec3 currentLightColor = ui_submarineLights ? lightColors[i] : glm::vec3(0.0f);
			pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", currentLightColor);
		}

		pbrShader.setBool("useAlbedoMap", true);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, subAlbedoTex);
		pbrShader.setInt("albedoMap", 3);

		pbrShader.setBool("useNormalMap", false);
		pbrShader.setVec3("albedo", ui_albedoColor);
		pbrShader.setFloat("ao", ui_ambientOcclusion);
		pbrShader.setFloat("metallic", ui_metallic);
		pbrShader.setFloat("roughness", ui_roughness);
		pbrShader.setMat4("model", subModelMatrix);

		submarine.Draw(pbrShader);

		pbrShader.setMat4("model", floorMatrix);
		pbrShader.setFloat("metallic", 0.0f);
		pbrShader.setFloat("roughness", 0.9f);
		pbrShader.setFloat("ao", 1.0f);

		pbrShader.setBool("useAlbedoMap", true);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, seabedAlbedoTex);
		pbrShader.setInt("albedoMap", 3);

		pbrShader.setBool("useNormalMap", true);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, seabedNormalMap);
		pbrShader.setInt("normalMap", 2);

		seabed.Draw(pbrShader);

		skyboxShader.use();
		skyboxShader.setMat4("view", view);
		skyboxShader.setMat4("projection", projection);
		skybox.Draw(skyboxShader);

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
		ImGui::Begin("GRK: Interactive U-Boat Diorama");
		ImGui::Separator();
		ImGui::Text("System Controls");
		ImGui::Text("Press 'C' to toggle Cursor lock.");
		ImGui::Text("Cursor state: %s", g_cursorDisabled ? "LOCKED (Camera)" : "FREE (UI Active)");
		ImGui::Separator();
		ImGui::Text("Submarine PBR Materials");
		ImGui::ColorEdit3("Steel Albedo", &ui_albedoColor[0]);
		ImGui::SliderFloat("Metallic (Metalness)", &ui_metallic, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &ui_roughness, 0.0f, 1.0f);
		ImGui::SliderFloat("Ambient Occlusion", &ui_ambientOcclusion, 0.0f, 1.0f);
		ImGui::Separator();
		ImGui::Text("Environment (A14 / Fog)");
		ImGui::SliderFloat("Current Intensity", &ui_flowMapIntensity, 0.0f, 2.0f);
		ImGui::Separator();
		ImGui::Text("Submarine controls");
		ImGui::Checkbox("Toggle Lights (F)", &ui_submarineLights);
		ImGui::Separator();
		ImGui::Text("Diagnostics");
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
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