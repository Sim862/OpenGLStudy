#version 330 core
layout(location = 0) in vec3 aPos;   // 위치
layout(location = 1) in vec2 aUV; // 정점 색

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}

