#include "Mesh.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ext/matrix_transform.hpp>



using namespace glm;

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
    

    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    setupMesh();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO) ;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        &indices[0], GL_STATIC_DRAW);

    // Координаты вершин
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Нормали вершин
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Текстурные координаты вершин
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // Bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    //Bones
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer( 5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneID));
    
    // Bone weights 
    glEnableVertexAttribArray(6);
    glVertexAttribPointer( 6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));
    
    glBindVertexArray(0);

}

void Mesh::Draw(Renderer::ShaderProgram& shader)
{

    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        
        glActiveTexture(GL_TEXTURE0 + textures[i].id);
        std::string number;
        std::string name = textures[i].type;
        if (name == "diffuseTexture")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);
        else if (name == "normalMap")
            number = std::to_string(normalNr++);
   
        shader.setInt(name.c_str(), textures[i].id);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
        
        
    }
    glActiveTexture(GL_TEXTURE0);

    // Отрисовываем меш
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); 
}

void Mesh::SetUV(float u0, float v0, float u1, float v1)
{
    vertices[0].TexCoords = { u0, v1 };
    vertices[1].TexCoords = { u1, v1 };
    vertices[2].TexCoords = { u1, v0 };
    vertices[3].TexCoords = { u0, v0 };

    UpdateVBO();
}

void Mesh::UpdateVBO()
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        vertices.size() * sizeof(Vertex),
        vertices.data()
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Model::Draw(Renderer::ShaderProgram& shader)
{
    shader.setBool("UseSolidColor", useSolidColor);
    for (unsigned int i = 0; i < meshes.size(); i++)
        
        meshes[i]->Draw(shader);
}

void Model::SetUV(float u0, float v0, float u1, float v1)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i]->SetUV(u0, v0, u1, v1);
}

void Model::loadModel(std::string path)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ValidateDataStructure);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    m_GlobalInverseTransform = glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation));

    ExtractSkeleton(scene->mRootNode, -1);
    
    processNode(scene->mRootNode, scene);

    BuildBoneHierarchy(scene->mRootNode, -1);

    LoadAnimationsFromAssimp(scene);

    /*std::cout << "Meshes loaded: " << meshes.size() << std::endl;
    for (auto& mesh : meshes)
        std::cout << "Vertices: " << mesh->vertices.size()
        << ", Indices: " << mesh->indices.size()
        << ", Textures: " << mesh->textures.size() << std::endl;*/

    
}

void Model::ExtractSkeleton(aiNode* node, int parentIndex) {
    SkeletonNode skelNode;
    skelNode.name = node->mName.C_Str();
    skelNode.parent = parentIndex;

    skelNode.bindLocal = ConvertMatrix(node->mTransformation);

    int currentIndex = (int)skeleton.nodes.size();
    skeleton.nodes.push_back(skelNode);

    if (parentIndex != -1) {
        skeleton.nodes[parentIndex].children.push_back(currentIndex);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ExtractSkeleton(node->mChildren[i], currentIndex);
    }
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // Обрабатываем все меши (если они есть) у выбранного узла
    
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(move(processMesh(mesh, scene)));
    }

    // И проделываем то же самое для всех дочерних узлов
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }

}

void Model::LoadAnimationsFromAssimp(const aiScene* scene)
{
    animations.clear(); 
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* aiAnim = scene->mAnimations[i];
        Animation anim;

        anim.duration = (float)aiAnim->mDuration;
        anim.ticksPerSecond = (aiAnim->mTicksPerSecond != 0) ? (float)aiAnim->mTicksPerSecond : 25.0f;

        for (unsigned int c = 0; c < aiAnim->mNumChannels; c++) {
            aiNodeAnim* channel = aiAnim->mChannels[c];
            BoneAnimation boneAnim;

            // 1. Грузим позиции (их может быть 2)
            for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                boneAnim.positions.push_back({
                    (float)channel->mPositionKeys[k].mTime,
                    glm::vec3(channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z)
                    });
            }

            // 2. Грузим вращения (их может быть 150)
            for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) {
                boneAnim.rotations.push_back({
                    (float)channel->mRotationKeys[k].mTime,
                    glm::quat(channel->mRotationKeys[k].mValue.w, channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z)
                    });
            }

            // 3. Грузим масштабы
            for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) {
                boneAnim.scales.push_back({
                    (float)channel->mScalingKeys[k].mTime,
                    glm::vec3(channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z)
                    });
            }

            anim.channels[channel->mNodeName.C_Str()] = boneAnim;
        }
        animations.push_back(anim);
    }
}

void Model::BuildBoneHierarchy(aiNode* node, int parentBone)
{
    std::string name = node->mName.C_Str();

    int currentBone = parentBone;

    auto it = boneMap.find(name);
    if (it != boneMap.end()) {
        currentBone = it->second;
        bones[currentBone].parent = parentBone;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        BuildBoneHierarchy(node->mChildren[i], currentBone);
    }
}

