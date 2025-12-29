#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox; 

void main()
{
    // 1. 采样纹理颜色
    vec4 texColor = texture(skybox, TexCoords);

    // 2. 调整亮度系数 (0.0 ~ 1.0)
    // 0.5 表示亮度减半，0.2 表示很暗，1.0 是原图
    float factor = 0.5; 

    // 3. 输出变暗后的颜色
    FragColor = texColor * factor;
}