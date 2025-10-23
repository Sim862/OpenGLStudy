#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "stb_image.h" // 구현은 src/stb_image_impl.cpp

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/detail/setup.hpp> 

// 창 크기 상수
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

static GLint  g_wrapModes[3] = { GL_REPEAT,GL_MIRRORED_REPEAT,GL_CLAMP_TO_EDGE };
static int    g_wrapIdx = 0;
static bool   g_linearFilter = true;
static GLuint tex0 = 0, tex1 = 0;


static bool gPushP = false;
static bool gPushO = false;

static bool gUsePerspectiveM = false;
static bool gPushW = false;
static bool gPushS = false;
static bool gPushA = false;
static bool gPushD = false;

static void applyTexParams(GLuint tex) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, g_wrapModes[g_wrapIdx]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, g_wrapModes[g_wrapIdx]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_linearFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_linearFilter ? GL_LINEAR : GL_NEAREST);
}
static GLuint makeTexture2D(const char* path) {
	int w, h, nc; stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path, &w, &h, &nc, 0);
	if (!data) { std::cerr << "Load fail: " << path << "\n"; return 0; }
	GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
	applyTexParams(t);
	GLenum fmt = (nc == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return t;
}

// 프레임버퍼 크기 변경 콜백: 창이 리사이즈될 때 실제 렌더링 영역(뷰포트)도 맞춰줌
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}


static void CheckShaderCompile(GLuint shader, const char* name)
{
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint len = 0; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetShaderInfoLog(shader, len, &len, log.data());
		fprintf(stderr, "[Shader Compile Error] %s\n%s\n", name, log.c_str());
	}
}

static void CheckProgramLink(GLuint prog)
{
	GLint success = 0; glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetProgramInfoLog(prog, len, &len, log.data());
		fprintf(stderr, "[Program Link Error]\n%s\n", log.c_str());
	}
}

GLuint CreateShaderProgram(const char* vs, const char* fs)
{
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vs, nullptr);
	glCompileShader(v);
	CheckShaderCompile(v, "Vertex");

	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fs, nullptr);
	glCompileShader(f);
	CheckShaderCompile(f, "Fragment");

	GLuint p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p);
	CheckProgramLink(p);

	glDeleteShader(v);
	glDeleteShader(f);
	return p;
}


static std::string ReadFile(const char* path) {
	std::ifstream f(path, std::ios::binary);
	if (!f) {
		fprintf(stderr, "Failed to open %s", path);
		return {};
	}
	std::ostringstream ss; ss << f.rdbuf();
	return ss.str();
}

