#include "Model.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "AppState.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <limits>
#include <array>

namespace {
    // Konwersja macierzy wewnetrznej Assimp na format macierzy matematycznej biblioteki GLM
    glm::mat4 AssimpToGlm(const aiMatrix4x4& matrix) {
        glm::mat4 result;
        result[0][0] = matrix.a1;
        result[1][0] = matrix.a2;
        result[2][0] = matrix.a3;
        result[3][0] = matrix.a4;
        result[0][1] = matrix.b1;
        result[1][1] = matrix.b2;
        result[2][1] = matrix.b3;
        result[3][1] = matrix.b4;
        result[0][2] = matrix.c1;
        result[1][2] = matrix.c2;
        result[2][2] = matrix.c3;
        result[3][2] = matrix.c4;
        result[0][3] = matrix.d1;
        result[1][3] = matrix.d2;
        result[2][3] = matrix.d3;
        result[3][3] = matrix.d4;
        return result;
    }

    // Bezpieczne wstawianie identyfikatora i wagi kosci w pierwszy wolny slot w tablicy wierzcholka (maks. 4 wplywy)
    void AddBoneData(Vertex& vertex, const int boneID, const float weight) {
        for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.BoneIDs[i] == -1) {
                vertex.BoneIDs[i] = boneID;
                vertex.BoneWeights[i] = weight;
                return;
            }
        }
    }

    // Prosta heurystyka odgadujaca material na bazie nazwy siatki z FBX (np. "eye_left", "teeth_top")
    MeshMaterialKind DetectMeshMaterialKind(const aiMesh* mesh, const aiNode* node) {
        const std::string meshName = mesh != nullptr ? mesh->mName.C_Str() : "";
        const std::string nodeName = node != nullptr ? node->mName.C_Str() : "";

        auto lowered = [](std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
                });
            return value;
            };

        const std::string meshLower = lowered(meshName);
        const std::string nodeLower = lowered(nodeName);

        if (meshLower.find("eye") != std::string::npos || nodeLower.find("eye") != std::string::npos) {
            return MeshMaterialKind::Eye;
        }
        if (meshLower.find("teeth") != std::string::npos || nodeLower.find("teeth") != std::string::npos ||
            meshLower.find("tooth") != std::string::npos || nodeLower.find("tooth") != std::string::npos) {
            return MeshMaterialKind::Teeth;
        }
        if (meshLower.find("body") != std::string::npos || nodeLower.find("body") != std::string::npos ||
            meshLower.find("geo") != std::string::npos || nodeLower.find("geo") != std::string::npos) {
            return MeshMaterialKind::Body;
        }

        return MeshMaterialKind::Unknown;
    }

    std::string ToLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
            });
        return value;
    }
}

Model::Model(const std::string& path) {
    loadModel(path);
}

void Model::Draw(Shader& shader, const AppState* appState) const {
    for (const auto& mesh : meshes) {
        glm::vec3 posOffset(0.0f);
        glm::vec3 rotOffset(0.0f);

        // Nakladanie ewentualnych offsetow tuningujacych testy (oczy / zeby) z poziomu interfejsu (AppState)
        if (appState != nullptr) {
            if (mesh.materialKind == MeshMaterialKind::Eye) {
                posOffset = appState->eyeMeshPositionOffset;
                rotOffset = appState->eyeMeshRotationOffset;
            }
            else if (mesh.materialKind == MeshMaterialKind::Teeth) {
                posOffset = appState->teethMeshPositionOffset;
                rotOffset = appState->teethMeshRotationOffset;
            }
        }

        // Procedura "przyklejania" oczu i zebow. Siatki te uzywaja matrycy attachmentTransform i podazaja
        // za swoja kosc-matka (jaw / skull), mimo braku zapisanych dla nich precyzyjnych wag wewnatrz modelu FBX
        if (mesh.attachedBoneIndex >= 0 && static_cast<std::size_t>(mesh.attachedBoneIndex) < boneInfo.size()) {
            const glm::mat4 boneAttach = boneInfo[static_cast<std::size_t>(mesh.attachedBoneIndex)].attachmentTransform;
            if (glm::length(posOffset) > 0.001f || glm::length(rotOffset) > 0.001f) {
                mesh.DrawWithOffset(shader, boneAttach, posOffset, rotOffset);
            }
            else {
                const glm::mat4 final =
                    boneAttach *
                    mesh.attachmentCorrection;

                mesh.DrawWithTransform(shader, final);
                // mesh.DrawWithTransform(shader, boneAttach);
            }
        }
        else {
            mesh.Draw(shader);
        }
    }
}

