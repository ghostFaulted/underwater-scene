    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 5) in ivec4 aBoneIDs;
    layout (location = 6) in vec4 aBoneWeights;

    const int MAX_BONES = 100;

    uniform mat4 lightSpaceMatrix;
    uniform mat4 model;
    uniform bool uUseBoneSkinning;
    uniform mat4 uBoneTransforms[MAX_BONES];
    uniform mat4 uNodeTransform = mat4(1.0);
    uniform int uMeshMaterialKind = 0;

    mat4 BoneMatrix(int index) {
        int clamped = clamp(index, 0, MAX_BONES - 1);
        return uBoneTransforms[clamped];
    }

    mat4 EvaluateSkinningMatrix() {
        if (!uUseBoneSkinning || uMeshMaterialKind != 0) {
            return uNodeTransform;
        }

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
        gl_Position = lightSpaceMatrix * model * vec4(skinnedPos, 1.0);
    }