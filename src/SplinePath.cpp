#include "SplinePath.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>

namespace {
    constexpr float kEpsilon = 1e-5f;
    constexpr float kOrthogonalityTolerance = 1e-3f;
    constexpr std::size_t kSamplesPerSegment = 16;

    float ClampDot(const float value) {
        return std::clamp(value, -1.0f, 1.0f);
    }

    glm::vec3 NormalizeOrFallback(const glm::vec3 &value, const glm::vec3 &fallback) {
        const float valueLengthSquared = glm::dot(value, value);
        if (valueLengthSquared > kEpsilon) {
            return value * (1.0f / std::sqrt(valueLengthSquared));
        }

        const float fallbackLengthSquared = glm::dot(fallback, fallback);
        if (fallbackLengthSquared > kEpsilon) {
            return fallback * (1.0f / std::sqrt(fallbackLengthSquared));
        }

        return {1.0f, 0.0f, 0.0f};
    }

    glm::vec3 ProjectOntoPlane(const glm::vec3 &vector, const glm::vec3 &planeNormal) {
        return vector - planeNormal * glm::dot(vector, planeNormal);
    }

    glm::vec3 BuildDownBiasedNormal(const glm::vec3 &tangent) {
        const glm::vec3 down(0.0f, -1.0f, 0.0f);
        glm::vec3 normal = ProjectOntoPlane(down, tangent);

        if (glm::dot(normal, normal) <= kEpsilon) {
            const glm::vec3 fallbackAxis = std::abs(tangent.y) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f)
                                                                       : glm::vec3(0.0f, 0.0f, 1.0f);
            normal = ProjectOntoPlane(glm::cross(tangent, fallbackAxis), tangent);
        }

        normal = NormalizeOrFallback(normal, glm::vec3(0.0f, -1.0f, 0.0f));
        if (glm::dot(normal, down) < 0.0f) {
            normal = -normal;
        }

        return normal;
    }

    float SignedAngleAroundAxis(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &axis) {
        const glm::vec3 nAxis = NormalizeOrFallback(axis, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 fromProjected = NormalizeOrFallback(ProjectOntoPlane(from, nAxis), from);
        const glm::vec3 toProjected = NormalizeOrFallback(ProjectOntoPlane(to, nAxis), to);

        const float sine = glm::dot(nAxis, glm::cross(fromProjected, toProjected));
        const float cosine = ClampDot(glm::dot(fromProjected, toProjected));
        return std::atan2(sine, cosine);
    }

    glm::vec3 RotateAroundAxis(const glm::vec3 &vector, glm::vec3 axis, const float angle) {
        axis = NormalizeOrFallback(axis, glm::vec3(0.0f, 1.0f, 0.0f));

        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);

        return vector * cosine + glm::cross(axis, vector) * sine + axis * glm::dot(axis, vector) * (1.0f - cosine);
    }
}

SplinePath::SplinePath(const std::vector<glm::vec3> &controlPoints, const bool closedLoop) {
    if (controlPoints.size() < 4) {
        throw std::invalid_argument("At least 4 control points are required for a Catmull-Rom spline.");
    }

    this->controlPoints = controlPoints;
    this->closedLoop = closedLoop;
    BuildFrames();

#ifndef NDEBUG
    ValidateFrames();
#endif
}

std::size_t SplinePath::GetSegmentCount() const {
    return closedLoop ? controlPoints.size() : controlPoints.size() - 3;
}

glm::vec3 SplinePath::SamplePositionOnSegment(const std::size_t segmentIndex, const float localT) const {
    if (closedLoop) {
        const auto n = static_cast<int>(controlPoints.size());
        const auto i = static_cast<int>(segmentIndex) % n;
        const glm::vec3 &p0 = controlPoints[(i - 1 + n) % n];
        const glm::vec3 &p1 = controlPoints[i];
        const glm::vec3 &p2 = controlPoints[(i + 1) % n];
        const glm::vec3 &p3 = controlPoints[(i + 2) % n];
        return CatmullRom(p0, p1, p2, p3, localT);
    }

    return CatmullRom(
        controlPoints[segmentIndex],
        controlPoints[segmentIndex + 1],
        controlPoints[segmentIndex + 2],
        controlPoints[segmentIndex + 3],
        localT
    );
}

