set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# Minimum CMake version required for FetchContent
cmake_minimum_required(VERSION 3.15)
project(UnderwaterScene VERSION 1.0.0 LANGUAGES C CXX)

# Enforce modern C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ------------------------------------------------------------------------------
# 1. GLFW (Window creation and input)
# ------------------------------------------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

# ------------------------------------------------------------------------------
# 3. GLM (Mathematics)
# ------------------------------------------------------------------------------
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 0.9.9.8)
FetchContent_MakeAvailable(glm)

# ------------------------------------------------------------------------------
# 4. Assimp (3D Model loading)
# ------------------------------------------------------------------------------
# Disable tests and extra tools to significantly speed up initial compilation
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(assimp GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v5.3.1)
FetchContent_MakeAvailable(assimp)

# ------------------------------------------------------------------------------
# 5. stb_image (Texture loading)
# ------------------------------------------------------------------------------
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb)

# ------------------------------------------------------------------------------
# 6. Dear ImGui (UI)
# ------------------------------------------------------------------------------
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.90)
FetchContent_MakeAvailable(imgui)

# Compile ImGui and its backends as a static library
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# ------------------------------------------------------------------------------
# Main Executable Setup
# ------------------------------------------------------------------------------
add_executable(UnderwaterScene 
    src/main.cpp
    src/stb_image.cpp 
    src/glad.c
        src/Camera.cpp
        src/Camera.h
        src/SplinePath.cpp
        src/SplinePath.h
)

add_executable(SplinePathTest
    src/SplinePathTest.cpp
    src/SplinePath.cpp
)

# Include directories
target_include_directories(UnderwaterScene PUBLIC
    include
    ${stb_SOURCE_DIR}
)

# Link all dependencies
target_link_libraries(UnderwaterScene PRIVATE
    glfw
    glm
    assimp
    imgui
)

target_link_libraries(SplinePathTest PRIVATE glm)
