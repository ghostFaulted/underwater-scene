#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;   // <-- НОВОЕ
layout (location = 4) in vec3 aBitangent; // <-- НОВОЕ

out vec2 TexCoords;
out vec3 WorldPos;
out mat3 TBN; 
out vec4 FragPosLightSpace;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    TBN = mat3(T, B, N);

    FragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0); 
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}