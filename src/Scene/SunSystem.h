#ifndef SUN_SYSTEM_H
#define SUN_SYSTEM_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Core/Camera.h"
#include <vector>

class SunSystem {
public:
    // --- 构造函数 ---
    SunSystem(); // 添加构造函数用于初始化属性

    // --- 太阳的物理属性（供外部读取给主Shader使用） ---
    glm::vec3 direction;   // 太阳光的方向
    glm::vec3 worldPos;    // 太阳在世界空间的位置
    glm::vec3 color;       // 太阳光的实时颜色
    float intensity;       // 光照强度
    float ambient;         // 环境光强度

    // --- 系统接口 ---
    void Init(const char* vertPath, const char* fragPath);
    void Update(float deltaTime, float timeSlider);
    void Render(Camera& camera);
    
private:
    unsigned int LoadShader(const char* vertPath, const char* fragPath);

    unsigned int VAO = 0, VBO = 0;
    unsigned int shader = 0;

    unsigned int indexCount = 0; // 新增：记录球体有多少个索引点
    unsigned int EBO = 0;        // 新增：索引缓冲对象

    // Uniform 缓存
    GLint locView = -1;
    GLint locProj = -1;
    GLint locModel = -1;
    GLint locSunColor = -1;
};

#endif