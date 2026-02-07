#pragma once

#include "GameObject.hpp"
#include <unordered_map>
#include <vector>

#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray

struct TerrainChunk {
    glm::ivec2 coord;
    Mesh* mesh = nullptr; 
    btRigidBody* body = nullptr;
    btCollisionShape* shape = nullptr;
    btTriangleMesh* triangleMesh = nullptr;

    void release(btDynamicsWorld* world);
};

class Terrain : public Component {
public:
    bool enabled = true;
    bool lastEnabled = true;

    siv::PerlinNoise noise{ 123u };
    Model* terrainModel = nullptr;
    MeshRenderer* meshRenderer = nullptr;

    
    //bool flat = true;
    int viewDistance = 10;
    int chunkSize = 16;
    float scale = 1.0f;
    float heightMultiplier = 15.0f;
    unsigned int textureID = 0;
    int m_lod = 2;
    bool flat = true;
    Renderer::ShaderProgram& shaderProgram;

    Terrain(Renderer::ShaderProgram& shader);
    ~Terrain();

    void Update(float dt) override;
    void OnEnable() override;
    json Serialize()
    {
        return{
            {"type","Terrain"},
            {"chunkSize",chunkSize},
            {"viewDistance",viewDistance},
            {"m_lod", m_lod },
            {"flat", flat }
        };


    }
    void Deserialize(const json& j)
    {
        if (j.contains("viewDistance"))viewDistance = j["viewDistance"];
        if (j.contains("chunkSize"))chunkSize = j["chunkSize"];
        if(j.contains("flat"))flat = j["flat"];
    }
    void loadChunk(int cx, int cz);
 

    void drawInspector()
    {
        if (ImGui::CollapsingHeader("Terrain"))
        {
            ImGui::Checkbox("Enabled#1", &enabled);
            ImGui::DragInt("view distance", &viewDistance);
            ImGui::DragInt("chunk size", &chunkSize);
            ImGui::DragInt("m_lod", &m_lod);
            ImGui::Checkbox("flat#1", &flat);
        }

    };
    float getHeight(int x, int z);
    long long getChunkKey(int x, int z) { return ((long long)x << 32) | (unsigned int)z; }

    glm::ivec2 worldToChunk(glm::vec3 worldPos);

private:
    std::unordered_map<long long, TerrainChunk> chunks;
    void generateChunkMesh(int cx, int cz, TerrainChunk& outChunk, int lod = 1);
};