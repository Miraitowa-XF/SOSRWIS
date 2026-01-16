#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 引入我们的核心库
#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Core/Collision.h"
#include "Renderer/Model.h"
#include "Renderer/Skybox.h"

#include <iostream>
#include <algorithm>  // for min/max logic inside main if needed
#include "stb_image.h"

//引入下雪场景必要的头文件
#include "Scene/Scene.h"

// 太阳系统
#include "Scene/SunSystem.h"

unsigned int planeVAO, planeVBO;
unsigned int snowTexture;

// --- 全局变量 ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 模型阴影的边缘清晰度
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; // 分辨率越高越清晰
//const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096; // 高分辨率参数设置，但是对于集显设备可能会在运行过程中卡死

// 摄像机系统
Camera camera(glm::vec3(0.0f, 3.0f, 0.0f));     // 初始位置的确定
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool isLampOn = true; // 路灯默认开启
bool lKeyPressed = false; // 按键防抖
// 增加一个防抖动变量
bool tabPressed = false;

// 鼠标锁定状态 (默认是锁定的)
bool isCursorLocked = true;
//按键防抖
bool altPressed = false;

bool showColliders = false; // 是否显示空气墙
unsigned int debugCubeVAO = 0, debugCubeVBO = 0;

// 定义一个结构体，用来管理场景里的每一个物体
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

// 存储所有场景对象的列表
std::vector<SceneObject> allObjects;

// 存储所有障碍物的碰撞盒列表
std::vector<AABB> sceneColliders;

//下雪场景必要全局变量
SnowScene snowyScene;
static double lastToggleTimeF = 0.0;
static double toggleCooldown = 0.15;

// 太阳系统
SunSystem sunSystem;
float dayTime = 0.0f; // 【修改】0.0 代表午夜 (00:00)，也就是程序启动就是黑夜


// 回调函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// 手动添加一个障碍物 (空气墙)
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

//初始化snowyscene
void initSnowyScene()
{
    snowyScene.Init("assets/shaders/particle.vert", "assets/shaders/particle.frag", "assets/textures/snow/snowflake.png");
    snowyScene.GetParticleSystem().SetSpawnRate(100.0f);
    snowyScene.GetParticleSystem().SetWind(glm::vec3(0.2f, 0.0f, 0.1f));
    snowyScene.GetParticleSystem().SetActive(false);
}

// 太阳系统
void initSunSystem()
{
    sunSystem.Init("assets/shaders/sun.vert", "assets/shaders/sun.frag");
}

// 封装的绘制场景函数
// 参数：当前使用的 Shader
void drawScene(Shader& shader, const std::vector<SceneObject>& objects, Model& ground)
{
    // 绘制物体时关闭剔除，让树叶双面可见！
    glDisable(GL_CULL_FACE);

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

    // 画完可以开回来，或者就一直关着也行
    glEnable(GL_CULL_FACE);
}

