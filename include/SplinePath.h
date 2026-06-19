#ifndef UNDERWATERSCENE_SPLINEPATH_H
#define UNDERWATERSCENE_SPLINEPATH_H
#include <cstddef>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>


class SplinePath {
    struct Frame {
        float t;
        glm::vec3 position;
        glm::vec3 tangent;
        glm::vec3 normal;
        glm::vec3 binormal;
    };

    std::vector<glm::vec3> controlPoints;
    std::vector<Frame> frames;
    float sampleStep = 0.0f;
    bool closedLoop = false;

    [[nodiscard]] static glm::vec3 CatmullRom(glm::vec3 p0, glm::vec3 p1,
                                              glm::vec3 p2, glm::vec3 p3, float t);

    [[nodiscard]] static glm::vec3 CatmullRomTangent(glm::vec3 p0, glm::vec3 p1,
                                                     glm::vec3 p2, glm::vec3 p3, float t);

    [[nodiscard]] static glm::vec3 CatmullRomSecondDerivative(glm::vec3 p0, glm::vec3 p1,
                                                              glm::vec3 p2, glm::vec3 p3, float t);

    [[nodiscard]] std::size_t GetSegmentCount() const;

    [[nodiscard]] glm::vec3 SamplePositionOnSegment(std::size_t segmentIndex, float localT) const;

    [[nodiscard]] glm::vec3 SampleTangentOnSegment(std::size_t segmentIndex, float localT) const;

    void BuildFrames();

    void ValidateFrames() const;

public:
    explicit SplinePath(const std::vector<glm::vec3> &controlPoints, bool closedLoop = false);

    [[nodiscard]] glm::vec3 GetPosition(float t) const;

    [[nodiscard]] glm::vec3 GetTangent(float t) const;

    [[nodiscard]] float GetSignedCurvature(float t) const;

    [[nodiscard]] glm::mat4 GetTransform(float t) const;
};


#endif //UNDERWATERSCENE_SPLINEPATH_H
