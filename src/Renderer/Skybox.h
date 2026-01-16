#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>

#include "../Core/Shader.h"
#include "stb_image.h"

class Skybox
{
public:
    // 构造函数：传入包含6张图片路径的 vector
    Skybox(std::vector<std::string> faces);

    // 绘制函数
    void Draw(const glm::mat4& view, const glm::mat4& projection, float brightness = 1.0f);

private:
    unsigned int skyboxVAO, skyboxVBO;
    unsigned int textureID;
    Shader* skyboxShader; // 天空盒专用的 Shader

    // 加载 CubeMap 的辅助函数
    unsigned int loadCubemap(std::vector<std::string> faces);
};