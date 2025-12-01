#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../Renderer/ShaderProgram.h"

class ShadowMap 
{
public:
	ShadowMap(unsigned int width = 1024, unsigned int height = 1024);
	~ShadowMap();

    void beginRender();
    void endRender(unsigned int screenWidth, unsigned int screenHeight);

    void bindDepthTexture(unsigned int textureUnit = 1) const;

    void setLightSpaceMatrix(const glm::mat4& lightSpace);
    const glm::mat4& getLightSpaceMatrix() const { return lightSpaceMatrix; }

    Renderer::ShaderProgram& getShader() { return *depthShader; }
    unsigned int getDepthTex() { return depthMapTex; };
    
    unsigned int width, height;
private:

    unsigned int depthMapFBO;
    unsigned int depthMapTex;
    

    glm::mat4 lightSpaceMatrix;
    Renderer::ShaderProgram* depthShader;

};


