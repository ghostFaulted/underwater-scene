#ifndef UNDERWATERSCENE_CAMERA_H
#define UNDERWATERSCENE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Klasa reprezentujaca kamere oparta na kwaternionach (eliminuje problem Gimbal Lock)
class Camera {
    glm::vec3 position;         // Pozycja kamery w swiecie 3D
    glm::quat orientation;      // Kwaternion reprezentujacy rotacje
    float movementSpeed;        // Predkosc przemieszczania (WASD)
    float mouseSensitivity;     // Czulosc myszki

public:
    explicit Camera(const glm::vec3& startPosition = glm::vec3(0.0f, 0.0f, 3.0f));

    // Przetwarza ruch myszki w celu obrotu kamery
    void ProcessMouse(float deltaX, float deltaY);

    // Przetwarza stan klawiatury w celu ruchu kamery
    void ProcessKeyboard(bool moveForward, bool moveBackward, bool moveLeft, bool moveRight, float deltaTime);

    // Ustawia sztywno nowa pozycje kamery (uzywane np. w trybie Excursion)
    void SetPosition(const glm::vec3& newPosition);

    // Wymusza skierowanie wzroku kamery na dany punkt w swiecie
    void LookAt(const glm::vec3& target);

    // Zwraca macierz widoku (View Matrix) dla shadera
    [[nodiscard]] glm::mat4 GetViewMatrix() const;

    // Gettery wektorow kierunkowych kamery
    [[nodiscard]] glm::vec3 GetForward() const;
    [[nodiscard]] glm::vec3 GetUp() const;
    [[nodiscard]] glm::vec3 GetRight() const;
    [[nodiscard]] const glm::vec3& GetPosition() const;
};

#endif //UNDERWATERSCENE_CAMERA_H