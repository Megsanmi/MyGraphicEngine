#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;

uniform mat4 model_matrix;
uniform mat4 mesh_matrix;
uniform mat4 bones[100];
uniform mat4 view_projection_matrix;
uniform bool useSkinning = false;

out vec2 TexCoord;
out vec3 frag_position;
out mat3 TBN;
out vec3 frag_normal;

void main() {

    vec4 finalPos;
    vec3 finalNormal;
    vec3 finalTangent;
    vec3 finalBitangent;

    if(useSkinning) {
        // --- Скиннинг для анимированных объектов ---
        finalPos = vec4(0.0);
        finalNormal = vec3(0.0);
        finalTangent = vec3(0.0);
        finalBitangent = vec3(0.0);

        for(int i = 0; i < 4; i++) {
            int id = aBoneIDs[i];
            float w = aWeights[i];
            mat4 boneMat = bones[id];

            finalPos += boneMat * vec4(vertex_position, 1.0) * w;
            finalNormal += mat3(boneMat) * aNormal * w;
            finalTangent += mat3(boneMat) * aTangent * w;
            finalBitangent += mat3(boneMat) * aBitangent * w;
        }
    } else {
        // --- Обычные объекты ---
        finalPos = vec4(vertex_position, 1.0);
        finalNormal = aNormal;
        finalTangent = aTangent;
        finalBitangent = aBitangent;
    }

    mat4 modelMesh = model_matrix * mesh_matrix;
    frag_position = vec3(modelMesh * finalPos);

    mat3 normalMatrix = transpose(inverse(mat3(modelMesh)));
    frag_normal = normalize(normalMatrix * finalNormal);

    vec3 T = normalize(normalMatrix * finalTangent);
    vec3 B = normalize(normalMatrix * finalBitangent);
    vec3 N = frag_normal;
    TBN = mat3(T, B, N);

    TexCoord = aTexCoord;
    gl_Position = view_projection_matrix * modelMesh * finalPos;
}