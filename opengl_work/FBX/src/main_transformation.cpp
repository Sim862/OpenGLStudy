#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

static float g_mix = 0.2f;
static GLint  g_wrapModes[3] = { GL_REPEAT,GL_MIRRORED_REPEAT,GL_CLAMP_TO_EDGE };
static int    g_wrapIdx = 0;
static bool   g_linearFilter = true;
static GLuint tex0 = 0, tex1 = 0;

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

// 입력 처리 헬퍼: ESC를 누르면 창 닫기 플래그 세팅함
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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


int main() {
    // 1) GLFW
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(960, 600, "Transformations", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 2) GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return -1;
    }

    // 4) 정점(사각형) + 색
    float vertices[] = {
        // pos            // color         // uv
         0.5f,  0.5f,0,   1,0,0,           1,1,
         0.5f, -0.5f,0,   0,1,0,           1,0,
        -0.5f, -0.5f,0,   0,0,1,           0,0,
        -0.5f,  0.5f,0,   1,1,0,           0,1
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    tex0 = makeTexture2D("assets/container.jpg");
    tex1 = makeTexture2D("assets/awesomeface.png");

    // 3) 셰이더 
    GLuint program = CreateShaderProgramFromFiles("shaders/transform.vert", "shaders/transform.frag");
    glUseProgram(program);

    glUniform1i(glGetUniformLocation(program, "uTex0"), 0);
    glUniform1i(glGetUniformLocation(program, "uTex1"), 1);


    GLint locTransform = glGetUniformLocation(program, "uTransform");


    double last = glfwGetTime();
    bool zPrev = false, xPrev = false;

    glEnable(GL_BLEND);


    // 5) 루프
    while (!glfwWindowShouldClose(window)) {

        double now = glfwGetTime();
        float dt = float(now - last);
        last = now;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            g_mix = std::min(1.0f, g_mix + 0.7f * dt);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            g_mix = std::max(0.0f, g_mix - 0.7f * dt);

        bool zNow = (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS);
        bool xNow = (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS);
        if (zNow && !zPrev) {
            g_linearFilter = !g_linearFilter; applyTexParams(tex0); applyTexParams(tex1);
            std::cout << "Filter: " << (g_linearFilter ? "LINEAR" : "NEAREST") << "\n";
        }
        if (xNow && !xPrev) {
            g_wrapIdx = (g_wrapIdx + 1) % 3; applyTexParams(tex0); applyTexParams(tex1);
            std::cout << "Wrap: " << (g_wrapIdx == 0 ? "REPEAT" : g_wrapIdx == 1 ? "MIRRORED_REPEAT" : "CLAMP_TO_EDGE") << "\n";
        }
        zPrev = zNow; xPrev = xNow;

        // 1) 화면 초기화(배경색)
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 2) 그리기
        glUseProgram(program);
        glUniform1f(glGetUniformLocation(program, "uMix"), g_mix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex1);

        glBindVertexArray(VAO);

        // (A) 왼쪽: 이동 → 회전 → 축소
        glm::mat4 M1(1.0f);
        M1 = glm::translate(M1, glm::vec3(-0.5f, 0.0f, 0.0f));
        float angle = static_cast<float>(now);
        M1 = glm::rotate(M1, angle, glm::vec3(0.0f, 0.0f, 1.0f));  // OK

        M1 = glm::scale(M1, glm::vec3(0.6f));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M1));

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // (B) 오른쪽: 이동 → 시간 스케일
        glm::mat4 M2(1.0f);
        M2 = glm::translate(M2, glm::vec3(0.5f, 0.0f, 0.0f));
        float s = 0.5f + 0.25f * (std::sin(now) + 1.0f); // 0.5 ~ 1.0
        M2 = glm::scale(M2, glm::vec3(s));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M2));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 피벗 회전: 점 p=(0.2,0.2) 기준 회전 → M = T(p) * R(θ) * T(-p) 적용.
        // (C) 중앙 아래: 피벗 회전
        glm::mat4 M3(1.0f);
        M3 = glm::translate(M3, glm::vec3(0.0f, -0.5f, 0.0f));
        M3 = glm::translate(M3, glm::vec3(0.2f, 0.2f, 0.0f));
        angle = -static_cast<float>(now);
        M3 = glm::rotate(M3, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        M3 = glm::translate(M3, glm::vec3(-0.2f, -0.2f, 0.0f));
        M3 = glm::scale(M3, glm::vec3(0.6f));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M3));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        //비균등 스케일: x만 2배, y는 0.5배 + 회전 조합 시 왜곡 관찰.
        // (D) 중앙 위: 비균등 스케일 + 회전

        glm::mat4 M4(1.0f);
        M4 = glm::translate(M4, glm::vec3(0.0f, 0.5f, 0.0f));
        angle = static_cast<float>(now);
        M4 = glm::rotate(M4, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        M4 = glm::scale(M4, glm::vec3(2.0f, 0.5f, 1.0f));

        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M4));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //키보드 이동: 화살표 키 입력으로 translate 벡터 실시간 변경.
        // (E) 중앙: 키보드 이동
        static glm
            ::vec3
            translate(0.0f, 0.0f, 0.0f);
        float moveSpeed = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            translate.x -= moveSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            translate.x += moveSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            translate.y -= moveSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            translate.y += moveSpeed * dt;
        glm::mat4 M5(1.0f);
        M5 = glm::translate(M5, translate);
        M5 = glm::scale(M5, glm::vec3(0.4f));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M5));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);



        // 3) 프레임 마무리
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}

