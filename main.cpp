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

unsigned int planeVAO, planeVBO;
unsigned int snowTexture;

// --- 全局变量 ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 【保留】摄像机系统
Camera camera(glm::vec3(0.0f, 3.0f, 0.0f));     // 初始位置的确定
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 增加一个防抖动变量
bool tabPressed = false;

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
    // 【新增】开启混合 (解决玻璃透明)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 【新增】开启面剔除 (解决动漫模型黑色乱码)
    glEnable(GL_CULL_FACE);

    // 2. 编译 Shader (不变)
    Shader ourShader("assets/shaders/basic.vert", "assets/shaders/basic.frag");


    std::vector<std::string> faces
    {
        "assets/textures/skybox/px.png",
        "assets/textures/skybox/nx.png",
        "assets/textures/skybox/py.png",
        "assets/textures/skybox/ny.png",
        "assets/textures/skybox/pz.png",
        "assets/textures/skybox/nz.png"
    };

    // 【新增】创建 Skybox 对象
    Skybox* skybox = new Skybox(faces);

    // 加载模型资源
    // 加载 GLTF 模型前，通常建议关闭翻转，否则纹理会反
    stbi_set_flip_vertically_on_load(false);

    std::cout << "Loading Model..." << std::endl;
    // 请确保 assets/models/house/house.obj 存在，否则程序会报错
    Model houseModel("assets/models/snowy_wooden_hut/scene.gltf");
    Model groundModel("assets/models/snow_floor/scene.gltf");
    Model snowmanModel("assets/models/snow_man/scene.gltf");
	Model house2Model("assets/models/lowpoly_snow_house/scene.gltf");
	Model treesModel("assets/models/newtrees/newtrees.gltf");
	Model wellModel("assets/models/old_well/scene.gltf");
	Model containerModel("assets/models/rusty_container/scene.gltf");
	Model busModel("assets/models/bus/scene.gltf");
	Model villageModel("assets/models/snowy_village/scene.gltf");
	Model mailboxModel("assets/models/mailbox/scene.gltf");
	Model christmasTreesModel("assets/models/christmas_tree/scene.gltf");
	Model benchModel("assets/models/bench/scene.gltf");
	Model lampModel("assets/models/street_lamp/scene.gltf");
	Model jonModel("assets/models/jon_snow/scene.gltf");
	Model dragonModel("assets/models/snow_dragon/scene.gltf");
	Model reslerianaModel("assets/models/resleriana/scene.gltf");
	Model fairyModel("assets/models/garden_fairy/scene.gltf");
	Model figure1("assets/models/figure1/scene.gltf");
	Model figure2("assets/models/figure2/scene.gltf");
	Model fountain("assets/models/fountain/scene.gltf");

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
        ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));       // (1.0, 1.0, 1.0) 代表纯白色光。
		ourShader.setVec3("lightPos", glm::vec3(1.0f, 5.0f, 2.0f));         // 光源位置
        ourShader.setVec3("viewPos", camera.Position);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // 这里 GetViewMatrix()
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        //// 绘制房子模型
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 3.0f, -40.0f));               // 定义位置
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));   // 旋转：定义朝向 参数1：当前的矩阵 参数2：旋转的角度（弧度制，所以用 glm::radians 转换角度）参数3：旋转轴（这里是绕 Y 轴旋转，即左右转头）
        model = glm::scale(model, glm::vec3(2.5f));                       // 定义缩放大小
        ourShader.setMat4("model", model);
        houseModel.Draw(ourShader);

		// jon_snow 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, 0.0f, -30.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.07f));
        ourShader.setMat4("model", model);
		jonModel.Draw(ourShader);

		// figure1 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(15.0f, 0.0f, -19.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);
        figure1.Draw(ourShader);

		// figure2 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(15.0f, 0.0f, -14.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);
        figure2.Draw(ourShader);

		// fairy 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.8f, -25.0f));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(3.0f));
        ourShader.setMat4("model", model);
		fairyModel.Draw(ourShader);

		// fountain 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f, 0.0f, -15.0f));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.015f));
        ourShader.setMat4("model", model);
        fountain.Draw(ourShader);

		// dragon 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(30.0f, 0.0f, -43.0f));
        model = glm::rotate(model, glm::radians(-65.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(5.0f));
        ourShader.setMat4("model", model);
        dragonModel.Draw(ourShader);

		// 路灯模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-17.0f, 0.0f, 15.0f));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f));
        ourShader.setMat4("model", model);
        lampModel.Draw(ourShader);

		// 绘制第二个房子模型    
		model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 20.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(15.0f));
		ourShader.setMat4("model", model);
		house2Model.Draw(ourShader);

		// 绘制村庄模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-25.0f, 0.0f, -20.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);
        villageModel.Draw(ourShader);

		// resleriana 模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(15.0f, 0.0f, -17.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.7f));
        ourShader.setMat4("model", model);
        reslerianaModel.Draw(ourShader);

		// 邮箱模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 0.1f, -35.0f));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        mailboxModel.Draw(ourShader);

        // 雪人模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-5.0f, 0.5f, -35.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);
        snowmanModel.Draw(ourShader);

		// 树模型
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(25.0f, 0.0f, 25.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		ourShader.setMat4("model", model);
		treesModel.Draw(ourShader);

		// 长椅模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(18.0f, 0.0f, 10.0f));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(2.0f));
        ourShader.setMat4("model", model);
        benchModel.Draw(ourShader);

		// 圣诞树模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(25.0f, 0.0f, -15.0f));
        model = glm::scale(model, glm::vec3(2.0f));
        ourShader.setMat4("model", model);
        christmasTreesModel.Draw(ourShader);

		// 井模型
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(35.0f, 0.0f, 15.0f));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.01f));
		ourShader.setMat4("model", model);
        wellModel.Draw(ourShader);

		// 集装箱模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-25.0f, 0.0f, 20.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(3.0f));
        ourShader.setMat4("model", model);
        containerModel.Draw(ourShader);

		// 公交车模型
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-35.0f, 4.0f, 20.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f));
        ourShader.setMat4("model", model);
        busModel.Draw(ourShader);

        // 地面 (外部模型) 
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(25.0f, 0.0f, -25.0f));
        model = glm::scale(model, glm::vec3(0.25f));
        ourShader.setMat4("model", model);
        groundModel.Draw(ourShader);

        // 绘制天空盒（注意：Skybox::Draw 使用的是没有平移的 view）
        skybox->Draw(view, projection);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwTerminate();
    return 0;
}

