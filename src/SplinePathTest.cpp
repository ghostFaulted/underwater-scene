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

        SplinePath spline(controlPoints, true);

        std::cout << "[SplinePathTest] orthogonality check" << std::endl;
        glm::vec3 firstNormal(0.0f);
        glm::vec3 lastNormal(0.0f);
        glm::vec3 firstBinormal(0.0f);
        glm::vec3 lastBinormal(0.0f);

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

            if (i == 0) {
                firstNormal = up;
                firstBinormal = right;
            }
            if (i == 12) {
                lastNormal = up;
                lastBinormal = right;
            }

            assert(up.y <= 0.25f);
        }

        const float seamNormalDelta = glm::length(firstNormal - lastNormal);
        const float seamBinormalDelta = glm::length(firstBinormal - lastBinormal);

        std::cout << "[SplinePathTest] seam delta normal=" << seamNormalDelta
                  << " binormal=" << seamBinormalDelta << std::endl;

        assert(seamNormalDelta < 5e-2f);
        assert(seamBinormalDelta < 5e-2f);
    }
}

int main() {
    RunSplinePathChecks();
    std::cout << "SplinePath tests passed." << std::endl;
    return 0;
}