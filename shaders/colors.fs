#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main() {
    // 這裡實作了教學提到的逐分量相乘
    vec3 result = lightColor * objectColor;
    FragColor = vec4(result, 1.0);
}