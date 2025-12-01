#pragma once 

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    /*
    Vertex(glm::vec3 pos, glm::vec2 Text, glm::vec3 norm) {
        Position = pos;
        Normal = norm;
        TexCoords = Text;
    }
    */
};