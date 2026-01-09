#version 460
#define MAX_LIGHTS 10

in vec2 TexCoord;
in vec3 frag_normal;
in vec3 frag_position;
in mat3 TBN;


out vec4 out_color;

uniform vec3 light_directions[MAX_LIGHTS]; 
uniform vec3 light_color;     
uniform vec3 ambient_color;   
uniform vec3 solidColor = vec3(0.5,0.5,0.5); 
uniform sampler2D ourTexture;
uniform sampler2D normalMap;

uniform int lightCount;
uniform sampler2D shadowMaps[MAX_LIGHTS];
uniform mat4 lightSpaceMatrices[MAX_LIGHTS];

uniform bool isShaded;
uniform bool UseNormalMap;
uniform bool UseSolidColor;


float ShadowCalculation(int lightIndex, vec3 normal)
{
    vec4 fragPosLightSpace =
        lightSpaceMatrices[lightIndex] * vec4(frag_position, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    //float bias = max(0.0005 * (1.0 - dot(normal, normalize(-light_directions[lightIndex]))), 0.0005);
    float bias = 0.00005;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMaps[lightIndex], 0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth =
                texture(shadowMaps[lightIndex],
                        projCoords.xy + vec2(x, y) * texelSize).r;

            shadow += (projCoords.z - bias > pcfDepth ? 1.0 : 0.0);
        }
    }

    return shadow / 9.0;
}

void main() {
    

    vec3 normal = texture(normalMap, TexCoord).xyz;
    
    normal = normal * 2.0 - 1.0;
    normal = -normalize(TBN * normal);

    vec3 N = UseNormalMap ? normalize(normal) : normalize(frag_normal) ;


    
   
    


    vec3 tex = texture(ourTexture, TexCoord).rgb;
    if (UseSolidColor) tex = solidColor;

    vec3 lighting = light_color * tex;
    for (int i = 0; i <lightCount;i++)
    {
        vec3 L = normalize(-light_directions[i]);        
        float diff =isShaded ? max(dot(N, L), 0.0) : 1.f ;
   
        float shadow = isShaded ? ShadowCalculation(i,N) : 0.0f;
        lighting += (1.0 - shadow) * diff * light_color * tex;
   }

    out_color = vec4(lighting, 1.0);
}