#pragma once

#include "GameObject.hpp"
#include "json.hpp"
#include <fstream>

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
	Renderer::ShaderProgram& GlobalShaderProgram;//глобальный шейдер для сцены(по надобности можно сделать несколько шейдеров)
	GLFWwindow* window;

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
};

