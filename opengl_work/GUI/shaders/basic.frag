#version 330 core
in vec2 vUV;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 FragColor;
void main()
{
    vec4 c0 = texture(texture1, vUV);
    vec4 c1 = texture(texture2, vUV);
    FragColor = mix(c0, c1, 0.5);
}

