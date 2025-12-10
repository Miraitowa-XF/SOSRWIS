#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox; // 注意这里是 samplerCube

void main()
{
    FragColor = texture(skybox, TexCoords);
}