static GLuint CreateShaderProgramFromFiles(const char* vsPath, const char* fsPath) {
	std::string vsCode = ReadFile(vsPath);
	std::string fsCode = ReadFile(fsPath);
	if (vsCode.empty() || fsCode.empty()) {
		fprintf(stderr, "Shader source empty: %s or %s", vsPath, fsPath);
		return 0;
	}
	const char* vsSrc = vsCode.c_str();
	const char* fsSrc = fsCode.c_str();

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vsSrc, nullptr);
	glCompileShader(v); CheckShaderCompile(v, "Vertex");

	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fsSrc, nullptr);
	glCompileShader(f); CheckShaderCompile(f, "Fragment");

	GLuint p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p); CheckProgramLink(p);

	glDeleteShader(v); glDeleteShader(f);
	return p;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	gPushP = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
	gPushO = (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS);

	gUsePerspectiveM = (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS);
	gPushW = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
	gPushA = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
	gPushS = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
	gPushD = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);

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
	GLuint cube = CreateShaderProgramFromFiles("shaders/basic.vert", "shaders/basic.frag");

	// 3) 큐브 정점(위치 xyz + 색 rgb) 36개 (각 면 2삼각형*6면)
	float vertices[] = {
		// 뒤쪽 면 (빨강)
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,   1.0f, 0.0f, 0.0f,

		// 앞쪽 면 (초록)
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f,

		// 왼쪽 면 (파랑)
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,

		// 오른쪽 면 (노랑)
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 1.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 1.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   1.0f, 1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,

		// 바닥 (보라)
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,   1.0f, 0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,   1.0f, 0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,   1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   1.0f, 0.0f, 1.0f,

		// 천장 (하늘색)
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,   0.0f, 1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.0f, 1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.0f, 1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,   0.0f, 1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,   0.0f, 1.0f, 1.0f
	};
	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// 위치(0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// UV
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// Color
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Uniform 위치
	glUseProgram(cube);
	GLint locModel = glGetUniformLocation(cube, "uModel");
	GLint locView = glGetUniformLocation(cube, "uView");
	GLint locProj = glGetUniformLocation(cube, "uProj");

	tex0 = makeTexture2D("assets/container.jpg");
	glUniform1i(glGetUniformLocation(cube, "uTex0"), 0);



	// 카메라/투영 기본값
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);


	float currentFrame = 0;
	float deltaTime = 0.0f;  // 현재 프레임과 이전 프레임의 시간 차
	float lastFrame = 0.0f;  // 이전 프레임의 시간 기록
	float r = 0;

	float x = 0;
	float y = 0;
	float z = 0;

	// --- View: 카메라를 뒤로 빼서 바라보기 ---
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

	glm::mat4 model(1);
	model = glm::translate(model, glm::vec3(0, 0, -1));

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 현재 시간 (GLFW는 초 단위로 반환)
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;


		if (gUsePerspectiveM) {
			// --- Model: 큐브를 회전시켜 변환 흐름을 관찰 ---
			r += deltaTime;
			view = glm::rotate(view, r * glm::radians(30.0f * 0.01f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y축 회전
			view = glm::rotate(view, r * glm::radians(17.0f * 0.01f), glm::vec3(1.0f, 0.0f, 0.0f)); // X축 회전

			//model = glm::rotate(model, r * glm::radians(30.0f * 0.01f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y축 회전
			//model = glm::rotate(model, r * glm::radians(17.0f * 0.01f), glm::vec3(1.0f, 0.0f, 0.0f)); // X축 회전

		};

		if (gPushW) {

			glm::mat4 worldMove = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -deltaTime));
			model = worldMove * model;
		}
		if (gPushS) {
			glm::mat4 worldMove = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, deltaTime));
			model = worldMove * model;
		}
		if (gPushA) {
			glm::mat4 worldMove = glm::translate(glm::mat4(1.0f), glm::vec3(-deltaTime, 0, 0));
			model = worldMove * model;
		}
		if (gPushD) {
			glm::mat4 worldMove = glm::translate(glm::mat4(1.0f), glm::vec3(deltaTime, 0, 0));
			model = worldMove * model;
		}


		// --- Projection: P키로 원근/직교 전환 ---
		float aspect = (height == 0) ? 1.0f : (float)width / (float)height;
		aspect = 1;
		glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

		if (gPushP) {
			std::vector projectVerts(std::begin(vertices), std::end(vertices));
			
			glm::vec4 v4;
			int num = 0;
			for (int i = 0; i < projectVerts.size(); i+=8)
			{
				glm::vec3 v3(projectVerts[i], projectVerts[i + 1], projectVerts[i + 2]);

				std::cerr << num++ << "\n";
				std::cerr << v3[0] << ", " << v3[1] << ", " << v3[2] << std::endl;
				glm::vec4 v4(v3, 1);
				
				std::cerr << "회전, 이동 벡터를 적용하기 위해 v4로 확장" << std::endl;
				std::cerr << v4[0] << ", " << v4[1] << ", " << v4[2] << ", " << v4[3] << std::endl;

				std::cerr << "모델 변환으로 월드 좌표로 변환" << std::endl;
#pragma region model log
				std::cerr << model[0][0] << ", " << model[1][0] << ", " << model[2][0] << ", " << model[3][0] << std::endl;
				std::cerr << model[0][1] << ", " << model[1][1] << ", " << model[2][1] << ", " << model[3][1] << std::endl;
				std::cerr << model[0][2] << ", " << model[1][2] << ", " << model[2][2] << ", " << model[3][2] << std::endl;
				std::cerr << model[0][3] << ", " << model[1][3] << ", " << model[2][3] << ", " << model[3][3] << std::endl;
				std::cerr << std::endl;
#pragma endregion

				v4 = model * v4;
				std::cerr << v4[0] << ", " << v4[1] << ", " << v4[2] << ", " << v4[3] << std::endl;


				std::cerr << "뷰 변환으로 카메라 좌표로 변환" << std::endl;
#pragma region view log
				std::cerr << view[0][0] << ", " << view[1][0] << ", " << view[2][0] << ", " << view[3][0] << std::endl;
				std::cerr << view[0][1] << ", " << view[1][1] << ", " << view[2][1] << ", " << view[3][1] << std::endl;
				std::cerr << view[0][2] << ", " << view[1][2] << ", " << view[2][2] << ", " << view[3][2] << std::endl;
				std::cerr << view[0][3] << ", " << view[1][3] << ", " << view[2][3] << ", " << view[3][3] << std::endl;
				std::cerr << std::endl;
#pragma endregion
				//OpenGL은 z=-1~+1 범위밖의 값은 클리핑해서 안 그림
				v4 = view * v4;
				std::cerr << v4[0] << ", " << v4[1] << ", " << v4[2] << ", " << v4[3] << std::endl;


				std::cerr << "투영 변환으로 클립 좌표로 변환(클리핑 용)" << std::endl;
				//-w <= x <= w 가 아니라면 클리핑
#pragma region Projection log
				std::cerr << proj[0][0] << ", " << proj[1][0] << ", " << proj[2][0] << ", " << proj[3][0] << std::endl;
				std::cerr << proj[0][1] << ", " << proj[1][1] << ", " << proj[2][1] << ", " << proj[3][1] << std::endl;
				std::cerr << proj[0][2] << ", " << proj[1][2] << ", " << proj[2][2] << ", " << proj[3][2] << std::endl;
				std::cerr << proj[0][3] << ", " << proj[1][3] << ", " << proj[2][3] << ", " << proj[3][3] << std::endl;
				std::cerr << std::endl;
#pragma endregion
				std::cerr << "투영 변환으로 행렬 곱이 끝남";
				v4 = proj * v4;
				std::cerr << v4[0] << ", " << v4[1] << ", " << v4[2] << ", " << v4[3]<< std::endl;


				std::cerr << "-------------------여기 까지 버텍스 쉐이더------------------ \n" << std::endl;

				std::cerr << "투영 나눗셈으로 NDC 공간으로 변환" << std::endl;
#pragma region Projection log
				std::cerr << "[" << proj[0][0] << ", " << proj[1][0] << ", " << proj[2][0] << ", " << proj[3][0] << "] / " << v4.w  << std::endl;
#pragma endregion
				v4 /= v4.w;
				v4.z *= -1;
				std::cerr << v4[0] << ", " << v4[1] << ", " << v4[2] << std::endl;

				projectVerts[i] = v4[0];
				projectVerts[i+1] = v4[1];
				projectVerts[i+2] = v4[2];
				std::cerr << "\n\n\n";
			}

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, projectVerts.size() * sizeof(projectVerts[0]), projectVerts.data(), GL_STATIC_DRAW);
		}

		if (gPushO) {
			
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			std::cerr << "PushO\n";
		}

		// 업로드(열우선: 전치 필요 없음 -> GL_FALSE)
		glUseProgram(cube);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);
		glfwPollEvents();

		// 창 제목에 모드 표시
		std::string title = std::string("Coordinate Systems - ") + (gPushP ? "Perspective(P to toggle)" : "Orthographic(P to toggle)");
		glfwSetWindowTitle(window, title.c_str());
	}

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(cube);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
