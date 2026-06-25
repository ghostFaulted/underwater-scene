#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(const glm::vec3& startPosition)
    : position(startPosition), orientation(glm::identity<glm::quat>()), movementSpeed(10.0f), mouseSensitivity(0.0025f) {}

void Camera::ProcessMouse(float deltaX, float deltaY) {
    glm::vec3 forward = GetForward();

    float yaw = std::atan2(forward.z, forward.x);
    float pitch = std::asin(glm::clamp(forward.y, -1.0f, 1.0f));

    yaw += deltaX * mouseSensitivity;
    pitch -= deltaY * mouseSensitivity;

    constexpr float pitchLimit = glm::radians(89.0f);
    pitch = glm::clamp(pitch, -pitchLimit, pitchLimit);

    glm::vec3 newForward;
    newForward.x = std::cos(yaw) * std::cos(pitch);
    newForward.y = std::sin(pitch);
    newForward.z = std::sin(yaw) * std::cos(pitch);
    newForward = glm::normalize(newForward);

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f), newForward, glm::vec3(0.0f, 1.0f, 0.0f));

    orientation = glm::quat_cast(view);
}

void Camera::ProcessKeyboard(bool moveForward, bool moveBackward, bool moveLeft, bool moveRight, float deltaTime) {
    const float velocity = movementSpeed * deltaTime;

    if (moveForward) {
        position += GetForward() * velocity;
    }
    if (moveBackward) {
        position -= GetForward() * velocity;
    }
    if (moveLeft) {
        position -= GetRight() * velocity;
    }
    if (moveRight) {
        position += GetRight() * velocity;
    }
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::toMat4(orientation) * glm::translate(glm::mat4(1.0f), -position);
}

glm::vec3 Camera::GetForward() const {
    const glm::quat worldOrientation = glm::conjugate(orientation);
    return glm::normalize(worldOrientation * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Camera::GetUp() const {
    const glm::quat worldOrientation = glm::conjugate(orientation);
    return glm::normalize(worldOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::GetRight() const {
    const glm::quat worldOrientation = glm::conjugate(orientation);
    return glm::normalize(worldOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
}

const glm::vec3& Camera::GetPosition() const {
    return position;
}

void Camera::SetPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

