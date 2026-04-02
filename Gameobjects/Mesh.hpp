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
#include <unordered_map>
#include <ext.hpp>
#include <memory>
#include <ext/matrix_transform.hpp>
#include <iostream>


using namespace std;
struct VectorKey { float time; glm::vec3 value; };
struct QuatKey { float time; glm::quat value; };



struct BoneAnimation {
    std::vector<VectorKey> positions;
    std::vector<QuatKey>   rotations;
    std::vector<VectorKey> scales;

    glm::mat4 Sample(float time) const {
        // Интерполяция позиции
        glm::vec3 translation(0.0f);
        if (!positions.empty()) {
            if (positions.size() == 1) translation = positions[0].value;
            else {
                size_t i = 0;
                for (; i < positions.size() - 1; ++i)
                    if (time < positions[i + 1].time) break;
                float factor = (time - positions[i].time) / (positions[i + 1].time - positions[i].time);
                translation = glm::mix(positions[i].value, positions[i + 1].value, glm::clamp(factor, 0.0f, 1.0f));
            }
        }
        glm::quat rotation(1, 0, 0, 0);
        if (!rotations.empty()) {
            if (rotations.size() == 1) rotation = rotations[0].value;
            else {
                size_t i = 0;
                for (; i < rotations.size() - 1; ++i)
                    if (time < rotations[i + 1].time) break;
                float factor = (time - rotations[i].time) / (rotations[i + 1].time - rotations[i].time);
                rotation = glm::slerp(rotations[i].value, rotations[i + 1].value, glm::clamp(factor, 0.0f, 1.0f));
            }
        }
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(glm::normalize(rotation)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)); // Масштаб обычно 1
    }

private:
    
    template<typename T>
    int GetKeyIndex(float time, const std::vector<T>& container) const {
        for (unsigned int i = 0; i < container.size() - 1; ++i) {
            if (time < container[i + 1].time) return i;
        }
        return 0;
    }

    glm::vec3 InterpolatePosition(float time) const {
        if (positions.empty()) return glm::vec3(0.0f);
        if (positions.size() == 1) return positions[0].value;

        int i1 = GetKeyIndex(time, positions);
        int i2 = i1 + 1;

        float factor = (time - positions[i1].time) / (positions[i2].time - positions[i1].time);
        return glm::mix(positions[i1].value, positions[i2].value, glm::clamp(factor, 0.0f, 1.0f));
    }

    glm::quat InterpolateRotation(float time) const {
        if (rotations.empty()) return glm::quat(1, 0, 0, 0);
        if (rotations.size() == 1) return rotations[0].value;

        int i1 = GetKeyIndex(time, rotations);
        int i2 = i1 + 1;

        float factor = (time - rotations[i1].time) / (rotations[i2].time - rotations[i1].time);
        return glm::slerp(rotations[i1].value, rotations[i2].value, glm::clamp(factor, 0.0f, 1.0f));
    }

    glm::vec3 InterpolateScale(float time) const {
        if (scales.empty()) return glm::vec3(1.0f);
        if (scales.size() == 1) return scales[0].value;

        int i1 = GetKeyIndex(time, scales);
        int i2 = i1 + 1;

        float factor = (time - scales[i1].time) / (scales[i2].time - scales[i1].time);
        return glm::mix(scales[i1].value, scales[i2].value, glm::clamp(factor, 0.0f, 1.0f));
    }
};

struct Bone
{
    string name;
    int parent;
    glm::mat4 inverseBindPose;
    glm::mat4 localTransform;

    unsigned int ID;
};

struct Animation {
    float duration = 0.0f;
    float ticksPerSecond = 25.0f;
    std::unordered_map<std::string, BoneAnimation> channels;

    bool HasChannel(const std::string& boneName) {
        return channels.find(boneName) != channels.end();
    }
    
};

struct SkeletonNode {
    std::string name;
    int parent = -1;
    std::vector<int> children;

    glm::mat4 bindLocal;      
};

struct Skeleton {
    std::vector<SkeletonNode> nodes;
    int rootIndex = 0;
};


struct Material
{
    glm::vec3 albedo = glm::vec3(1.0f);
    float metallic = 0.0f;
    float roughness = 1.0f;
    float ao = 0.0f;
    float alpha = 1.0f;

    bool hasAlbedoMap = false;
    bool hasNormalMap = false;
    bool hasMetallicMap = false;
    bool hasRoughnessMap = false;
    bool hasMaterialAlpha = false;
};

class Mesh 
{
public:
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = default;     
    Mesh& operator=(Mesh&&) noexcept = default;

    // Mesh-данные
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture*>      textures;
    glm::mat4 transform;

    Material material;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture*> textures, glm::mat4 transform = glm::mat4(1));
    ~Mesh()
    {
       
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);

    }
    void SetUV(float u0, float v0, float u1, float v1);
    void Draw(Renderer::ShaderProgram& shader);
    void setupMesh();
    void UpdateVBO();
    
    unsigned int VAO, VBO, EBO;
private:
};

class Model 
{
public:
    string name;
    glm::vec3 position{ 0,0,0 };
    glm::vec3 scale{1,1,1};
    glm::vec3 rotationAxis{0,1,0};

    float rotationAngle = 0;

    std::vector<std::unique_ptr<Mesh>> meshes;
    Skeleton skeleton;
    int ref = 0;

    vector<Bone> bones;
    std::vector<Animation> animations;
    unordered_map<string, unsigned int> boneMap;
    glm::mat4 m_GlobalInverseTransform;
    std::vector<glm::mat4> finalBoneMatrices;

    std::unordered_map<string, Texture*> textureCache;
    
    Model() {}
    Model(std::string path)
    {
        loadModel(path);
    };

    Model(vector<Vertex> vertices,
        vector<unsigned int> indices,
        vector<Texture*> textures)
    {
        meshes.emplace_back(std::make_unique<Mesh>(vertices, indices, textures));
    };
    ~Model()
    {
        for(auto& m : meshes)
        {
            for (auto& tex : m->textures)
            {
                if (tex->id)
                    glDeleteTextures(1, &tex->id);
            }

           
        }
    };

    void SetUV(float u0, float v0, float u1, float v1);
    void Draw(Renderer::ShaderProgram& shader);


    unsigned int TextureFromFile(const char* path, const string& directory, bool gamma);

    void BuildBoneHierarchy(aiNode* node, int parentBone);
    int GetOrCreateBoneIndex(aiBone* aiBone)
    {
        string name = aiBone->mName.C_Str();

        size_t fbxTag = name.find("_$AssimpFbx$");
        if (fbxTag != string::npos) {
            name = name.substr(0, fbxTag);
        }

        auto it = boneMap.find(name);
        if (it != boneMap.end())
            return it->second;
        Bone bone;
        bone.name = name;
        bone.parent = -1;
      

        bone.inverseBindPose = ConvertMatrix(aiBone->mOffsetMatrix);
        bone.ID = bones.size();

        bones.push_back(bone);
        boneMap[name] = bone.ID;
        return bone.ID;
    };

private:
    string directory = "Assets";

    glm::mat4 ConvertMatrix(const aiMatrix4x4& from) {
        glm::mat4 to;
     
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }
    void ExtractSkeleton(aiNode* node, int parentIndex);
    void LoadAnimationsFromAssimp(const aiScene* scene);
    void loadModel(string path);
    void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform);
    std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene, glm::mat4 transform);
    vector<Texture*> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene);
};

