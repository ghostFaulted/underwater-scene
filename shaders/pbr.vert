#version 330 core
// Atrybuty wierzcholka wgrywane z C++ (zgodne ze struktura Vertex z Mesh.h)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;     // Identyfikatory kosci majacych wplyw
layout (location = 6) in vec4 aBoneWeights;  // Wagi (sila) wplywu poszczegolnych kosci

const int MAX_BONES = 100;

// Zmienne wyjsciowe przekazywane do Fragment Shadera
out vec2 TexCoords;
out vec3 WorldPos;
out mat3 TBN; 
out vec4 FragPosSunSpace;   // Pozycja wierzcholka z perspektywy Slonca (do cieni)
out vec4 FragPosSpotSpace;  // Pozycja wierzcholka z perspektywy Reflektora (do cieni)

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 sunLightMatrix;
uniform mat4 spotLightMatrix;

// Zmienne sluzace do obslugi animacji szkieletowej (Skinning)
uniform bool uUseBoneSkinning;
uniform mat4 uBoneTransforms[MAX_BONES];
// Transformacja dla obiektow bez kosci (np. zeby, oczy)
uniform mat4 uNodeTransform = mat4(1.0);
uniform int uMeshMaterialKind = 0; // 0=Body, 1=Eye, 2=Teeth

// Pobranie macierzy kosci z zabezpieczeniem przed wyjsciem poza tablice
mat4 BoneMatrix(int index) {
    int clamped = clamp(index, 0, MAX_BONES - 1);
    return uBoneTransforms[clamped];
}

// Funkcja wyliczajaca ostateczna macierz deformacji wierzcholka (Skinning Matrix)
mat4 EvaluateSkinningMatrix() {
    // Jesli skinning jest wylaczony globalnie lub siatka nie jest cialem rekina (tylko np. zebami),
    // uzywamy sztywnej transformacji calego wezla.
    if (!uUseBoneSkinning || uMeshMaterialKind != 0) {
        return uNodeTransform;
    }

    // Jesli wierzcholek nie ma przypisanych zadnych wag, rowniez uzywamy transformacji wezla.
    float weightSum = aBoneWeights.x + aBoneWeights.y + aBoneWeights.z + aBoneWeights.w;
    if (weightSum <= 0.0001) {
        return uNodeTransform;
    }

    // Miksowanie macierzy kosci na podstawie ich wag (tzw. Linear Blend Skinning - LBS)
    mat4 skin = mat4(0.0);
    skin += BoneMatrix(aBoneIDs.x) * aBoneWeights.x;
    skin += BoneMatrix(aBoneIDs.y) * aBoneWeights.y;
    skin += BoneMatrix(aBoneIDs.z) * aBoneWeights.z;
    skin += BoneMatrix(aBoneIDs.w) * aBoneWeights.w;

    return skin;
}

void main()
{
    // 1. Aplikacja deformacji szkieletowej
    mat4 skin = EvaluateSkinningMatrix();
    vec3 skinnedPos = vec3(skin * vec4(aPos, 1.0));
    // Normalne i tangensy tez musza byc obrocone przez kosci
    vec3 skinnedTangent = normalize(mat3(skin) * aTangent);
    vec3 skinnedBitangent = normalize(mat3(skin) * aBitangent);
    vec3 skinnedNormal = normalize(mat3(skin) * aNormal);

    TexCoords = aTexCoords;
    // 2. Aplikacja przesuniecia i obrotu w swiecie
    WorldPos = vec3(model * vec4(skinnedPos, 1.0));

    // 3. Obliczanie macierzy TBN (Tangent-Bitangent-Normal) uzywanej potem 
    // we Fragment Shaderze do nakladania szczegolow z mapy normalnych.
    vec3 T = normalize(mat3(model) * skinnedTangent);
    vec3 B = normalize(mat3(model) * skinnedBitangent);
    vec3 N = normalize(mat3(model) * skinnedNormal);
    TBN = mat3(T, B, N);

    // 4. Transformacja pozycji do ukladu swiatel, aby pobrac pozniej piksel z mapy cieni
    FragPosSunSpace = sunLightMatrix * vec4(WorldPos, 1.0);
    FragPosSpotSpace = spotLightMatrix * vec4(WorldPos, 1.0);
    
    // 5. Ostateczna projekcja punktu na ekran kamery
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}