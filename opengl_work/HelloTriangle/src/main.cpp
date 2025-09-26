// 1. 헤더 포함: GLAD가 항상 GLFW보다 먼저 OpenGL 헤더를 include해야 함
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>

#include <iostream>

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
    // 2. GLFW 초기화
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }

    // 3. 컨텍스트 버전 및 프로파일 지정(예: OpenGL 3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 메이저 버전 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // 마이너 버전 3 → 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 코어 프로파일 사용
    // macOS 호환(필요시)
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 4. 창 생성(너비, 높이, 창 제목)
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // 5. 현재 스레드의 컨텍스트로 만들기(이제부터 OpenGL 호출 가능)
    glfwMakeContextCurrent(window);

    // 6. GLAD로 OpenGL 함수 포인터 로딩(플랫폼별 주소 얻기)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // 7. 첫 뷰포트 설정(왼쪽 아래(0,0) ~ (SCR_WIDTH, SCR_HEIGHT))
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // 8. 리사이즈 콜백 등록(창 크기 바뀌면 자동으로 glViewport 맞춰줌)
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

#pragma region 삼각형 2개 만들기
    //// NDC 좌표 (반시계 방향)
   //float vertices1[] = {
   //    -0.9f, -0.5f, 0.0f,
   //    0.0f, -0.5f, 0.0f,
   //    -0.45f, 0.5f, 0.0f,
   //};
   //float vertices2[] = {
   //    0.0f, -0.5f, 0.0f,
   //    0.9f, -0.5f, 0.0f,
   //    0.45f, 0.5f, 0.0f
   //};
   //unsigned int VBO[2], VAO[2];

   //// VAO, VBO 생성 단계
   //glGenVertexArrays(2, VAO);
   //glGenBuffers(2, VBO);

   //// 설정 단계 bind
   //glBindVertexArray(VAO[0]);
   //glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
   //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);
   //glVertexAttribPointer(
   //    0, // attribute index (location=0)
   //    3, // vec3 → 3개 구성요소
   //    GL_FLOAT, // 각 구성요소 타입
   //    GL_FALSE, // 정규화 불필요
   //    3 * sizeof(float), // stride: 한 정점에서 다음 정점까지 바이트 간격
   //    (void*)0 // 시작 오프셋
   //);
   //// 해당 속성 활성화
   //glEnableVertexAttribArray(0);

   //glBindVertexArray(VAO[1]);
   //glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
   //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
   //glVertexAttribPointer(
   //    0, // attribute index (location=0)
   //    3, // vec3 → 3개 구성요소
   //    GL_FLOAT, // 각 구성요소 타입
   //    GL_FALSE, // 정규화 불필요
   //    3 * sizeof(float), // stride: 한 정점에서 다음 정점까지 바이트 간격
   //    (void*)0 // 시작 오프셋
   //);
   //// 해당 속성 활성화
   //glEnableVertexAttribArray(0);
#pragma endregion

   
#pragma region 정점에 색상 추가
    float colored[] = {
        //  위치              // 색상
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
    };

    unsigned VAO, VBO;
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colored), colored, GL_STATIC_DRAW);


    // stride = 6 * sizeof(float)
    // 위치 속성: offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

#pragma endregion



    // 3) 정점 속성 포인터 설정 (위치 속성만 있는 경우)
    // layout(location = 0)에 vec3 입력을 연결
    // 인자: (인덱스, 크기, 타입, 정규화 여부, stride, offset)



    // (선택) 정리
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);

// (가정) GLFW로 창/컨텍스트 생성 완료, GLAD 초기화 완료

    GLuint program = CreateShaderProgramFromFiles("shaders/uniform.vert", "shaders/uniform.frag");

    // 프로그램 링크 후, 한 번만 uniform 위치를 찾아둡니다.
    GLint colorLoc = glGetUniformLocation(program, "uColor");

    // 렌더 루프 직전에 와이어프레임 모드 켜기
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window))
    {
        float t = (float)glfwGetTime();               // 경과 시간(초)
        float g = 0.5f * std::sin(t) + 0.5f;          // 0~1 사이로 변환
        glUseProgram(program);                           // (중요) 활성화 먼저
        glUniform4f(colorLoc, 0.0f, g, 1.0f - g, 1.0f); // 파랑<->초록 계열 변화
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();

    }
    // 10. 종료 처리
    glfwTerminate();
    return 0;
}
