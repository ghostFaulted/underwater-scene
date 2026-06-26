#include "SceneResources.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <filesystem>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <stdexcept>

std::string ResolveProjectPath(const std::string& relativePath);

namespace {
    // Sprytna funkcja wyszukujaca glowny folder projektu na dysku.
    // Dzieki temu nie trzeba martwic sie o sciezki wzgledne przy uruchamianiu pliku .exe z roznych folderow "build".
    std::filesystem::path FindProjectRoot() {
        const auto start = std::filesystem::current_path();

        for (auto candidate = start; ; candidate = candidate.parent_path()) {
            const bool hasProjectMarkers =
                std::filesystem::exists(candidate / "CMakeLists.txt") &&
                std::filesystem::exists(candidate / "assets") &&
                std::filesystem::exists(candidate / "shaders") &&
                std::filesystem::exists(candidate / "include") &&
                std::filesystem::exists(candidate / "src");

            if (hasProjectMarkers) {
                return candidate;
            }

            if (candidate == candidate.parent_path()) {
                break;
            }
        }

        throw std::runtime_error("Unable to locate project root from current working directory");
    }

    // Zwraca zapamietana sciezke roota zeby nie iterowac co teksture
    const std::filesystem::path& ProjectRoot() {
        static const std::filesystem::path root = FindProjectRoot();
        return root;
    }

    // Funkcja uzywajaca zewnetrznej biblioteki stb_image do czytania plikow jpg/png do formatu bitowego
    unsigned int LoadTexture(const char* path) {
        unsigned int textureID = 0;
        glGenTextures(1, &textureID);

        const std::string resolvedPath = ResolveProjectPath(path);

        std::string ext = std::filesystem::path(resolvedPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (ext.empty()) {
            std::cout << "[FAIL] Texture has no extension: " << resolvedPath << std::endl;
        }

        // Ladowanie formatow standardowych
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tga") {
            int width = 0, height = 0, nrComponents = 0;
            // stbi_load automatycznie czyta i konwertuje piksele
            unsigned char* data = stbi_load(resolvedPath.c_str(), &width, &height, &nrComponents, 0);
            if (data != nullptr) {
                GLenum format = GL_RGB;
                if (nrComponents == 1) {
                    format = GL_RED;
                }
                else if (nrComponents == 2) {
                    format = GL_RG;
                }
                else if (nrComponents == 3) {
                    format = GL_RGB;
                }
                else if (nrComponents == 4) {
                    format = GL_RGBA;
                }

                glBindTexture(GL_TEXTURE_2D, textureID);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                // Wrzucenie bitmapy do pamieci VRAM OpenGL
                glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

                // Automatyczne generowanie mip-map dla dalszych odleglosci ulatwiajace teksturowanie
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                // Zwalnianie pamieci RAM (tekstura jest w VRAM)
                stbi_image_free(data);
                std::cout << "[OK] Loaded: " << resolvedPath
                    << " (" << width << "x" << height
                    << ", channels=" << nrComponents << ")" << std::endl;
                return textureID;
            }
            std::cout << "[FAIL] stbi_load failed for: " << resolvedPath << std::endl;
        }

        if (ext == ".dds") {
            std::cout << "[WARN] DDS not supported via stb_image. Using white placeholder for: " << resolvedPath << std::endl;
        }
        else {
            std::cout << "[FAIL] Could not load texture: " << resolvedPath << std::endl;
        }

        // Mechanizm bezpieczenstwa: jesli nie uda sie zaladowac sciezki, tworyzmy teksture wielkosci 1x1 
        // ktora ma biale wypelnienie (Fallback placeholder) zeby nie zepsuc shadera PBR.
        constexpr unsigned char whitePixel[] = { 255, 255, 255, 255 };
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return textureID;
    }

    // Pomocnicza funkcja generujaca VBO i VAO bezposrednio z prostych wektorow glm::vec3
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

// Mapuje podane relatywne sciezki do pelnych absolutnych w przestrzeni dyskowej SO
std::string ResolveProjectPath(const std::string& relativePath) {
    const std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        return path.lexically_normal().string();
    }

    return (ProjectRoot() / path).lexically_normal().string();
}

