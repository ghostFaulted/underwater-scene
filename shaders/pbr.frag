#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in mat3 TBN; 
in vec4 FragPosSunSpace;
in vec4 FragPosSpotSpace;

uniform sampler2D normalMap; 
uniform bool useNormalMap;  
uniform sampler2D detailNormalMap;
uniform bool useDetailNormalMap;
uniform float detailNormalStrength;

uniform sampler2D albedoMap; 
uniform bool useAlbedoMap;
uniform sampler2D albedoMapEye;
uniform sampler2D albedoMapTeeth;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// Параметры для специальных материалов
uniform vec3 teethColor = vec3(0.95, 0.95, 0.95);  // Белые зубы
uniform float teethMetallic = 0.05;
uniform float teethRoughness = 0.3;

uniform vec3 camPos;

uniform sampler2D sunShadowMap;
uniform sampler2D spotShadowMap;
uniform sampler2D normalMapEye;
uniform sampler2D normalMapTeeth;
uniform int uMeshMaterialKind = 0;
uniform bool uDebugMaterialKindView = false;
uniform bool uDebugRawAlbedoView = false;
uniform bool uDebugSpotOnlyView = false;

// Spotlight uniforms (set from C++ RenderPbrScene)
uniform vec3 spotPosition;
uniform vec3 spotDirection;
uniform vec3 spotColor;
uniform float spotIntensity;
uniform float innerCutoff;
uniform float outerCutoff;
uniform bool spotEnabled;

uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;
uniform float dirLightIntensity;

uniform vec3 waterColor;
uniform float fogDensity;
uniform bool uDebugSpotShadowCompareView;
uniform bool uDebugSpotCenterProbeView;

const float PI = 3.14159265359;

