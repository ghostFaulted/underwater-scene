#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

Camera::Camera(const glm::vec3& startPosition)
    : position(startPosition), orientation(glm::identity<glm::quat>()), movementSpeed(10.0f), mouseSensitivity(0.0025f) {
}

// Aktualizacja rotacji kamery na podstawie ruchu myszki
void Camera::ProcessMouse(float deltaX, float deltaY) {
    // Odzyskanie wektora patrzenia z obecnego kwaternionu
    glm::vec3 forward = GetForward();

    // Wyciagniecie obecnych katow Yaw (lewo-prawo) i Pitch (gora-dol)
    float yaw = std::atan2(forward.z, forward.x);
    float pitch = std::asin(glm::clamp(forward.y, -1.0f, 1.0f));

    // Aplikacja przesuniecia z uwzglednieniem czulosci
    yaw += deltaX * mouseSensitivity;
    pitch -= deltaY * mouseSensitivity;

    // Ograniczenie wychylenia w pionie do 89 stopni, aby nie dalo sie zrobic "fikolka"
    constexpr float pitchLimit = glm::radians(89.0f);
    pitch = glm::clamp(pitch, -pitchLimit, pitchLimit);

    // Obliczenie nowego wektora patrzenia po zmienionych katach
    glm::vec3 newForward;
    newForward.x = std::cos(yaw) * std::cos(pitch);
    newForward.y = std::sin(pitch);
    newForward.z = std::sin(yaw) * std::cos(pitch);
    newForward = glm::normalize(newForward);

    // Utworzenie macierzy LookAt dla nowej orientacji (bez zmiany pozycji) i zamiana na Kwaternion
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f), newForward, glm::vec3(0.0f, 1.0f, 0.0f));
    orientation = glm::quat_cast(view);
}

// Poruszanie sie kamery w obrebie wlasnego ukladu lokalnego
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

// Setter pozycji
void Camera::SetPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

// Matematyczne obrocenie kamery w kierunku obiektu
void Camera::LookAt(const glm::vec3& target) {
    glm::vec3 dir = target - position;
    if (glm::length(dir) < 0.001f) return;

    glm::vec3 upRef = glm::vec3(0.0f, 1.0f, 0.0f);
    // Zabezpieczenie przed ekstremalnym patrzeniem prosto w gore lub w dol (wektor dir pokrywa sie z UP)
    if (std::abs(glm::dot(glm::normalize(dir), upRef)) > 0.999f) {
        upRef = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f), dir, upRef);
    orientation = glm::quat_cast(view);
}

// Konwersja rotacji i translacji z powrotem na macierz widoku 4x4 dla OpenGL
glm::mat4 Camera::GetViewMatrix() const {
    return glm::toMat4(orientation) * glm::translate(glm::mat4(1.0f), -position);
}

// Wydobycie wektora 'Do Przodu' z rotacji kamery
glm::vec3 Camera::GetForward() const {
    const glm::quat worldOrientation = glm::conjugate(orientation);
    return glm::normalize(worldOrientation * glm::vec3(0.0f, 0.0f, -1.0f));
}

// Wydobycie wektora 'W Gore' z rotacji kamery
glm::vec3 Camera::GetUp() const {
    const glm::quat worldOrientation = glm::conjugate(orientation);
    return glm::normalize(worldOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
}

// Wydobycie wektora 'W Prawo' z rotacji kamery
glm::vec3 Camera::GetRight() const {
    const glm::quat worldOrientation = glm::conjugate(orientation);
    return glm::normalize(worldOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
}

const glm::vec3& Camera::GetPosition() const {
    return position;
}