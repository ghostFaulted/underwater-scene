#pragma once

#include <cstddef>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Shader.h"

struct SplineSwimSettings {
    std::size_t boneCount = 8;
    float bodyMinForward = -3.2f;
    float bodyMaxForward = 3.6f;
    float waveAmplitudeDegrees = 18.0f;
    float waveFrequencyHz = 1.25f;
    float wavePhaseLag = 0.45f;
    float curvatureToBiasDegrees = 1200.0f;
    float tailBiasGain = 1.35f;
};

class SplineSwimAnimator {
public:
    static constexpr std::size_t kMaxBones = 16;

    explicit SplineSwimAnimator(SplineSwimSettings settings = {});

    void BuildPose(float animationTimeSeconds, float signedCurvature);
    void UploadToShader(Shader& shader, bool enabled) const;

private:
    SplineSwimSettings settings;
    std::vector<glm::mat4> bones;
};

