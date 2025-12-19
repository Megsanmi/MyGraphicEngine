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
#include "../Renderer/ShaderProgram.hpp"
class Mesh {
public:
    // Mesh-данные
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    
    

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void SetUV(float u0, float v0, float u1, float v1);
    void Draw(Renderer::ShaderProgram& shader);
    void setupMesh();
    void UpdateVBO();


private:
    // Данные для рендеринга
    unsigned int VAO, VBO, EBO;
};

class Model
{
public:
    Model(std::string path)
    {
        loadModel(path);
    }

    Model(std::vector<Vertex> vertices,
        std::vector<unsigned int> indices,
        std::vector<Texture> textures)
    {
        meshes.emplace_back(vertices, indices, textures);
    }

    void SetUV(float u0, float v0, float u1, float v1);
    void Draw(Renderer::ShaderProgram& shader);

    std::string name;
    glm::vec3 position{ 0,0,0 };
    glm::vec3 scale{1,1,1};
    glm::vec3 rotationAxis{0,1,0};
    float rotationAngle = 0;

    std::vector<Mesh> meshes;

    unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);
private:
    
    // Данные модели
    
    std::string directory = "Assets";
    
    

    

    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
};

