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

    [[nodiscard]] static glm::vec3 CatmullRom(glm::vec3 p0, glm::vec3 p1,
                                              glm::vec3 p2, glm::vec3 p3, float t);

    [[nodiscard]] static glm::vec3 CatmullRomTangent(glm::vec3 p0, glm::vec3 p1,
                                                     glm::vec3 p2, glm::vec3 p3, float t);

    void BuildFrames();

    void ValidateFrames() const;

public:
    explicit SplinePath(const std::vector<glm::vec3> &controlPoints);

    [[nodiscard]] glm::vec3 GetPosition(float t) const;

    [[nodiscard]] glm::mat4 GetTransform(float t) const;
};


#endif //UNDERWATERSCENE_SPLINEPATH_H
