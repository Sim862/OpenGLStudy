#version 330 core
// fragment shader
in vec3 vColor;
out vec4 FragColor;
void main(){
    FragColor = vec4(vColor, 1.0);
}