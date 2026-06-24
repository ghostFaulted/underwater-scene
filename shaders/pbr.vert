#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

const int MAX_BONES = 100;

out vec2 TexCoords;
out vec3 WorldPos;
out mat3 TBN; 
out vec4 FragPosSunSpace;
out vec4 FragPosSpotSpace;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 sunLightMatrix;
uniform mat4 spotLightMatrix;
uniform bool uUseBoneSkinning;
uniform mat4 uBoneTransforms[MAX_BONES];
// For meshes without bone weights (eyes, teeth), apply node transform.
uniform mat4 uNodeTransform = mat4(1.0);
uniform int uMeshMaterialKind = 0;

mat4 BoneMatrix(int index) {
    int clamped = clamp(index, 0, MAX_BONES - 1);
    return uBoneTransforms[clamped];
}

mat4 EvaluateSkinningMatrix() {
    // If skinning disabled globally or this mesh is not a skinned body mesh,
    // fall back to the node transform (eyes, teeth, other non-skinned objects).
    if (!uUseBoneSkinning || uMeshMaterialKind != 0) {
        return uNodeTransform;
    }

    // If there are no bone weights, also use node transform as a fallback.
    float weightSum = aBoneWeights.x + aBoneWeights.y + aBoneWeights.z + aBoneWeights.w;
    if (weightSum <= 0.0001) {
        return uNodeTransform;
    }

    mat4 skin = mat4(0.0);
    skin += BoneMatrix(aBoneIDs.x) * aBoneWeights.x;
    skin += BoneMatrix(aBoneIDs.y) * aBoneWeights.y;
    skin += BoneMatrix(aBoneIDs.z) * aBoneWeights.z;
    skin += BoneMatrix(aBoneIDs.w) * aBoneWeights.w;


    return skin;
}

void main()
{
    mat4 skin = EvaluateSkinningMatrix();
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

    FragPosSunSpace = sunLightMatrix * vec4(WorldPos, 1.0);
    FragPosSpotSpace = spotLightMatrix * vec4(WorldPos, 1.0);
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}