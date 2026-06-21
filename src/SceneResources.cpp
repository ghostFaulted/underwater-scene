#include "SceneResources.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <filesystem>
#include <iostream>
#include <algorithm>

namespace {
    // Try to load texture from path, supporting common image formats.
    // For DDS and other specialized formats, print a warning and return white placeholder.
    unsigned int LoadTexture(const char* path) {
        unsigned int textureID = 0;
        glGenTextures(1, &textureID);

        // Resolve path through fallbacks (current dir, parent dir, etc.)
        std::string resolvedPath = path;
        if (!std::filesystem::exists(resolvedPath)) {
            std::string parentPath = std::string("../") + path;
            if (std::filesystem::exists(parentPath)) {
                resolvedPath = parentPath;
            } else {
                // Try sibling directories
                std::filesystem::path p(path);
                if (!p.parent_path().empty()) {
                    // Last component is filename
                    std::string filename = p.filename().string();
                    std::string dir = p.parent_path().string();
                    std::string altPath = std::string("../") + dir + "/" + filename;
                    if (std::filesystem::exists(altPath)) {
                        resolvedPath = altPath;
                    }
                }
            }
        }

        // Check file extension
        std::string ext;
        {
            size_t lastDot = resolvedPath.find_last_of('.');
            if (lastDot != std::string::npos) {
                ext = resolvedPath.substr(lastDot);
                // Convert to lowercase
                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return std::tolower(c); });
            }
        }

        // ── Try stbi_load for standard formats ────────────────────────────
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga") {
            int width = 0, height = 0, nrComponents = 0;
            unsigned char* data = stbi_load(resolvedPath.c_str(), &width, &height, &nrComponents, 0);
            if (data != nullptr) {
                GLenum format = GL_RGB;
                if (nrComponents == 1) {
                    format = GL_RED;
                } else if (nrComponents == 2) {
                    format = GL_RG;
                } else if (nrComponents == 3) {
                    format = GL_RGB;
                } else if (nrComponents == 4) {
                    format = GL_RGBA;
                }

                glBindTexture(GL_TEXTURE_2D, textureID);
                // Avoid row-alignment overruns for RGB/RG textures with odd widths.
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                stbi_image_free(data);
                std::cout << "[OK] Loaded: " << path
                          << " (" << width << "x" << height
                          << ", channels=" << nrComponents << ")" << std::endl;
                return textureID;
            }
            std::cout << "[FAIL] stbi_load failed for: " << path << std::endl;
        }

        // ── DDS and other formats: create white placeholder ────────────────
        if (ext == ".dds") {
            std::cout << "[WARN] DDS not supported via stb_image. Using white placeholder for: " << path << std::endl;
            std::cout << "       To use DDS textures, convert them to PNG or JPEG." << std::endl;
        } else {
            std::cout << "[FAIL] Could not load texture: " << path << std::endl;
        }

        // Create 1x1 white placeholder texture
        constexpr unsigned char whitePixel[] = { 255, 255, 255, 255 };
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    // if (std::filesystem::exists(relativePath)) {
    //     return relativePath;
    // }

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
    std::cout
    << std::filesystem::absolute(pbrFragPath)
    << std::endl;
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
        LoadTexture("assets/shark/greatwhiteshark.png"),              // sharkAlbedo (diffuse)
        LoadTexture("assets/shark/greatwhiteshark_DDNDIF.png"),       // sharkNormal (normal map)
        LoadTexture("assets/shark/greatwhiteshark_spec.png"),         // sharkDetailNormal (specular, repurposed)
        LoadTexture("assets/shark/greatwhiteshark_Eye.png"),          // sharkEyeAlbedo
        LoadTexture("assets/shark/greatwhiteshark_Eye_DDNDIF.png"),   // sharkEyeNormal
        LoadTexture("assets/shark/greatwhiteshark_teeth.png"),        // sharkTeethAlbedo
        LoadTexture("assets/shark/greatwhiteshark_teeth_DDNDIF.png"), // sharkTeethNormal
        LoadTexture("assets/ocean_floor/model.jpg"),                  // seabedAlbedo
        LoadTexture("assets/ocean_floor/seabed_normal.png")           // seabedNormal
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

