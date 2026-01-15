// =========================================================================
// 这是一个测试代码，测试你的环境是否正常配置完成，如果运行成功并且输出结果全为OK，则表示你环境已经配置好了。
// 预处理指令与头文件包含 （如果现实错误信息：后面有“::”的名称一定是类名或命名空间名
//                                        后面有“::”的名称一定是类名或命名空间名
//                                        请忽视报错，直接运行，通常可以直接运行）
// =========================================================================

#include "stb_image.h"

// 2. ImGui 头文件
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// 3. OpenGL 与 窗口库 (GLAD 必须在 GLFW 之前)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// 4. GLM 数学库
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 5. Assimp 模型加载库
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// 6. 标准库
#include <iostream>
#include <filesystem> // C++17 标准文件系统库
#include <string>

// 7. 自定义库 (src 目录)
#include "example.h"

// =========================================================================
// 全局设置
// =========================================================================
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 辅助打印函数
void printStatus(const std::string& component, bool success, const std::string& message) {
    if (success)
        std::cout << "[ OK ] " << component << ": " << message << std::endl;
    else
        std::cerr << "[FAIL] " << component << ": " << message << std::endl;
}

// 窗口回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// =========================================================================
// 主函数
// =========================================================================
int main()
{
    std::cout << "\n========== SOSRWIS 全面系统自检启动 ==========\n" << std::endl;

    // -----------------------------------------------------------
    // TEST 1: 验证 CoreLib (自定义源码)
    // -----------------------------------------------------------
    {
        int res = add(100, 200);
        if (res == 300) printStatus("CoreLib (Src)", true, "add(100, 200) = 300");
        else printStatus("CoreLib (Src)", false, "add() 函数计算错误");
    }

    // -----------------------------------------------------------
    // TEST 2: 验证 Assets 资源目录 (C++17 filesystem)
    // -----------------------------------------------------------
    {
        // CMake 应该把 assets 文件夹复制到了 exe 同级目录
        if (std::filesystem::exists("assets")) {
            printStatus("Assets", true, "检测到 assets 文件夹");

            // 进一步检查 assets 内部结构 (可选)
            if (std::filesystem::exists("assets/models"))
                std::cout << "       -> models 目录存在" << std::endl;
            else
                std::cout << "       -> [警告] assets/models 缺失" << std::endl;
        }
        else {
            printStatus("Assets", false, "未找到 assets 文件夹! 请检查 CMake 配置或手动复制。");
        }
    }

    // -----------------------------------------------------------
    // TEST 3: 验证 GLM (数学库)
    // -----------------------------------------------------------
    {
        glm::vec3 v(1.0f, 0.0f, 0.0f);
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::translate(trans, glm::vec3(1.0f, 1.0f, 0.0f));
        glm::vec4 result = trans * glm::vec4(v, 1.0f);

        // 原始(1,0,0) + 移动(1,1,0) = (2,1,0)
        if (result.x == 2.0f && result.y == 1.0f)
            printStatus("GLM", true, "矩阵变换计算正确");
        else
            printStatus("GLM", false, "矩阵计算结果异常");
    }

    // -----------------------------------------------------------
    // TEST 4: 验证 Assimp (模型加载库链接)
    // -----------------------------------------------------------
    {
        // 只要这行代码不报错，说明链接成功
        Assimp::Importer importer;
        // 随便打印个版本号证明它活着
        // 注意：如果你是静态编译，这证明库已经打进去了
        printStatus("Assimp", true, "Importer 实例化成功 (静态链接正常)");
    }

    // -----------------------------------------------------------
    // TEST 5: 验证 stb_image (图像加载)
    // -----------------------------------------------------------
    {
        int width, height, nrChannels;
        // 我们尝试加载一个不存在的文件，只要编译通过且能运行到判断逻辑，说明库没问题
        unsigned char* data = stbi_load("assets/textures/non_existent.jpg", &width, &height, &nrChannels, 0);
        if (!data) {
            // 这是预期的，因为我们没给真正的图片。
            // 关键是 stbi_load 函数被成功调用了，没有报 "Undefined Symbol"
            printStatus("stb_image", true, "函数链接正常 (测试加载空文件返回 NULL)");
        }
    }

    // -----------------------------------------------------------
    // TEST 6: 初始化 GLFW 窗口
    // -----------------------------------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SOSRWIS - All Libs Test", NULL, NULL);
    if (window == NULL) {
        printStatus("GLFW", false, "窗口创建失败");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    printStatus("GLFW", true, "窗口创建成功");

    // -----------------------------------------------------------
    // TEST 7: 初始化 GLAD
    // -----------------------------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printStatus("GLAD", false, "OpenGL 加载失败");
        return -1;
    }
    printStatus("GLAD", true, (std::string("OpenGL Version: ") + (char*)glGetString(GL_VERSION)));

    // -----------------------------------------------------------
    // TEST 8: 初始化 ImGui (UI 交互库)
    // -----------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // 设置暗黑主题

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    printStatus("ImGui", true, "UI 上下文初始化完成");

    std::cout << "\n========== 所有库检测完毕，进入渲染循环 ==========\n" << std::endl;

    // =========================================================================
    // 渲染循环
    // =========================================================================
    bool show_demo_window = true;
    while (!glfwWindowShouldClose(window))
    {
        // 1. 处理输入
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        glfwPollEvents();

        // 2. 开启 ImGui 新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 3. 构建 ImGui 界面
        {
            ImGui::Begin("SOSRWIS Control Panel Test"); // 创建一个窗口
            ImGui::Text("If you see this, ImGui is working!");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            static float f = 0.0f;
            ImGui::SliderFloat("Float Test", &f, 0.0f, 1.0f);

            if (ImGui::Button("Close App"))
                glfwSetWindowShouldClose(window, true);

            ImGui::End();
        }

        // 4. 渲染 OpenGL 背景
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // 深灰色背景
        glClear(GL_COLOR_BUFFER_BIT);

        // 5. 渲染 ImGui 到屏幕
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 6. 交换缓冲
        glfwSwapBuffers(window);
    }

    // 清理资源
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}