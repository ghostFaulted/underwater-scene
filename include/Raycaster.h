#pragma once

#include <glm/glm.hpp>

// Reprezentacja promienia, startujacego z okreslonego punktu, w okreslonym wektorowym kierunku.
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

// Obwiednia (Axis-Aligned Bounding Box) w ksztalcie prostopadloscianu do taniego sprawdzania kolizji.
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Klasa posiadajaca mechanizmy do zamiany 2D obrazu na linie (Ray) wychodzaca wgab 3D sceny.
class Raycaster {
public:
    // Konwertuje polozenie myszki na oknie w odpowiadajacy mu wektor patrzenia przez rzut kamery.
    static Ray ScreenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4& view, const glm::mat4& projection);

    // Test przeciecia (Slab Method) promienia 3D z pudelkiem kolizyjnym AABB
    static bool IntersectAABB(const Ray& ray, const AABB& box, float& intersectionDistance);
};