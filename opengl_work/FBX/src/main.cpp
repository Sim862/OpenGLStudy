#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>
#include <iostream>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"	// 셰이더 유틸리티 클래스
#include "camera.h"
#include "model.h"

// [추가] ImGui 헤더
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// 콜백 함수 선언
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

GLFWwindow* initGLFWAndCreateWindow(int width, int height, const char* title);
bool initGLAD();
void setupCallbacks(GLFWwindow* window);
void initImGui(GLFWwindow* window);
void shutdownImGui();

void setupCubeData(unsigned int& VBO, unsigned int& cubeVAO, unsigned int& lightVAO);
unsigned int loadTexture2D(const char* path);

void buildImGuiUI();
void drawScene(Shader& lightingShader,
	Shader& lightCubeShader,
	unsigned int cubeVAO,
	unsigned int lightVAO,
	unsigned int diffuseMap,
	unsigned int specularMap,
	Model* model);

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
// 창 크기 상수
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;
bool firstMouse = true;       // 마우스가 처음 입력되었는지
//마우스 입력 관련 변수
float lastX = SCR_WIDTH / 2.0f; // 화면 중앙 X
float lastY = SCR_HEIGHT / 2.0f; // 화면 중앙 Y

// 프레임 시간
float deltaTime = 0.0f; // 현재 프레임과 마지막 프레임 사이의 시간
float lastFrame = 0.0f; // 마지막 프레임의 시간

// [추가] UI 모드 토글을 위한 전역 변수
bool g_UiMode = false;

// 광원과 조명 파라미터 (ImGui로 조절)
glm::vec3 gLightPos(1.2f, 1.0f, 2.0f);       // 광원 위치
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);     // 기본: 흰색 빛

float gAmbientStrength = 0.1f;  // 주변광 계수
float gDiffuseStrength = 1.0f;  // 난반사 계수
float gSpecularStrength = 0.5f;  // 정반사 계수

static bool gUsePerspective = true;

const int a = 10;

const glm::vec3 gCubePositions[10] = {
	glm::vec3(0.0f, 0.0f, -3.0f),
	glm::vec3(0.0f, 1.0f, -3.0f),
	glm::vec3(0.0f, 2.0f, -3.0f),
	glm::vec3(0.0f, 3.0f, -3.0f),
	glm::vec3(0.0f, 4.0f, -3.0f),
	glm::vec3(0.0f, 5.0f, -3.0f),
	glm::vec3(0.0f, 6.0f, -3.0f),
	glm::vec3(0.0f, 7.0f, -3.0f),
	glm::vec3(0.0f, 8.0f, -3.0f),
	glm::vec3(0.0f, 9.0f, -3.0f)
};

const unsigned int gCubeCount = sizeof(gCubePositions) / sizeof(glm::vec3);

// 프레임버퍼 크기 변경 콜백: 창이 리사이즈될 때 실제 렌더링 영역(뷰포트)도 맞춰줌
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{


	// UI 모드이면, 카메라 회전을 멈추고 마우스 콜백을 종료함
	if (g_UiMode)
	{
		firstMouse = true; // UI 모드에서 나올 때 마우스가 튀는 것을 방지
		return;
	}

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	// 마우스가 처음 창에 들어왔을 때, lastX/Y가 현재 마우스 위치로 점프하는 것을 방지
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;

		camera.MouseSensitivity = 0.01f;

		firstMouse = false;
	}

	// 이전 프레임과 현재 프레임의 마우스 이동량(offset) 계산
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // Y축은 위로 갈수록 값이 작아지므로 반대
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset, firstMouse);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yposIn) {

	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yposIn); // ImGui에게 스크롤 전달

	// ImGui 창이 마우스를 캡처(사용) 중이면, 카메라 줌을 막음
	if (ImGui::GetIO().WantCaptureMouse)
		return;


	camera.ProcessMouseScroll(yposIn);
}


void processInput(GLFWwindow* window)
{
	// UI 모드이면 카메라 이동(WASD) 및 종료(ESC) 키 입력을 막음
	if (g_UiMode)
		return;
	// (선택적) ImGui가 키보드를 사용 중이면 (e.g. 텍스트 필드) 막음
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// W (앞으로): cameraPos += (정면 방향 * 속도)
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	// S (뒤로): cameraPos -= (정면 방향 * 속도)
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	// (참고) 실제로는 cameraRight를 구한 뒤, 
	// cameraUp = glm::cross(cameraRight, cameraFront) 로 Up 벡터를 다시 계산해주는 것이
	// 카메라가 기울어지지 않게 하는(직교성을 유지하는) 좋은 방법임.
}

// [추가] ImGui는 키보드/마우스 '클릭' 콜백이 필요함
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods); // ImGui에게 키 전달

	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		g_UiMode = !g_UiMode; // UI 모드 토글

		if (g_UiMode) {
			// UI 모드: 마우스 커서를 보이게 함
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			// FPS 모드: 마우스 커서를 숨김
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods); // ImGui에게 마우스 버튼 전달

}

// GLFW 초기화 + Window 생성
GLFWwindow* initGLFWAndCreateWindow(int width, int height, const char* title)
{
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return nullptr;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(window);
	return window;
}

