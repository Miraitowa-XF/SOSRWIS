#pragma once

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

// 引入 Shader 类，因为 Mesh 需要知道把数据传给哪个 Shader
#include "../Core/Shader.h" 

// 定义顶点结构体
struct Vertex {
    glm::vec3 Position;  // 位置
    glm::vec3 Normal;    // 法线
    glm::vec2 TexCoords; // 纹理坐标
};

// 定义纹理结构体
struct Texture {
    unsigned int id;
    std::string type; // "texture_diffuse" 或 "texture_specular"
    std::string path; // 文件路径，用于防止重复加载
};

class Mesh {
public:
    // 网格数据
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    unsigned int VAO;

    // 构造函数
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    // 绘制函数
    void Draw(Shader& shader);

private:
    // 渲染数据
    unsigned int VBO, EBO;
    // 初始化缓冲
    void setupMesh();
};