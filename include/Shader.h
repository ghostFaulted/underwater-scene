#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Obiektowa warstwa abstrakcji do obslugi programow cieniowania (shaderow GLSL).
class Shader {
public:
    // Indeks programu na serwerze GPU OpenGL
    unsigned int ID;

    // Konstruktor odczytujacy i kompilujacy kody programow na starcie aplikacji.
    Shader(const char* vertexPath, const char* fragmentPath);

    // Aktywowanie danego zestawu shaderow przed wykonaniem glDrawElements
    void use();

    // Szereg narzedzi do modyfikacji tzw. Unformow wewnatrz dzialajacego programu
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    // Pomocnicza metoda sprawdzajaca bledy z kompilatora GLSL (przydatne przy pisaniu kodow shaderow)
    void checkCompileErrors(unsigned int shader, std::string type);
};