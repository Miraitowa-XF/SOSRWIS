#pragma once

#include <glad/glad.h> // 包含 glad 来获取所有的 OpenGL 头文件
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID; // 着色器程序 ID

    // 构造函数：传入顶点和片段着色器的路径
    Shader(const char* vertexPath, const char* fragmentPath);

    // 激活着色器
    void use();

    // uniform 工具函数 (后续传参用)
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;

private:
    // 检查编译错误的辅助函数
    void checkCompileErrors(unsigned int shader, std::string type);
};