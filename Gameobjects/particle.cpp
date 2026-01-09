#include "particle.hpp"


ParticleSystem::ParticleSystem(Renderer::ShaderProgram& s) : s(s)
{

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // UV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    glBindVertexArray(0);
}

void ParticleSystem::drawInspector()
{
    if (ImGui::CollapsingHeader("Particle system"))
    {
        ImGui::DragFloat("particle life", &emitter.particleLife, 0.1f);
        ImGui::DragFloat("spawn rate", &emitter.spawnRate, 1.f);
        ImGui::DragFloat("speed", &speed, 0.1f);
        ImGui::DragFloat3("start position", &startPos.x, 0.1f);
        ImGui::ColorEdit3("Color", &color.x);
    }
};

json ParticleSystem::Serialize()
{
    return{
        {"type","ParticleSystem"},
        {"speed",speed},
        {"particleLife",emitter.particleLife},
        {"spawnRate",emitter.spawnRate},
       
    };
};
void ParticleSystem::Deserialize(const json& j) {
    speed = j["speed"];
    emitter.particleLife = j["particleLife"];
    emitter.spawnRate = j["spawnRate"];

}

void ParticleSystem::Update(float dt)
{

    if (!state.finished)
    {
        int segCount = order.size() / 2;

        int i0 = order[state.currentSegment * 2];
        int i1 = order[state.currentSegment * 2 + 1];

        vec3 a = points[i0];
        vec3 b = points[i1];

        state.t += dt * state.speed;

        if (state.t >= 1.0f)
        {
            state.t = 0.0f;
            state.currentSegment++;

            if (state.currentSegment >= segCount)
                state.finished = true;
        }
        else
        {
            vec3 pos = mix(a, b, state.t);
            SpawnParticle(pos * 30.0f);
        }

        for (auto it = particles.begin(); it != particles.end(); )
        {
            if (state.finished)
            {
                it->velocity = vec3(random(-1, 1), random(-1, 1), random(-1, 1)) * 20.f;
            }
            it->position += it->velocity * dt;
            it->life -= dt;
            
            if (it->life <= 0.0f)
                it = particles.erase(it);
            else
                ++it;
        }
    }


    for (auto it = particles.begin(); it != particles.end(); )
    {
        it->position += it->velocity * dt;
        it->life -= dt;

        if (it->life <= 0.0f)
            it = particles.erase(it);
        else
            ++it;
    }


    draw(gameObject->GetWorldMatrix());
}

void ParticleSystem::SpawnParticle(vec3 pos)
{
    particle p;
    p.life = emitter.particleLife;

    p.velocity.x = random(-1, 1);
    p.velocity.y = random(-1, 1);
    p.velocity.z = random(-1, 1);
    p.velocity *= speed;
    p.position = pos;
    p.size = 1.f;

    particles.push_back(p);
};

void ParticleSystem::draw(mat4 m)
{

    s.setBool("UseSolidColor", true);
    for (auto& p : particles)
    {
        s.setVec3("solidColor", color);
        glm::mat4 model = m * glm::translate(glm::mat4(1.0f), p.position);
        model = glm::scale(model, glm::vec3(p.size));
        s.setMatrix4("model_matrix", model);
        glUseProgram(s.m_ID);
        glBindTexture(GL_TEXTURE_2D, 1);
        glBindVertexArray(VAO);

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
};