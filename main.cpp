#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 引入我们的核心库
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Core/Collision.h"
#include "Renderer/Model.h" // <--- 【新增】引入模型类
#include "Renderer/Skybox.h"

#include <iostream>
#include <algorithm>  // for min/max logic inside main if needed
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

bool showColliders = false; // 是否显示空气墙
unsigned int debugCubeVAO = 0, debugCubeVBO = 0;

// 【新增】定义一个结构体，用来管理场景里的每一个物体
struct SceneObject {
    Model* model;       // 模型指针
    glm::vec3 position; // 位置
    glm::vec3 scale;    // 缩放
    float rotationAngle; // 旋转角度 (度)
    glm::vec3 rotationAxis; // 旋转轴

    SceneObject(Model* m, glm::vec3 pos, glm::vec3 s, float rot, glm::vec3 axis)
        : model(m), position(pos), scale(s), rotationAngle(rot), rotationAxis(axis) {
    }
};

// 【新增】存储所有场景对象的列表
std::vector<SceneObject> allObjects;

// 【新增】存储所有障碍物的碰撞盒列表
std::vector<AABB> sceneColliders;

// 回调函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// 【新增】手动添加一个障碍物 (空气墙)
// center: 盒子的中心坐标
// size: 盒子的长宽高 (例如: vec3(10.0f, 5.0f, 1.0f) 代表一面 10米宽、1米厚的墙)
void addInvisibleWall(glm::vec3 center, glm::vec3 size) {
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 min = center - halfSize;
    glm::vec3 max = center + halfSize;

    // 直接加入碰撞列表
    sceneColliders.push_back(AABB(min, max));
}

