#include "Mesh.h"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
    this->vertices = std::move(vertices);
    this->indices = std::move(indices);
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Zaladowanie danych wierzcholkow do pameci GPU (VBO)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // Zaladowanie danych o kolejnosci wierzcholkow (EBO - Element Buffer Object)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Pozycje wierzcholkow
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // Koordynaty UV tekstury
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // Normalne siatki
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // Tangens (do mapowania normalnych)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // Bitangens
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    // ID Kosci dla skinningu (nalezy uzyc funkcji dla liczb calkowitych - glVertexAttribIPointer)
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIDs));
    // Wagi Kosci dla skinningu
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, BoneWeights));

    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) const {
    // Przesyla do shadera transformacje jako unform w przypadku siatek bez animacji kosci (np. zeby)
    shader.setMat4("uNodeTransform", nodeTransform);
    shader.setInt("uMeshMaterialKind", static_cast<int>(materialKind));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::DrawWithTransform(Shader& shader, const glm::mat4& transform) const {
    shader.setMat4("uNodeTransform", transform);
    shader.setInt("uMeshMaterialKind", static_cast<int>(materialKind));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Pozwala nadpisac pozycje i obrot (np z ImGui dla dopasowywania parametrow oczu/zebow testowo)
void Mesh::DrawWithOffset(Shader& shader, const glm::mat4& transform, const glm::vec3& posOffset, const glm::vec3& rotOffsetDeg) const {
    glm::mat4 local(1.0f);
    local = glm::translate(local, posOffset);

    // Kwaternionowy system obrotu dla przesuniec z interfejsu
    const float rotLenSq = rotOffsetDeg.x * rotOffsetDeg.x + rotOffsetDeg.y * rotOffsetDeg.y + rotOffsetDeg.z * rotOffsetDeg.z;
    if (rotLenSq > 1e-8f) {
        const glm::quat qx = glm::angleAxis(glm::radians(rotOffsetDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::quat qy = glm::angleAxis(glm::radians(rotOffsetDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat qz = glm::angleAxis(glm::radians(rotOffsetDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
        local = local * glm::mat4_cast(qz * qy * qx);
    }

    shader.setMat4("uNodeTransform", transform * local);
    shader.setInt("uMeshMaterialKind", static_cast<int>(materialKind));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}