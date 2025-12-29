#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix; // 光源视角的 投影 * 视图 矩阵
uniform mat4 model;            // 模型矩阵

void main()
{
    // 将顶点转换到光空间
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}