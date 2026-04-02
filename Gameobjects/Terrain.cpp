
// ================= TERRAIN CPP =================
#include "Terrain.hpp"
#include "scene.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

void TerrainChunk::release(btDynamicsWorld* world) {
    if (world && body) {
        world->removeRigidBody(body);
    }
    if (body) {
        if (body->getMotionState()) delete body->getMotionState();
        delete body;
    }
    if (shape) delete shape;
    if (triangleMesh) delete triangleMesh;
}

Terrain::Terrain(Renderer::ShaderProgram& shader)
    : shaderProgram(shader) {
    terrainModel = new Model();

    textureID = terrainModel->TextureFromFile("grass.jpg", "assets/textures", false);
    tex = new Texture(textureID, "diffuseTexture", "");
}

Terrain::~Terrain() {
    for (auto& pair : chunks) {
        pair.second.release(gameObject->scene->dynamicsWorld);
    }
    delete terrainModel;
}

void Terrain::OnEnable() {
    meshRenderer = gameObject->GetComponent<MeshRenderer>();
    if (!meshRenderer) {
        meshRenderer = gameObject->AddComponent<MeshRenderer>(terrainModel, shaderProgram);
    }
    else {
        meshRenderer->model = terrainModel;
    }

    gameObject->transform->position = glm::vec3(0);
}

