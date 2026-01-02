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
    // 轨迹计算
    float angle = (timeSlider * 2.0f * 3.14159265f) - 1.570796f;
    float y = std::sin(angle);
    float z = std::cos(angle);
    direction = glm::normalize(glm::vec3(0.0f, y, z));
    worldPos = direction * 80.0f;

    float height = direction.y; // 太阳高度 [-1, 1]

    // --- 定义更真实的颜色点 ---
    glm::vec3 colorNoon = glm::vec3(1.0f, 1.0f, 0.95f);  // 正午：暖白色（不是纯白，带一点点阳光感）
    glm::vec3 colorGoldenHour = glm::vec3(1.0f, 0.85f, 0.5f);  // 晨/傍：金黄色（太阳升起一段距离后）
    glm::vec3 colorSunset = glm::vec3(1.0f, 0.4f, 0.15f);  // 日出落：深橙红（地平线附近）
    glm::vec3 colorDusk = glm::vec3(0.3f, 0.15f, 0.3f);  // 暮光：微紫/深红（刚落山或刚升起时天边的余晖）
    glm::vec3 colorNight = glm::vec3(0.02f, 0.02f, 0.05f); // 深夜：极暗蓝黑色

    if (height > 0.0f) {
        // --- 白天逻辑 (高度 0.0 到 1.0) ---
        if (height > 0.4f) {
            // 中午时段：从金黄色过渡到暖白色 (height 0.4 -> 1.0)
            float t = (height - 0.4f) / 0.6f;
            color = glm::mix(colorGoldenHour, colorNoon, t);
            intensity = 1.2f; // 正午光照最强
        }
        else if (height > 0.1f) {
            // 上午/下午：从深橙红过渡到金黄色 (height 0.1 -> 0.4)
            float t = (height - 0.1f) / 0.3f;
            color = glm::mix(colorSunset, colorGoldenHour, t);
            intensity = 0.8f + t * 0.4f;
        }
        else {
            // 日出/日落瞬间：从微紫余晖过渡到深橙红 (height 0.0 -> 0.1)
            float t = height / 0.1f;
            color = glm::mix(colorDusk, colorSunset, t);
            intensity = 0.2f + t * 0.6f;
        }
        ambient = 0.15f + height * 0.1f; // 环境光随高度增强
    }
    else {
        // --- 晚上逻辑 (高度 -1.0 到 0.0) ---
        // 模拟落山后的余晖消散
        float nightT = glm::clamp(-height * 5.0f, 0.0f, 1.0f); // 快速进入黑夜
        color = glm::mix(colorDusk, colorNight, nightT);

        intensity = 0.0f; // 晚上主光源关闭
        ambient = glm::mix(0.15f, 0.03f, nightT); // 环境光随之变暗
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