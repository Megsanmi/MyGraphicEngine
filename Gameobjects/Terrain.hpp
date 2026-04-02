// ================= TERRAIN HPP =================
#pragma once

#include "GameObject.hpp"
#include "Texture.hpp"
#include <unordered_map>
#include <vector>

#define NodeArray BulletNodeArray
#include <btBulletDynamicsCommon.h>
#undef NodeArray

struct TerrainChunk {
    glm::ivec2 coord;
    std::unique_ptr<Mesh> mesh = nullptr;
    Mesh* meshPtr = nullptr;

    btRigidBody* body = nullptr;
    btCollisionShape* shape = nullptr;
    btTriangleMesh* triangleMesh = nullptr;

    int lod = 1; // store current LOD

    void release(btDynamicsWorld* world);
};

class Terrain : public Component {
public:
    bool enabled = true;
    bool lastEnabled = true;

    siv::PerlinNoise noise{ 999u };
    Model* terrainModel = nullptr;
    MeshRenderer* meshRenderer = nullptr;
    Texture* tex = nullptr;
    float tiling = 1.0;


    int viewDistance = 10;
    int chunkSize = 16;
    float scale = 1.0f;
    float heightMultiplier = 15.0f;
    unsigned int textureID = 0;

    int m_lod = 2; // base LOD divisor
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
            {"tiling", tiling },
            {"flat", flat }
        };


    }
    void Deserialize(const json& j)
    {
        if (j.contains("viewDistance"))viewDistance = j["viewDistance"];
        if (j.contains("chunkSize"))chunkSize = j["chunkSize"];
        if (j.contains("m_lod"))m_lod = j["m_lod"];
        if (j.contains("flat"))flat = j["flat"];
        if (j.contains("tiling"))tiling = j["tiling"];
    }
    

    void loadChunk(int cx, int cz);

    void drawInspector()
    {
        if (ImGui::CollapsingHeader("Terrain"))
        {
            ImGui::Checkbox("Enabled#1", &enabled);
            ImGui::DragFloat("Tiling texture", &tiling,0.01);
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

    // LOD safety helper
    int safeLOD(int lod)
    {
        if (lod <= 0) lod = 1;
        return lod;
    }
};

