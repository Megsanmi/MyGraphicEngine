#pragma once 

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
 
    glm::ivec4 boneID {0,0,0,0};
    glm::vec4 weights { 0,0,0,0 };
};