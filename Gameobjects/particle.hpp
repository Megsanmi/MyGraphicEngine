#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include "GameObject.hpp"


using namespace glm;
using namespace std;

class particle 
{
public:
	vec3 position = { 0,0,0 };
	float size = 0.1f;
    vec3 velocity;
    float life;

};

struct DrawPathState
{
    int currentSegment = 0;   // какая линия
    float t = 0.0f;           // прогресс по линии [0..1]
    float speed = 3.f;       // скорость рисования
    bool finished = false;
};

class Emmiter
{
public:
    float particleLife = 15;
    float spawnRate = 1;
};
class ParticleSystem : public Component
{
public:
    Renderer::ShaderProgram& s;
    unsigned int t = 0;
    Emmiter emitter;
    
    vec3 startPos;
    vector<particle> particles;
    vec3 color;
    float speed;
    DrawPathState state;
    vector<vec3> points = {
    {  0.0f,        1.0f      , 0  },  // 0 верх
    {  0.9510565f,  0.3090169f, 0  },  // 1 право-верх
    {  0.5877852f, -0.8090169f, 0  },  // 2 право-низ
    { -0.5877852f, -0.8090169f, 0  },  // 3 лево-низ
    { -0.9510565f,  0.3090169f, 0  }   // 4 лево-верх
    };

    vector<int> order = {
    0, 3,
    3, 1,
    1, 4,
    4, 2,
    2, 0
    };

    ParticleSystem(Renderer::ShaderProgram& s);
    void OnEnable() {};
    void drawInspector() override;
    json Serialize() override;
    void Deserialize(const json& j) override;
    
    void Update(float dt) override;

    void SpawnParticle(vec3 pos);
    void draw(mat4 m);

private:
    float random(float min, float max)
    {
        return min + (float(rand()) / float(RAND_MAX)) * (max - min);
    }

    GLuint VAO, VBO, EBO;

    float vertices[16] = 
    { 
        -0.5f, -0.5f, 0.0f, 0.0f, 
        0.5f, -0.5f, 1.0f, 0.0f, 
        0.5f, 0.5f, 1.0f, 1.0f, 
        -0.5f, 0.5f, 0.0f, 1.0f 
    };
    

    unsigned int indices[6] = {
        0, 1, 2, 2, 3, 0,
    };
};