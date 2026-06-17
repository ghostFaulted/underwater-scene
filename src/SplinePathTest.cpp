#include "../include/SplinePath.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

namespace {
    void RunSplinePathChecks() {
        auto controlPoints = std::vector<glm::vec3>{};
        controlPoints.emplace_back(-2.0f, -1.0f, 0.0f);
        controlPoints.emplace_back(-1.0f, -0.5f, 1.5f);
        controlPoints.emplace_back(0.0f, -0.2f, 2.5f);
        controlPoints.emplace_back(1.5f, 0.3f, 1.0f);
        controlPoints.emplace_back(3.0f, 0.0f, -0.5f);
        controlPoints.emplace_back(4.2f, -0.4f, -1.8f);

        SplinePath spline(controlPoints);

        std::cout << "[SplinePathTest] orthogonality check" << std::endl;
        for (int i = 0; i <= 12; ++i) {
            const auto t = static_cast<float>(i) / 12.0f;
            const auto transform = spline.GetTransform(t);

            const glm::vec3 right = glm::vec3(transform[0]);
            const glm::vec3 up = glm::vec3(transform[1]);
            const glm::vec3 forward = glm::vec3(transform[2]);
            const glm::vec3 position = glm::vec3(transform[3]);

            const float dotRU = std::abs(glm::dot(right, up));
            const float dotRF = std::abs(glm::dot(right, forward));
            const float dotUF = std::abs(glm::dot(up, forward));

            std::cout
                << "  t=" << t
                << " pos=(" << position.x << ", " << position.y << ", " << position.z << ")"
                << " | dot(R,U)=" << dotRU
                << " dot(R,F)=" << dotRF
                << " dot(U,F)=" << dotUF
                << std::endl;

            assert(dotRU < 1e-3f);
            assert(dotRF < 1e-3f);
            assert(dotUF < 1e-3f);
            assert(std::abs(glm::length(right) - 1.0f) < 1e-3f);
            assert(std::abs(glm::length(up) - 1.0f) < 1e-3f);
            assert(std::abs(glm::length(forward) - 1.0f) < 1e-3f);
        }
    }
}

int main() {
    RunSplinePathChecks();
    std::cout << "SplinePath tests passed." << std::endl;
    return 0;
}