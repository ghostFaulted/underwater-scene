#pragma once

#include <cstddef>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Shader.h"

// Struktura konfiguracyjna parametryzujaca proceduralna animacje plywania po splajnie.
// Pozwala kontrolowac ilosc kosci oraz to, jak mocno i szybko model faluje.
struct SplineSwimSettings {
    std::size_t boneCount = 8;         // Liczba wirtualnych kosci do symulacji ciala
    float bodyMinForward = -3.2f;      // Zasiegi ciala w osi Z (od - do) sluzace do rownomiernego rozlozenia kosci
    float bodyMaxForward = 3.6f;
    float waveAmplitudeDegrees = 18.0f;// Maksymalne odchylenie fali w stopniach (jak mocno wygina sie ogon)
    float waveFrequencyHz = 1.25f;     // Czestotliwosc machania (cykle na sekunde)
    float wavePhaseLag = 0.45f;        // Przesuniecie fazowe tworzace plynna fale (wazowaty ruch) zamiast sztywnego machania
    float curvatureToBiasDegrees = 1200.0f; // Wplyw krzywizny splajnu (zakretu) na wygiecie ciala
    float tailBiasGain = 1.35f;        // Dodatkowy mnoznik wygiecia dla samego ogona podczas zakretu
};

// Klasa odpowiedzialna za wyliczanie macierzy wirtualnych kosci symulujacych ruch wezowy.
// Rozni sie od SharkSkeletonAnimator tym, ze generuje kosci od zera (dla obiektow bez wlasnego szkieletu).
class SplineSwimAnimator {
public:
    static constexpr std::size_t kMaxBones = 16; // Twardy limit ilosci generowanych kosci by nie obciazac shadera

    explicit SplineSwimAnimator(SplineSwimSettings settings = {});

    // Glowna funkcja obliczajaca matematyczne wychylenia kosci w danym momencie czasu i na zakrecie
    void BuildPose(float animationTimeSeconds, float signedCurvature);

    // Wysyla wyliczone macierze transformacji wprost do tablicy 'uSplineBones' wewnatrz shadera PBR
    void UploadToShader(Shader& shader, bool enabled) const;

private:
    SplineSwimSettings settings;
    std::vector<glm::mat4> bones; // Przechowuje gotowe macierze lokalne dla kazdej kosci
};