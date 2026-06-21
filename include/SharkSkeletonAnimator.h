#pragma once

#include <glm/vec3.hpp>
#include <string>
#include <vector>

class Model;

struct SharkSwimSettings {
    float waveFrequencyHz = 1.15f;
    float headAmplitudeDeg = 1.5f;
    float tailAmplitudeDeg = 22.0f;
    float wavePhaseLag = 2.4f;
    float turnYawGainDeg = 14.0f;
    float turnRollGainDeg = 6.0f;
    float finCounterRollDeg = 4.0f;
    float responseHz = 10.0f;
    glm::vec3 localYawAxis{0.0f, 1.0f, 0.0f};
    glm::vec3 localRollAxis{0.0f, 0.0f, 1.0f};
};

class SharkSkeletonAnimator {
public:
    explicit SharkSkeletonAnimator(SharkSwimSettings settings = {});

    void InitializeFromModel(const Model& model);
    void ApplyPose(Model& model, float animationTimeSeconds, float signedCurvature);

    [[nodiscard]] const std::vector<std::string>& GetDebugLines() const;
    [[nodiscard]] const std::vector<std::string>& GetAnimatedBones() const;
    [[nodiscard]] const std::vector<float>& GetLastAppliedAnglesDegrees() const;

private:
    SharkSwimSettings settings;
    std::vector<std::string> bodyBones;
    std::vector<std::string> finBones;
    std::vector<std::string> animatedBones;
    std::vector<float> lastAppliedAnglesDegrees;
    std::vector<float> lastAppliedRollDegrees;
    std::vector<std::string> debugLines;
    float lastAnimationTimeSeconds = 0.0f;
    bool hasLastAnimationTime = false;
};



