#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <vector>
#include "../Gameobjects/GameObject.hpp"
#include "ShaderProgram.hpp"
#include "ShadowMap.hpp"


namespace std {};

class Light : public Component
{
public:
	glm::vec3 lightDir;
	glm::vec3 color = { 0.5f,0.5f,0.5f };
	glm::vec3 ambient = { 0.3f,0.3f,0.3f };

	Renderer::ShaderProgram& shaderprogram;
	ShadowMap Shadowmap;

	float nearPlane = 0.1f;
	float farPlane = 10000.f;
	
	float planeW = 1000.f; 
	float planeH = 1000.f;

	int WIDTH = 1920;
	int HEIGHT = 1024;

	bool drawPlane = false;
	Light(int W,int H,std::vector<std::unique_ptr<GameObject>>& objects, Renderer::ShaderProgram& s);

	void OnEnable();
	
	void drawShade();

	void DrawShadowMap(glm::mat4 model,unsigned int texID);

	void drawInspector()
	{
		
		if (ImGui::CollapsingHeader("Light"))
		{
			ImGui::DragFloat2("shadow zone: ", &planeW, 0.1f);
			ImGui::ColorEdit3("Color light", &color.x);
			ImGui::Checkbox("Draw shadow plane", &drawPlane);
		}
	};
		
	void Update(float dt)
	{
		
	}

	void Deserialize(const json& j) override {
		color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
		ambient = glm::vec3(j["ambient"][0], j["ambient"][1], j["ambient"][2]);


	};

	json Serialize() override {
		return{
				{"type","Light"},
				{"color",{color.x,color.y,color.z}},
				{"ambient",{ambient.x,ambient.y,ambient.z}},
			
		};
	};

	mat4 getLightSpaceMatrix() { return lightSpaceMatrix; };

private:

	glm::mat4 lightSpaceMatrix;

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