glm::vec3 SplinePath::SampleTangentOnSegment(const std::size_t segmentIndex, const float localT) const {
    if (closedLoop) {
        const auto n = static_cast<int>(controlPoints.size());
        const auto i = static_cast<int>(segmentIndex) % n;
        const glm::vec3 &p0 = controlPoints[(i - 1 + n) % n];
        const glm::vec3 &p1 = controlPoints[i];
        const glm::vec3 &p2 = controlPoints[(i + 1) % n];
        const glm::vec3 &p3 = controlPoints[(i + 2) % n];
        return CatmullRomTangent(p0, p1, p2, p3, localT);
    }

    return CatmullRomTangent(
        controlPoints[segmentIndex],
        controlPoints[segmentIndex + 1],
        controlPoints[segmentIndex + 2],
        controlPoints[segmentIndex + 3],
        localT
    );
}

void SplinePath::BuildFrames() {
    const std::size_t segments = GetSegmentCount();
    const std::size_t sampleCount = std::max<std::size_t>(2, segments * kSamplesPerSegment + 1);

    sampleStep = 1.0f / static_cast<float>(sampleCount - 1);
    frames.clear();
    frames.reserve(sampleCount);

    glm::vec3 previousTangent(0.0f);
    glm::vec3 previousNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 previousBinormal(1.0f, 0.0f, 0.0f);
    bool hasPreviousFrame = false;

    auto sampleTangent = [this, segments](const float t) {
        const float scaledT = t * static_cast<float>(segments);
        const int segmentIndex = std::min(static_cast<int>(std::floor(scaledT)), static_cast<int>(segments) - 1);
        const float localT = scaledT - static_cast<float>(segmentIndex);

        return SampleTangentOnSegment(static_cast<std::size_t>(segmentIndex), localT);
    };

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) * sampleStep;
        const glm::vec3 position = GetPosition(t);
        glm::vec3 tangent = NormalizeOrFallback(sampleTangent(t), previousTangent);

        if (!hasPreviousFrame) {
            glm::vec3 normal = BuildDownBiasedNormal(tangent);
            glm::vec3 binormal = NormalizeOrFallback(glm::cross(tangent, normal), glm::vec3(1.0f, 0.0f, 0.0f));

            frames.push_back(Frame{t, position, tangent, normal, binormal});
            previousTangent = tangent;
            previousNormal = normal;
            previousBinormal = binormal;
            hasPreviousFrame = true;
            continue;
        }

        const glm::vec3 rotationAxis = glm::cross(previousTangent, tangent);
        const float axisLengthSquared = glm::dot(rotationAxis, rotationAxis);
        const float dotProduct = ClampDot(glm::dot(previousTangent, tangent));
        const float angle = std::acos(dotProduct);

        glm::vec3 normal = previousNormal;
        if (axisLengthSquared > kEpsilon && angle > kEpsilon) {
            normal = RotateAroundAxis(previousNormal, rotationAxis, angle);
        }

        normal = normal - tangent * glm::dot(normal, tangent);
        normal = NormalizeOrFallback(normal, previousNormal);

        glm::vec3 binormal = NormalizeOrFallback(glm::cross(tangent, normal), previousBinormal);

        if (glm::dot(binormal, previousBinormal) < 0.0f) {
            normal = -normal;
            binormal = -binormal;
        }

        frames.push_back(Frame{t, position, tangent, normal, binormal});
        previousTangent = tangent;
        previousNormal = normal;
        previousBinormal = binormal;
    }

    if (closedLoop && frames.size() > 1) {
        const glm::vec3 seamAxis = NormalizeOrFallback(frames.front().tangent + frames.back().tangent, frames.front().tangent);
        const float seamTwist = SignedAngleAroundAxis(frames.back().normal, frames.front().normal, seamAxis);

        if (std::abs(seamTwist) > kEpsilon) {
            const float invLastIndex = 1.0f / static_cast<float>(frames.size() - 1);

            for (std::size_t i = 0; i < frames.size(); ++i) {
                const float blend = static_cast<float>(i) * invLastIndex;
                const float angle = seamTwist * blend;

                Frame &frame = frames[i];
                frame.normal = RotateAroundAxis(frame.normal, frame.tangent, angle);
                frame.binormal = RotateAroundAxis(frame.binormal, frame.tangent, angle);

                frame.normal = NormalizeOrFallback(frame.normal, BuildDownBiasedNormal(frame.tangent));
                frame.binormal = NormalizeOrFallback(glm::cross(frame.tangent, frame.normal), frame.binormal);
            }

            frames.back().normal = frames.front().normal;
            frames.back().binormal = frames.front().binormal;
        }
    }
}

