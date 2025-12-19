#pragma once
#include <glm.hpp>
#include "../Gameobjects/scene.hpp"

using namespace glm;

struct SpriteSheet
{
    unsigned int textureID;
    int columns;
    int rows;
};


class Sprite : public Component
{
public:
    Model* model;
    std::vector<Texture> textures;

    int columns = 8;
    int rows = 12;

    int currentFrame = 0;
    int totalFrames = columns * rows;

    float fps = 8.0f;
    float timer = 0.0f;

    bool loop = true;
    bool playing = true;

    Renderer::ShaderProgram& shaderProgram;

    MeshRenderer* renderer = nullptr;
    Sprite(Renderer::ShaderProgram& shader) : shaderProgram(shader) {}

    void OnEnable() override
    {
        Texture texture;
        texture.id = model->TextureFromFile("textures/anim.jpg", "assets", false);
        texture.type = "texture_diffuse";
        textures.push_back(texture);
        model = new Model(vertices, indices, textures);

        if (gameObject->GetComponent<MeshRenderer>())
        {
            renderer = gameObject->GetComponent<MeshRenderer>();
            renderer->model = model;
        }
        else renderer = gameObject->AddComponent<MeshRenderer>(model, shaderProgram);
    }

    void drawInspector()
    {
        if (ImGui::CollapsingHeader("Sprite"))
        {
     
        }

    };

    void Update(float dt) override
    {
        timer += 0.013;
        float frameTime = 1.0f / fps;
        if (timer >= frameTime) {
            timer -= frameTime;
            currentFrame++;
            if (currentFrame >= totalFrames)
                currentFrame = loop ? 0 : totalFrames-1;
            UpdateUV();
        }
    }
    
    void UpdateUV()
    {
        int x = currentFrame % columns;
        int y = currentFrame / columns;
        
        float u0 = (float)x / columns;
        float v0 = (float)y / rows;
        float u1 = u0 + 1.0f / columns;
        float v1 = v0 + 1.0f / rows;

        
        renderer->model->meshes[0].SetUV(u0, v0, u1, v1);
    }


    std::vector<Vertex> vertices = {
        { {-0.5f,-0.5f,0}, {0,0,1}, {0,0} },
        { { 0.5f,-0.5f,0}, {0,0,1}, {1,0} },
        { { 0.5f, 0.5f,0}, {0,0,1}, {1,1} },
        { {-0.5f, 0.5f,0}, {0,0,1}, {0,1} },
    };

    std::vector<unsigned int> indices = {
        0,1,2,
        2,3,0
    };
   
};