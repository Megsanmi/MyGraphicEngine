#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <vector>
#include "../Gameobjects/GameObject.hpp"
#include "ShaderProgram.h"
#include "ShadowMap.hpp"

namespace std {};

class Light : public Component
{
public:
	glm::vec3 position = { 0,0,0 }; 
	glm::vec3 color = { 0.6f,0.6f,0.6f };
	glm::vec3 ambient = { 0.3f,0.3f,0.3f };

	Renderer::ShaderProgram& shaderprogram;
	ShadowMap& Shadowmap;

	
	float nearPlane = 0.1f;
	float farPlane = 1000.f;

	float planeW = 100.f; 
	float planeH = 100.f;

	int WIDTH;
	int HEIGHT;

	Light(int W,int H,std::vector<std::unique_ptr<GameObject>>& objects,ShadowMap& shadowmap, Renderer::ShaderProgram& s);
	


	
	void DrawShadowMap(glm::mat4 model,unsigned int texID);



	void drawShade(int W_WIDTH, int W_HEIGH);


	void drawInspector()
	{
		

		ImGui::DragFloat2("shadow zone: ", &planeW, 0.1f);
		ImGui::ColorEdit3("rotationEuler", &color.x);


	};

	void Update(float dt)
	{
		drawShade(WIDTH, HEIGHT);
	}


private:
	float quadVertices[30] = {
   -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
   -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
	0.5f, -0.5f, 0.0f,  1.0f, 0.0f,

   -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
	0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
	0.5f,  0.5f, 0.0f,  1.0f, 1.0f
	};
	unsigned int quadVAO, quadVBO;
	std::vector<std::unique_ptr<GameObject>>& m_objects;
	

};

