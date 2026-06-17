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
#include "SplinePath.h"
#include "Shader.h" 
#include "Skybox.h"
#include "Model.h"

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
		(void)scancode; (void)mods;
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (key == GLFW_KEY_C && action == GLFW_PRESS) {
			g_cursorDisabled = !g_cursorDisabled;
			if (g_cursorDisabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else {
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
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
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
			}
			else {
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
			data.push_back(positions[i].x); data.push_back(positions[i].y); data.push_back(positions[i].z);
			if (uv.size() > 0) { data.push_back(uv[i].x); data.push_back(uv[i].y); }
			if (normals.size() > 0) { data.push_back(normals[i].x); data.push_back(normals[i].y); data.push_back(normals[i].z); }
		}

		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		unsigned int stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

int main() {
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "GRK: U-Boat Diorama", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }

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

	Shader pbrShader("shaders/pbr.vert", "shaders/pbr.frag");
	Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

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

	std::vector<glm::vec3> splinePoints = {
		glm::vec3(-15.0f,  0.0f, -10.0f),
		glm::vec3(-10.0f,  3.0f,  10.0f),
		glm::vec3(0.0f, -2.0f,  15.0f),
		glm::vec3(10.0f,  4.0f,  10.0f),
		glm::vec3(15.0f,  0.0f, -10.0f),
		glm::vec3(0.0f, -4.0f, -20.0f),
		glm::vec3(-15.0f,  0.0f, -10.0f)
	};
	SplinePath spline(splinePoints);

	glm::vec3 lightPositions[] = {
		glm::vec3(-20.0f,  20.0f, 20.0f), glm::vec3(20.0f,  20.0f, 20.0f),
		glm::vec3(-20.0f, -20.0f, 20.0f), glm::vec3(20.0f, -20.0f, 20.0f)
	};
	glm::vec3 lightColors[] = {
		glm::vec3(500.0f), glm::vec3(500.0f), glm::vec3(500.0f), glm::vec3(500.0f)
	};

	auto lastFrameTime = static_cast<float>(glfwGetTime());

	while (!glfwWindowShouldClose(window)) {
		const auto currentFrameTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		if (g_cursorDisabled) {
			g_camera.ProcessKeyboard(g_keyDown[GLFW_KEY_W], g_keyDown[GLFW_KEY_S], g_keyDown[GLFW_KEY_A], g_keyDown[GLFW_KEY_D], deltaTime);
		}

		glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float aspect = static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
		glm::mat4 view = g_camera.GetViewMatrix();
		glm::vec3 cameraPos = g_camera.GetPosition();

		float splineSpeed = 0.025f;
		float t = glm::fract(currentFrameTime * splineSpeed);

		glm::mat4 origTransform = spline.GetTransform(t);
		glm::vec3 position = glm::vec3(origTransform[3]);
		glm::vec3 tangent = glm::vec3(origTransform[2]);

		glm::vec3 forward = glm::normalize(glm::vec3(tangent.x, 0.0f, tangent.z));
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::normalize(glm::cross(up, forward));

		glm::mat4 modelMatrix(1.0f);
		modelMatrix[0] = glm::vec4(right, 0.0f);
		modelMatrix[1] = glm::vec4(up, 0.0f);
		modelMatrix[2] = glm::vec4(forward, 0.0f);
		modelMatrix[3] = glm::vec4(position, 1.0f);

		float correctAngle = 0.0f;
		modelMatrix = glm::rotate(modelMatrix, glm::radians(correctAngle), glm::vec3(0.0f, 0.0f, 1.0f));

		pbrShader.use();
		pbrShader.setMat4("projection", projection);
		pbrShader.setMat4("view", view);
		pbrShader.setMat4("model", modelMatrix);
		pbrShader.setVec3("camPos", cameraPos);

		pbrShader.setVec3("albedo", ui_albedoColor);
		pbrShader.setFloat("ao", ui_ambientOcclusion);
		pbrShader.setFloat("metallic", ui_metallic);
		pbrShader.setFloat("roughness", ui_roughness);

		for (unsigned int i = 0; i < 4; ++i) {
			pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
			glm::vec3 currentLightColor = ui_submarineLights ? lightColors[i] : glm::vec3(0.0f);
			pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", currentLightColor);
		}

		submarine.Draw(pbrShader);

		skyboxShader.use();
		skyboxShader.setMat4("view", view);
		skyboxShader.setMat4("projection", projection);
		skybox.Draw(skyboxShader);

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
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}