#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 vertex_normal;

uniform mat4 model_matrix;
uniform mat4 view_projection_matrix;
uniform mat4 lightSpaceMatrix;

out vec2 TexCoord;
out vec3 frag_normal;
out vec3 frag_position;
out vec4 fragPosLightSpace;


void main() {
    fragPosLightSpace = lightSpaceMatrix * model_matrix * vec4(vertex_position, 1.0);
    gl_Position = view_projection_matrix * model_matrix * vec4(vertex_position, 1.0);
    
    frag_normal = mat3(transpose(inverse(model_matrix))) * vertex_normal;
    frag_position = vec3(model_matrix * vec4(vertex_position, 1.0));
    gl_Position = view_projection_matrix * model_matrix * vec4(vertex_position, 1.0);
    TexCoord = aTexCoord;
}