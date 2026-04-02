#pragma once

#include "GameObject.hpp"
#include "json.hpp"
#include <fstream>
#include "DebugDraw.hpp"

#include <fstream>
#include <sstream>

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
	Renderer::ShaderProgram* terrainShader;
	Renderer::ShaderProgram* GlobalShaderProgram;//глобальный шейдер для сцены(по надобности можно сделать несколько шейдеров)

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

	//                     RENDER SETTINGS
	float fogFar = 200.f;
	float fogNear = 50.f;



	Scene(int w, int h);

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


	string textFromfile( string path)
	{
		std::ifstream shader_path(path);
		std::string shader = "NO FRAGMENT SHADER";

		if (shader_path.is_open()) {
			std::stringstream buffer;
			buffer << shader_path.rdbuf();
			shader = buffer.str();
			shader_path.close();
		}
		else {
			std::cerr << "Cant open file " << path << std::endl;
		}

		return shader;
	}
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
uniform mat4 mesh_matrix;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning = false;

void main()
{
    vUv = aUv;

    vec4 skinnedPos = vec4(vertex_position, 1.0);

    if(useSkinning)
    {
        skinnedPos = bones[aBoneIDs.x] * vec4(vertex_position,1.0) * aWeights.x +
                     bones[aBoneIDs.y] * vec4(vertex_position,1.0) * aWeights.y +
                     bones[aBoneIDs.z] * vec4(vertex_position,1.0) * aWeights.z +
                     bones[aBoneIDs.w] * vec4(vertex_position,1.0) * aWeights.w;
    }

    gl_Position = lightSpaceMatrix * model_matrix * mesh_matrix * skinnedPos;
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