void Model::UploadBoneTransforms(Shader& shader, const bool enableSkinning) const {
    shader.setBool("uUseBoneSkinning", enableSkinning && HasSkinningData());
    if (!enableSkinning || !HasSkinningData()) {
        return;
    }

    // Przeliczenie rekurencyjne drzewa szkieletu i zebranie finalnych macierzy do tablic
    const_cast<Model*>(this)->CalculateBoneTransforms(rootNode, glm::mat4(1.0f));

    const std::size_t boneCount = std::min<std::size_t>(boneInfo.size(), MAX_BONES);
    for (std::size_t i = 0; i < boneCount; ++i) {
        shader.setMat4("uBoneTransforms[" + std::to_string(i) + "]", boneInfo[i].finalTransform);
    }
}

bool Model::HasSkinningData() const {
    return !boneInfo.empty();
}

std::vector<std::string> Model::GetBoneNames() const {
    std::vector<std::pair<int, std::string> > indexedBones;
    indexedBones.reserve(boneNameToIndex.size());

    for (const auto& [name, index] : boneNameToIndex) {
        indexedBones.emplace_back(index, name);
    }

    std::sort(indexedBones.begin(), indexedBones.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
        });

    std::vector<std::string> names;
    names.reserve(indexedBones.size());
    for (const auto& [index, name] : indexedBones) {
        (void)index;
        names.push_back(name);
    }

    return names;
}

// Funkcja tworzaca tablice mapujaca kosci z wyznaczonym optycznym srodkiem geometrycznym
std::vector<Model::BoneDebugInfo> Model::GetBoneDebugInfo() const {
    std::vector<BoneDebugInfo> debugInfo;
    if (boneInfo.empty()) {
        return debugInfo;
    }

    std::vector<glm::vec3> weightedPositionSums(boneInfo.size(), glm::vec3(0.0f));
    std::vector<float> weightSums(boneInfo.size(), 0.0f);

    // Sumowanie wag na podstawie wplywow u wierzcholkow
    for (const Mesh& mesh : meshes) {
        for (const Vertex& vertex : mesh.vertices) {
            for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                const int boneID = vertex.BoneIDs[i];
                const float weight = vertex.BoneWeights[i];
                if (boneID < 0 || weight <= 0.0f) {
                    continue;
                }

                const std::size_t idx = static_cast<std::size_t>(boneID);
                if (idx >= boneInfo.size()) {
                    continue;
                }

                weightedPositionSums[idx] += vertex.Position * weight;
                weightSums[idx] += weight;
            }
        }
    }

    std::vector<std::pair<int, std::string> > indexedBones;
    indexedBones.reserve(boneNameToIndex.size());
    for (const auto& [name, index] : boneNameToIndex) {
        indexedBones.emplace_back(index, name);
    }
    std::sort(indexedBones.begin(), indexedBones.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
        });

    debugInfo.reserve(indexedBones.size());
    for (const auto& [index, name] : indexedBones) {
        BoneDebugInfo info;
        info.name = name;
        info.index = index;

        if (index >= 0 && static_cast<std::size_t>(index) < boneInfo.size()) {
            const std::size_t idx = static_cast<std::size_t>(index);
            info.totalWeight = weightSums[idx];
            // Srednia wazona dajaca wyliczony srodek kosci w ukladzie modelu
            if (weightSums[idx] > 0.0001f) {
                info.weightedCenter = weightedPositionSums[idx] * (1.0f / weightSums[idx]);
            }
        }

        debugInfo.push_back(info);
    }

    return debugInfo;
}

