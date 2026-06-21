#include "SharkSkeletonAnimator.h"

#include "Model.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <iomanip>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
    constexpr float kPi = 3.14159265358979323846f;

    std::string ToLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    bool ContainsAny(const std::string& lower, const std::initializer_list<const char*>& tokens) {
        for (const char* token : tokens) {
            if (lower.find(token) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool IsBodyBoneName(const std::string& name) {
        const std::string lower = ToLower(name);
        return ContainsAny(lower, {"spine", "tail", "caudal", "back"});
    }

    bool IsFinBoneName(const std::string& name) {
        const std::string lower = ToLower(name);
        return ContainsAny(lower, {"fin", "dorsal", "pectoral", "pelvic", "anal"});
    }

    // Canonical FBX rig order: front-to-back, amplitude increases towards the tail.
    // Root is intentionally excluded — it anchors the whole model without deformation.
    const std::vector<std::string> kKnownRigOrder = {
        "Spine01", "Spine02", "Spine03",
        "Tail01",  "Tail02",  "Tail03"
    };

    struct CandidateBone {
        std::string name;
        float weight = 0.0f;
        glm::vec3 center{0.0f};
    };

    glm::vec3 SafeNormalize(const glm::vec3& axis, const glm::vec3& fallback) {
        const float lenSq = glm::dot(axis, axis);
        if (lenSq <= 1e-8f) {
            return fallback;
        }
        return axis * (1.0f / std::sqrt(lenSq));
    }

    glm::vec3 InferDominantAxis(const std::vector<CandidateBone>& bones) {
        if (bones.empty()) {
            return {0.0f, 1.0f, 0.0f};
        }

        glm::vec3 minC(std::numeric_limits<float>::max());
        glm::vec3 maxC(-std::numeric_limits<float>::max());

        for (const auto& b : bones) {
            minC = glm::min(minC, b.center);
            maxC = glm::max(maxC, b.center);
        }

        const glm::vec3 extent = maxC - minC;
        if (extent.x >= extent.y && extent.x >= extent.z) {
            return {1.0f, 0.0f, 0.0f};
        }
        if (extent.y >= extent.x && extent.y >= extent.z) {
            return {0.0f, 1.0f, 0.0f};
        }
        return {0.0f, 0.0f, 1.0f};
    }

    void SortBonesByAxis(std::vector<CandidateBone>& bones, const glm::vec3& axis) {
        if (bones.size() < 2) {
            return;
        }

        const glm::vec3 normalizedAxis = SafeNormalize(axis, {0.0f, 1.0f, 0.0f});
        std::stable_sort(bones.begin(), bones.end(), [&](const CandidateBone& left, const CandidateBone& right) {
            const float leftProj = glm::dot(left.center, normalizedAxis);
            const float rightProj = glm::dot(right.center, normalizedAxis);
            if (std::abs(leftProj - rightProj) > 1e-5f) {
                return leftProj < rightProj;
            }
            return left.weight > right.weight;
        });
    }

    glm::mat4 BuildLocalRotation(float yawDeg, float rollDeg, const SharkSwimSettings& settings) {
        const glm::vec3 yawAxis = SafeNormalize(settings.localYawAxis, {0.0f, 1.0f, 0.0f});
        const glm::vec3 rollAxis = SafeNormalize(settings.localRollAxis, {0.0f, 0.0f, 1.0f});

        const glm::quat qYaw = glm::angleAxis(glm::radians(yawDeg), yawAxis);
        const glm::quat qRoll = glm::angleAxis(glm::radians(rollDeg), rollAxis);
        return glm::mat4_cast(qYaw * qRoll);
    }

    float SmoothTowards(float previousValue, float targetValue, float blend) {
        if (blend <= 0.0f) {
            return targetValue;
        }

        return glm::mix(previousValue, targetValue, glm::clamp(blend, 0.0f, 1.0f));
    }
}

SharkSkeletonAnimator::SharkSkeletonAnimator(SharkSwimSettings settings)
    : settings(settings) {
}

void SharkSkeletonAnimator::InitializeFromModel(const Model& model) {
    bodyBones.clear();
    finBones.clear();
    animatedBones.clear();
    debugLines.clear();
    lastAppliedAnglesDegrees.clear();
    lastAppliedRollDegrees.clear();

    const std::vector<Model::BoneDebugInfo> boneInfo = model.GetBoneDebugInfo();

    std::vector<CandidateBone> namedBodyCandidates;
    std::vector<CandidateBone> bodyCandidates;
    std::vector<CandidateBone> finCandidates;

    // First, try the canonical body chain from exact rig names.
    {
        namedBodyCandidates.reserve(kKnownRigOrder.size());

        for (const auto& rigName : kKnownRigOrder) {
            for (const auto& info : boneInfo) {
                if (ToLower(info.name) == ToLower(rigName) && info.totalWeight > 0.01f) {
                    namedBodyCandidates.push_back({info.name, info.totalWeight, info.weightedCenter});
                    break;
                }
            }
        }
    }

    // Heuristic candidates: body and fins are collected independently.
    bodyCandidates.reserve(boneInfo.size());
    finCandidates.reserve(boneInfo.size());

    for (const auto& info : boneInfo) {
        if (info.totalWeight > 0.01f) {
            if (IsBodyBoneName(info.name)) {
                bodyCandidates.push_back({info.name, info.totalWeight, info.weightedCenter});
            }
            if (IsFinBoneName(info.name)) {
                finCandidates.push_back({info.name, info.totalWeight, info.weightedCenter});
            }
        }
    }

    if (namedBodyCandidates.size() >= 2) {
        bodyCandidates = namedBodyCandidates;
        debugLines.push_back("FBX named body chain detected");
    }

    if (bodyCandidates.size() < 2) {
        bodyCandidates.clear();
        for (const auto& info : boneInfo) {
            if (info.totalWeight > 0.01f && !IsFinBoneName(info.name)) {
                bodyCandidates.push_back({info.name, info.totalWeight, info.weightedCenter});
            }
        }
        if (bodyCandidates.size() < 2) {
            bodyCandidates.clear();
            for (const auto& info : boneInfo) {
                if (info.totalWeight > 0.01f) {
                    bodyCandidates.push_back({info.name, info.totalWeight, info.weightedCenter});
                }
            }
            finCandidates.clear();
        }
        debugLines.push_back("Fallback body chain: heuristic weighted bones");
    }

    if (finCandidates.empty()) {
        for (const auto& info : boneInfo) {
            if (info.totalWeight <= 0.01f) {
                continue;
            }
            const std::string lower = ToLower(info.name);
            if (ContainsAny(lower, {"dorsal", "pectoral", "pelvic", "anal"})) {
                finCandidates.push_back({info.name, info.totalWeight, info.weightedCenter});
            }
        }
    }

    const glm::vec3 bodyAxis = InferDominantAxis(bodyCandidates);
    SortBonesByAxis(bodyCandidates, bodyAxis);
    SortBonesByAxis(finCandidates, bodyAxis);

    bodyBones.reserve(bodyCandidates.size());
    for (const auto& bone : bodyCandidates) {
        bodyBones.push_back(bone.name);
    }

    finBones.reserve(finCandidates.size());
    for (const auto& bone : finCandidates) {
        finBones.push_back(bone.name);
    }

    animatedBones.reserve(bodyBones.size() + finBones.size());
    animatedBones.insert(animatedBones.end(), bodyBones.begin(), bodyBones.end());
    animatedBones.insert(animatedBones.end(), finBones.begin(), finBones.end());

    std::ostringstream axisLine;
    axisLine << "Dominant body axis: ("
             << std::fixed << std::setprecision(2)
             << bodyAxis.x << ", " << bodyAxis.y << ", " << bodyAxis.z << ")";
    debugLines.push_back(axisLine.str());
    debugLines.push_back("Body chain bones: " + std::to_string(bodyBones.size()));
    for (std::size_t i = 0; i < bodyBones.size(); ++i) {
        debugLines.push_back("  body[" + std::to_string(i) + "] " + bodyBones[i]);
    }

    debugLines.push_back("Fin chain bones: " + std::to_string(finBones.size()));
    for (std::size_t i = 0; i < finBones.size(); ++i) {
        debugLines.push_back("  fin[" + std::to_string(i) + "] " + finBones[i]);
    }

    lastAppliedAnglesDegrees.assign(animatedBones.size(), 0.0f);
    lastAppliedRollDegrees.assign(animatedBones.size(), 0.0f);
    hasLastAnimationTime = false;
    lastAnimationTimeSeconds = 0.0f;
}

void SharkSkeletonAnimator::ApplyPose(Model& model, const float animationTimeSeconds, const float signedCurvature) {
    model.ClearProceduralBoneOffsets();
    if (animatedBones.empty()) {
        return;
    }

    const float basePhase = animationTimeSeconds * settings.waveFrequencyHz * 2.0f * kPi;
    const std::size_t bodyCount = bodyBones.size();
    const std::size_t finCount = finBones.size();
    const std::vector<float> previousYaw = lastAppliedAnglesDegrees;
    const std::vector<float> previousRoll = lastAppliedRollDegrees;

    float deltaTimeSeconds = 0.0f;
    if (hasLastAnimationTime) {
        deltaTimeSeconds = std::max(0.0f, animationTimeSeconds - lastAnimationTimeSeconds);
    }
    lastAnimationTimeSeconds = animationTimeSeconds;
    hasLastAnimationTime = true;

    const bool canSmooth = !previousYaw.empty() && previousYaw.size() == animatedBones.size() &&
                           previousRoll.size() == animatedBones.size() &&
                           deltaTimeSeconds > 0.0f && settings.responseHz > 0.0f;
    const float blend = canSmooth ? (1.0f - std::exp(-settings.responseHz * deltaTimeSeconds)) : 1.0f;

    auto amplitudeEnvelope = [](float s) {
        return s * s * s;
    };

    auto getPrev = [&](const std::vector<float>& values, std::size_t index, float fallback) {
        return index < values.size() ? values[index] : fallback;
    };

    for (std::size_t i = 0; i < bodyCount; ++i) {
        const float s = (bodyCount <= 1) ? 1.0f : static_cast<float>(i) / static_cast<float>(bodyCount - 1);
        const float env = amplitudeEnvelope(s);

        const float wave = std::sin(basePhase - s * settings.wavePhaseLag);
        const float targetWaveDeg = glm::mix(settings.headAmplitudeDeg, settings.tailAmplitudeDeg, env) * wave;

        const float turnYawDeg = glm::clamp(signedCurvature * settings.turnYawGainDeg, -20.0f, 20.0f) * env;
        const float turnRollDeg = -glm::clamp(signedCurvature * settings.turnRollGainDeg, -10.0f, 10.0f) * env;

        const float targetYaw = targetWaveDeg + turnYawDeg;
        const float targetRoll = turnRollDeg;

        const float finalYaw = canSmooth
            ? SmoothTowards(getPrev(previousYaw, i, targetYaw), targetYaw, blend)
            : targetYaw;
        const float finalRoll = canSmooth
            ? SmoothTowards(getPrev(previousRoll, i, targetRoll), targetRoll, blend)
            : targetRoll;

        model.SetProceduralBoneOffset(bodyBones[i], BuildLocalRotation(finalYaw, finalRoll, settings));
        lastAppliedAnglesDegrees[i] = finalYaw;
        lastAppliedRollDegrees[i] = finalRoll;
    }

    const std::size_t finOffset = bodyCount;
    for (std::size_t i = 0; i < finCount; ++i) {
        const float s = (finCount <= 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(finCount - 1);
        const float finPhase = basePhase + s * 0.35f;
        const float targetRoll = std::sin(finPhase) * settings.finCounterRollDeg * (1.0f - 0.4f * std::abs(signedCurvature));

        const float finalRoll = canSmooth
            ? SmoothTowards(getPrev(previousRoll, finOffset + i, targetRoll), targetRoll, blend)
            : targetRoll;

        model.SetProceduralBoneOffset(finBones[i], BuildLocalRotation(0.0f, finalRoll, settings));
        lastAppliedAnglesDegrees[finOffset + i] = finalRoll;
        lastAppliedRollDegrees[finOffset + i] = finalRoll;
    }
}

const std::vector<std::string>& SharkSkeletonAnimator::GetDebugLines() const {
    return debugLines;
}

const std::vector<std::string>& SharkSkeletonAnimator::GetAnimatedBones() const {
    return animatedBones;
}

const std::vector<float>& SharkSkeletonAnimator::GetLastAppliedAnglesDegrees() const {
    return lastAppliedAnglesDegrees;
}