std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;
    glm::vec4 color;
    vector<Bone> bones;
    int BoneCounter = 0;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // Обрабатываем координаты вершин, нормали и текстурные координаты

        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        
        vertex.Position = vector;
        
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;

        vertex.Normal = vector;  

        if (mesh->mTextureCoords[0]) 
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

        }
        
        else vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    
    for (uint b = 0; b < mesh->mNumBones; b++)
    {
        aiBone* bone = mesh->mBones[b];

        int boneID = GetOrCreateBoneIndex(bone);

        for (uint w = 0; w < bone->mNumWeights; w++) {
            aiVertexWeight vw = bone->mWeights[w];

            int vertexId = vw.mVertexId;
            float weight = vw.mWeight;


            for (int i = 0; i < 4; i++) {

                if (vertices[vertexId].weights[i] == 0.0f) {
                    vertices[vertexId].boneID[i] = boneID;
                    vertices[vertexId].weights[i] = weight;
                    break;
                }
            }
         
        }

    }
   
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
   
    // ШАГ 0: Инициализация (обязательно обнуляем, чтобы не было мусора)
    for (auto& v : vertices) {
        v.Tangent = glm::vec3(0.0f);
        v.Bitangent = glm::vec3(0.0f);
    }

    // ШАГ 1: Накопление (проходим по всем треугольникам)
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;

        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float f = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

        // Если UV треугольника «схлопнуты», f будет 0. 
        // В этом случае этот конкретный треугольник не даст вклада в тангент.
        if (std::abs(f) > 1e-6f) {
            f = 1.0f / f;

            glm::vec3 t;
            t.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            t.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            t.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            glm::vec3 b;
            b.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            b.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            b.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

            v0.Tangent += t; v1.Tangent += t; v2.Tangent += t;
            v0.Bitangent += b; v1.Bitangent += b; v2.Bitangent += b;
        }
    }

    // ШАГ 2: Финальная обработка (проходим по каждой вершине один раз)
    for (auto& v : vertices) {
        if (glm::length(v.Tangent) < 0.001f) {
            // Затычка для ствола или объектов без UV
            glm::vec3 c1 = glm::cross(v.Normal, glm::vec3(0.0, 0.0, 1.0));
            glm::vec3 c2 = glm::cross(v.Normal, glm::vec3(0.0, 1.0, 0.0));
            v.Tangent = (glm::length(c1) > glm::length(c2)) ? glm::normalize(c1) : glm::normalize(c2);
            v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
        }
        else {
            // Ортогонализация Грамма-Шмидта (важно для красоты нормал-маппинга)
            v.Tangent = glm::normalize(v.Tangent - v.Normal * glm::dot(v.Normal, v.Tangent));

            // Пересчитываем битангенту, чтобы она была строго перпендикулярна
            v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
        }
    }
    
    
    if (mesh->mMaterialIndex >= 0)
    {
        aiColor4D baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuseTexture", scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "", scene);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        

        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normalMap", scene);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> normalMapsEmbended = loadMaterialTextures(material, aiTextureType_NORMAL_CAMERA, "normalMap", scene);
        textures.insert(textures.end(), normalMapsEmbended.begin(), normalMapsEmbended.end());
        
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &baseColor))
        {
            vec4 color = vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        }
    }
    else 
    {
        std::cout << "no materials \n";
    }

    return std::make_unique<Mesh>(vertices, indices, textures);
}


unsigned int TextureFromEmbedded(aiTexture* tex) {

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data;
    if (tex->mHeight == 0) {
        // PNG/JPEG
        data = stbi_load_from_memory((unsigned char*)tex->pcData, tex->mWidth, &width, &height, &nrComponents, 0);
    }
    else {
        // RAW image
        data = (unsigned char*)tex->pcData;
        width = tex->mWidth;
        height = tex->mHeight;
        nrComponents = 4;
    }
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        stbi_image_free(data);
        
    }
    return textureID;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene)
{
    std::vector<Texture> textures;

    unsigned int count = mat->GetTextureCount(type);

    if (count == 0 && typeName == "diffuseTexture")
    {
        useSolidColor = false;
    }

    for (unsigned int i = 0; i < count; i++)
    {

        aiString str;
        mat->GetTexture(type, i, &str);
        Texture texture;
        const aiTexture* embeddedTex = scene->GetEmbeddedTexture(str.C_Str());
        if (embeddedTex) {
            std::cout << "Loading embedded texture: " << str.C_Str() << std::endl;
            int texIndex = atoi(str.C_Str() + 1);
            aiTexture* tex = scene->mTextures[texIndex];
            texture.id = TextureFromEmbedded(tex);
            texture.type = typeName;
            texture.path = "embedded";

            textures.push_back(texture);
        }

        else {
            texture.id = TextureFromFile(str.C_Str(), directory, false);
            texture.type = typeName;
            texture.path = str.C_Str();

            textures.push_back(texture);
        }

    }

    return textures;
}

unsigned int Model::TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (!data)
    {
        std::cout << "Texture missing: " << filename << ", using default\n";
    }
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        stbi_image_free(data);
    }
    else
    {

        std::cout << "Texture failed to load at path: " << path << std::endl;

        stbi_image_free(data);
    }

    return textureID;
}