void Terrain::Update(float dt) {
    if (!gameObject->scene->camera) return;
    if (!enabled) return;

    if (meshRenderer && meshRenderer->enabled) {
        meshRenderer->model = terrainModel;
    }

    glm::ivec2 camChunk = worldToChunk(gameObject->scene->camera->position);

    for (int z = -viewDistance; z <= viewDistance; z++) {
        for (int x = -viewDistance; x <= viewDistance; x++) {
            loadChunk(camChunk.x + x, camChunk.y + z);
        }
    }

    for (auto it = chunks.begin(); it != chunks.end(); )
    {
        glm::ivec2 c = it->second.coord;

        if (std::abs(c.x - camChunk.x) > viewDistance + 10 ||
            std::abs(c.y - camChunk.y) > viewDistance + 10)
        {
            TerrainChunk& chunk = it->second;

            gameObject->scene->dynamicsWorld->removeRigidBody(chunk.body);

            auto& mVec = terrainModel->meshes;
            mVec.erase(std::remove_if(mVec.begin(), mVec.end(),
                [&](const std::unique_ptr<Mesh>& m)
                {
                    return m.get() == chunk.meshPtr;
                }), mVec.end());

            chunk.release(gameObject->scene->dynamicsWorld);
            it = chunks.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Terrain::loadChunk(int cx, int cz) {
    long long key = getChunkKey(cx, cz);

    if (chunks.find(key) != chunks.end()) return;

    TerrainChunk chunk;
    chunk.coord = { cx, cz };

    // SAFE LOD
    int lod = safeLOD(m_lod);

    generateChunkMesh(cx, cz, chunk, lod);
    chunk.lod = lod;

    chunks[key] = std::move(chunk);
}

void Terrain::generateChunkMesh(int cx, int cz, TerrainChunk& outChunk, int lod) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    lod = safeLOD(lod);

    float offset_x = cx * chunkSize * scale;
    float offset_z = cz * chunkSize * scale;

    int segments = std::max(1, chunkSize / lod);

    // prevent broken meshes
    if (segments < 1) segments = 1;

    for (int z = 0; z <= segments; ++z) {
        for (int x = 0; x <= segments; ++x) {
            Vertex v;

            float worldX = (cx * chunkSize + x * lod);
            float worldZ = (cz * chunkSize + z * lod);

            float y = 0;
            if (!flat) y = getHeight((int)worldX, (int)worldZ);

            v.Position = glm::vec3(
                x * scale * lod + offset_x,
                y,
                z * scale * lod + offset_z
            );

            v.TexCoords = glm::vec2(worldX, worldZ) * tiling;
           

            v.Normal = glm::vec3(0, 0, 0);

            vertices.push_back(v);
        }
    }

    outChunk.triangleMesh = new btTriangleMesh();

    for (int z = 0; z < segments; ++z) {
        for (int x = 0; x < segments; ++x) {
            int r1 = z * (segments + 1);
            int r2 = (z + 1) * (segments + 1);

            unsigned int i0 = r1 + x;
            unsigned int i1 = r1 + x + 1;
            unsigned int i2 = r2 + x;
            unsigned int i3 = r2 + x + 1;

            indices.insert(indices.end(), { i0, i2, i1, i1, i2, i3 });

            auto toBt = [](glm::vec3 v) { return btVector3(v.x, v.y, v.z); };

            outChunk.triangleMesh->addTriangle(toBt(vertices[i0].Position), toBt(vertices[i2].Position), toBt(vertices[i1].Position));
            outChunk.triangleMesh->addTriangle(toBt(vertices[i1].Position), toBt(vertices[i2].Position), toBt(vertices[i3].Position));
        }
    }

    // normals
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 edge1 = vertices[indices[i + 1]].Position - vertices[indices[i]].Position;
        glm::vec3 edge2 = vertices[indices[i + 2]].Position - vertices[indices[i]].Position;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vertices[indices[i]].Normal += normal;
        vertices[indices[i + 1]].Normal += normal;
        vertices[indices[i + 2]].Normal += normal;
    }

    for (auto& v : vertices)
        v.Normal = glm::normalize(v.Normal);

    // physics
    outChunk.shape = new btBvhTriangleMeshShape(outChunk.triangleMesh, true);
    btDefaultMotionState* ms = new btDefaultMotionState();

    btRigidBody::btRigidBodyConstructionInfo info(0, ms, outChunk.shape);
    outChunk.body = new btRigidBody(info);

    gameObject->scene->dynamicsWorld->addRigidBody(outChunk.body);

    // render
    outChunk.mesh = std::make_unique<Mesh>(vertices, indices, std::vector<Texture*>{ tex });
    terrainModel->meshes.push_back(std::move(outChunk.mesh));

    outChunk.meshPtr = terrainModel->meshes.back().get();
}

glm::ivec2 Terrain::worldToChunk(glm::vec3 worldPos) {
    float s = chunkSize * scale;
    return { (int)std::floor(worldPos.x / s), (int)std::floor(worldPos.z / s) };
}

float Terrain::getHeight(int x, int z)
{
    float fx = (float)x;
    float fz = (float)z;

    // ===== ¡»ŒÃ =====
    float biome = noise.noise2D(fx * 0.0005f, fz * 0.0005f);
    biome = biome * 0.5f + 0.5f;

    float mountainMask = glm::smoothstep(0.6f, 0.8f, biome);
    float plainsMask = 1.0f - mountainMask;

    // ===== –¿¬Õ»Õ€ (œÀŒ— »≈) =====
    float plains = 0.0f;

    float smallNoise = noise.noise2D(fx * 0.02f, fz * 0.02f);
    plains += smallNoise * 0.3f;

    // ===== √Œ–€ =====
    float mountains = 0.0f;

    float freq = 0.005f;
    float amp = 15.0f;

    for (int i = 0; i < 5; i++)
    {
        float n = noise.noise2D(fx * freq, fz * freq);

        n = 1.0f - fabs(n);
        n *= n; // Ò„Î‡ÊË‚‡ÌËÂ

        mountains += n * amp;

        freq *= 2.0f;
        amp *= 0.5f;
    }

    // ===== –≈ » (œÀ¿¬Õ€≈) =====
    float river = 1.0f;

    float r = noise.noise2D(fx * 0.001f, fz * 0.001f);
    r = fabs(r);

    float riverWidth = 0.03f;

    float riverMask = glm::smoothstep(0.0f, riverWidth, r);

    float riverShape = 1.0f - riverMask;
    riverShape = riverShape * riverShape;

    river = -riverShape * 2.0f;

    // ===== —Ã≈ÿ»¬¿Õ»≈ =====
    float height = 0.0f;

    height += plains * plainsMask;
    height += mountains * mountainMask;
    height += river;

    return height * heightMultiplier - 30.f;
}