// GLAD 초기화
bool initGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}
	return true;
}

// 콜백 등록 및 초기 입력 모드 설정
void setupCallbacks(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// 초기엔 FPS 모드로 시작 (커서 숨김)
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

// ImGui 초기화
void initImGui(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// 우리가 콜백을 직접 전달할 것이므로 false
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init("#version 330");
}

// ImGui 종료
void shutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// ==========================================
// 데이터 설정: 정점/VAO/VBO, 광원 VAO
// ==========================================

void setupCubeData(unsigned int& VBO, unsigned int& cubeVAO, unsigned int& lightVAO)
{
	// position + normal + texcoord (8 floats)
	float vertices[] = {
		// positions          // normals           // texcoords
		// 뒤쪽 면 (z = -0.5, normal = (0,0,-1))
		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,

		// 앞쪽 면 (z = +0.5, normal = (0,0,1))
		-0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

		// 왼쪽 면 (x = -0.5, normal = (-1,0,0))
		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

		// 오른쪽 면 (x = +0.5, normal = (1,0,0))
		 0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

		 // 아래쪽 면 (y = -0.5, normal = (0,-1,0))
		 -0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,
		  0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
		  0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
		  0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
		 -0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
		 -0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,

		 // 위쪽 면 (y = +0.5, normal = (0,1,0))
		 -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
		  0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		  0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
		  0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
		 -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
		 -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
	};

	// 큐브용 VAO/VBO
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// position attribute (location = 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// normal attribute (location = 1)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// texcoord attribute (location = 2)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// 광원용 VAO (position만 사용)
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// VAO 해제
	glBindVertexArray(0);
}

unsigned int loadTexture2D(const char* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// wrapping / filtering 옵션
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // 이미지 상하 반전
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, format,
			width, height, 0, format,
			GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << path << std::endl;
		textureID = 0; // 실패 표시
	}
	stbi_image_free(data);

	return textureID;
}

void buildImGuiUI()
{
	if (!g_UiMode)
		return;

	ImGui::Begin("Camera Control");

	// 카메라 위치
	ImGui::DragFloat3("Position", glm::value_ptr(camera.Position), 0.01f);
	// Yaw / Pitch
	ImGui::DragFloat("Yaw", &camera.Yaw, 0.5f);
	ImGui::DragFloat("Pitch", &camera.Pitch, 0.5f, -89.0f, 89.0f);

	// FOV
	ImGui::DragFloat("Zoom (FOV)", &camera.Zoom, 0.1f, 1.0f, 90.0f);

	// 카메라 리셋
	if (ImGui::Button("Reset Camera")) {
		camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
	}

	ImGui::Separator();
	ImGui::Text("Lighting");

	// 광원 위치
	ImGui::DragFloat3("Light Pos", glm::value_ptr(gLightPos), 0.01f);

	// 광원 색상
	ImGui::ColorEdit3("Light Color", glm::value_ptr(gLightColor));

	// 각 계수
	ImGui::SliderFloat("Ambient", &gAmbientStrength, 0.0f, 1.0f);
	ImGui::SliderFloat("Diffuse", &gDiffuseStrength, 0.0f, 2.0f);
	ImGui::SliderFloat("Specular", &gSpecularStrength, 0.0f, 2.0f);

	if (ImGui::Button("Reset Light")) {
		gLightPos = glm::vec3(1.2f, 1.0f, 2.0f);
		gLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		gAmbientStrength = 0.1f;
		gDiffuseStrength = 1.0f;
		gSpecularStrength = 0.5f;
	}

	ImGui::End();
}