void Model::ClearProceduralBoneOffsets() {
    proceduralBoneOffsets.clear();
}

void Model::SetProceduralBoneOffset(const std::string& boneName, const glm::mat4& localOffset) {
    proceduralBoneOffsets[boneName] = localOffset;
}

// Glowna funkcja ladujaca z uzyciem biblioteki Assimp
void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    // Opcja LimitBoneWeights gwarantuje ze Assimp przytnie mniejsze wplywy kosci, zostawiajac tylko 4 najwazniejsze
    const unsigned int importFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights;

    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    std::cout << "\n[MODEL] Loading: " << path << std::endl;
    std::cout << "[MODEL] Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "[MODEL] Materials: " << scene->mNumMaterials << std::endl;

    // Log mesh info
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        std::cout << "  [" << i << "] Mesh: " << mesh->mName.C_Str()
            << " | Vertices: " << mesh->mNumVertices
            << " | Bones: " << mesh->mNumBones << std::endl;
    }

    directory = std::filesystem::path(path).parent_path().string();
    globalInverseTransform = glm::inverse(AssimpToGlm(scene->mRootNode->mTransformation));

    // Budowanie hierarchii kosci bazujac na drzewie wezlow
    ExtractNodeHierarchy(rootNode, scene->mRootNode);
    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
    CalculateBoneTransforms(
        rootNode,
        glm::mat4(1.0f));


    std::cout << "[MODEL] Loaded " << meshes.size() << " mesh(es), "
        << boneNameToIndex.size() << " bone(s)" << std::endl << std::endl;

    // ==== Post-Processing ==== 
    // Automatyczne podpinanie wolnych meszy (zeby, galeczki oczne) ktore na sztywno nie posiadaja przypisanych wag w plikach modelu.
    const auto boneDebug = GetBoneDebugInfo();
    auto boneNameByIndex = [&](const int index) -> std::string {
        for (const auto& [name, idx] : boneNameToIndex) {
            if (idx == index) {
                return name;
            }
        }
        return "";
        };

    for (auto bone : boneDebug) {
        std::cout << bone.index << "   " << bone.name << "\n"
            << bone.weightedCenter[0] << "  " << bone.weightedCenter[1] << "   " << bone.weightedCenter[2] << std::endl;
    }

    // Heurystyki wyszukiwania kosci na podstawie slownikow nazwowych (jaw, head, spine itp.)
    auto findBoneByHints = [&](const std::array<const char*, 5>& hints) -> int {
        for (const char* hint : hints) {
            for (const auto& [name, idx] : boneNameToIndex) {
                if (ToLower(name).find(hint) != std::string::npos) {
                    return idx;
                }
            }
        }
        return -1;
        };

    // Wyszukiwanie kosci za pomoca najblizszego dystansu euklidesowego do wezla kosci
    auto findNearestBoneToMeshCentroid = [&](const Mesh& mesh, float& outDist) -> int {
        outDist = std::numeric_limits<float>::max();
        int bestIndex = -1;
        glm::vec3 meshCentroid(0.0f);
        if (!mesh.vertices.empty()) {
            for (const auto& v : mesh.vertices) {
                meshCentroid += v.Position;
            }
            meshCentroid *= (1.0f / static_cast<float>(mesh.vertices.size()));
        }

        for (const auto& b : boneDebug) {
            if (b.totalWeight <= 0.001f) {
                continue;
            }
            const float d = glm::distance(meshCentroid, b.weightedCenter);
            if (d < outDist) {
                outDist = d;
                bestIndex = b.index;
            }
        }

        return bestIndex;
        };

    // Podpiecie oczne i zebowe
    if (!boneDebug.empty()) {
        const std::array<const char*, 5> eyeHints = { "head", "eye", "jaw", "spine01", "spine1" };
        const std::array<const char*, 5> teethHints = { "jaw", "head", "teeth", "spine01", "spine1" };

        for (auto& m : meshes) {
            if (m.materialKind != MeshMaterialKind::Eye && m.materialKind != MeshMaterialKind::Teeth) {
                continue;
            }

            // Pominiecie jesli mesze maja juz wygenerowane z Assimp tablice wag
            bool hasWeights = false;
            for (const auto& v : m.vertices) {
                for (unsigned int wi = 0; wi < MAX_BONE_INFLUENCE; ++wi) {
                    if (v.BoneWeights[wi] > 0.0001f) {
                        hasWeights = true;
                        break;
                    }
                }
                if (hasWeights) {
                    break;
                }
            }
            if (hasWeights) {
                continue;
            }

            const std::array<const char*, 5>& hints =
                (m.materialKind == MeshMaterialKind::Eye) ? eyeHints : teethHints;
            int attached = findBoneByHints(hints);
            float dist = -1.0f;
            std::string strategy = "name-hint";

            // Rezygnacja ze slownikow i proba dystansu jezeli zaden hint nie pasuje
            if (attached < 0) {
                attached = findNearestBoneToMeshCentroid(m, dist);
                strategy = "nearest-centroid";
            }

            if (attached >= 0) {
                m.attachedBoneIndex = attached;
                const std::string boneName = boneNameByIndex(attached);
                std::cout << "[MODEL] Attached mesh '" << m.name << "' to bone '" << boneName
                    << "' (strategy=" << strategy;
                if (dist >= 0.0f) {
                    std::cout << ", dist=" << dist;
                }
                std::cout << ")\n";
            }
        }
    }

    // Wyliczanie lokalnych korekt dla "przyklejonych" siatek wzgledem ich rodzicielskiej kosci
    for (auto& m : meshes) {
        if (m.attachedBoneIndex < 0) {
            continue;
        }

        glm::vec3 meshCentroid(0.0f);
        if (!m.vertices.empty()) {
            for (const auto& v : m.vertices) {
                meshCentroid += v.Position;
            }
            meshCentroid *= (1.0f / static_cast<float>(m.vertices.size()));
        }

        // Szukanie pozycji kosci w ukladzie lokalnym modelu
        glm::vec3 bonePos(0.0f);
        for (const auto& b : boneDebug) {
            if (b.index == m.attachedBoneIndex) {
                bonePos = b.weightedCenter;
                break;
            }
        }

        // Kalkulacja wektora ktory na sztywno zniweluje odleglosc by centroid idealnie siedzial na kosci
        const glm::vec3 offset = bonePos - meshCentroid;
        const glm::mat4 offsetMat = glm::translate(glm::mat4(1.0f), offset);

        m.nodeTransform = m.nodeTransform * offsetMat;
        m.attachmentCorrection =
            glm::inverse(
                boneInfo[m.attachedBoneIndex].attachmentTransform)
            *
            m.nodeTransform;

        auto p =
            glm::vec3(
                boneInfo[m.attachedBoneIndex]
                .attachmentTransform[3]);

        auto bp =
            glm::vec3(
                boneInfo[m.attachedBoneIndex]
                .attachmentTransform[3]);

        auto np =
            glm::vec3(
                m.nodeTransform[3]);

        std::cout
            << m.name
            << "\nbone: "
            << bp.x << " "
            << bp.y << " "
            << bp.z
            << "\nnode: "
            << np.x << " "
            << np.y << " "
            << np.z
            << std::endl;

        std::cout
            << "boneAttach="
            << p.x << ", "
            << p.y << ", "
            << p.z << std::endl;

        const glm::mat4& M =
            boneInfo[m.attachedBoneIndex]
            .attachmentTransform;

        for (int r = 0; r < 4; ++r) {
            std::cout
                << M[0][r] << " "
                << M[1][r] << " "
                << M[2][r] << " "
                << M[3][r] << "\n";
        }

        std::cout << "[MODEL] Mesh '" << m.name << "': centroid=" << meshCentroid.x << "," << meshCentroid.y
            << "," << meshCentroid.z << " -> bonePos=" << bonePos.x << "," << bonePos.y << ","
            << bonePos.z << ", applied offset=(" << offset.x << "," << offset.y << "," << offset.z << ")\n";
    }
}

