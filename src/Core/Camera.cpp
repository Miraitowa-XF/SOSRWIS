#include "Camera.h"
#include <cmath>

// 小的数值安全保护
static inline float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// 指数平滑参数转换： 将“速度（每秒）” 和 deltaTime 转换为插值因子 alpha（0..1）
// 使用公式 alpha = 1 - exp(-speed * deltaTime)
// 这样可以获得帧率无关的平滑行为：speed 越大响应越快（更接近即时），speed 越小越平滑
static inline float smoothingAlpha(float speed, float deltaTime) {
    if (speed <= 0.0f) return 0.0f;
    // 防止 deltaTime 太大导致 exp 下溢差异，通常 deltaTime <= 0.1
    return 1.0f - std::exp(-speed * deltaTime);
}

// ---------------------- 构造 ----------------------
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Position(position),
    WorldUp(up),
    Yaw(yaw),
    Pitch(pitch),
    TargetYaw(yaw),
    TargetPitch(pitch),
    MovementSpeed(DEFAULT_SPEED),
    MouseSensitivity(DEFAULT_SENSITIVITY),
    Zoom(DEFAULT_ZOOM),
    TargetZoom(DEFAULT_ZOOM),
    RotationSmoothSpeed(12.0f), // 建议 8~20，越大越快（接近即时），越小越平滑
    ZoomSmoothSpeed(12.0f)      // 缩放也做平滑（可调）
{
    updateCameraVectors();
}

// ---------------------- Update（每帧调用） ----------------------
void Camera::Update(float deltaTime)
{
    if (deltaTime <= 0.0f) return;

    // 计算角度平滑插值系数（frame-rate independent）
    float alpha = smoothingAlpha(RotationSmoothSpeed, deltaTime);

    // 平滑地逼近目标角度
    // 处理角度跨 360 的情况：我们按简单方式处理（Yaw 一般不超大）
    Yaw = Yaw + (TargetYaw - Yaw) * alpha;
    Pitch = Pitch + (TargetPitch - Pitch) * alpha;

    // 限制 Pitch 到安全范围，避免翻转
    Pitch = clampf(Pitch, -89.0f, 89.0f);

    // 缩放平滑（可选）
    float alphaZoom = smoothingAlpha(ZoomSmoothSpeed, deltaTime);
    Zoom = Zoom + (TargetZoom - Zoom) * alphaZoom;
    Zoom = clampf(Zoom, 1.0f, 90.0f);

    // 更新方向向量
    updateCameraVectors();
}

// ---------------------- 获取视图矩阵 ----------------------
glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

// ---------------------- 键盘移动 ----------------------
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;

    if (FPS_Mode)
    {
        // FPS 模式：只能在 XZ 平面上移动
        if (direction == FORWARD)
            Position += glm::normalize(glm::vec3(Front.x, 0.0f, Front.z)) * velocity;
        if (direction == BACKWARD)
            Position -= glm::normalize(glm::vec3(Front.x, 0.0f, Front.z)) * velocity;
        if (direction == LEFT)
            Position -= glm::normalize(glm::vec3(Right.x, 0.0f, Right.z)) * velocity;
        if (direction == RIGHT)
            Position += glm::normalize(glm::vec3(Right.x, 0.0f, Right.z)) * velocity;

        // 强制锁定高度 (模拟重力)
        Position.y = GroundHeight;
    }
    else
    {
        // 上帝模式：自由飞翔 (原代码)
        if (direction == FORWARD) Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT) Position -= Right * velocity;
        if (direction == RIGHT) Position += Right * velocity;
        if (direction == UP) Position += WorldUp * velocity;
        if (direction == DOWN) Position -= WorldUp * velocity;
    }
}

// ---------------------- 鼠标移动 ----------------------
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    // 1. 灵敏度
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    // 2. 限制单帧最大旋转角度（单位：度）
    const float MAX_ROT_PER_FRAME = 2.5f;

    xoffset = glm::clamp(xoffset, -MAX_ROT_PER_FRAME, MAX_ROT_PER_FRAME);
    yoffset = glm::clamp(yoffset, -MAX_ROT_PER_FRAME, MAX_ROT_PER_FRAME);

    // 3. 更新目标角度
    TargetYaw += xoffset;
    TargetPitch += yoffset;

    if (constrainPitch)
        TargetPitch = glm::clamp(TargetPitch, -89.0f, 89.0f);
}


// ---------------------- 鼠标滚轮 ----------------------
void Camera::ProcessMouseScroll(float yoffset)
{
    // 将滚轮直接映射到目标缩放（可平滑到 Zoom）
    // 常见做法：减小 yoffset -> 缩小视角 -> 放大场景
    TargetZoom -= yoffset;
    TargetZoom = clampf(TargetZoom, 1.0f, 90.0f);
}

// ---------------------- 更新方向向量 ----------------------
void Camera::updateCameraVectors()
{
    // 从欧拉角计算 Front 向量
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // 右向和上向
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::ProcessMouseMovementNormalized(float nx, float ny, bool constrainPitch)
{
    // 参数设计（可调）
    // fullScreenYawDeg = 当鼠标在一帧内相当于“跨越整个窗口宽”时对应的角度（度）
    // 例如：fullScreenYawDeg = 180 表示把鼠标从窗口左边拖到右边对应 180°
    const float fullScreenYawDeg = 180.0f; // 可调
    const float fullScreenPitchDeg = 90.0f; // 上下视角范围（限制幅度）

    // sensitivity 是一个系数（0.0 ~ 1.0），控制手感
    // 推荐初始值：0.8 ~ 1.0（因为我们已经按屏幕归一化）
    float sensitivity = this->MouseSensitivity; // 你可在外部设置，例如 0.5~1.0

    // 计算这帧的角度增量（度）
    float dYawDeg = nx * fullScreenYawDeg * sensitivity;
    float dPitchDeg = ny * fullScreenPitchDeg * sensitivity;

    // 单帧角度保护：最大角度限制（deg）
    const float MAX_DEG_PER_FRAME = 3.5f; // 推荐 1.5 ~ 4.0，根据个人手感调节
    if (dYawDeg > MAX_DEG_PER_FRAME) dYawDeg = MAX_DEG_PER_FRAME;
    if (dYawDeg < -MAX_DEG_PER_FRAME) dYawDeg = -MAX_DEG_PER_FRAME;
    if (dPitchDeg > MAX_DEG_PER_FRAME) dPitchDeg = MAX_DEG_PER_FRAME;
    if (dPitchDeg < -MAX_DEG_PER_FRAME) dPitchDeg = -MAX_DEG_PER_FRAME;

    // 更新目标角度（TargetYaw/TargetPitch）
    TargetYaw += dYawDeg;
    TargetPitch += dPitchDeg;

    // 俯仰限制
    if (constrainPitch)
    {
        if (TargetPitch > 89.0f) TargetPitch = 89.0f;
        if (TargetPitch < -89.0f) TargetPitch = -89.0f;
    }
}
