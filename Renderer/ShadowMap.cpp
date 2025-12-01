#include "ShadowMap.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

ShadowMap::ShadowMap(unsigned int width, unsigned int height)
	: width(width), height(height)
{
    // Создаём FBO и depth texture
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMapTex);
    glBindTexture(GL_TEXTURE_2D, depthMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Загружаем depth шейдер
    //std::ifstream vsFile("Shaders/depth_shader_v.txt");
    //std::ifstream fsFile("Shaders/depth_shader_f.txt");
    //std::stringstream vsBuffer, fsBuffer;
    //vsBuffer << vsFile.rdbuf();
    //fsBuffer << fsFile.rdbuf();
    const char* vsBuffer = R"(
    #version 460

    layout(location = 0) in vec3 vertex_position;

    uniform mat4 model_matrix;
    uniform mat4 lightSpaceMatrix;

    void main()
    {
        gl_Position = lightSpaceMatrix * model_matrix * vec4(vertex_position, 1.0);
    }
    )";
    const char* fsBuffer = R"(
    #version 460

    void main()
    {
        
    }   
    )";
    
    depthShader = new Renderer::ShaderProgram(vsBuffer, fsBuffer);

    
    // Матрица вида света 
    glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
    glm::mat4 lightProjection = glm::ortho(-10.f, 10.f, -10.f, 10.f, 1.f, 20.f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    lightSpaceMatrix = lightProjection * lightView;

}

ShadowMap::~ShadowMap()
{
    glDeleteTextures(1, &depthMapTex);
    glDeleteFramebuffers(1, &depthMapFBO);
    delete depthShader;

}

void ShadowMap::beginRender()
{
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    depthShader->use();
    depthShader->setMatrix4("lightSpaceMatrix", lightSpaceMatrix);
}

void ShadowMap::endRender(unsigned int screenWidth, unsigned int screenHeight)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}


void ShadowMap::bindDepthTexture(unsigned int textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthMapTex);
}

void ShadowMap::setLightSpaceMatrix(const glm::mat4& mat)
{
    lightSpaceMatrix = mat;
}
