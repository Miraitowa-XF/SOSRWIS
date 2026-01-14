#include "SunSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// 构造函数：初始化物理属性为正午状态
SunSystem::SunSystem() {
    direction = glm::vec3(0.0f, 1.0f, 0.0f);    // 垂直向下照射
    worldPos = glm::vec3(0.0f, 80.0f, 0.0f);    // 位于正上方高空
    color = glm::vec3(1.0f, 1.0f, 1.0f);        // 纯白光
    intensity = 1.0f;                           // 全强度
    ambient = 0.2f;                             // 基础环境光
}


unsigned int SunSystem::LoadShader(const char* vertPath, const char* fragPath) {
    auto loadFile = [](const char* path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "SunSystem: Failed to open: " << path << std::endl;
            return std::string();
        }
        else {
            printf("SunSystem: Successfully opened shader %s\n", path);
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
        };

    std::string vertCode = loadFile(vertPath);
    std::string fragCode = loadFile(fragPath);
    if (vertCode.empty() || fragCode.empty()) return 0;

    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    int success;
    char infoLog[512];

    // 顶点着色器
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cout << "ERROR::SUN_VERT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 片段着色器
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cout << "ERROR::SUN_FRAG::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

     // 把着色器链接到程序中
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
        std::cout << "ERROR::SUN_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void SunSystem::Init(const char* vertPath, const char* fragPath) {
    // ---------------------------------------------------------
    // 1. 动态生成球体几何数据 (UV Sphere 算法)
    // ---------------------------------------------------------
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 32; // 经度细分数（数值越大越圆）
    const unsigned int Y_SEGMENTS = 32; // 纬度细分数
    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            // 使用球面坐标系公式
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);

            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }
    indexCount = static_cast<unsigned int>(indices.size());

    // ---------------------------------------------------------
    // 2. 绑定 OpenGL 缓冲 (VAO / VBO / EBO)
    // ---------------------------------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // 顶点缓冲
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 索引缓冲 (非常关键)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // 设置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // ---------------------------------------------------------
    // 3. 加载 Shader (保持你原有的逻辑)
    // ---------------------------------------------------------
    shader = LoadShader(vertPath, fragPath);
    locModel = glGetUniformLocation(shader, "model");
    locView = glGetUniformLocation(shader, "view");
    locProj = glGetUniformLocation(shader, "projection");
    locSunColor = glGetUniformLocation(shader, "sunColor");

    // 【新增】获取强度和相机位置的 Uniform
    glUseProgram(shader);
    glUniform1f(glGetUniformLocation(shader, "sunIntensity"), 1.0f); // 给个默认值防止为0

    glBindVertexArray(0);
}

void SunSystem::Update(float deltaTime, float timeSlider) {
    // 1. 计算太阳的原始轨迹 (0.0=午夜, 0.5=正午)
    // angle: 0.0 -> -90度(下), 0.5 -> 90度(上)
    float angle = (timeSlider * 2.0f * 3.14159265f) - 1.570796f;
    float y = std::sin(angle);
    float z = std::cos(angle);

    // 真实的太阳方向
    glm::vec3 sunDir = glm::normalize(glm::vec3(0.0f, y, z));
    float height = sunDir.y; // [-1, 1]

    // --- 定义颜色 ---
    glm::vec3 colorNoon = glm::vec3(1.0f, 1.0f, 0.98f);   // 正午：亮白
    glm::vec3 colorSunset = glm::vec3(1.0f, 0.45f, 0.1f);   // 日落：橙红
    glm::vec3 colorDark = glm::vec3(0.02f, 0.02f, 0.05f); // 黑暗时刻：极暗蓝黑
    glm::vec3 colorMoon = glm::vec3(0.6f, 0.7f, 1.0f);    // 月光：冷蓝白

    // --- 状态机逻辑 ---

    // 阈值定义：
    // height > 0.1 : 白天
    // height 0.1 ~ -0.1 : 黎明/黄昏 (光线快速变暗)
    // height -0.1 ~ -0.3 : 至暗时刻 (太阳落下，月亮还没升起/刚落下)
    // height < -0.3 : 深夜 (月亮高挂)

    if (height > -0.15f) {
        // === 白天 / 黄昏 模式 ===

        // 设定位置为太阳位置
        direction = sunDir;
        worldPos = direction * 80.0f;

        if (height > 0.1f) {
            // 正常白天
            float t = std::min((height - 0.1f) / 0.4f, 1.0f);
            color = glm::mix(colorSunset, colorNoon, t);
            intensity = 1.0f + t * 0.3f; // 1.0 ~ 1.3
            ambient = 0.3f + t * 0.1f;   // 0.3 ~ 0.4
        }
        else {
            // 日出/日落过渡期 (height -0.15 ~ 0.1)
            // 归一化 t: 0(最暗) ~ 1(日落色)
            float t = (height + 0.15f) / 0.25f;
            color = glm::mix(colorDark, colorSunset, t);

            // 强度急速下降，制造"天黑了"的感觉
            intensity = t * 0.8f;
            ambient = 0.05f + t * 0.25f;
        }
    }
    else {
        // === 深夜 / 月亮 模式 ===

        // 关键逻辑：当太阳落到地平线以下很深时，我们让"光源"变成月亮
        // 月亮位置通常在太阳对面 (-sunDir)

        direction = -sunDir; // 反转方向
        worldPos = direction * 80.0f;

        // 计算月亮高度 (注意 height 是太阳高度，是负的，所以月亮高度是 -height)
        float moonHeight = -height;

        if (moonHeight > 0.3f) {
            // 月亮高挂 (深夜)
            color = colorMoon;
            intensity = 0.3f; // 月光强度较低 (营造氛围，不要太亮)
            ambient = 0.1f;   // 环境光冷且暗
        }
        else {
            // 月出/月落过渡 (至暗时刻)
            // 此时太阳刚下去，月亮还没完全上来，或者反之
            float t = (moonHeight - 0.15f) / 0.15f; // 过渡
            t = glm::clamp(t, 0.0f, 1.0f);

            color = glm::mix(colorDark, colorMoon, t);
            intensity = t * 0.3f;
            ambient = 0.02f + t * 0.08f;
        }
    }
}

void SunSystem::Render(Camera& camera) {
    if (direction.y < -0.2f) return;

    glUseProgram(shader);

    // 设置变换矩阵 (保持原样)
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 1280.0f / 720.0f, 0.1f, 300.0f);

    if (locView != -1) glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
    if (locProj != -1) glUniformMatrix4fv(locProj, 1, GL_FALSE, &projection[0][0]);
    if (locSunColor != -1) glUniform3fv(locSunColor, 1, &color[0]);

    // 必须传递这些参数，否则太阳在 Shader 里计算结果为 0 (黑色)
    glUniform1f(glGetUniformLocation(shader, "sunIntensity"), intensity);
    glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, &camera.Position[0]);

    glm::mat4 model(1.0f);
    model = glm::translate(model, worldPos);
    model = glm::scale(model, glm::vec3(6.0f));

    if (locModel != -1) glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);

    // 使用索引绘图
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}