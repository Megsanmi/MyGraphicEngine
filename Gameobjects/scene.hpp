#pragma once

#include "GameObject.hpp"
#include "json.hpp"
#include <fstream>
#include "DebugDraw.hpp"

class Light;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

using namespace std;
using namespace nlohmann;
class Scene {
public:
	Renderer::ShaderProgram* ShadowShader;
	Renderer::ShaderProgram& GlobalShaderProgram;//глобальный шейдер для сцены(по надобности можно сделать несколько шейдеров)
	GLFWwindow* window;
	MyDebugDrawer* debugDrawer;

	unsigned int IDs = 0;
	std::unordered_map<int, GameObject*> idToMap;
	std::unordered_map<string, Model*> modelCache;
	std::vector<std::unique_ptr<GameObject>> objects;
	std::vector<Light*> lights;
	
	Transform* camera;

	int width = 1920;
	int height = 1024;

	btDefaultCollisionConfiguration* collisionConfig = nullptr;
	btCollisionDispatcher* dispatcher = nullptr;
	btBroadphaseInterface* broadphase = nullptr;
	btSequentialImpulseConstraintSolver* solver = nullptr;
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr; 

	Scene(int w, int h, Renderer::ShaderProgram& shaderProgram);

	void shadowRender();
;

	GameObject* Addobject(const string name);

	GameObject* GetEntityByID(int id) {
		if (idToMap.find(id) != idToMap.end()) {
			return idToMap[id];
		}
		return nullptr; // Если объект не найден
	}

	void SetParent(int childID, int parentID) {
		GameObject* child = GetEntityByID(childID);
		GameObject* parent = GetEntityByID(parentID);

		if (child && parent) {
			parent->AddChild(child);
		}
	}

	void DeleteObject(int index);

	void Update(float dt);

	void InitPhysics();

	//сохранение всех объектов сцены в файл
	json SaveScene();


	//здесь загрузка сцены из json файла
	void clear();
	void LoadScene();

	//Загрузка объекта и всех его компонентов 
	std::unique_ptr<GameObject> LoadGameObject(const json& j);

private:

	const char* vsBuffer = R"(
#version 460

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 aUv;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aWeights;

out vec2 vUv;

uniform mat4 bones[100];
uniform mat4 model_matrix;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning = false;

void main()
{
    vUv = aUv;
    mat4 skin = mat4(1.0);
    
    if(useSkinning)
    {
        skin = bones[aBoneIDs.x] * aWeights.x + 
               bones[aBoneIDs.y] * aWeights.y + 
               bones[aBoneIDs.z] * aWeights.z + 
               bones[aBoneIDs.w] * aWeights.w;
    }

    gl_Position = lightSpaceMatrix * model_matrix * skin * vec4(vertex_position, 1.0);
}
)";

	const char* fsBuffer = R"(
#version 460

in vec2 vUv;
uniform sampler2D diffuseTexture;

void main()
{
    // В версии 460 лучше использовать texture() вместо texture2D
    float alpha = texture(diffuseTexture, vUv).a;
    
    if (alpha < 0.4) 
    {
        discard;
    }
}
)";
	
};