// Rekurencyjna funkcja budujaca plaska liste Siatek(Meshes) z drzewiastej struktury Node'ow Assimpa
void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
    glm::mat4 nodeTransform = parentTransform * AssimpToGlm(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, nodeTransform, node));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, nodeTransform);
    }
}

// Wyciaganie informacji o jednym wierzcholku z posrod poszatkowanych list wewnetrznych biblioteki
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeTransform, const aiNode* sourceNode) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals()) {
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else {
            vertex.Normal = glm::vec3(0.0f);
        }

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f);
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        else {
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    ReadBoneWeights(vertices, mesh);
    Mesh resultMesh(vertices, indices);
    resultMesh.nodeTransform = nodeTransform;
    resultMesh.materialKind = DetectMeshMaterialKind(mesh, sourceNode);
    resultMesh.name = mesh->mName.C_Str();
    return resultMesh;
}

// Konstruowanie wlasnego, odczepionego drzewa wezlow, na ktorym latwiej manipulowac w C++
void Model::ExtractNodeHierarchy(NodeData& destination, const aiNode* source) {
    destination.name = source->mName.C_Str();
    destination.transform = AssimpToGlm(source->mTransformation);
    destination.children.clear();
    destination.children.reserve(source->mNumChildren);

    for (unsigned int i = 0; i < source->mNumChildren; ++i) {
        NodeData child;
        ExtractNodeHierarchy(child, source->mChildren[i]);
        destination.children.push_back(std::move(child));
    }
}

