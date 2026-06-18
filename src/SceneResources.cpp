#include "SceneResources.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <filesystem>
#include <iostream>

namespace {
    unsigned int LoadTexture(const char* path) {
        unsigned int textureID = 0;
        glGenTextures(1, &textureID);

        int width = 0;
        int height = 0;
        int nrComponents = 0;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 4);
        if (data == nullptr) {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
            return textureID;
        }

        constexpr GLenum format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return textureID;
    }

    void UploadLineBuffer(const std::vector<glm::vec3>& vertices, unsigned int& vao, unsigned int& vbo) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

std::string ResolveShaderPath(const std::string& relativePath) {
    if (std::filesystem::exists(relativePath)) {
        return relativePath;
    }

    std::string parentPath = "../" + relativePath;
    if (std::filesystem::exists(parentPath)) {
        return parentPath;
    }

    return relativePath;
}

ShaderSet LoadShaders() {
    const std::string pbrVertPath = ResolveShaderPath("shaders/pbr.vert");
    const std::string pbrFragPath = ResolveShaderPath("shaders/pbr.frag");
    const std::string skyVertPath = ResolveShaderPath("shaders/skybox.vert");
    const std::string skyFragPath = ResolveShaderPath("shaders/skybox.frag");
    const std::string lineVertPath = ResolveShaderPath("shaders/lines.vert");
    const std::string lineFragPath = ResolveShaderPath("shaders/lines.frag");
    const std::string shadowVertPath = ResolveShaderPath("shaders/shadow.vert");
    const std::string shadowFragPath = ResolveShaderPath("shaders/shadow.frag");

    std::cout << "[DEBUG] Shaders loaded successfully" << std::endl;
    return {
        pbrVertPath,
        pbrFragPath,
        skyVertPath,
        skyFragPath,
        lineVertPath,
        lineFragPath,
        shadowVertPath,
        shadowFragPath
    };
}

TextureSet LoadTextures() {
    return {
        LoadTexture("assets/submarine/ger_sub_diffuse.png"),
        LoadTexture("assets/ocean_floor/model.jpg"),
        LoadTexture("assets/ocean_floor/seabed_normal.png")
    };
}

std::vector<std::string> GetSkyboxFaces() {
    return {
        "assets/skybox/uw_rt.jpg",
        "assets/skybox/uw_lf.jpg",
        "assets/skybox/uw_up.jpg",
        "assets/skybox/uw_dn.jpg",
        "assets/skybox/uw_bk.jpg",
        "assets/skybox/uw_ft.jpg"
    };
}

std::vector<glm::vec3> GenerateControlPoints() {
    return {
        {0.0f, -2.0f, 0.0f},
        {5.0f, -1.0f, -5.0f},
        {10.0f, -4.0f, -10.0f},
        {15.0f, -8.0f, -15.0f},
        {20.0f, -12.0f, -10.0f},
        {25.0f, -15.0f, 0.0f},
        {20.0f, -12.0f, 10.0f},
        {10.0f, -6.0f, 20.0f},
        {0.0f, -2.0f, 25.0f},
        {-10.0f, -5.0f, 20.0f},
        {-20.0f, -10.0f, 10.0f},
        {-25.0f, -14.0f, 0.0f},
        {-20.0f, -10.0f, -10.0f},
        {-10.0f, -4.0f, -20.0f},
        {0.0f, 0.0f, -25.0f},
        {10.0f, 3.0f, -15.0f},
        {15.0f, 6.0f, -5.0f},
        {10.0f, 4.0f, 5.0f},
        {5.0f, 1.0f, 8.0f},
        {2.0f, -1.0f, 4.0f}
    };
}

TrajectoryDebugBuffers CreateTrajectoryDebugBuffers(const SplinePath& splinePath, const bool useClosedTrajectory) {
    TrajectoryDebugBuffers buffers{};
    buffers.closedLoop = useClosedTrajectory;

    {
        constexpr int splinePointsNumber = 1000;
        std::vector<glm::vec3> binormalLineVertices;
        binormalLineVertices.reserve(splinePointsNumber * 2);
        std::vector<glm::vec3> normalLineVertices;
        normalLineVertices.reserve(splinePointsNumber * 2);

        for (int i = 0; i <= splinePointsNumber; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(splinePointsNumber);
            const auto transform = splinePath.GetTransform(t);
            const auto pos = glm::vec3(transform[3]);
            const auto binormal = glm::vec3(transform[0]);
            const auto normal = glm::vec3(transform[1]);

            binormalLineVertices.push_back(pos);
            binormalLineVertices.push_back(pos + binormal * 0.5f);
            normalLineVertices.push_back(pos);
            normalLineVertices.push_back(pos + normal * 0.5f);
        }

        buffers.normalLineCount = static_cast<unsigned int>(normalLineVertices.size());
        buffers.binormalLineCount = static_cast<unsigned int>(binormalLineVertices.size());

        UploadLineBuffer(normalLineVertices, buffers.normalVAO, buffers.normalVBO);
        UploadLineBuffer(binormalLineVertices, buffers.binormalVAO, buffers.binormalVBO);
    }

    {
        constexpr int splinePointsNumber = 500;
        std::vector<glm::vec3> pathLineVertices;
        pathLineVertices.reserve(splinePointsNumber);

        const int sampleCount = useClosedTrajectory ? splinePointsNumber : splinePointsNumber + 1;
        for (int i = 0; i < sampleCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(splinePointsNumber);
            const auto transform = splinePath.GetTransform(t);
            pathLineVertices.emplace_back(transform[3]);
        }

        buffers.pathLineCount = static_cast<unsigned int>(pathLineVertices.size());
        UploadLineBuffer(pathLineVertices, buffers.pathVAO, buffers.pathVBO);
    }

    std::cout << "[NORMAL] VAO=" << buffers.normalVAO << " vertices=" << buffers.normalLineCount << std::endl;
    std::cout << "[BINORMAL] VAO=" << buffers.binormalVAO << " vertices=" << buffers.binormalLineCount << std::endl;
    std::cout << "[PATH] VAO=" << buffers.pathVAO << " vertices=" << buffers.pathLineCount << std::endl;

    return buffers;
}

void DestroyTrajectoryDebugBuffers(TrajectoryDebugBuffers& buffers) {
    glDeleteBuffers(1, &buffers.normalVBO);
    glDeleteVertexArrays(1, &buffers.normalVAO);
    glDeleteBuffers(1, &buffers.binormalVBO);
    glDeleteVertexArrays(1, &buffers.binormalVAO);
    glDeleteBuffers(1, &buffers.pathVBO);
    glDeleteVertexArrays(1, &buffers.pathVAO);

    buffers = {};
}

ShadowMapResources CreateShadowMapResources(const unsigned int width, const unsigned int height) {
    ShadowMapResources resources{};
    resources.width = width;
    resources.height = height;

    glGenFramebuffers(1, &resources.framebuffer);
    glGenTextures(1, &resources.depthMap);
    glBindTexture(GL_TEXTURE_2D, resources.depthMap);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        static_cast<GLsizei>(resources.width),
        static_cast<GLsizei>(resources.height),
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, resources.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resources.depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return resources;
}

void DestroyShadowMapResources(ShadowMapResources& resources) {
    glDeleteTextures(1, &resources.depthMap);
    glDeleteFramebuffers(1, &resources.framebuffer);
    resources = {};
}

