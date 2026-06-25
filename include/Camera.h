#ifndef UNDERWATERSCENE_CAMERA_H
#define UNDERWATERSCENE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
    glm::vec3 position;
    glm::quat orientation;
    float movementSpeed;
    float mouseSensitivity;

public:
    explicit Camera(const glm::vec3& startPosition = glm::vec3(0.0f, 0.0f, 3.0f));

    void ProcessMouse(float deltaX, float deltaY);
    void ProcessKeyboard(bool moveForward, bool moveBackward, bool moveLeft, bool moveRight, float deltaTime);
    void SetPosition(const glm::vec3& newPosition);

    [[nodiscard]] glm::mat4 GetViewMatrix() const;

    [[nodiscard]] glm::vec3 GetForward() const;
    [[nodiscard]] glm::vec3 GetUp() const;
    [[nodiscard]] glm::vec3 GetRight() const;
    [[nodiscard]] const glm::vec3& GetPosition() const;
};


#endif //UNDERWATERSCENE_CAMERA_H
