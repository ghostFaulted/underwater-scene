#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

const int MAX_SPLINE_BONES = 16;

out vec2 TexCoords;
out vec3 WorldPos;
out mat3 TBN; 
out vec4 FragPosLightSpace;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform bool uEnableSplineSkinning;
uniform int uSplineBoneCount;
uniform float uSplineBodyMin;
uniform float uSplineBodyMax;
uniform mat4 uSplineBones[MAX_SPLINE_BONES];

mat4 EvaluateSplineSkinMatrix() {
    if (!uEnableSplineSkinning || uSplineBoneCount < 2) {
        return mat4(1.0);
    }

    float span = max(0.0001, uSplineBodyMax - uSplineBodyMin);
    float normalized = clamp((aPos.z - uSplineBodyMin) / span, 0.0, 1.0);
    float mapped = normalized * float(uSplineBoneCount - 1);

    int leftIndex = int(floor(mapped));
    int rightIndex = min(leftIndex + 1, uSplineBoneCount - 1);
    float alpha = mapped - float(leftIndex);

    return mix(uSplineBones[leftIndex], uSplineBones[rightIndex], alpha);
}

void main()
{
    mat4 skin = EvaluateSplineSkinMatrix();
    vec3 skinnedPos = vec3(skin * vec4(aPos, 1.0));
    vec3 skinnedTangent = normalize(mat3(skin) * aTangent);
    vec3 skinnedBitangent = normalize(mat3(skin) * aBitangent);
    vec3 skinnedNormal = normalize(mat3(skin) * aNormal);

    TexCoords = aTexCoords;
    WorldPos = vec3(model * vec4(skinnedPos, 1.0));

    vec3 T = normalize(mat3(model) * skinnedTangent);
    vec3 B = normalize(mat3(model) * skinnedBitangent);
    vec3 N = normalize(mat3(model) * skinnedNormal);
    TBN = mat3(T, B, N);

    FragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0); 
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}