std::vector<Model::MeshDebugInfo> Model::GetMeshDebugInfo() const {
    std::vector<MeshDebugInfo> out;
    out.reserve(meshes.size());
    for (const auto& m : meshes) {
        MeshDebugInfo info;
        info.name = m.name;
        info.kind = m.materialKind;
        // Punkt obrotu brany z transformacji wezla (czesto to po prostu srodek ukladu wexportowany z FBX)
        info.pivotTranslation = glm::vec3(m.nodeTransform[3]);

        // Wyliczanie srodka i wymiarow boxa lokalnego
        if (!m.vertices.empty()) {
            glm::vec3 minP = m.vertices[0].Position;
            glm::vec3 maxP = m.vertices[0].Position;
            glm::vec3 sum(0.0f);
            for (const auto& v : m.vertices) {
                sum += v.Position;
                minP = glm::min(minP, v.Position);
                maxP = glm::max(maxP, v.Position);
            }
            info.centroid = sum * (1.0f / static_cast<float>(m.vertices.size()));
            info.boundsExtent = maxP - minP;
        }
        info.attachedBoneIndex = m.attachedBoneIndex;
        if (m.attachedBoneIndex >= 0) {
            for (const auto& [boneName, idx] : boneNameToIndex) {
                if (idx == m.attachedBoneIndex) {
                    info.attachedBoneName = boneName;
                    break;
                }
            }
        }
        out.push_back(info);
    }
    return out;
}

