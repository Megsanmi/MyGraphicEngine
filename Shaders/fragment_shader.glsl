#version 460

in vec2 TexCoord;
in vec3 frag_normal;
in vec3 frag_position;
in vec4 fragPosLightSpace;
in mat3 TBN;


out vec4 out_color;

uniform vec3 light_direction; 
uniform vec3 light_color;     
uniform vec3 ambient_color;   
uniform sampler2D ourTexture;
uniform sampler2D shadowMap;
uniform sampler2D normalMap;


uniform bool isShaded;
uniform bool UseNormalMap;
uniform bool UseSolidColor;


float ShadowCalculation(vec4 fragPosLight)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 0.0;

    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth ? 1.0 : 0.0);
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main() {
    

    vec3 normal = texture(normalMap, TexCoord).xyz;
    normal.y = normal.y;
    normal = normal * 2.0 - 1.0;
    normal = -normalize(TBN * normal);

    vec3 N = UseNormalMap ? normalize(normal) : normalize(frag_normal) ;


    vec3 L = normalize(-light_direction);
  
    float diff =max(dot(N, L), 0.0) ;
    
    float shadow = isShaded ? ShadowCalculation(fragPosLightSpace) : 0.0f;

    vec3 tex = texture(ourTexture, TexCoord).rgb;
    if (UseSolidColor) tex = vec3(0.5,0.5,0.5);

    vec3 lighting = (ambient_color + (1.0 - shadow) * diff * light_color) * tex;
          

    out_color = vec4(lighting, 1.0);
}