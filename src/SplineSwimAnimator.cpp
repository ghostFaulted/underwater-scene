#include "SplineSwimAnimator.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr float kPi = 3.14159265358979323846f;

    float DegreesToRadians(const float value) {
        return value * (kPi / 180.0f);
    }

    float TailWeight(const std::size_t index, const std::size_t count) {
        if (count <= 1) {
            return 1.0f;
        }

        const float t = static_cast<float>(index) / static_cast<float>(count - 1);
        return t * t;
    }
}

SplineSwimAnimator::SplineSwimAnimator(SplineSwimSettings settings)
    : settings(std::move(settings)) {
    const std::size_t clampedCount = std::clamp<std::size_t>(this->settings.boneCount, 2, kMaxBones);
    this->settings.boneCount = clampedCount;
    bones.assign(clampedCount, glm::mat4(1.0f));
}

void SplineSwimAnimator::BuildPose(const float animationTimeSeconds, const float signedCurvature) {
    const std::size_t boneCount = bones.size();

    const float bodyRange = std::max(0.001f, settings.bodyMaxForward - settings.bodyMinForward);
    const float basePhase = animationTimeSeconds * settings.waveFrequencyHz * 2.0f * kPi;

    for (std::size_t i = 0; i < boneCount; ++i) {
        const float normalized = static_cast<float>(i) / static_cast<float>(boneCount - 1);
        const float pivotForward = settings.bodyMinForward + normalized * bodyRange;
        const float tailWeight = TailWeight(i, boneCount);

        const float waveAngle = std::sin(basePhase - normalized * settings.wavePhaseLag) *
                                settings.waveAmplitudeDegrees * tailWeight;
        const float turnBias = signedCurvature * settings.curvatureToBiasDegrees *
                               tailWeight * settings.tailBiasGain;

        const float angleRadians = DegreesToRadians(waveAngle + turnBias);

        glm::mat4 boneTransform(1.0f);
        boneTransform = glm::translate(boneTransform, glm::vec3(0.0f, 0.0f, pivotForward));
        boneTransform = glm::rotate(boneTransform, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        boneTransform = glm::translate(boneTransform, glm::vec3(0.0f, 0.0f, -pivotForward));

        bones[i] = boneTransform;
    }
}

void SplineSwimAnimator::UploadToShader(Shader& shader, const bool enabled) const {
    shader.setBool("uEnableSplineSkinning", enabled);
    shader.setInt("uSplineBoneCount", enabled ? static_cast<int>(bones.size()) : 0);
    shader.setFloat("uSplineBodyMin", settings.bodyMinForward);
    shader.setFloat("uSplineBodyMax", settings.bodyMaxForward);

    for (std::size_t i = 0; i < bones.size(); ++i) {
        shader.setMat4("uSplineBones[" + std::to_string(i) + "]", bones[i]);
    }
}