vec3 BlendRNM(vec3 baseN, vec3 detailN) {
    vec3 n1 = normalize(baseN);
    vec3 n2 = normalize(detailN);
    vec3 t = vec3(n1.xy + n2.xy, n1.z * n2.z);
    return normalize(t);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(sampler2D map, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    if (fragPosLightSpace.w <= 0.0) return 0.0;
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(map, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(map, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;
    return shadow;
}

void main() {       
    vec3 currentAlbedo = albedo;
    float currentMetallic = metallic;
    float currentRoughness = roughness;
    vec3 sampledAlbedo = vec3(1.0);

    if (useAlbedoMap) {
        if (uMeshMaterialKind == 1) {
            // Глаза
            sampledAlbedo = texture(albedoMapEye, TexCoords).rgb;
            currentAlbedo = sampledAlbedo;
            currentAlbedo = pow(currentAlbedo, vec3(2.2));
            currentAlbedo *= albedo;
        } else if (uMeshMaterialKind == 2) {
            // Зубы: используем белый цвет
            sampledAlbedo = texture(albedoMapTeeth, TexCoords).rgb;
            currentAlbedo = sampledAlbedo;
            currentAlbedo = pow(currentAlbedo, vec3(2.2));
            currentAlbedo *= teethColor;  // Переопределяем цвет на зубной
            currentMetallic = teethMetallic;
            currentRoughness = teethRoughness;
        } else {
            // Тело
            sampledAlbedo = texture(albedoMap, TexCoords).rgb;
            currentAlbedo = sampledAlbedo;
            currentAlbedo = pow(currentAlbedo, vec3(2.2));
            currentAlbedo *= albedo;
        }
    }

    if (uDebugMaterialKindView) {
        vec3 debugColor = vec3(0.8, 0.8, 0.8);
        if (uMeshMaterialKind == 0) {
            debugColor = vec3(0.1, 0.5, 1.0); // Body
        } else if (uMeshMaterialKind == 1) {
            debugColor = vec3(0.1, 1.0, 0.2); // Eyes
        } else if (uMeshMaterialKind == 2) {
            debugColor = vec3(1.0, 0.2, 0.2); // Teeth
        }
        FragColor = vec4(debugColor, 1.0);
        return;
    }

    if (uDebugRawAlbedoView) {
        FragColor = vec4(sampledAlbedo, 1.0);
        return;
    }

    vec3 N;
    if (useNormalMap) {
        if (uMeshMaterialKind == 1) {
            N = texture(normalMapEye, TexCoords).rgb;
        } else if (uMeshMaterialKind == 2) {
            N = texture(normalMapTeeth, TexCoords).rgb;
        } else {
            N = texture(normalMap, TexCoords).rgb;
        }
        N = normalize(N * 2.0 - 1.0);
        // DirectX/OpenGL flip for DDS normal maps (Y-up vs Y-down convention)
        N.y = -N.y;

        if (useDetailNormalMap) {
            vec3 detailN = texture(detailNormalMap, TexCoords * 4.0).rgb;
            detailN = normalize(detailN * 2.0 - 1.0);
            detailN.y = -detailN.y;
            detailN.xy *= detailNormalStrength;
            detailN.z = sqrt(max(0.0001, 1.0 - dot(detailN.xy, detailN.xy)));
            N = BlendRNM(N, detailN);
        }

        N = normalize(TBN * N);
    } else {
        N = normalize(TBN[2]); 
    }

    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, currentAlbedo, currentMetallic);

    vec3 Lo = vec3(0.0);
    vec3 spotOnlyLo = vec3(0.0);

    vec3 L_dir = normalize(-dirLightDirection);
    float sunShadow = ShadowCalculation(sunShadowMap, FragPosSunSpace, N, L_dir);
    float spotShadow = 0.0;
    float spotIntensityFactor = 0.0;

    vec3 H_dir = normalize(V + L_dir);
    vec3 radiance_dir = dirLightColor * dirLightIntensity;

    float NDF_dir = DistributionGGX(N, H_dir, roughness);   
    float G_dir   = GeometrySmith(N, V, L_dir, roughness);      
    vec3 F_dir    = fresnelSchlick(max(dot(H_dir, V), 0.0), F0);       
    
    vec3 nom_dir    = NDF_dir * G_dir * F_dir; 
    float denom_dir = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L_dir), 0.0) + 0.001; 
    vec3 spec_dir = nom_dir / denom_dir;
    
    vec3 kS_dir = F_dir;
    vec3 kD_dir = vec3(1.0) - kS_dir;
    kD_dir *= 1.0 - metallic;     

    float NdotL_dir = max(dot(N, L_dir), 0.0);
    
    Lo += (1.0 - sunShadow) * (kD_dir * currentAlbedo / PI + spec_dir) * radiance_dir * NdotL_dir;

    if (spotEnabled) {
        // Spotlight (camera flashlight) contribution
        vec3 L_spot = normalize(spotPosition - WorldPos);
        vec3 H_spot = normalize(V + L_spot);
        float dist_spot = length(spotPosition - WorldPos);
        float attenuation_spot = 1.0 / (dist_spot * dist_spot);
        vec3 radiance_spot = spotColor * spotIntensity * attenuation_spot;

        float NDF_spot = DistributionGGX(N, H_spot, currentRoughness);
        float G_spot = GeometrySmith(N, V, L_spot, currentRoughness);
        vec3 F_spot = fresnelSchlick(max(dot(H_spot, V), 0.0), F0);

        vec3 nom_spot = NDF_spot * G_spot * F_spot;
        float denom_spot = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L_spot), 0.0) + 0.001;
        vec3 spec_spot = nom_spot / denom_spot;

        vec3 kS_spot = F_spot;
        vec3 kD_spot = vec3(1.0) - kS_spot;
        kD_spot *= 1.0 - currentMetallic;

        float NdotL_spot = max(dot(N, L_spot), 0.0);

        // spotlight smooth edge
        float theta = dot(normalize(-L_spot), normalize(spotDirection));
        float epsilon = innerCutoff - outerCutoff;
        float intensity = clamp((theta - outerCutoff) / max(epsilon, 0.0001), 0.0, 1.0);

        spotShadow = ShadowCalculation(spotShadowMap, FragPosSpotSpace, N, L_spot);
        spotIntensityFactor = intensity;

        vec3 spotContribution = (1.0 - spotShadow) * intensity * (kD_spot * currentAlbedo / PI + spec_spot) * radiance_spot * NdotL_spot;
        Lo += spotContribution;
        spotOnlyLo += spotContribution;
    }

    if (uDebugSpotShadowCompareView) {
        FragColor = vec4(
            clamp(1.0 - sunShadow, 0.0, 1.0),
            clamp(1.0 - spotShadow, 0.0, 1.0),
            clamp(spotIntensityFactor, 0.0, 1.0),
            1.0
        );
        return;
    }

    if (uDebugSpotCenterProbeView)
    {
        vec3 p = FragPosSpotSpace.xyz / FragPosSpotSpace.w;
        p = p * 0.5 + 0.5;

        float depth =
            texture(
                spotShadowMap,
                p.xy).r;

        FragColor =
            vec4(vec3(depth),1.0);

        return;
    }

    if (uDebugSpotOnlyView) {
        vec3 spotDebug = spotOnlyLo / (spotOnlyLo + vec3(1.0));
        spotDebug = pow(spotDebug, vec3(1.0 / 2.2));
        FragColor = vec4(spotDebug, 1.0);
        return;
    }

    vec3 ambient = vec3(0.03) * currentAlbedo * ao;
    vec3 color = ambient + Lo;

    float cameraDist = length(camPos - WorldPos);
    float fogFactor = exp(-pow((cameraDist * fogDensity), 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    color = mix(waterColor, color, fogFactor);

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}