// ... 下面是所有的回调函数 (processInput, mouse_callback 等) ...
void processInput(GLFWwindow* window)
{
    // 1. ESC 退出
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 2. TAB 切换摄像机模式 (FPS <-> God Mode)
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed)
    {
        camera.FPS_Mode = !camera.FPS_Mode; // 切换布尔值
        tabPressed = true; // 锁定，直到松开按键

        // 打印提示，方便调试
        if (camera.FPS_Mode)
            std::cout << "Switched to: FPS Mode (Walking)" << std::endl;
        else
            std::cout << "Switched to: Free Mode (Flying)" << std::endl;
    }
    // 松开 TAB 键后，解除锁定
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
    {
        tabPressed = false;
    }

    // ============================================================
    //  记录移动前的位置
    // ============================================================
    glm::vec3 oldPosition = camera.Position;

    // ============================================================
    //  奔跑控制 (按住 R 键加速)
    // ============================================================
    // 定义两种速度
    float normalSpeed = 5.0f;  // 正常走路速度 (和 Camera.h 里的默认值一致)
    float runSpeed = 10.0f; // 奔跑速度 (4倍速，觉得慢可以改成 20.0f)
    // 检测按键
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        camera.MovementSpeed = runSpeed; // 加速
    }
    else
    {
        camera.MovementSpeed = normalSpeed; // 恢复
    }
    // ============================================================

    // 3. 基础移动 (WASD)
    // 具体的移动逻辑（是飞还是走）由 Camera 类内部的 FPS_Mode 变量决定
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // 4. 垂直移动
    // SPACE: 上升
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);

    // LEFT_CONTROL: 下降 (替换掉了原来的 LEFT_SHIFT)
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    // ============================================================
    //  检测位置是否改变，并打印
    // ============================================================
    if (camera.Position != oldPosition)
    {
        std::cout << "Current Pos: [ "
            << camera.Position.x << ", "
            << camera.Position.y << ", "
            << camera.Position.z << " ]" << std::endl;
    }
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