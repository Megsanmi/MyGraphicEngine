#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 model_matrix;
uniform mat4 view_projection_matrix;
uniform mat4 lightSpaceMatrix;

out vec2 TexCoord;
out vec3 frag_normal;
out vec3 frag_position;
out vec4 fragPosLightSpace;
out mat3 TBN;


void main() {
    fragPosLightSpace = lightSpaceMatrix * model_matrix * vec4(vertex_position, 1.0);
    gl_Position = view_projection_matrix * model_matrix * vec4(vertex_position, 1.0);
    
    vec3 T = normalize(vec3(model_matrix * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(model_matrix * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model_matrix * vec4(aNormal, 0.0)));

    TBN = mat3(T,B,N);

    frag_normal = mat3(transpose(inverse(model_matrix))) * aNormal;
    frag_position = vec3(model_matrix * vec4(vertex_position, 1.0));

    TexCoord = aTexCoord;
}