int main()
{
    // 1. 初始化 GLFW (不变)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 开启 4 倍抗锯齿
    glfwWindowHint(GLFW_SAMPLES, 4);

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
    // 开启混合 (解决玻璃透明)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 开启面剔除 (解决动漫模型黑色乱码)
    glEnable(GL_CULL_FACE);

    // 启用多重采样
    glEnable(GL_MULTISAMPLE);

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
    // 请确保 assets/models/***/***.gltf 存在，否则程序会报错, 如果加载失败也会在控制台输出
    Model houseModel("assets/models/snowy_wooden_hut/scene.gltf");
    Model groundModel("assets/models/snow_floor/scene.gltf");
    Model snowmanModel("assets/models/snow_man/scene.gltf");
    Model house2Model("assets/models/lowpoly_snow_house/scene.gltf");
    Model treesModel("assets/models/newtrees/scene.gltf");
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
    // 喷泉旁边的路灯
    allObjects.push_back(SceneObject(&lampModel, glm::vec3(-8.0f, 0.0f, -12.0f), glm::vec3(2.0f), 45.0f, glm::vec3(0, 1, 0)));
    // 3. 长椅路灯
    allObjects.push_back(SceneObject(&lampModel, glm::vec3(22.0f, 0.0f, 10.0f), glm::vec3(2.0f), -45.0f, glm::vec3(0, 1, 0)));
    // 4. 村庄路灯
    allObjects.push_back(SceneObject(&lampModel, glm::vec3(6.0f, 0.0f, -40.0f), glm::vec3(2.0f), 135.0f, glm::vec3(0, 1, 0)));

    // (大工程)手动定义不可通行的区域
    // 你需要利用之前写的“打印坐标”功能，走到墙边，记下坐标，然后在这里写代码
    // 例如：在广场中心加一堵空气墙
    // 参数：中心点(0, 2, -45)， 尺寸(宽10，高10，厚10)
    
    // 空气墙
    addInvisibleWall(glm::vec3(0.0f, 3.0f, -40.0f), glm::vec3(6.0f, 8.0f, 6.0f)); // 龙旁边的房子
    addInvisibleWall(glm::vec3(0.0f, 0.5f, 19.0f), glm::vec3(23.0f, 15.0f, 15.0f)); // 树旁边的房子
    addInvisibleWall(glm::vec3(-27.0f, 7.0f, -6.5f), glm::vec3(10.0f, 14.0f, 13.0f)); // 三栋中靠车的房子
    addInvisibleWall(glm::vec3(-25.0f, 7.0f, -19.5f), glm::vec3(11.0f, 14.0f, 10.5f)); // 三栋中居中的房子
    addInvisibleWall(glm::vec3(-25.0f, 7.0f, -30.8f), glm::vec3(10.5f, 18.0f, 10.0f)); // 三栋中靠龙的房子
    addInvisibleWall(glm::vec3(-25.0f, 3.0f, 20.0f), glm::vec3(8.0f, 8.0f, 17.0f)); // 集装箱
    addInvisibleWall(glm::vec3(-35.0f, 4.0f, 20.5f), glm::vec3(7.0f, 8.0f, 18.0f)); // 巴士
    // 喷泉的空气墙绘制
    addInvisibleWall(glm::vec3(-0.4f, 2.0f, -15.8f), glm::vec3(7.0f, 7.0f, 7.0f));
    addInvisibleWall(glm::vec3(-0.4f, 2.0f, -15.8f), glm::vec3(8.0f, 7.0f, 6.0f));
    addInvisibleWall(glm::vec3(-0.4f, 2.0f, -15.8f), glm::vec3(6.0f, 7.0f, 8.0f));
    addInvisibleWall(glm::vec3(-0.4f, 2.0f, -15.8f), glm::vec3(9.0f, 7.0f, 4.8f));
    addInvisibleWall(glm::vec3(-0.4f, 2.0f, -15.8f), glm::vec3(4.8f, 7.0f, 9.0f));
    // 雕像空气墙绘制
    addInvisibleWall(glm::vec3(-0.0f, 2.0f, -25.0f), glm::vec3(1.0f, 7.0f, 1.0f));
    // 路灯空气墙绘制
    addInvisibleWall(glm::vec3(-8.0f, 2.0f, -12.0f), glm::vec3(0.5f, 7.0f, 0.5f));
    addInvisibleWall(glm::vec3(22.0f, 2.0f, 10.0f), glm::vec3(0.5f, 7.0f, 0.5f));
    addInvisibleWall(glm::vec3(6.0f, 2.0f, -40.0f), glm::vec3(0.5f, 7.0f, 0.5f));
    addInvisibleWall(glm::vec3(-17.0f, 2.0f, 15.0f), glm::vec3(0.5f, 7.0f, 0.5f));
    // 圣诞树空气墙绘制
    addInvisibleWall(glm::vec3(18.0f, 2.0f, -8.0f), glm::vec3(5.5f, 10.0f, 5.5f));
    addInvisibleWall(glm::vec3(25.0f, 2.0f, -15.0f), glm::vec3(5.5f, 10.0f, 5.5f));
    addInvisibleWall(glm::vec3(35.0f, 2.0f, -5.0f), glm::vec3(5.5f, 10.0f, 5.5f));
    addInvisibleWall(glm::vec3(33.0f, 2.0f, -23.5f), glm::vec3(5.5f, 10.0f, 5.5f));
    addInvisibleWall(glm::vec3(15.0f, 2.0f, -25.0f), glm::vec3(5.5f, 10.0f, 5.5f));
    // 松树空气墙
    addInvisibleWall(glm::vec3(25.0f, 2.0f, 25.0f), glm::vec3(5.5f, 20.0f, 5.5f));
    // 雪人空气墙
    addInvisibleWall(glm::vec3(-5.0f, 1.0f, -35.0f), glm::vec3(1.2f, 2.0f, 1.2f));
    // 楼栋前的雪人空气墙
    addInvisibleWall(glm::vec3(-17.0f, 2.0f, -14.0f), glm::vec3(1.5f, 4.0f, 1.5f));
    // 邮箱空气墙
    addInvisibleWall(glm::vec3(5.0f, 1.0f, -35.0f), glm::vec3(0.5f, 2.0f, 0.5f));
    // 桌子空气墙
    addInvisibleWall(glm::vec3(18.0f, 0.0f, 10.0f), glm::vec3(4.0f, 2.0f, 3.0f));
    // 水井空气墙
    addInvisibleWall(glm::vec3(34.7f, 1.0f, 15.0f), glm::vec3(1.5f, 2.0f, 1.5f));
    // 圣诞树旁边人物 
    addInvisibleWall(glm::vec3(15.0f, 2.0f, -14.0f), glm::vec3(3.0f, 4.0f, 1.5f));
    addInvisibleWall(glm::vec3(15.0f, 2.0f, -17.0f), glm::vec3(0.5f, 4.0f, 0.8f));
    addInvisibleWall(glm::vec3(15.0f, 1.0f, -19.0f), glm::vec3(0.7f, 2.0f, 0.3f));
    // 勇士
    addInvisibleWall(glm::vec3(10.0f, 2.0f, -30.0f), glm::vec3(1.2f, 6.0f, 0.5f));
    


    // 初始化下雪场景
    initSnowyScene();

    // 太阳系统
    initSunSystem();

    // 4. 渲染循环
    // ==========================================
    // 配置阴影贴图 FBO
    // ==========================================
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
        // 碰撞检测回退逻辑
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

        // 更新下雪粒子 (必须在每一帧开始时做)
        snowyScene.Update(deltaTime);

        // 太阳系统
        sunSystem.Update(deltaTime, dayTime);

        ourShader.use();

        //      // 设置光照和相机矩阵
        //      ourShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));       // (1.0, 1.0, 1.0) 代表纯白色光。
              //ourShader.setVec3("lightPos", glm::vec3(0.0f, 20.0f, 0.0f));         // 光源位置
        //      ourShader.setVec3("viewPos", camera.Position);


        //// 定义光源位置 (需要固定，不能乱跑)
        //glm::vec3 lightPos(10.0f, 20.0f, 10.0f); // 模拟太阳，放高一点

        // 太阳系统
        glm::vec3 lightPos = sunSystem.worldPos;

        // ============================================================
        // 1. 第一遍渲染：从光源视角生成深度图 (Shadow Pass)
        // ============================================================

        // 计算光空间矩阵 (正交投影适合定向光/太阳光)
        float near_plane = 1.0f, far_plane = 300.0f;
        // 下面的参数决定了阴影覆盖的范围，太小会导致远处没影子，太大导致影子模糊
        glm::mat4 lightProjection = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0)); // 使用 sunSystem.worldPos 作为 lightPos 计算 lightSpaceMatrix
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

        // ===========================================
        // 【升级】传递 4 盏路灯的参数
        // ===========================================

        // 灯泡的偏移高度 (灯杆高 2.0 倍，灯泡大概在 7.0 高度)
        float lampHeightOffset = 7.0f;

        // 定义 4 盏灯的底座坐标 (跟上面添加模型的坐标保持一致)
        glm::vec3 lampPositions[] = {
            glm::vec3(-17.0f, 0.0f, 15.0f),  // 1. 原路灯
            glm::vec3(-8.0f, 0.0f, -12.0f),  // 2. 喷泉路灯
            glm::vec3(22.0f, 0.0f, 10.0f),   // 3. 长椅路灯
            glm::vec3(6.0f, 0.0f, -40.0f)  // 4. 村庄路灯
        };

        // 循环传递数组给 Shader
        for (int i = 0; i < 4; i++)
        {
            std::string number = std::to_string(i);

            // 位置：底座坐标 + 高度偏移
            ourShader.setVec3("pointLights[" + number + "].position", lampPositions[i] + glm::vec3(0.0f, lampHeightOffset, 0.0f));

            // 颜色：暖黄光
            ourShader.setVec3("pointLights[" + number + "].color", glm::vec3(1.0f, 0.8f, 0.4f));

            // 衰减参数 (覆盖范围约 50 米)
            ourShader.setFloat("pointLights[" + number + "].constant", 1.0f);
            ourShader.setFloat("pointLights[" + number + "].linear", 0.09f);
            ourShader.setFloat("pointLights[" + number + "].quadratic", 0.032f);
        }

        // 总开关 (受 G 键控制)
        ourShader.setBool("lampOn", isLampOn);
        
        // 太阳系统
        // 将太阳的实时数据传给场景物体的着色器
        ourShader.setVec3("lightPos", lightPos);      // 太阳光方向
        ourShader.setVec3("lightColor", sunSystem.color);        // 太阳光颜色
        ourShader.setFloat("sunIntensity", sunSystem.intensity); // 太阳光强度
        ourShader.setFloat("ambientStrength", sunSystem.ambient); // 随时间变化的环境光
        ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix); // 阴影矩阵
        ourShader.setVec3("viewPos", camera.Position);


        // 绑定阴影贴图到 15 号槽
        glActiveTexture(GL_TEXTURE15);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 300.0f);
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

        // 最后绘制雪花 (必须在最后，因为它是半透明的)
        snowyScene.Render(camera);

        // 太阳系统
        sunSystem.Render(camera);

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

    // 鼠标锁定/解锁切换 (Left Alt)
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && !altPressed)
    {
        isCursorLocked = !isCursorLocked; // 切换状态
        altPressed = true;

        if (isCursorLocked) {
            // 锁定鼠标：隐藏光标，无限移动 (FPS模式)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // 重置 firstMouse，防止切回来时视角乱跳
            firstMouse = true;
            std::cout << "Mouse: LOCKED (Camera Control)" << std::endl;
        }
        else {
            // 解放鼠标：显示光标，可以移出窗口
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "Mouse: UNLOCKED (UI Mode)" << std::endl;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE)
    {
        altPressed = false;
    }

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

    //下雪天气开关：O/P, L，O是下中雪、P是停止下雪、L是下大雪，K是下小雪
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS){
        snowyScene.setSmallSnow(true);
		double t = glfwGetTime();
		if (t - lastToggleTimeF > toggleCooldown) {
			snowyScene.GetParticleSystem().SetSpawnRate(400.0f); // 下小雪
			snowyScene.GetParticleSystem().SetActive(true);
			lastToggleTimeF = t;
            printf("small snow: ON\n");
        }
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        snowyScene.setSmallSnow(false);
        double t = glfwGetTime();
        if (t - lastToggleTimeF > toggleCooldown) {
            snowyScene.GetParticleSystem().SetSpawnRate(800.0f); // 下中雪
            snowyScene.GetParticleSystem().SetActive(true);
            lastToggleTimeF = t;
            printf("mid Snow: ON\n");
        }
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        snowyScene.setSmallSnow(false);
        double t = glfwGetTime();
        if (t - lastToggleTimeF > toggleCooldown) {
            snowyScene.GetParticleSystem().SetActive(false);
            lastToggleTimeF = t;
            printf("Snow: OFF\n");
        }
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        snowyScene.setSmallSnow(false);
        double t = glfwGetTime();
        if (t - lastToggleTimeF > toggleCooldown) {
            snowyScene.GetParticleSystem().SetSpawnRate(1600.0f); // 下大雪
            snowyScene.GetParticleSystem().SetActive(true);
            lastToggleTimeF = t;
            printf("Heavy snow: ON\n");
        }
    }
    // 太阳系统
    // 控制太阳时间 (例如：按键盘左/右键)
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        dayTime += 0.1f * deltaTime; // 时间前进
        if (dayTime > 1.0f) dayTime = 0.0f;

        // 确保 dayTime 在 0.0 ~ 1.0 之间循环
        if (dayTime > 1.0f) dayTime -= 1.0f;
        if (dayTime < 0.0f) dayTime += 1.0f;

        // --- 核心换算逻辑 ---
        // 1. 一天总共有 1440 分钟
        int totalMinutes = static_cast<int>(dayTime * 1440);

        // 2. 计算小时和分钟
        int hours = (totalMinutes / 60) % 24;
        int minutes = totalMinutes % 60;

        // 3. 输出时间。使用 %02d 可以确保不足两位时补0（例如 04:05）
        // \r 是回车符，可以让输出在同一行刷新，不会刷屏
        printf("\rCurrent Simulation Time: [%02d:%02d] (dayTime: %.4f)", hours, minutes, dayTime);
        fflush(stdout); // 强制刷新缓冲区，确保实时显示
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        dayTime -= 0.1f * deltaTime; // 时间后退
        if (dayTime < 0.0f) dayTime = 1.0f;

        // 确保 dayTime 在 0.0 ~ 1.0 之间循环
        if (dayTime > 1.0f) dayTime -= 1.0f;
        if (dayTime < 0.0f) dayTime += 1.0f;

        // --- 核心换算逻辑 ---
        // 1. 一天总共有 1440 分钟
        int totalMinutes = static_cast<int>(dayTime * 1440);

        // 2. 计算小时和分钟
        int hours = (totalMinutes / 60) % 24;
        int minutes = totalMinutes % 60;

        // 3. 输出时间。使用 %02d 可以确保不足两位时补0（例如 04:05）
        // \r 是回车符，可以让输出在同一行刷新，不会刷屏
        printf("\rCurrent Simulation Time: [%02d:%02d] (dayTime: %.4f)", hours, minutes, dayTime);
        fflush(stdout); // 强制刷新缓冲区，确保实时显示
    }

    // 按 'G' 键切换路灯
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !lKeyPressed)
    {
        isLampOn = !isLampOn;
        lKeyPressed = true;
        if (isLampOn) std::cout << "Street Lamp: ON" << std::endl;
        else std::cout << "Street Lamp: OFF" << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
    {
        lKeyPressed = false;
    }
}       

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    // 如果鼠标是解锁状态，直接返回，不计算视角旋转
    if (!isCursorLocked) return;

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

