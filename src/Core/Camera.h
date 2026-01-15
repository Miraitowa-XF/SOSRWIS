#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*
 * 优化说明（必读）：
 * - 鼠标移动会修改 TargetYaw / TargetPitch（目标角度）
 * - 每帧调用 Update(deltaTime) 会使用指数平滑将当前角度平滑逼近目标角度
 * - 这样能显著降低鼠标抖动与视角“割裂”感，同时保留灵敏度响应
 *
 * 使用方法（示例）：
 *   Camera camera(glm::vec3(0.0f, 1.8f, 5.0f));
 *   // 在 GLFW 回调或主循环处理中：
 *   camera.ProcessMouseMovement(dx, dy);
 *   camera.ProcessKeyboard(FORWARD, deltaTime);
 *   camera.Update(deltaTime);                // 每帧必须调用
 *   glm::mat4 view = camera.GetViewMatrix();
 */

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// 默认参数
static constexpr float DEFAULT_YAW = -90.0f;        // 偏航角 0度朝向 +X 轴，-90度朝向 -Z 轴
static constexpr float DEFAULT_PITCH = 0.0f;        // 俯仰角
static constexpr float DEFAULT_SPEED = 3.5f;
static constexpr float DEFAULT_SENSITIVITY = 0.005f;
static constexpr float DEFAULT_ZOOM = 45.0f;

class Camera
{
public:
    // --- 公有属性（可直接设置/查询） ---
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // 当前角度（实际用于计算 Front）
    float Yaw;
    float Pitch;

    // 目标角度（由鼠标输入直接修改）
    float TargetYaw;
    float TargetPitch;

    // 选项
    float MovementSpeed;       // 平移速度
    float MouseSensitivity;    // 鼠标灵敏度（影响目标角度变更幅度）
    float Zoom;                // 当前 FOV（度）
    float TargetZoom;          // 目标 FOV（可用于平滑缩放，如果需要）

    // 平滑参数（以每秒为单位的“速度”）
    // 更大的值意味着更快逼近目标（接近无平滑），更小的值意味着更平滑（慢响应）
    float RotationSmoothSpeed; // 角度平滑速度（单位：1/秒）
    float ZoomSmoothSpeed;     // 缩放平滑速度（单位：1/秒）

    bool FPS_Mode = false; // 默认关闭，按键开启
    float GroundHeight = 2.5f; // 人的眼睛高度

public:
    // 构造函数
    Camera(
		glm::vec3 position = glm::vec3(0.0f, 1.8f, 5.0f),       // 这里是默认的值，后续在main.cpp中会覆盖
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),             // 定义世界向上方向
        float yaw = DEFAULT_YAW,
        float pitch = DEFAULT_PITCH
    );

    // 每帧调用，用于平滑更新（必须在渲染前每帧调用）
    void Update(float deltaTime);

    // 获取当前视图矩阵（调用时假定已经执行 Update）
    glm::mat4 GetViewMatrix() const;

    // 处理键盘移动（会立即改变 Position）
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    // 处理鼠标移动：更新目标角度（不直接改变当前角度）
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // 处理滚轮：改变目标缩放（TargetZoom），可在 Update 中平滑到 Zoom
    void ProcessMouseScroll(float yoffset);

    // 可选：直接设置目标位置/角度
    void SetPosition(const glm::vec3& pos) { Position = pos; }
    void SetTargetAngles(float yaw, float pitch) { TargetYaw = yaw; TargetPitch = pitch; }

    // 传入归一化的偏移量（[-1,1] 大小），由 Camera 将其映射为角度增量
    void ProcessMouseMovementNormalized(float nx, float ny, bool constrainPitch = true);

private:
    // 更新 Front / Right / Up（基于当前 Yaw / Pitch）
    void updateCameraVectors();
};
