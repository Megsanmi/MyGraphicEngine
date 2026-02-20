#include "light.hpp"
#include "../Gameobjects/scene.hpp"

Light::Light(std::vector<std::unique_ptr<GameObject>>& objects, Renderer::ShaderProgram& s)
    :m_objects(objects),
    shaderprogram(s)
{ 

}

Light::~Light() {
    OnDisable();
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    
}

void Light::OnEnable()
{
    Shadowmap = new ShadowMap(*gameObject->scene->ShadowShader,1000,1000);
   
    WIDTH = gameObject->scene->width;
    HEIGHT = gameObject->scene->height;

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    if (!gameObject || !gameObject->scene) return;
    gameObject->scene->lights.push_back(this);
}
void Light::drawInspector()
{

    if (ImGui::CollapsingHeader("Light"))
    {
        ImGui::Checkbox("enabled", &enabled);
        ImGui::DragFloat2("shadow zone: ", &planeW, 0.1f);
        ImGui::ColorEdit3("Color light", &color.x);
        ImGui::Checkbox("Draw shadow plane", &drawPlane);
        ImGui::DragInt("map size: ", &mapSize);
        if (ImGui::Button("Resize Map")) Shadowmap = new ShadowMap(*gameObject->scene->ShadowShader, mapSize, mapSize);
    }
};

void Light::Deserialize(const json& j) {
    if (j.contains("color")) color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
    if (j.contains("ambient")) ambient = glm::vec3(j["ambient"][0], j["ambient"][1], j["ambient"][2]);
    if (j.contains("mapSize")) mapSize = j["mapSize"];
    if (j.contains("planeH")) planeH = j["planeH"];
    if (j.contains("planeW")) planeW = j["planeW"];

    //Shadowmap = new ShadowMap(*gameObject->scene->ShadowShader, mapSize, mapSize);
};


void Light::OnDisable()
{
    if (!gameObject || !gameObject->scene) return;
    auto& lights = gameObject->scene->lights;
    lights.erase(std::remove(lights.begin(), lights.end(), this), lights.end());

};
void Light::DrawShadowMap(glm::mat4 model, unsigned int texID)
{
    shaderprogram.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    shaderprogram.setMatrix4("model_matrix", model);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
};
void Light::drawShade()
{
    auto transform = gameObject->GetComponent<Transform>();
    if (!transform) return;

 
    glm::vec3 rotationEuler = transform->rotationEuler;

    float yaw = rotationEuler.y; 
    float pitch = rotationEuler.x;

 
    lightDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    lightDir.y = sin(glm::radians(pitch));
    lightDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    glm::vec3 lightPos = gameObject->scene->camera->position - lightDir * 500.f;

    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, { 0,1,0 });
    

    glm::mat4 lightProjection = glm::ortho(-planeW/2, planeW/2, -planeH/2, planeH/2, nearPlane, farPlane);

    lightSpaceMatrix = lightProjection * lightView;


    glm::vec3 planePos = lightPos + lightDir * nearPlane;

    glm::vec3 forward = glm::normalize(lightDir);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    glm::mat4 rotation = glm::mat4(
        glm::vec4(right, 0.0f),
        glm::vec4(up, 0.0f),
        glm::vec4(-forward, 0.0f),
        glm::vec4(0, 0, 0, 1)
    );

    glm::mat4 model = glm::translate(glm::mat4(1.0f), planePos) * rotation * glm::scale(glm::mat4(1.0f), glm::vec3(planeW, planeH, 1.0f));
   
    Shadowmap->beginRender();

    
    
    for (auto& obj : m_objects)
    {
        auto mesh = obj->GetComponent<MeshRenderer>();
        if (mesh && mesh->isShaded) mesh->Draw(Shadowmap->getShader());
            
    }
  
    Shadowmap->endRender(WIDTH, HEIGHT);

    Shadowmap->setLightSpaceMatrix(lightSpaceMatrix);

    shaderprogram.use();
    
    if (drawPlane) DrawShadowMap(model, Shadowmap->getDepthTex());
}