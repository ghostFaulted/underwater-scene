#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>
#include "Shader.h"

constexpr unsigned int MAX_BONE_INFLUENCE = 4;

enum class MeshMaterialKind {
    Body,
    Eye,
    Teeth,
    Unknown
};

struct Vertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;
    glm::vec3 Normal;
    glm::vec3 Tangent;   
    glm::vec3 Bitangent;
    std::array<int, MAX_BONE_INFLUENCE> BoneIDs{ -1, -1, -1, -1 };
    std::array<float, MAX_BONE_INFLUENCE> BoneWeights{ 0.0f, 0.0f, 0.0f, 0.0f };
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    MeshMaterialKind materialKind = MeshMaterialKind::Unknown;
    // Node transform from FBX hierarchy; applied to meshes without bone weights
    glm::mat4 nodeTransform{1.0f};
    // Optional name for debug / identification
    std::string name;
    // If non-negative, this mesh will follow the named bone's final transform
    int attachedBoneIndex = -1;

    glm::mat4 attachmentCorrection = glm::mat4(1.0f);

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    void Draw(Shader& shader) const;
    // Draw but use a custom node transform (used when attaching mesh to a bone)
    void DrawWithTransform(Shader& shader, const glm::mat4& transform) const;
    // Apply runtime offset to transform (position + rotation).
    void DrawWithOffset(Shader& shader, const glm::mat4& transform, const glm::vec3& posOffset, const glm::vec3& rotOffsetDeg) const;

private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};