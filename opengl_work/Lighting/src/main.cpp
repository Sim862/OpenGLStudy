#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>
#include <iostream>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// [추가] ImGui 헤더
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shader_m.h"	// 셰이더 유틸리티 클래스
#include "camera.h"

// 창 크기 상수
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

static GLint  g_wrapModes[3] = { GL_REPEAT,GL_MIRRORED_REPEAT,GL_CLAMP_TO_EDGE };
static int    g_wrapIdx = 0;
static bool   g_linearFilter = true;
static GLuint tex0 = 0, tex1 = 0;

// [추가] UI 모드 토글을 위한 전역 변수
bool g_UiMode = false;

static bool gUsePerspective = true;
static bool gPrevP = false;

// 프레임 시간
float deltaTime = 0.0f; // 현재 프레임과 마지막 프레임 사이의 시간
float lastFrame = 0.0f; // 마지막 프레임의 시간

// 광원과 조명 파라미터 (ImGui로 조절)
glm::vec3 gLightPos(1.2f, 1.0f, 2.0f);       // 광원 위치
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);     // 기본: 흰색 빛

float gAmbientStrength = 0.1f;  // 주변광 계수
float gDiffuseStrength = 1.0f;  // 난반사 계수
float gSpecularStrength = 0.5f;  // 정반사 계수


// 카메라 이동 관련 변수
//glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);
//glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
//glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
//
// 카메라 방향 관련 변수
//float yaw = -90.0f; // Yaw는 Y축 기준 회전 (초기값 -90은 -Z축을 보게 함)
//float pitch = 0.0f;  // Pitch는 X축 기준 회전
//
 //마우스 입력 관련 변수
float lastX = 800.0f / 2.0f; // 화면 중앙 X
float lastY = 600.0f / 2.0f; // 화면 중앙 Y
bool firstMouse = true;       // 마우스가 처음 입력되었는지

Camera camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

// 프레임버퍼 크기 변경 콜백: 창이 리사이즈될 때 실제 렌더링 영역(뷰포트)도 맞춰줌
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
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

		camera.MouseSensitivity = 0.0001f;

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

