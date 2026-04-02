#version 460
#define MAX_LIGHTS 10

in vec2 TexCoord;
in vec3 frag_normal;
in vec3 frag_position;
in mat3 TBN;

out vec4 out_color;

// --- LIGHTS ---
uniform vec3 light_directions[MAX_LIGHTS]; 
uniform vec3 light_color = vec3(1.0);
uniform vec3 ambient_color = vec3(0.1);
uniform int lightCount;

// --- CAMERA ---
uniform vec3 cameraPos;

// --- MATERIAL ---
uniform sampler2D diffuseTexture;
uniform sampler2D normalMap;
uniform sampler2D specularMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;

uniform bool UseNormalMap;
uniform bool hasAlbedoMap;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;

uniform bool UseSolidColor;
uniform vec3 solidColor = vec3(0.5);
uniform float matMetallic = 0.0;
uniform float matRoughness = 1;
uniform float matAo = 1;
uniform float Alpha ;

// --- SHADOWS ---
layout(binding = 20) uniform sampler2D shadowMaps[MAX_LIGHTS];
uniform mat4 lightSpaceMatrices[MAX_LIGHTS];
uniform bool isShaded;

uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;

// --- FUNCTIONS ---
float ShadowCalculation(int lightIndex, vec3 N)
{
    vec4 fragPosLightSpace = lightSpaceMatrices[lightIndex] * vec4(frag_position, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float bias = max(0.00005 * (1.0 - dot(N, normalize(light_directions[lightIndex]))), 0.00005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMaps[lightIndex], 0);

    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(shadowMaps[lightIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth ? 1.0 : 0.0);
        }
    return shadow / 9.0;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (3.141592 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

// --- MAIN ---
void main()
{
    vec3 albedo = solidColor;
    float metallic = matMetallic;
    float roughness = matRoughness;

    if(hasAlbedoMap)
    {
        vec4 tex = texture(diffuseTexture, TexCoord);
        albedo = tex.rgb;
        if(tex.a < 0.1) discard;
    }
   

    if(hasMetallicMap)
    {
        metallic = texture(metallicMap, TexCoord).r;
    }

    if(hasRoughnessMap)
    {
        roughness = texture(roughnessMap, TexCoord).r;
    }

    vec4 tex = texture(diffuseTexture, TexCoord);
    albedo = tex.rgb;
    float alpha = tex.w;
    
    
     if(UseSolidColor)
     {
        vec4 tex = vec4(solidColor, alpha);
        albedo = tex.rgb;
        if(tex.a < 0.1) discard;
     }
    

    vec3 N = normalize(frag_normal);
    if (UseNormalMap)
    {
        vec3 n = texture(normalMap, TexCoord).xyz * 2.0 - 1.0;
        N = normalize(TBN * n);
    }

    vec3 V = normalize(cameraPos - frag_position);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    if (UseSolidColor)
    {
        albedo = solidColor.rgb;
    }

    vec3 Lo = ambient_color * albedo; // ambient

    for (int i = 0; i < lightCount; i++)
    {
        vec3 L = normalize(-light_directions[i]); // если свет = направление к камере, убрать минус
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float shadow = isShaded ? ShadowCalculation(i, N) : 0.0;

        // PBR BRDF
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = D * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
        vec3 specular = numerator / denom;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        vec3 diffuse = kD * albedo / 3.141592;

        vec3 color = (diffuse + specular) * light_color * NdotL;
        Lo += (1.0 - shadow) * color*2;
    }

    vec3 color = Lo;
    
    float dist = length(cameraPos - frag_position);

    float fogFactor = (fogFar - dist) / (fogFar - fogNear);
    fogFactor = clamp(fogFactor, 0.0, 1.0);


    if(isShaded)
        color = pow(color, vec3(1.0/0.8));
    else 
        color = albedo;
    color = mix(fogColor,color,fogFactor);
    
    //color =  texture(roughnessMap, TexCoord).rgb;
    
    out_color = vec4(color, alpha);
}