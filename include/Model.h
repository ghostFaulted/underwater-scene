#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>
#include <string>
#include <vector>

struct AppState;

class Model {
public:
    static constexpr std::size_t MAX_BONES = 100;

    struct BoneDebugInfo {
        std::string name;
        int index = -1;
        float totalWeight = 0.0f;
        glm::vec3 weightedCenter{0.0f};
    };

    Model(const std::string& path);
    void Draw(Shader& shader, const AppState* appState = nullptr) const;
    void UploadBoneTransforms(Shader& shader, bool enableSkinning) const;

    [[nodiscard]] bool HasSkinningData() const;
    [[nodiscard]] std::vector<std::string> GetBoneNames() const;
    [[nodiscard]] std::vector<BoneDebugInfo> GetBoneDebugInfo() const;
    struct MeshDebugInfo {
        std::string name;
        MeshMaterialKind kind = MeshMaterialKind::Unknown;
        glm::vec3 pivotTranslation{0.0f};
        glm::vec3 centroid{0.0f};
        glm::vec3 boundsExtent{0.0f};
        int attachedBoneIndex = -1;
        std::string attachedBoneName;
    };
    [[nodiscard]] std::vector<MeshDebugInfo> GetMeshDebugInfo() const;

    void ClearProceduralBoneOffsets();
    void SetProceduralBoneOffset(const std::string& boneName, const glm::mat4& localOffset);
    // Apply a local rotation (in model space) to meshes whose name contains the pattern
    void ApplyLocalRotationToMeshes(const std::string& namePattern, const glm::mat4& localRotation);

private:
    struct BoneInfo {
        glm::mat4 offset{1.0f};
        glm::mat4 finalTransform{1.0f};
        glm::mat4 attachmentTransform{1.0f};
    };

    struct NodeData {
        std::string name;
        glm::mat4 transform{1.0f};
        std::vector<NodeData> children;
    };

    std::vector<Mesh> meshes;
    std::string directory;

    std::unordered_map<std::string, int> boneNameToIndex;
    std::vector<BoneInfo> boneInfo;
    std::unordered_map<std::string, glm::mat4> proceduralBoneOffsets;
    NodeData rootNode;
    glm::mat4 globalInverseTransform{1.0f};

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeTransform, const aiNode* sourceNode);
    void ExtractNodeHierarchy(NodeData& destination, const aiNode* source);
    void ReadBoneWeights(std::vector<Vertex>& vertices, const aiMesh* sourceMesh);
    void CalculateBoneTransforms(const NodeData& node, const glm::mat4& parentTransform);
};