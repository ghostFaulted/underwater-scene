#include "Raycaster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Ray Raycaster::ScreenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4& view, const glm::mat4& projection) {
    // 1. Normalized Device Coordinates (NDC): [-1, 1]
    float x = (2.0f * static_cast<float>(mouseX)) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight; // Y axis is inverted
    float z = 1.0f;
    glm::vec3 ray_nds = glm::vec3(x, y, z);

    // 2. Homogeneous Clip Coordinates (Clip Space)
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

    // 3. Eye Coordinates (Eye Space)
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

    // 4. World Coordinates (World Space)
    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    // Extract camera position from view matrix
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 ray_origin = glm::vec3(invView[3]);

    return { ray_origin, ray_wor };
}

bool Raycaster::IntersectAABB(const Ray& ray, const AABB& box, float& t) {
    glm::vec3 invDir = 1.0f / ray.direction;

    glm::vec3 t0 = (box.min - ray.origin) * invDir;
    glm::vec3 t1 = (box.max - ray.origin) * invDir;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float tmin_max = std::max(std::max(tmin.x, tmin.y), tmin.z);
    float tmax_min = std::min(std::min(tmax.x, tmax.y), tmax.z);

    if (tmax_min >= tmin_max && tmax_min >= 0.0f) {
        t = tmin_max > 0.0f ? tmin_max : tmax_min;
        return true;
    }
    return false;
}