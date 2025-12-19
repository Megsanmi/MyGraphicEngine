#include "light.hpp"
#include "../Gameobjects/scene.hpp"

Light::Light(int W, int H, std::vector<std::unique_ptr<GameObject>>& objects, Renderer::ShaderProgram& s)
    :m_objects(objects),
    shaderprogram(s),
    Shadowmap(10000, 10000),
    WIDTH(W),
    HEIGHT(H)
{ 

    
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


}

void Light::OnEnable()
{
    if (!gameObject || !gameObject->scene) return;
    gameObject->scene->lights.push_back(this);
}

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

    glm::vec3 lightPos = transform->position - lightDir * 500.f;

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
   
    Shadowmap.beginRender();

    
    
    for (auto& obj : m_objects)
    {
        auto mesh = obj->GetComponent<MeshRenderer>();
        if (mesh && mesh->isShaded) mesh->Draw(Shadowmap.getShader());
            
    }
  
    Shadowmap.endRender(WIDTH, HEIGHT);

    Shadowmap.setLightSpaceMatrix(lightSpaceMatrix);

    shaderprogram.use();
    
    if (drawPlane) DrawShadowMap(model, Shadowmap.getDepthTex());
}