#include "Terrain.hpp"
#include "scene.hpp" 
#include <iostream>
#include <algorithm>

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
}

Terrain::~Terrain() {
    for (auto& pair : chunks) {
        //pair.second.release(gameObject->scene->dynamicsWorld);
        delete pair.second.mesh;
    }
    delete terrainModel;
}

void Terrain::OnEnable() {

    textureID = terrainModel->TextureFromFile("grass.jpg", "assets/textures", false);

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

    glm::ivec2 camChunk = worldToChunk(gameObject->scene->camera->position);

  
    for (int z = -viewDistance; z <= viewDistance; z++) {
        for (int x = -viewDistance; x <= viewDistance; x++) {
            loadChunk(camChunk.x + x, camChunk.y + z);
        }
    }

 
    for (auto it = chunks.begin(); it != chunks.end(); ) {
        glm::ivec2 c = it->second.coord;
        if (std::abs(c.x - camChunk.x) > viewDistance + 10 ||
            std::abs(c.y - camChunk.y) > viewDistance + 10) {

            TerrainChunk& chunk = it->second;


            auto& mVec = terrainModel->meshes;
            mVec.erase(std::remove_if(mVec.begin(), mVec.end(), [&](const Mesh& m) {
                return m.VAO == chunk.mesh->VAO;
                }), mVec.end());

            chunk.release(gameObject->scene->dynamicsWorld);
            delete chunk.mesh; 

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
    generateChunkMesh(cx, cz, chunk);
    chunks[key] = chunk;
}

float Terrain::getHeight(int x, int z) {
    float freq = 0.05f;
    float h = noise.noise2D(x * freq, z * freq);
    return (h * 0.5f + 0.5f) * heightMultiplier;
}

void Terrain::generateChunkMesh(int cx, int cz, TerrainChunk& outChunk) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float offset_x = cx * chunkSize * scale;
    float offset_z = cz * chunkSize * scale;

    for (int z = 0; z <= chunkSize; ++z) {
        for (int x = 0; x <= chunkSize; ++x) {
            Vertex v;
            float worldX = (cx * chunkSize + x);
            float worldZ = (cz * chunkSize + z);
            float y = getHeight((int)worldX, (int)worldZ);

            v.Position = glm::vec3(x * scale + offset_x, y, z * scale + offset_z);
            v.TexCoords = glm::vec2((float)x / chunkSize, (float)z / chunkSize);
            v.Normal = glm::vec3(0, 0, 0);
            vertices.push_back(v);
        }
    }

    outChunk.triangleMesh = new btTriangleMesh();

    for (int z = 0; z < chunkSize; ++z) {
        for (int x = 0; x < chunkSize; ++x) {
            int r1 = z * (chunkSize + 1);
            int r2 = (z + 1) * (chunkSize + 1);
            unsigned int i0 = r1 + x, i1 = r1 + x + 1, i2 = r2 + x, i3 = r2 + x + 1;

            indices.insert(indices.end(), { i0, i2, i1, i1, i2, i3 });

            auto toBt = [](glm::vec3 v) { return btVector3(v.x, v.y, v.z); };
            outChunk.triangleMesh->addTriangle(toBt(vertices[i0].Position), toBt(vertices[i2].Position), toBt(vertices[i1].Position));
            outChunk.triangleMesh->addTriangle(toBt(vertices[i1].Position), toBt(vertices[i2].Position), toBt(vertices[i3].Position));
        }
    }

    //нормали
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 edge1 = vertices[indices[i + 1]].Position - vertices[indices[i]].Position;
        glm::vec3 edge2 = vertices[indices[i + 2]].Position - vertices[indices[i]].Position;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        vertices[indices[i]].Normal += normal;
        vertices[indices[i + 1]].Normal += normal;
        vertices[indices[i + 2]].Normal += normal;
    }
    for (auto& v : vertices) v.Normal = glm::normalize(v.Normal);

    // Физика
    outChunk.shape = new btBvhTriangleMeshShape(outChunk.triangleMesh, true);
    btDefaultMotionState* ms = new btDefaultMotionState();
    btRigidBody::btRigidBodyConstructionInfo info(0, ms, outChunk.shape);
    outChunk.body = new btRigidBody(info);
    gameObject->scene->dynamicsWorld->addRigidBody(outChunk.body);

    Texture tex{ textureID, "texture_diffuse", "" };
    outChunk.mesh = new Mesh(vertices, indices, { tex });

    terrainModel->meshes.push_back(*outChunk.mesh);
}

glm::ivec2 Terrain::worldToChunk(glm::vec3 worldPos) {
    float s = chunkSize * scale;
    return { (int)std::floor(worldPos.x / s), (int)std::floor(worldPos.z / s) };
}