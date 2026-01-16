#version 330 core
layout (location = 0) in vec3 aPos;      // 顶点位置
layout (location = 1) in vec3 aNormal;   // 法线
layout (location = 2) in vec2 aTexCoords;// 纹理坐标

out vec3 FragPos;   // 输出到片段着色器：世界坐标位置
out vec3 Normal;    // 输出到片段着色器：法线
out vec2 TexCoords; // 输出到片段着色器：纹理坐标
out vec4 FragPosLightSpace; // 输出光空间坐标

uniform mat4 model;      // 模型矩阵
uniform mat4 view;       // 观察矩阵
uniform mat4 projection; // 投影矩阵
uniform mat4 lightSpaceMatrix; // 接收光矩阵

void main()
{
    // 计算顶点的世界坐标
    FragPos = vec3(model * vec4(aPos, 1.0));
    // 计算法线（处理非均匀缩放）
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    // 传递纹理坐标
    TexCoords = aTexCoords;

    // 【新增】计算当前顶点在光空间的位置
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    // 最终的裁剪空间坐标
    gl_Position = projection * view * vec4(FragPos, 1.0);
}