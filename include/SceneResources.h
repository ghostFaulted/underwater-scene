#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "Shader.h"
#include "SplinePath.h"

struct ShaderSet {
    Shader pbr;
    Shader skybox;
    Shader lines;
    Shader shadow;

    ShaderSet(
        const std::string& pbrVertPath,
        const std::string& pbrFragPath,
        const std::string& skyVertPath,
        const std::string& skyFragPath,
        const std::string& lineVertPath,
        const std::string& lineFragPath,
        const std::string& shadowVertPath,
        const std::string& shadowFragPath
    )
        : pbr(pbrVertPath.c_str(), pbrFragPath.c_str())
        , skybox(skyVertPath.c_str(), skyFragPath.c_str())
        , lines(lineVertPath.c_str(), lineFragPath.c_str())
        , shadow(shadowVertPath.c_str(), shadowFragPath.c_str()) {}
};

struct TextureSet {
    unsigned int sharkAlbedo = 0;
    unsigned int sharkNormal = 0;
    unsigned int sharkDetailNormal = 0;
    unsigned int sharkEyeAlbedo = 0;
    unsigned int sharkEyeNormal = 0;
    unsigned int sharkTeethAlbedo = 0;
    unsigned int sharkTeethNormal = 0;
    unsigned int seabedAlbedo = 0;
    unsigned int seabedNormal = 0;
    unsigned int seabedFlowMap = 0;
    unsigned int submarineAlbedo = 0;
};

struct TrajectoryDebugBuffers {
    unsigned int normalVAO = 0;
    unsigned int normalVBO = 0;
    unsigned int normalLineCount = 0;

    unsigned int binormalVAO = 0;
    unsigned int binormalVBO = 0;
    unsigned int binormalLineCount = 0;

    unsigned int pathVAO = 0;
    unsigned int pathVBO = 0;
    unsigned int pathLineCount = 0;

    bool closedLoop = false;
};

struct ShadowMapResources {
    unsigned int framebuffer = 0;
    unsigned int depthMap = 0;
    unsigned int width = 2048;
    unsigned int height = 2048;
};

std::string ResolveProjectPath(const std::string& relativePath);
ShaderSet LoadShaders();
TextureSet LoadTextures();
std::vector<std::string> GetSkyboxFaces();
std::vector<glm::vec3> GenerateControlPoints();
TrajectoryDebugBuffers CreateTrajectoryDebugBuffers(const SplinePath& splinePath, bool useClosedTrajectory);
void DestroyTrajectoryDebugBuffers(TrajectoryDebugBuffers& buffers);
ShadowMapResources CreateShadowMapResources(unsigned int width, unsigned int height);
void DestroyShadowMapResources(ShadowMapResources& resources);
std::vector<glm::vec3> GenerateSubmarinePath();
std::vector<glm::vec3> GenerateCameraPath();