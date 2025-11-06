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

// 창 크기 상수
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

static GLint  g_wrapModes[3] = { GL_REPEAT,GL_MIRRORED_REPEAT,GL_CLAMP_TO_EDGE };
static int    g_wrapIdx = 0;
static bool   g_linearFilter = true;
static GLuint tex0 = 0, tex1 = 0;


static bool gUsePerspective = true;
static bool gPrevP = false;

// 프레임 시간
float deltaTime = 0.0f; // 현재 프레임과 마지막 프레임 사이의 시간
float lastFrame = 0.0f; // 마지막 프레임의 시간

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

void processInput(GLFWwindow* window)
{

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

	camera.ProcessMouseMovement(lastX, lastY, firstMouse);
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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to init GLAD\n"; return -1;
	}
	glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화 (중요)

	// 2) 셰이더
	// GLAD 초기화 이후에 작성 
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Shader는 컨텍스트/GLAD 이후에 생성해야 함
	Shader ourShader("shaders/basic.vert", "shaders/basic.frag");


	// 3) 큐브 정점(위치 xyz + 색 rgb) 36개 (각 면 2삼각형*6면)
	float vertices[] = {
	  -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// layout(location=0) position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// layout(location=1) texcoord
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


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
	ourShader.use();
	ourShader.setInt("texture1", 0);
	ourShader.setInt("texture2", 1);

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
		glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		processInput(window);

		// 게임 루프 시작 부분에 추가
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		
		ourShader.setMat4("view", camera.GetViewMatrix());

		glm::mat4 projection(1.0f);
		projection = glm::perspective(glm::radians(45.0f),
			(float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

		ourShader.setMat4("projection", projection);

		glBindVertexArray(VAO);
		for (unsigned int i = 0; i < 10; ++i) {
			glm::mat4 model(1.0f);
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			ourShader.setMat4("model", model);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}


		glfwSwapBuffers(window);
		glfwPollEvents();

		// 창 제목에 모드 표시
		std::string title = std::string("Coordinate Systems - ") + (gUsePerspective ? "Perspective(P to toggle)" : "Orthographic(P to toggle)");
		glfwSetWindowTitle(window, title.c_str());
	}

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

