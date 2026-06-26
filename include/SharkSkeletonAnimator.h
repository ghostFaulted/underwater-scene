#pragma once

#include <glm/vec3.hpp>
#include <string>
#include <vector>

class Model;

// Struktura konfiguracyjna opisujaca fizyczne parametry plywania rekina.
// Pozwala w locie modyfikowac jak bardzo wygina sie cialo i z jaka czestotliwoscia.
struct SharkSwimSettings {
    float waveFrequencyHz = 1.15f;    // Jak szybko rekin macha ogonem (cykle na sekunde)
    float headAmplitudeDeg = 1.5f;    // Maksymalne wychylenie glowy (musi byc male by rekin nie rzucal sie na boki)
    float tailAmplitudeDeg = 22.0f;   // Maksymalne wychylenie koncowki ogona (najwieksza sila napedowa)
    float wavePhaseLag = 2.4f;        // Opóźnienie fazy fali miedzy segmentami kregoslupa (iluzja "plynnego" wyginania)

    // Parametry reakcji na krzywizne splajnu (zakrety)
    float turnYawGainDeg = 14.0f;     // Wzmocnienie wygiecia calego ciala w strone skretu (lewo/prawo)
    float turnRollGainDeg = 6.0f;     // Przechyl na boki (beczka) w trakcie ostrego zakretu

    float finCounterRollDeg = 4.0f;   // Niewielki ruch pletw bocznych stabilizujacy cialo
    float responseHz = 10.0f;         // Tlumienie (wygladzanie) animacji, zapobiega naglym przeskokom klatek

    // Zdefiniowane lokalne osie obrotu z programu 3D (Maya/Blender) dla kosci
    glm::vec3 localYawAxis{ 0.0f, 1.0f, 0.0f };
    glm::vec3 localRollAxis{ 0.0f, 0.0f, 1.0f };
};

// Klasa proceduralnego animatora. Zamiast sztywno odtwarzac klatki kluczowe z pliku, 
// samodzielnie przelicza matematyczna fale i naklada ja na kosci modelu.
class SharkSkeletonAnimator {
public:
    explicit SharkSkeletonAnimator(SharkSwimSettings settings = {});

    // Przeszukuje zmapowane kosci z zaladowanego Modelu i zgaduje ktore z nich to kregoslup a ktore to pletwy
    void InitializeFromModel(const Model& model);

    // Glowna funkcja odpalana co klatke - wylicza sinusy fali i dodaje do modelu lokalne obroty kosci
    void ApplyPose(Model& model, float animationTimeSeconds, float signedCurvature);

    // Narzedzia do debugowania pokazujace na ekranie znaleziona hierarchie i osie
    [[nodiscard]] const std::vector<std::string>& GetDebugLines() const;
    [[nodiscard]] const std::vector<std::string>& GetAnimatedBones() const;
    [[nodiscard]] const std::vector<float>& GetLastAppliedAnglesDegrees() const;

private:
    SharkSwimSettings settings;
    std::vector<std::string> bodyBones; // Przechowuje nazwy kosci ciagu glownego (Kregoslup + Ogon)
    std::vector<std::string> finBones;  // Przechowuje nazwy wolnych pletw
    std::vector<std::string> animatedBones;

    // Pamieta poprzedni stan obrotu do interpolacji (zapobiega skakaniu animacji)
    std::vector<float> lastAppliedAnglesDegrees;
    std::vector<float> lastAppliedRollDegrees;

    std::vector<std::string> debugLines;
    float lastAnimationTimeSeconds = 0.0f;
    bool hasLastAnimationTime = false;
};