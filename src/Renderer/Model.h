#pragma once

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "../Core/Shader.h"

#include <string>
#include <vector>

class Model
{
public:
    // 存储所有的网格
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded; // 缓存已加载的纹理

    // 构造函数：直接传入路径加载
    Model(std::string const& path, bool gamma = false);

    // 绘制模型
    void Draw(Shader& shader);

private:
    bool gammaCorrection;
    // 加载模型函数
    void loadModel(std::string const& path);

    // 递归处理节点
    void processNode(aiNode* node, const aiScene* scene);

    // 将 Assimp 的 mesh 转换为我们的 Mesh
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // 加载材质纹理
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};