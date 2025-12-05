#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "stb_image.h" // 구현은 src/stb_image_impl.cpp

// 창 크기 상수
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(800, 600, "Single Texture", nullptr, nullptr);
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

    float verts[] = {
        // pos            // uv
         0.5f,  0.5f,0,   1,1,
         0.5f, -0.5f,0,   1,0,
        -0.5f, -0.5f,0,   0,0,
        -0.5f,  0.5f,0,   0,1
    };
    unsigned idx[] = { 0,1,3, 1,2,3 };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);

    // 텍스처 파라미터 & 업로드
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //stbi 이미지 로드
    stbi_set_flip_vertically_on_load(true);
    int w, h, nc; 
    unsigned char* data = stbi_load("assets/awesomeface.png", &w, &h, &nc, 0);
    if (data) {
        GLenum fmt = (nc == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    GLuint prog = CreateShaderProgramFromFiles("shaders/tex_single.vert", "shaders/tex_single.frag");
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0); // sampler->unit0

    while (!glfwWindowShouldClose(win)) {
        glClearColor(0.1f, 0.1f, 0.12f, 1); 
        glClear(GL_COLOR_BUFFER_BIT);
        
        // 그리기
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 프레임 마무리
        glfwSwapBuffers(win); 
        glfwPollEvents();
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}

