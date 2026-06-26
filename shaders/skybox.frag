#version 330 core
out vec4 FragColor;

// Wektor trojwymiarowy pelniacy role adresu UV wewnatrz sferycznej mapy szescianu (Cubemap)
in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    // Bardzo proste pobranie punktu z odpowiedniej scianki szescianu w oparciu o zinterpolowany wektor od srodka sceny
    FragColor = texture(skybox, TexCoords);
}