#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>
#include "Shader.h"

// Maksymalna ilosc kosci mogaca jednoczesnie wplywac na pojedynczy wierzcholek w procesie Skinningu
constexpr unsigned int MAX_BONE_INFLUENCE = 4;

// Enum sluzacy do rozrozniania czesci ciala dla latwiejszego przypisywania materialow i debugowania
enum class MeshMaterialKind {
    Body,
    Eye,
    Teeth,
    Unknown
};

// Reprezentacja jednego wierzcholka ze wszystkimi atrybutami (do bufora VBO)
struct Vertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    // ID kosci majacych wplyw na wierzcholek (-1 to brak wplywu)
    std::array<int, MAX_BONE_INFLUENCE> BoneIDs{ -1, -1, -1, -1 };
    // Wagi opisujace jak mocno dana kosc deformuje siatke w tym miejscu
    std::array<float, MAX_BONE_INFLUENCE> BoneWeights{ 0.0f, 0.0f, 0.0f, 0.0f };
};

// Klasa zarzadzajaca konkretna geometria (siatka 3D) na karcie graficznej
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    MeshMaterialKind materialKind = MeshMaterialKind::Unknown;

    // Bazowa transformacja wezla z hierarchii modelu FBX (dla siatek nie posiadajacych wplywu kosci)
    glm::mat4 nodeTransform{ 1.0f };
    std::string name;

    // Jesli indeks >= 0, ta konkretna siatka bedzie "przyklejana" do kosci o tym indeksie
    int attachedBoneIndex = -1;
    // Korekta przesuniecia punktu srodkowego siatki wzgledem kosci
    glm::mat4 attachmentCorrection = glm::mat4(1.0f);

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    // Rysuje obiekt wykorzystujac bazowy nodeTransform
    void Draw(Shader& shader) const;

    // Rysuje obiekt nadpisujac jego pozycje wymuszona macierza (uzywane przy podpinaniu oczu do kosci czaszki)
    void DrawWithTransform(Shader& shader, const glm::mat4& transform) const;

    // Rysuje obiekt dodatkowo doliczajac dynamiczne offsety testowe z interfejsu uzytkownika
    void DrawWithOffset(Shader& shader, const glm::mat4& transform, const glm::vec3& posOffset, const glm::vec3& rotOffsetDeg) const;

private:
    unsigned int VAO, VBO, EBO;

    // Inicjalizuje bufory OpenGL i odpowiednio binduje atrybuty wierzcholkow
    void setupMesh();
};