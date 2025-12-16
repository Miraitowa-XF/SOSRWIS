#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 引入我们的核心库
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Renderer/Model.h" // <--- 【新增】引入模型类
#include "Renderer/Skybox.h"

#include <iostream>
#include "stb_image.h"

// --- 全局变量 ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 【保留】摄像机系统
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 回调函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main()
{
    // 1. 初始化 GLFW (不变)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SOSRWIS - Snowy Scene", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // 2. 编译 Shader (不变)
    Shader ourShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");


    std::vector<std::string> faces
    {
        "assets/textures/skybox/right.jpg",
        "assets/textures/skybox/left.jpg",
        "assets/textures/skybox/top.jpg",
        "assets/textures/skybox/bottom.jpg",
        "assets/textures/skybox/front.jpg",
        "assets/textures/skybox/back.jpg"
    };

    // 【新增】创建 Skybox 对象
    Skybox* skybox = new Skybox(faces);

    // 3. 【新增】加载模型
    // 这一步替代了之前“定义立方体 vertices 数组”和“配置 VAO/VBO”的过程
    // 记得在 main 函数里设置一下 stb_image 的翻转
    stbi_set_flip_vertically_on_load(true);

    std::cout << "Loading Model..." << std::endl;
    // 请确保 assets/models/house/house.obj 存在，否则程序会报错
    Model houseModel("assets/models/house/house.obj");
    std::cout << "Model Loaded!" << std::endl;

    // 4. 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput(window);

        // *** 新增：每帧更新相机（把鼠标目标角度应用并平滑） ***
        camera.Update(deltaTime);

        //// 可选：当你觉得响应迟钝时，短时提高速度
        camera.RotationSmoothSpeed = 15.0f;
        camera.MouseSensitivity = 0.8f;

        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // 设置光照和相机矩阵
        ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec3("lightPos", glm::vec3(1.0f, 5.0f, 2.0f));
        ourShader.setVec3("viewPos", camera.Position);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // 这里 GetViewMatrix() 假定 Update 已经被调用过
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        // 绘制模型
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(0.1f));
        ourShader.setMat4("model", model);
        houseModel.Draw(ourShader);

        // 绘制天空盒（注意：Skybox::Draw 使用的是没有平移的 view）
        skybox->Draw(view, projection);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwTerminate();
    return 0;
}

// ... 下面是所有的回调函数 (processInput, mouse_callback 等) ...
// ... 请把之前 Camera 测试代码里下面那部分直接复制过来即可，不需要改动 ...
// ... 这里为了节省篇幅省略了，但你需要保留它们 ...
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }

    // 原始像素偏移
    float xoffset_px = xpos - lastX;
    float yoffset_px = lastY - ypos; // 注意 y 方向取反

    lastX = xpos;
    lastY = ypos;

    // 归一化到 [-1, 1]，按窗口尺寸（使用 framebuffer 大小会更稳）
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float nx = xoffset_px / (float)fbWidth;   // -1 .. 1 (approx)
    float ny = yoffset_px / (float)fbHeight;  // -1 .. 1 (approx)

    // 可选诊断：取消注释以观察数值（短期）
    // static float acc=0; acc += 1.0f; if(acc>30){ acc=0; printf("px=(%.1f,%.1f) norm=(%.4f,%.4f)\n", xoffset_px, yoffset_px, nx, ny); }

    // 将归一化位移传递给 Camera；Camera 内部将把它映射为角度
    // 这里传入归一化值而不是像素
    camera.ProcessMouseMovementNormalized(nx, ny);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}