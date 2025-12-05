#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos; // world space 상에서의 프래그먼트에 해당하는 위치
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{   
    // 월드 좌표계 위치
    FragPos = vec3(model * vec4(aPos, 1.0));

    // 노멀
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // UV
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}