ShaderSet LoadShaders() {
    const std::string pbrVertPath = ResolveProjectPath("shaders/pbr.vert");
    const std::string pbrFragPath = ResolveProjectPath("shaders/pbr.frag");
    const std::string skyVertPath = ResolveProjectPath("shaders/skybox.vert");
    const std::string skyFragPath = ResolveProjectPath("shaders/skybox.frag");
    const std::string lineVertPath = ResolveProjectPath("shaders/lines.vert");
    const std::string lineFragPath = ResolveProjectPath("shaders/lines.frag");
    const std::string shadowVertPath = ResolveProjectPath("shaders/shadow.vert");
    const std::string shadowFragPath = ResolveProjectPath("shaders/shadow.frag");
    std::cout << "[DEBUG] Shaders loaded from project root: " << ProjectRoot() << std::endl;
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
        LoadTexture("assets/shark/greatwhiteshark.png"),              // sharkAlbedo
        LoadTexture("assets/shark/greatwhiteshark_DDNDIF.png"),       // sharkNormal
        LoadTexture("assets/shark/greatwhiteshark_spec.png"),         // sharkDetailNormal
        LoadTexture("assets/shark/greatwhiteshark_Eye.png"),          // sharkEyeAlbedo
        LoadTexture("assets/shark/greatwhiteshark_Eye_DDNDIF.png"),   // sharkEyeNormal
        LoadTexture("assets/shark/greatwhiteshark_teeth.png"),        // sharkTeethAlbedo
        LoadTexture("assets/shark/greatwhiteshark_teeth_DDNDIF.png"), // sharkTeethNormal
        LoadTexture("assets/ocean_floor/model.jpg"),                  // seabedAlbedo
        LoadTexture("assets/ocean_floor/seabed_normal.png"),          // seabedNormal
        LoadTexture("assets/ocean_floor/flowmap.jpg"),                // seabedFlowMap
        LoadTexture("assets/submarine/ger_sub_diffuse.png")           // submarineAlbedo
    };
}

std::vector<std::string> GetSkyboxFaces() {
    return {
        ResolveProjectPath("assets/skybox/uw_rt.jpg"),
        ResolveProjectPath("assets/skybox/uw_lf.jpg"),
        ResolveProjectPath("assets/skybox/uw_up.jpg"),
        ResolveProjectPath("assets/skybox/uw_dn.jpg"),
        ResolveProjectPath("assets/skybox/uw_bk.jpg"),
        ResolveProjectPath("assets/skybox/uw_ft.jpg")
    };
}

// Zmodyfikowane i ustalone punkty wezlowe pod wirtualny wektor Splajnu (rekina)
std::vector<glm::vec3> GenerateControlPoints() {
    return {
        { 27.0f,  3.0f,  45.0f},
        { 45.0f,  2.0f,  15.0f},
        { 42.0f,  4.0f, -12.0f},
        { 50.0f,  2.0f, -50.0f},
        {  0.0f,  1.0f, -52.0f},
        {-32.0f, 15.0f, -32.0f},
        {-22.0f, 0.0f,  10.0f},
        {-18.0f, 10.0f,  32.0f}
    };
}

// Trasa przejazdu dla kamery podazajacej (excursion submarine)
std::vector<glm::vec3> GenerateSubmarinePath() {
    return {
        {  0.0f,  10.0f,  55.0f},
        { 40.0f,   6.0f,  40.0f},
        { 55.0f,  6.0f,   0.0f},
        { 40.0f,   7.0f, -45.0f},
        {  0.0f,  -8.0f, -60.0f},
        {-45.0f,   4.0f, -40.0f},
        {-60.0f,   2.0f,   0.0f},
        {-40.0f,  12.0f,  40.0f}
    };
}

std::vector<glm::vec3> GenerateCameraPath() {
    return {
        {  0.0f,  15.0f,  75.0f},
        { 60.0f,  10.0f,  50.0f},
        { 85.0f,  25.0f,   0.0f},
        { 60.0f,  15.0f, -50.0f},
        {  0.0f,  10.0f, -75.0f},
        {-60.0f,  15.0f, -50.0f},
        {-85.0f,   8.0f,   0.0f},
        {-60.0f,  20.0f,  50.0f}
    };
}

// Funkcja debugowa samplujaca punkty co chwile ze skomplikowanego splajnu do prostych kresek zeby narysowac je za pomoca GL_LINES
TrajectoryDebugBuffers CreateTrajectoryDebugBuffers(const SplinePath& splinePath, const bool useClosedTrajectory) {
    TrajectoryDebugBuffers buffers{};
    buffers.closedLoop = useClosedTrajectory;

    {
        constexpr int splinePointsNumber = 1000;
        std::vector<glm::vec3> binormalLineVertices;
        binormalLineVertices.reserve(splinePointsNumber * 2);
        std::vector<glm::vec3> normalLineVertices;
        normalLineVertices.reserve(splinePointsNumber * 2);

        // Petla zczytujaca posrednie punkty uzywajac metody transformacji
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

// Funkcja rejestrujaca niestandardowy Framebuffer wykorzystywany potem w RenderShadowPass. 
// Generuje teksture, na ktorej nie zachowuje sie koloru pikseli, lecz informacje o buforze glebi z danego punktu.
ShadowMapResources CreateShadowMapResources(const unsigned int width, const unsigned int height) {
    ShadowMapResources resources{};
    resources.width = width;
    resources.height = height;

    glGenFramebuffers(1, &resources.framebuffer);
    glGenTextures(1, &resources.depthMap);
    glBindTexture(GL_TEXTURE_2D, resources.depthMap);

    // Zarejestrowanie tekstury wylacznie pod format GL_DEPTH_COMPONENT
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

    // Zapobieganie zjawisku rozmywania cieni poza dozwolona macierza cieni (border = czyste tlo 1.0)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Zmiana buforowania z zapisem do wlasnie przygotowanej tekstury
    glBindFramebuffer(GL_FRAMEBUFFER, resources.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resources.depthMap, 0);
    // Potwierdzenie opuszczenia obowiazku podawania wartosci rgb (color / read / write)
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