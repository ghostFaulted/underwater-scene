#include "SplineSwimAnimator.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr float kPi = 3.14159265358979323846f;

    // Funkcja pomocnicza do zamiany stopni na radiany wymagane przez funkcje trygonometryczne C++
    float DegreesToRadians(const float value) {
        return value * (kPi / 180.0f);
    }

    // Funkcja wagi potegowej - im blizej konca ciala (ogona), tym wartosc rosnie kwadratowo.
    // Dzieki temu glowa (index 0) rusza sie bardzo malo, a ogon bardzo mocno.
    float TailWeight(const std::size_t index, const std::size_t count) {
        if (count <= 1) {
            return 1.0f;
        }

        const float t = static_cast<float>(index) / static_cast<float>(count - 1);
        return t * t;
    }
}

SplineSwimAnimator::SplineSwimAnimator(SplineSwimSettings settings)
    : settings(std::move(settings)) {
    // Zabezpieczenie przed podaniem za malej badz za duzej (przekraczajacej limit shadera) ilosci kosci
    const std::size_t clampedCount = std::clamp<std::size_t>(this->settings.boneCount, 2, kMaxBones);
    this->settings.boneCount = clampedCount;
    bones.assign(clampedCount, glm::mat4(1.0f));
}

void SplineSwimAnimator::BuildPose(const float animationTimeSeconds, const float signedCurvature) {
    const std::size_t boneCount = bones.size();

    // Dlugosc ciala rozlozona w przestrzeni Z
    const float bodyRange = std::max(0.001f, settings.bodyMaxForward - settings.bodyMinForward);
    // Podstawowa "fala" czasu zaleznosc od czestotliwosci (Hz)
    const float basePhase = animationTimeSeconds * settings.waveFrequencyHz * 2.0f * kPi;

    for (std::size_t i = 0; i < boneCount; ++i) {
        // Pozycja znormalizowana [0.0 - 1.0] (0 = poczatek ciala, 1 = sam koniec)
        const float normalized = static_cast<float>(i) / static_cast<float>(boneCount - 1);
        // Pozycja lokalna kosci na osi przod-tyl (Z)
        const float pivotForward = settings.bodyMinForward + normalized * bodyRange;
        // Waga sily animacji nalezaca do tej konkretnej kosci
        const float tailWeight = TailWeight(i, boneCount);

        // Wyliczenie fali z opoznieniem fazowym, by imitowac odpychanie wody cialem
        const float waveAngle = std::sin(basePhase - normalized * settings.wavePhaseLag) *
            settings.waveAmplitudeDegrees * tailWeight;

        // Wplyw krzywizny podazania splajnem - pozwala cialu ugiac sie by wpasowac sie w luczek zakretu
        const float turnBias = signedCurvature * settings.curvatureToBiasDegrees *
            tailWeight * settings.tailBiasGain;

        // Sumowanie kata fali i ugiecia na zakrecie
        const float angleRadians = DegreesToRadians(waveAngle + turnBias);

        // Skonstruowanie macierzy deformacji wezla
        glm::mat4 boneTransform(1.0f);
        // Przesuwamy srodek obrotu z lokalnego 0,0,0 do faktycznego miejsca na ciele
        boneTransform = glm::translate(boneTransform, glm::vec3(0.0f, 0.0f, pivotForward));
        // Aplikujemy obrot (Yaw / Odchylenie na boki)
        boneTransform = glm::rotate(boneTransform, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        // Cofamy srodek ukladu spowrotem do 0,0,0, by macierz zachowywala sie prawidlowo w shaderze
        boneTransform = glm::translate(boneTransform, glm::vec3(0.0f, 0.0f, -pivotForward));

        bones[i] = boneTransform;
    }
}

// Funkcja wgrywajaca stan wirtualnych kosci do potoku karty graficznej (OpenGL Uniforms)
void SplineSwimAnimator::UploadToShader(Shader& shader, const bool enabled) const {
    shader.setBool("uEnableSplineSkinning", enabled);
    shader.setInt("uSplineBoneCount", enabled ? static_cast<int>(bones.size()) : 0);
    // Przeslanie parametrow ciala w celu proceduralnego przypisywania wag wewnatrz Vertex Shadera
    shader.setFloat("uSplineBodyMin", settings.bodyMinForward);
    shader.setFloat("uSplineBodyMax", settings.bodyMaxForward);

    // Iteracja po wszystkich macierzach i wgrywanie ich do stalych uSplineBones w shaderze GLSL
    for (std::size_t i = 0; i < bones.size(); ++i) {
        shader.setMat4("uSplineBones[" + std::to_string(i) + "]", bones[i]);
    }
}