#version 330 core
layout(location = 0) in vec3 aPos;   // 위치
layout(location = 1) in vec2 aUV; // 정점
layout(location = 2) in vec4 aColor; // 정점

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec2 vUV;
out vec4 vColor;

void main()
{
    vUV = aUV;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    //gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}