// 在空间中建立参考立方体（用于可视化添加碰撞盒子）
void initDebugCube() {
    float vertices[] = {
        // 只需要一个单位立方体的顶点 (0,0,0) 到 (1,1,1) 或者 (-0.5,-0.5,-0.5) 到 (0.5,0.5,0.5)
        // 这里用 -0.5 到 0.5 方便缩放
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f
    };
    glGenVertexArrays(1, &debugCubeVAO);
    glGenBuffers(1, &debugCubeVBO);
    glBindVertexArray(debugCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// 封装的绘制场景函数
// 参数：当前使用的 Shader
void drawScene(Shader& shader, const std::vector<SceneObject>& objects, Model& ground)
{
    // 1. 绘制所有物体
    for (const auto& obj : objects)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.position);
        model = glm::rotate(model, glm::radians(obj.rotationAngle), obj.rotationAxis);
        model = glm::scale(model, obj.scale);
        shader.setMat4("model", model);
        obj.model->Draw(shader);
    }

    // 2. 绘制地面
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(25.0f, 0.0f, -25.0f));
    model = glm::scale(model, glm::vec3(0.25f));
    shader.setMat4("model", model);
    ground.Draw(shader);
}

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


    // 3. Skybox
    std::vector<std::string> faces
    {
        "assets/textures/skybox/px.png",
        "assets/textures/skybox/nx.png",
        "assets/textures/skybox/py.png",
        "assets/textures/skybox/ny.png",
        "assets/textures/skybox/pz.png",
        "assets/textures/skybox/nz.png"
    };
    // 创建 Skybox 对象
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

    initDebugCube();
    // =================================================================================
    // 【关键步骤】配置场景对象列表
    // 这里我们将之前分散在 while 循环里的硬编码参数统一管理。
    // 参数顺序：模型指针, 位置, 缩放, 旋转角度, 旋转轴
    // =================================================================================

    // 1. 房子
    allObjects.push_back(SceneObject(&houseModel, glm::vec3(0.0f, 3.0f, -40.0f), glm::vec3(2.5f), -90.0f, glm::vec3(0, 1, 0)));
    // 2. Jon Snow
    allObjects.push_back(SceneObject(&jonModel, glm::vec3(10.0f, 0.0f, -30.0f), glm::vec3(0.07f), 180.0f, glm::vec3(0, 1, 0)));
    // 3. Figure 1
    allObjects.push_back(SceneObject(&figure1, glm::vec3(15.0f, 0.0f, -19.0f), glm::vec3(0.5f), -90.0f, glm::vec3(0, 1, 0)));
    // 4. Figure 2
    allObjects.push_back(SceneObject(&figure2, glm::vec3(15.0f, 0.0f, -14.0f), glm::vec3(0.5f), -90.0f, glm::vec3(0, 1, 0)));
    // 5. Fairy
    allObjects.push_back(SceneObject(&fairyModel, glm::vec3(0.0f, 2.8f, -25.0f), glm::vec3(3.0f), 0.0f, glm::vec3(0, 1, 0)));
    // 6. Fountain
    allObjects.push_back(SceneObject(&fountain, glm::vec3(-3.0f, 0.0f, -15.0f), glm::vec3(0.015f), 0.0f, glm::vec3(0, 1, 0)));
    // 7. Dragon
    allObjects.push_back(SceneObject(&dragonModel, glm::vec3(30.0f, 0.0f, -43.0f), glm::vec3(5.0f), -65.0f, glm::vec3(0, 1, 0)));
    // 8. Lamp
    allObjects.push_back(SceneObject(&lampModel, glm::vec3(-17.0f, 0.0f, 15.0f), glm::vec3(2.0f), 0.0f, glm::vec3(0, 1, 0)));
    // 9. House 2
    allObjects.push_back(SceneObject(&house2Model, glm::vec3(0.0f, 0.5f, 20.0f), glm::vec3(15.0f), 180.0f, glm::vec3(0, 1, 0)));
    // 10. Village
    allObjects.push_back(SceneObject(&villageModel, glm::vec3(-25.0f, 0.0f, -20.0f), glm::vec3(0.5f), 90.0f, glm::vec3(0, 1, 0)));
    // 11. Resleriana
    allObjects.push_back(SceneObject(&reslerianaModel, glm::vec3(15.0f, 0.0f, -17.0f), glm::vec3(0.7f), -90.0f, glm::vec3(0, 1, 0)));
    // 12. Mailbox
    allObjects.push_back(SceneObject(&mailboxModel, glm::vec3(5.0f, 0.1f, -35.0f), glm::vec3(1.0f), 0.0f, glm::vec3(0, 1, 0)));
    // 13. Snowman
    allObjects.push_back(SceneObject(&snowmanModel, glm::vec3(-5.0f, 0.5f, -35.0f), glm::vec3(0.5f), -90.0f, glm::vec3(0, 1, 0)));
    // 14. Trees
    allObjects.push_back(SceneObject(&treesModel, glm::vec3(25.0f, 0.0f, 25.0f), glm::vec3(2.0f), 0.0f, glm::vec3(0, 1, 0)));
    // 15. Bench
    allObjects.push_back(SceneObject(&benchModel, glm::vec3(18.0f, 0.0f, 10.0f), glm::vec3(2.0f), 0.0f, glm::vec3(0, 1, 0)));
    // 16. Christmas Tree
    allObjects.push_back(SceneObject(&christmasTreesModel, glm::vec3(25.0f, 0.0f, -15.0f), glm::vec3(2.0f), 0.0f, glm::vec3(0, 1, 0)));
    // 17. Well
    allObjects.push_back(SceneObject(&wellModel, glm::vec3(35.0f, 0.0f, 15.0f), glm::vec3(0.01f), 0.0f, glm::vec3(0, 1, 0)));
    // 18. Container
    allObjects.push_back(SceneObject(&containerModel, glm::vec3(-25.0f, 0.0f, 20.0f), glm::vec3(3.0f), 90.0f, glm::vec3(0, 1, 0)));
    // 19. Bus
    allObjects.push_back(SceneObject(&busModel, glm::vec3(-35.0f, 4.0f, 20.0f), glm::vec3(4.0f), 180.0f, glm::vec3(0, 1, 0)));

    // (大工程)手动定义不可通行的区域
    // 你需要利用之前写的“打印坐标”功能，走到墙边，记下坐标，然后在这里写代码
    // 例如：给房子后面加一堵空气墙
    // 参数：中心点(0, 2, -45)， 尺寸(宽20，高5，厚2)
    addInvisibleWall(glm::vec3(0.0f, 2.0f, -5.0f), glm::vec3(10.0f, 10.0f, 10.0f));

    // ==========================================
    // 【新增】配置阴影贴图 FBO
    // ==========================================
    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096; // 分辨率越高越清晰
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    // 创建深度纹理
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // 设置纹理过滤
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // 设置纹理环绕 (防止阴影以外的区域变黑)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 绑定到 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE); // 不需要颜色
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 加载阴影 Shader
    Shader depthShader("assets/shaders/shadow_depth.vert", "assets/shaders/shadow_depth.frag");

    // 配置主 Shader 的阴影纹理槽位 (设为 15，避开模型自带纹理)
    ourShader.use();
    ourShader.setInt("shadowMap", 15);

    // 4. 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ==========================================
        // 【新增】碰撞检测回退逻辑
        // ==========================================
        glm::vec3 oldPosition = camera.Position; // 1. 备份位置

        // 处理输入
        processInput(window);

        // 3. 检查碰撞 (仅在 FPS 模式下)
        if (camera.FPS_Mode)
        {
            // 定义玩家的身体大小 (0.3宽, 1.8高)
            glm::vec3 playerHalfSize(0.3f, 0.9f, 0.3f);
            AABB playerBox(camera.Position - playerHalfSize, camera.Position + playerHalfSize);

            bool hit = false;
            for (const auto& box : sceneColliders) {
                if (playerBox.checkCollision(box)) {
                    hit = true;
                    break;
                }
            }

            if (hit) {
                // 撞墙了！回退！
                camera.Position = oldPosition;
            }

            // 锁定高度 (模拟在地面行走)
            camera.Position.y = 3.0f; // 这里的 3.0 是你设定的眼睛高度
        }
        // ==========================================

        // 每帧更新相机（把鼠标目标角度应用并平滑）
        camera.Update(deltaTime);

        //// 可选：当你觉得响应迟钝时，短时提高速度
        camera.RotationSmoothSpeed = 15.0f;
        camera.MouseSensitivity = 0.8f;

        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

  //      // 设置光照和相机矩阵
  //      ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));       // (1.0, 1.0, 1.0) 代表纯白色光。
		//ourShader.setVec3("lightPos", glm::vec3(0.0f, 20.0f, 0.0f));         // 光源位置
  //      ourShader.setVec3("viewPos", camera.Position);


        // 定义光源位置 (需要固定，不能乱跑)
        glm::vec3 lightPos(10.0f, 20.0f, 10.0f); // 模拟太阳，放高一点

        // ============================================================
        // 1. 第一遍渲染：从光源视角生成深度图 (Shadow Pass)
        // ============================================================

        // 计算光空间矩阵 (正交投影适合定向光/太阳光)
        float near_plane = 1.0f, far_plane = 100.0f;
        // 下面的参数决定了阴影覆盖的范围，太小会导致远处没影子，太大导致影子模糊
        glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        depthShader.use();
        depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); // 切换到阴影图分辨率
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // 【技巧】渲染阴影时使用正面剔除，可以极大减少“阴影悬浮”问题
        glCullFace(GL_FRONT);

        // 调用我们提取出来的绘制函数
        drawScene(depthShader, allObjects, groundModel);

        glCullFace(GL_BACK); // 改回背面剔除
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_DEPTH_BUFFER_BIT);

        // 【技巧】渲染阴影时使用正面剔除，可以极大减少“阴影悬浮”问题
        glCullFace(GL_FRONT);

        // 调用我们提取出来的绘制函数
        drawScene(depthShader, allObjects, groundModel);

        glCullFace(GL_BACK); // 改回背面剔除
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ============================================================
        // 2. 第二遍渲染：正常绘制场景 (Render Pass)
        // ============================================================
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // 恢复屏幕分辨率
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // 传递光照参数
        ourShader.setVec3("lightColor", glm::vec3(1.0f));
        ourShader.setVec3("lightPos", lightPos);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix); // 传给 Shader 算坐标

        // 绑定阴影贴图到 15 号槽
        glActiveTexture(GL_TEXTURE15);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // 这里 GetViewMatrix()
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        // 绘制场景
        drawScene(ourShader, allObjects, groundModel);

        // 地面 (外部模型) 
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(25.0f, 0.0f, -25.0f));
        model = glm::scale(model, glm::vec3(0.25f));
        ourShader.setMat4("model", model);
        groundModel.Draw(ourShader);

        // 绘制天空盒（注意：Skybox::Draw 使用的是没有平移的 view）
        skybox->Draw(view, projection);

        // 绘制空气墙
        if (showColliders) {
            // 使用线框模式
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            // 这里可以使用一个简单的纯色 shader，或者复用 ourShader 但忽略纹理
            // 为了简单，我们复用 ourShader，但需要一个纯白纹理（你之前在 Model.cpp 里写的 GetDefaultWhiteTexture 很有用）
            // 或者简单粗暴地利用 basic.frag 的特性（如果没有绑定材质，可能会变黑，但线框能看清就行）

            ourShader.use();
            ourShader.setVec3("lightColor", glm::vec3(1.0f)); // 确保够亮

            glBindVertexArray(debugCubeVAO);

            for (const auto& box : sceneColliders) {
                // 计算中心点和大小
                glm::vec3 size = box.max - box.min;
                glm::vec3 center = box.min + size * 0.5f;

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, center);
                model = glm::scale(model, size); // 缩放成盒子大小

                ourShader.setMat4("model", model);
                // 线框绘制
                glDrawArrays(GL_LINES, 0, 24);
            }

            // 恢复填充模式
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

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

    // 切换碰撞和盒子的可视化，定义一个静态变量防止连按
    static bool f1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        showColliders = !showColliders;
        f1Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
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

