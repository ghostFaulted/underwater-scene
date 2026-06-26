#ifndef UNDERWATERSCENE_SPLINEPATH_H
#define UNDERWATERSCENE_SPLINEPATH_H
#include <cstddef>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Klasa wykorzystujaca wielomiany Catmull-Rom z implementacja metody PTF (Parallel Transport Frames).
// Zapobiega nienaturalnym "skretom" tzw. 'Gimbal Lock/Twisting' modeli poruszajacych sie wzdluz trajektorii trasy 3D.
class SplinePath {
    // Struktura wezla lokalnego (Klatka ukladu wspolrzednych)
    struct Frame {
        float t;               // Znormalizowany czas odciecia punktu (0..1)
        glm::vec3 position;    // Punkt na trasie
        glm::vec3 tangent;     // Tangens = Wektor przodu (przyspieszenia), gdzie patrzy splajn
        glm::vec3 normal;      // Normalna = Gora modelu
        glm::vec3 binormal;    // Binormalna = Strona obiektu (skrzydlo) wyznaczona z loczynu wektorowego T i N
    };

    std::vector<glm::vec3> controlPoints;  // Punkty podane w konstruktorze modelujace pętle
    std::vector<Frame> frames;             // Ekstrakcja wielu mikro-klatek stabilizujacych PTF
    float sampleStep = 0.0f;
    bool closedLoop = false;               // Czy algorytm ma automatycznie zaplatac poczatek i koniec z soba

    // Algorytmy podstawowe wielomianow 3-ciego stopnia (Pozycja)
    [[nodiscard]] static glm::vec3 CatmullRom(glm::vec3 p0, glm::vec3 p1,
        glm::vec3 p2, glm::vec3 p3, float t);

    // Pierwsza pochodna funkcji krzywej - daje nam tangens w tym punkcie
    [[nodiscard]] static glm::vec3 CatmullRomTangent(glm::vec3 p0, glm::vec3 p1,
        glm::vec3 p2, glm::vec3 p3, float t);

    // Druga pochodna wielomianu krzywej - potrzebna w matematyce by znalezc sile promienia okregu, by wyliczyc jej skret(krzywizne)
    [[nodiscard]] static glm::vec3 CatmullRomSecondDerivative(glm::vec3 p0, glm::vec3 p1,
        glm::vec3 p2, glm::vec3 p3, float t);

    [[nodiscard]] std::size_t GetSegmentCount() const;

    [[nodiscard]] glm::vec3 SamplePositionOnSegment(std::size_t segmentIndex, float localT) const;
    [[nodiscard]] glm::vec3 SampleTangentOnSegment(std::size_t segmentIndex, float localT) const;

    // Najwazniejsza funkcja zliczajaca Parallel Transport Frames minimalizujac Twisting
    void BuildFrames();

    // Sprawdzanie ortogonalnosci w trybie developerskim
    void ValidateFrames() const;

public:
    explicit SplinePath(const std::vector<glm::vec3>& controlPoints, bool closedLoop = false);

    [[nodiscard]] glm::vec3 GetPosition(float t) const;
    [[nodiscard]] glm::vec3 GetTangent(float t) const;

    // Zwraca krzywizne promieniowa, bardzo wazny odczyt pozwalajacy animowac kregoslup rekina (wiedza czy rekin wlasnie nawraca na trasie)
    [[nodiscard]] float GetSignedCurvature(float t) const;

    // Zwraca w pelni sformulowana macierz translacyjna wraz z poprawnym obrotem obiektu wokol wlasnej osi
    [[nodiscard]] glm::mat4 GetTransform(float t) const;
};

#endif //UNDERWATERSCENE_SPLINEPATH_H