void SplinePath::ValidateFrames() const {
    for (const Frame &frame: frames) {
        const float dotTN = glm::dot(frame.tangent, frame.normal);
        const float dotTB = glm::dot(frame.tangent, frame.binormal);
        const float dotNB = glm::dot(frame.normal, frame.binormal);
        const float lenT = glm::length(frame.tangent);
        const float lenN = glm::length(frame.normal);
        const float lenB = glm::length(frame.binormal);

        std::cerr
                << "[SplinePath] t=" << frame.t
                << " | dot(T,N)=" << dotTN
                << " dot(T,B)=" << dotTB
                << " dot(N,B)=" << dotNB
                << " | |T|=" << lenT
                << " |N|=" << lenN
                << " |B|=" << lenB
                << std::endl;

        assert(std::abs(dotTN) < kOrthogonalityTolerance);
        assert(std::abs(dotTB) < kOrthogonalityTolerance);
        assert(std::abs(dotNB) < kOrthogonalityTolerance);
        assert(std::abs(lenT - 1.0f) < kOrthogonalityTolerance);
        assert(std::abs(lenN - 1.0f) < kOrthogonalityTolerance);
        assert(std::abs(lenB - 1.0f) < kOrthogonalityTolerance);
    }
}

glm::vec3 SplinePath::GetPosition(const float t) const {
    const std::size_t segments = GetSegmentCount();
    float normalizedT = std::clamp(t, 0.0f, 1.0f);
    if (closedLoop) {
        normalizedT = t - std::floor(t);
    }

    const float scaledT = normalizedT * static_cast<float>(segments);

    const int segmentIndex = std::min(static_cast<int>(std::floor(scaledT)), static_cast<int>(segments) - 1);
    const float localT = scaledT - static_cast<float>(segmentIndex);

    return SamplePositionOnSegment(static_cast<std::size_t>(segmentIndex), localT);
}

glm::mat4 SplinePath::GetTransform(const float t) const {
    if (frames.empty()) {
        return glm::mat4(1.0f);
    }

    float normalizedT = std::clamp(t, 0.0f, 1.0f);
    if (closedLoop) {
        normalizedT = t - std::floor(t);
    }

    const float scaledT = normalizedT * static_cast<float>(frames.size() - 1);
    const auto leftIndex = static_cast<std::size_t>(std::floor(scaledT));
    std::size_t rightIndex = std::min(leftIndex + 1, frames.size() - 1);
    if (closedLoop && leftIndex == frames.size() - 1) {
        rightIndex = 0;
    }
    const float alpha = scaledT - static_cast<float>(leftIndex);

    const Frame &left = frames[leftIndex];
    const Frame &right = frames[rightIndex];

    const glm::vec3 position = glm::mix(left.position, right.position, alpha);

    glm::vec3 tangent = NormalizeOrFallback(glm::mix(left.tangent, right.tangent, alpha), left.tangent);
    glm::vec3 normal = glm::mix(left.normal, right.normal, alpha);
    normal = normal - tangent * glm::dot(normal, tangent);
    normal = NormalizeOrFallback(normal, left.normal);

    glm::vec3 binormal = NormalizeOrFallback(glm::cross(tangent, normal), left.binormal);
    if (glm::dot(binormal, left.binormal) < 0.0f) {
        normal = -normal;
        binormal = -binormal;
    }

    glm::mat4 transform(1.0f);
    transform[0] = glm::vec4(binormal, 0.0f);
    transform[1] = glm::vec4(normal, 0.0f);
    transform[2] = glm::vec4(tangent, 0.0f);
    transform[3] = glm::vec4(position, 1.0f);

    return transform;
}

glm::vec3 SplinePath::CatmullRom(const glm::vec3 p0, const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
                                 const float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;

    return 0.5f * (
               (2.0f * p1) +
               (-p0 + p2) * t +
               (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
               (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
           );
}

glm::vec3 SplinePath::CatmullRomTangent(const glm::vec3 p0, const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
                                        const float t) {
    const float t2 = t * t;

    return 0.5f * (
               (-p0 + p2) +
               2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t +
               3.0f * (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t2
           );
}
