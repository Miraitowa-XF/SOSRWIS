#pragma once
#include <glm/glm.hpp>
#include <algorithm> // for min/max

struct AABB {
    glm::vec3 min; // 盒子左下后角
    glm::vec3 max; // 盒子右上前角

    // 默认构造
    AABB() : min(0.0f), max(0.0f) {}
    // 构造函数
    AABB(glm::vec3 _min, glm::vec3 _max) : min(_min), max(_max) {}

    // 检测两个盒子是否重叠 (AABB Collision)
    bool checkCollision(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }
};