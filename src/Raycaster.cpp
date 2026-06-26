#include "Raycaster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// Implementacja odwrotnosci rzutowania Pipeline'u z technologii OpenGL
Ray Raycaster::ScreenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4& view, const glm::mat4& projection) {
    // Krok 1. Przeksztalcenie do przestrzeni ekranu NDC (Normalized Device Coordinates) [-1, 1]
    float x = (2.0f * static_cast<float>(mouseX)) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight; // W OpenGL Os Y skierowana jest zazwyczaj ku gorze
    float z = 1.0f; // Wektor wglab ekranu
    glm::vec3 ray_nds = glm::vec3(x, y, z);

    // Krok 2. Homogeniczne zmienne Clip-Space. Dodanie czynnika podzialu 'w'.
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

    // Krok 3. Przemnozenie przez macierz odwrotna rzutowania uzyskujac uklad lokalny patrzenia dla kamery (Eye Space)
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0); // Anulowanie wspolrzednej glebi W

    // Krok 4. Ostateczna macierz odwrotna widoku aby sprowadzic promnien do globalnej skali (World Space)
    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor); // Ustandaryzowanie do 1.0 jednostki.

    // Miejsce startowe promienia (Origin) to obecna zdekodowana pozycja samej kamery (Czwarta kolumna w macierzy Inverse View)
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 ray_origin = glm::vec3(invView[3]);

    return { ray_origin, ray_wor };
}

// Metoda przeciecia Slab polegajaca na sprawdzeniu kiedy Ray przebija sie przez plaszczyzny wyznaczajace boki prostopadloscianu AABB
bool Raycaster::IntersectAABB(const Ray& ray, const AABB& box, float& t) {
    // Odwrocony kierunek optymalizuje pozniejsze dzielenie (robimy szybsze mnozenie przez ulamki)
    glm::vec3 invDir = 1.0f / ray.direction;

    // Najpierw t0 przechodzi przez minimalne krance, zas t1 przez maksymalne
    glm::vec3 t0 = (box.min - ray.origin) * invDir;
    glm::vec3 t1 = (box.max - ray.origin) * invDir;

    // Znajdowanie najblizszego i najdalszego przebicia z kazdej osi (X,Y,Z)
    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    // Szukanie globalnie najblizszego punktu trafienia calej bryly
    float tmin_max = std::max(std::max(tmin.x, tmin.y), tmin.z);
    float tmax_min = std::min(std::min(tmax.x, tmax.y), tmax.z);

    // Promien trafil poprawnie o ile minimalna odleglosc wyjscia jest wieksza badz rowna odleglosci wejscia 
    // oraz cel znajduje sie przed lufa promienia (>=0.0) a nie poza nim.
    if (tmax_min >= tmin_max && tmax_min >= 0.0f) {
        t = tmin_max > 0.0f ? tmin_max : tmax_min;
        return true;
    }
    return false;
}