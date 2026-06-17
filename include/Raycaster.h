#pragma once

#include <glm/glm.hpp>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Raycaster {
public:
    static Ray ScreenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4& view, const glm::mat4& projection);
    static bool IntersectAABB(const Ray& ray, const AABB& box, float& intersectionDistance);
};