#version 330 core
layout (location = 0) in vec3 aPos;

const int MAX_SPLINE_BONES = 16;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
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
    vec3 skinnedPos = vec3(EvaluateSplineSkinMatrix() * vec4(aPos, 1.0));
    gl_Position = lightSpaceMatrix * model * vec4(skinnedPos, 1.0);
}