int main()
{
	// 1) 창/컨텍스트
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(960, 600, "Coordinate Systems - MVP", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// 마우스 이동/스크롤 콜백 등록
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// [수정] ImGui를 위해 키보드/마우스 '버튼' 콜백도 등록
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// [수정] 초기엔 FPS 모드로 시작 (커서 숨김)
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to init GLAD\n"; return -1;
	}
	glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화 (중요)


	// ImGui 초기화
	// ===================================================
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// ImGui를 GLFW와 OpenGL 3.3 버전에 맞게 설정
	// 'false'로 설정 -> 우리가 콜백을 수동으로 등록(key_callback 등)해서 전달하겠다는 의미
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init("#version 330"); // GLSL 버전에 맞게

	// 2) 셰이더
	// Shader는 컨텍스트/GLAD 이후에 생성해야 함
	// [변경] 카메라 셰이더 대신 조명용 셰이더 사용
	Shader ourShader("shaders/basic_lighting_tex.vert", "shaders/basic_lighting_tex.frag");
	// [추가] 광원(전구) 큐브를 그리기 위한 셰이더
	Shader lightCubeShader("shaders/light_cube.vert", "shaders/light_cube.frag");



	// 3) 큐브 정점(위치 xyz + 색 rgb) 36개 (각 면 2삼각형*6면)
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

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// layout(location=0) position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// layout(location=1) normal attribute 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// layout(location=2) texcoord
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	// 같은 VBO를 재사용하지만, position만 사용
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// 이후에는 VAO만 바꿔가며 사용
	glBindVertexArray(0);


	GLuint texture1 ;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture1);

	// texture1: container.jpg (RGB)
	glBindTexture(GL_TEXTURE_2D, texture1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int w, h, nc;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("assets/textures/container.jpg", &w, &h, &nc, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	stbi_image_free(data);
	

	GLuint texture2;
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &texture2);

	// texture2: awesomeface.png (RGBA)
	glBindTexture(GL_TEXTURE_2D, texture2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = stbi_load("assets/textures/awesomeface.png", &w, &h, &nc, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	stbi_image_free(data);


	// 샘플러 유닛 연결(한 번만)
	ourShader.use();;
	// [추가] 조명 셰이더용 재질 텍스처 유닛 연결
	ourShader.setInt("material.diffuse", 0);

	// 카메라/투영 기본값
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);

	glm::vec3 cubePositions[10] = {
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

	// main 함수 어딘가 (창 생성 후)
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 마우스 커서를 숨기고 창에 고정
	glfwSetCursorPosCallback(window, mouse_callback); // 마우스 움직임 콜백 등록

	while (!glfwWindowShouldClose(window))
	{

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		// 게임 루프 시작 부분에 추가
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// ImGui 새 프레임 시작
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		// [추가] ImGui UI 코드
		if (g_UiMode) { // UI 모드일 때만 창을 그림
			ImGui::Begin("Camera Control");

			// camera.Position (vec3) 조작
			ImGui::DragFloat3("Position", glm::value_ptr(camera.Position), 0.01f);

			// camera.Yaw/Pitch (float) 조작
			// [참고] 이 방식이 작동하려면, camera.GetViewMatrix() 함수가
			// 매번 Yaw/Pitch 값으로 Front 벡터를 새로 계산해야 함.
			// (LearnOpenGL의 기본 Camera 클래스는 이 가정이 맞음)
			ImGui::DragFloat("Yaw", &camera.Yaw, 0.5f);
			ImGui::DragFloat("Pitch", &camera.Pitch, 0.5f, -89.0f, 89.0f); // min/max로 제한

			// camera.Zoom (FOV) 조작
			ImGui::DragFloat("Zoom (FOV)", &camera.Zoom, 0.1f, 1.0f, 90.0f);

			// 리셋 버튼
			if (ImGui::Button("Reset Camera")) {
				// Camera 객체를 기본값으로 새로 생성해서 덮어씀
				camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
			}

			ImGui::Separator();
			ImGui::Text("Lighting");

			// 광원 위치
			ImGui::DragFloat3("Light Pos", glm::value_ptr(gLightPos), 0.01f);

			// 광원 색상
			ImGui::ColorEdit3("Light Color", glm::value_ptr(gLightColor));

			// 각 성분 계수
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
		// 중요!! 
		camera.ProcessMouseMovement(0.0f, 0.0f, true); // 내부 벡터 재계산 트릭


		glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		//-----------------------------------------------------
		// cube uniform Lighting, MVP setting
		//-----------------------------------------------------
		ourShader.use();

		// 조명/재질 유니폼 설정
		ourShader.setVec3("light.position", gLightPos);
		ourShader.setVec3("viewPos", camera.Position);

		// 계수들을 이용해서 ambient/diffuse/specular 색 계산
		glm::vec3 diffuseColor = gLightColor * gDiffuseStrength;
		glm::vec3 ambientColor = diffuseColor * gAmbientStrength;
		glm::vec3 specularColor = gLightColor * gSpecularStrength;

		ourShader.setVec3("light.ambient", ambientColor);
		ourShader.setVec3("light.diffuse", diffuseColor);
		ourShader.setVec3("light.specular", specularColor);

		// 재질 특성 (여기서는 텍스처 하나만 diffuse map으로 사용)
		ourShader.setFloat("material.shininess", 32.0f);

		// MVP Setting
		glm::mat4 projection(1.0f);
		projection = glm::perspective(glm::radians(45.0f),
			(float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", camera.GetViewMatrix());
		glBindVertexArray(VAO);


		for (unsigned int i = 0; i < 10; ++i) {
			glm::mat4 model(1.0f);
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			ourShader.setMat4("model", model);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}



		// Lighting Cube uniform MVP setting
		lightCubeShader.use();
		lightCubeShader.setMat4("projection", projection);
		lightCubeShader.setMat4("view", camera.GetViewMatrix());

		glm::mat4 lightModel = glm::mat4(1.0f);
		lightModel = glm::translate(lightModel, gLightPos);
		lightModel = glm::scale(lightModel, glm::vec3(0.2f)); // 작게
		lightCubeShader.setMat4("model", lightModel);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();

		// 창 제목에 모드 표시
		std::string title = std::string("Coordinate Systems - ") + (gUsePerspective ? "Perspective(P to toggle)" : "Orthographic(P to toggle)");
		glfwSetWindowTitle(window, title.c_str());
	}

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