// 씬 렌더링 함수 (큐브 + 광원)
void drawScene(Shader& lightingShader,
	Shader& lightCubeShader,
	unsigned int cubeVAO,
	unsigned int lightVAO,
	unsigned int diffuseMap,
	unsigned int specularMap,
	Model* model)
{
	// 텍스처 바인딩
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularMap);

	// 카메라 변환 행렬
	glm::mat4 projection = glm::perspective(
		glm::radians(camera.Zoom),
		(float)SCR_WIDTH / (float)SCR_HEIGHT,
		0.1f, 100.0f
	);
	glm::mat4 view = camera.GetViewMatrix();

	// ===== 조명 쉐이더 설정 =====
	lightingShader.use();
	lightingShader.setMat4("projection", projection);
	lightingShader.setMat4("view", view);

	// 조명/재질 유니폼 설정
	lightingShader.setVec3("light.position", gLightPos);
	lightingShader.setVec3("viewPos", camera.Position);

	glm::vec3 diffuseColor = gLightColor * gDiffuseStrength;
	glm::vec3 ambientColor = diffuseColor * gAmbientStrength;
	glm::vec3 specularColor = gLightColor * gSpecularStrength;

	lightingShader.setVec3("light.ambient", ambientColor);
	lightingShader.setVec3("light.diffuse", diffuseColor);
	lightingShader.setVec3("light.specular", specularColor);

	lightingShader.setFloat("material.shininess", 32.0f);

	// 큐브 그리기
	glBindVertexArray(cubeVAO);

	for (unsigned int i = 0; i < gCubeCount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, gCubePositions[i]);
		float angle = 20.0f * i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		lightingShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// === 새로 추가: Assimp로 로딩한 3D 모델 그리기 ===
	if (model)
	{
		// 모델 변환 행렬 설정 (위치/스케일 조정용임)
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.75f, 0.0f));
		modelMat = glm::scale(modelMat, glm::vec3(0.2f)); // 모델 크기에 맞게 적당히 조절함

		lightingShader.setMat4("model", modelMat);

		// 조명/뷰 설정은 위에서 이미 해 두었으므로 그대로 사용함
		model->Draw(lightingShader);
	}

	// ===== 광원 큐브 렌더링 =====
	lightCubeShader.use();
	lightCubeShader.setMat4("projection", projection);
	lightCubeShader.setMat4("view", view);

	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, gLightPos);
	lightModel = glm::scale(lightModel, glm::vec3(0.2f));
	lightCubeShader.setMat4("model", lightModel);

	glBindVertexArray(lightVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

Assimp::Importer importer;
void Anim() {
	const aiScene* scene = importer.ReadFile("assets/models/human.fbx", aiProcess_Triangulate);
	if (!scene || !scene->mRootNode) {
		std::cout << "Failed to load animation: " << importer.GetErrorString() << std::endl;
		return;
	}

	// 애니메이션 데이터 확인
	std::cout << "Animations: " << scene->mNumAnimations << std::endl;

	if (scene->mNumAnimations > 0) {
		aiAnimation* anim = scene->mAnimations[0];
		std::cout << "Name: " << anim->mName.C_Str() << std::endl;
		std::cout << "Duration: " << anim->mDuration << std::endl;
		std::cout << "Ticks/sec: " << anim->mTicksPerSecond << std::endl;
		std::cout << "Channels: " << anim->mNumChannels << std::endl;  // 본 개수

		// 각 본의 키프레임 정보
		for (int i = 0; i < anim->mNumChannels; i++) {
			aiNodeAnim* channel = anim->mChannels[i];
			std::cout << "Bone: " << channel->mNodeName.C_Str() << std::endl;
			std::cout << "  Position keys: " << channel->mNumPositionKeys << std::endl;
			std::cout << "  Rotation keys: " << channel->mNumRotationKeys << std::endl;
			std::cout << "  Scale keys: " << channel->mNumScalingKeys << std::endl;
		}
	}
}

int main()
{
	// 1. GLFW + Window 생성
	GLFWwindow* window = initGLFWAndCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LightingMaps");
	if (!window) return -1;
	glfwSwapInterval(1);

	// 2. GLAD 초기화
	if (!initGLAD()) {
		glfwTerminate();
		return -1;
	}

	// 3. 콜백 등록 및 마우스 모드 설정
	setupCallbacks(window);

	// 4. ImGui 초기화
	initImGui(window);

	// 5. 쉐이더 생성
	Shader lightingShader("shaders/basic_lighting_tex.vert", "shaders/basic_lighting_tex.frag");
	Shader lightCubeShader("shaders/light_cube.vert", "shaders/light_cube.frag");

	// 6. 깊이버퍼 사용
	glEnable(GL_DEPTH_TEST);

	// 7. 정점 데이터, VAO/VBO, 광원용 VAO 설정
	unsigned int VBO = 0, cubeVAO = 0, lightVAO = 0;
	setupCubeData(VBO, cubeVAO, lightVAO);

	// 8. 텍스처 로드 (diffuse, specular)
	unsigned int diffuseMap = loadTexture2D("assets/textures/container2.png");
	unsigned int specularMap = loadTexture2D("assets/textures/container2_specular.png");

	if (diffuseMap == 0 || specularMap == 0) {
		std::cerr << "Texture load failed. Check assets paths. \n";
	}

	// 샘플러 유닛 연결(한 번만)
	lightCubeShader.use();;
	// [추가] 조명 셰이더용 재질 텍스처 유닛 연결
	lightCubeShader.setInt("material.diffuse", 0);
	lightCubeShader.setInt("material.specular", 1);
	Model model("assets/models/human.fbx");
	Anim();
	while (!glfwWindowShouldClose(window))
	{
		// 뷰포트 해상도 측정과 설정 (창 크기 변경 대비)
		glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		// 게임 루프 시작 부분에 추가
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// ImGui 새 프레임 시작
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 코드 (창 구성)
		buildImGuiUI();

		// 카메라 내부 벡터 재계산 트릭
		camera.ProcessMouseMovement(0.0f, 0.0f, true); // 내부 벡터 재계산 트릭

		// 화면/깊이버퍼 클리어
		glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 3D 씬(큐브 + 광원) 렌더링
		drawScene(lightingShader, lightCubeShader,
			cubeVAO, lightVAO,
			diffuseMap, specularMap, &model);


		// ImGui 렌더링
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();

		// 창 제목에 모드 표시
		std::string title = std::string("Coordinate Systems - ") + (gUsePerspective ? "Perspective(P to toggle)" : "Orthographic(P to toggle)");
		glfwSetWindowTitle(window, title.c_str());
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	shutdownImGui();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

