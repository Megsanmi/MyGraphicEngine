#pragma once
#include <string>
#include <assimp/scene.h>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>


#include <glad/glad.h>
#include "Vertex.hpp"
#include "Texture.hpp"
#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#include "../Renderer/ShaderProgram.h"
class Mesh {
public:
    // Mesh-данные
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    
    

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void Draw(Renderer::ShaderProgram& shader);
private:
    // Данные для рендеринга
    unsigned int VAO, VBO, EBO;

    void setupMesh();
};

class Model
{
public:
    Model(std::string path)
    {
        loadModel(path);
    }
    void Draw(Renderer::ShaderProgram& shader);

    std::string name;
    glm::vec3 position{ 0,0,0 };
    glm::vec3 scale{1,1,1};
    glm::vec3 rotationAxis{0,1,0};
    float rotationAngle = 0;

private:
    
    // Данные модели
    std::vector<Mesh> meshes;
    std::string directory = "Assets";
    
    

    unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);

    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

