#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;

uniform mat4 model_matrix;
uniform mat4 bones[100];
uniform mat4 view_projection_matrix;

uniform bool useSkinning = false;

out vec2 TexCoord;
out vec3 frag_normal;
out vec3 frag_position;
out mat3 TBN;

out vec3 color;

void main() {
    

    mat4 boneTransform = mat4(1.f);

    if(useSkinning) {
        boneTransform = 
            bones[aBoneIDs.x] * aWeights.x +
            bones[aBoneIDs.y] * aWeights.y +
            bones[aBoneIDs.z] * aWeights.z +
            bones[aBoneIDs.w] * aWeights.w;
    }
  
    mat4 NormalMatrix = inverse(transpose(model_matrix));

    vec3 T = normalize(vec3(NormalMatrix * vec4(aTangent, 0.0)));
    vec3 N = normalize(vec3(NormalMatrix * vec4(aNormal, 0.0)));

    vec3 B = normalize(cross(N, T) * (dot(cross(N,T), aBitangent) < 0.0 ? -1.0 : 1.0)); 

    TBN = mat3(T,B,N);
    
    frag_normal = mat3(transpose(inverse(model_matrix))) * aNormal;

    color = vec3(aWeights.x,aWeights.y,aWeights.z);

    
    frag_position = vec3(model_matrix * boneTransform * vec4(vertex_position, 1.0));
    TBN = TBN * mat3(boneTransform);

    vec4 localPos = boneTransform * vec4(vertex_position, 1.0);
    
    
    gl_Position = view_projection_matrix * model_matrix * localPos;
    TexCoord = aTexCoord;
}