#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(const glm::vec3& startPosition)
    : position(startPosition), orientation(glm::identity<glm::quat>()), movementSpeed(3.0f), mouseSensitivity(0.0025f) {}

void Camera::ProcessMouse(float deltaX, float deltaY) {
    const float yaw = -deltaX * mouseSensitivity;
    const float pitch = -deltaY * mouseSensitivity;

    const glm::vec3 localRight = GetRight();
    const glm::vec3 globalUp(0.0f, 1.0f, 0.0f);

    orientation = glm::normalize(glm::angleAxis(pitch, localRight) * orientation);
    orientation = glm::normalize(glm::angleAxis(yaw, globalUp) * orientation);
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

