#version 330 core
in vec2 vUV;

uniform sampler2D uTex0;

out vec4 FragColor;
void main()
{
    vec4 c0 = texture(uTex0, vUV);
    FragColor = c0;
}