void Model::ApplyLocalRotationToMeshes(const std::string& namePattern, const glm::mat4& localRotation) {
    const std::string patLower = [&]() {
        std::string p = namePattern;
        std::transform(p.begin(), p.end(), p.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return p;
        }();

    for (auto& m : meshes) {
        std::string n = m.name;
        std::transform(n.begin(), n.end(), n.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (n.find(patLower) != std::string::npos) {
            // Nakladanie rotacji z zachowaniem istniejacej juz w wezle transformacji
            m.nodeTransform = m.nodeTransform * localRotation;
            std::cout << "[MODEL] ApplyLocalRotationToMeshes: rotated mesh '" << m.name << "' by pattern '" <<
                namePattern << "'\n";
        }
    }
}

void Model::ReadBoneWeights(std::vector<Vertex>& vertices, const aiMesh* sourceMesh) {
    for (unsigned int boneIndex = 0; boneIndex < sourceMesh->mNumBones; ++boneIndex) {
        const aiBone* bone = sourceMesh->mBones[boneIndex];
        const std::string boneName = bone->mName.C_Str();

        int mappedBoneIndex = 0;
        const auto boneIt = boneNameToIndex.find(boneName);
        if (boneIt == boneNameToIndex.end()) {
            mappedBoneIndex = static_cast<int>(boneInfo.size());
            boneNameToIndex[boneName] = mappedBoneIndex;
            // Poczatkowa rejestracja macierzy odwrotnej Offset (przyjmujaca wierzcholki na lokalna baze)
            boneInfo.push_back({ AssimpToGlm(bone->mOffsetMatrix), glm::mat4(1.0f), glm::mat4(1.0f) });
        }
        else {
            mappedBoneIndex = boneIt->second;
        }

        // Dolaczenie ID przypisanej kosci u kazdego zidentyfikowanego elementu vertexa z Assimp.
        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
            const aiVertexWeight& vertexWeight = bone->mWeights[weightIndex];
            if (vertexWeight.mVertexId < vertices.size()) {
                AddBoneData(vertices[vertexWeight.mVertexId], mappedBoneIndex, vertexWeight.mWeight);
            }
        }
    }

    // Normalizacja wag tak aby sumarycznie wszystkie 4 sloty dawaly 100% (1.0) deformacji (zabezpieczenie shaderow przed rozerwaniem geometrii)
    for (Vertex& vertex : vertices) {
        float weightSum = 0.0f;
        for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            weightSum += vertex.BoneWeights[i];
        }

        if (weightSum > 0.0001f) {
            const float invWeightSum = 1.0f / weightSum;
            for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                vertex.BoneWeights[i] *= invWeightSum;
            }
        }
    }
}

// Propagacja transformacji z pnia drzewa wezlow, az po najmniejsze galazki (kosci obracaja to co za nimi idzie wspolnie w matrycy parentTransform)
void Model::CalculateBoneTransforms(const NodeData& node, const glm::mat4& parentTransform) {
    glm::mat4 localTransform = node.transform;

    const auto overrideIt = proceduralBoneOffsets.find(node.name);
    // Nakladanie "sztywnych modyfikacji" (np SharkSkeletonAnimator podaje gotowe drgania ciala rekina wyprzedzajac model z pliku).
    if (overrideIt != proceduralBoneOffsets.end()) {
        localTransform = localTransform * overrideIt->second;
    }

    const glm::mat4 globalTransform = parentTransform * localTransform;

    const auto boneIt = boneNameToIndex.find(node.name);
    if (boneIt != boneNameToIndex.end()) {
        const int boneIndex = boneIt->second;
        if (boneIndex >= 0 && static_cast<std::size_t>(boneIndex) < boneInfo.size()) {
            boneInfo[boneIndex].attachmentTransform = globalInverseTransform * globalTransform;

            // Koncowa macierz z ktorej moga korzystac programy Shaderowe do obliczenia deformacji wierzcholka.
            boneInfo[boneIndex].finalTransform = globalInverseTransform * globalTransform * boneInfo[boneIndex].offset;
        }
    }

    for (const auto& child : node.children) {
        CalculateBoneTransforms(child, globalTransform);
    }
}