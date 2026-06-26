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

// Klasa ladujaca modele za pomoca biblioteki Assimp i zarzadzajaca ich hierarchia oraz animacja (skinning)
class Model {
public:
    static constexpr std::size_t MAX_BONES = 100;

    // Struktura diagnostyczna pomagajaca lokalizowac srodki ciezkosci kosci modelu w przestrzeni
    struct BoneDebugInfo {
        std::string name;
        int index = -1;
        float totalWeight = 0.0f;
        glm::vec3 weightedCenter{ 0.0f };
    };

    Model(const std::string& path);
    void Draw(Shader& shader, const AppState* appState = nullptr) const;

    // Przesyla kalkulowane matryce dla kosci bezposrednio do tablic shadera GPU
    void UploadBoneTransforms(Shader& shader, bool enableSkinning) const;

    [[nodiscard]] bool HasSkinningData() const;
    [[nodiscard]] std::vector<std::string> GetBoneNames() const;
    [[nodiscard]] std::vector<BoneDebugInfo> GetBoneDebugInfo() const;

    struct MeshDebugInfo {
        std::string name;
        MeshMaterialKind kind = MeshMaterialKind::Unknown;
        glm::vec3 pivotTranslation{ 0.0f };
        glm::vec3 centroid{ 0.0f };
        glm::vec3 boundsExtent{ 0.0f };
        int attachedBoneIndex = -1;
        std::string attachedBoneName;
    };
    [[nodiscard]] std::vector<MeshDebugInfo> GetMeshDebugInfo() const;

    // Zmiana i modyfikowanie orientacji kosci przez animatora programowego (SharkSkeletonAnimator)
    void ClearProceduralBoneOffsets();
    void SetProceduralBoneOffset(const std::string& boneName, const glm::mat4& localOffset);
    void ApplyLocalRotationToMeshes(const std::string& namePattern, const glm::mat4& localRotation);

private:
    // Struktura przetrzymujaca glowne parametry kosci potrzebne do skinningu
    struct BoneInfo {
        glm::mat4 offset{ 1.0f };               // Macierz Inverse-Bind przesuwajaca wierzcholki do przestrzeni kosci
        glm::mat4 finalTransform{ 1.0f };       // Ostateczna macierz gotowa do wgrania do shadera
        glm::mat4 attachmentTransform{ 1.0f };  // Transformacja sluzaca do podpinania sztywnych meshy do animowanych kosci
    };

    struct NodeData {
        std::string name;
        glm::mat4 transform{ 1.0f };
        std::vector<NodeData> children;
    };

    std::vector<Mesh> meshes;
    std::string directory;

    std::unordered_map<std::string, int> boneNameToIndex;
    std::vector<BoneInfo> boneInfo;
    std::unordered_map<std::string, glm::mat4> proceduralBoneOffsets;
    NodeData rootNode;
    glm::mat4 globalInverseTransform{ 1.0f };

    // Procesowanie Assimp
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeTransform, const aiNode* sourceNode);
    void ExtractNodeHierarchy(NodeData& destination, const aiNode* source);

    // Czytanie i dodawanie atrybutow wag kosci bezposrednio do wektorow wierzcholka
    void ReadBoneWeights(std::vector<Vertex>& vertices, const aiMesh* sourceMesh);

    // Obliczanie hierarchii i przekazywanie ruchu rekursywnie wzdluz drzewa wezlow
    void CalculateBoneTransforms(const NodeData& node, const glm::mat4& parentTransform);
};