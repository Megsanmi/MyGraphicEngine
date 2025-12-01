#pragma once
#include "GameObject.hpp"
#include "json.hpp"
#include <fstream>

using namespace std;
using namespace nlohmann;
class Scene {
public:
	Renderer::ShaderProgram& GlobalShaderProgram;//глобальный шейдер для сцены(по надобности можно сделать несколько шейдеров)
	
	std::vector<std::unique_ptr<GameObject>> objects;//нужно создавать именно массив ссылок а не объектов

	Scene(Renderer::ShaderProgram& shaderProgram) : GlobalShaderProgram(shaderProgram)
	{
		
	}

	GameObject* Addobject(const string name) 
	{
		auto obj = std::make_unique<GameObject>(name);
		GameObject* ptr = obj.get();
		objects.push_back(std::move(obj));
		return ptr;                                    //Возврат ссылки на объект позволит сразу после создания назначить ему компоненты
	}
	void DeleteObject(GameObject* object) {
		
	}
	void Update(float dt)
	{
		for(auto& obj: objects)
		{
			obj->Update(dt);
		}
	}
	//сохранение всех объектов сцены в файл
	json SaveScene()
	{
		json scene;
		for (auto& obj : objects) {
			json jObj;
			jObj["name"] = obj->name;

			for (auto& c : obj->components)
			{
				json jComp = c->Serialize();

				jObj["components"].push_back(jComp);
			};

			scene["objects"].push_back(jObj);
		}; 
		ofstream ofs("scene.json");
		ofs << scene.dump();
		return scene;
	}


	//здесь загрузка сцены из json файла
	void LoadScene(){
		ifstream file("scene.json");
		json j;
		file >> j;

		std::vector<std::unique_ptr<GameObject>> scene;

		for (auto& obj : j["objects"]) {
			objects.push_back(LoadGameObject(obj)); //
		}

	}


	//Загрузка объекта и всех его компонентов 
	std::unique_ptr<GameObject> LoadGameObject(const json& j)
	{
		
		string name = j["name"];
		
		//создаем ссылку на объект

		auto obj = std::make_unique<GameObject>(name);

		for (auto& c : j["components"]) {
			string type = c["type"];

			//загрузка и присваивание каждого компонента по имени
			if (type == "Transform")
			{
				Transform* t = obj->GetComponent<Transform>(); //создается по умолчанию
				t->Deserialize(c);
			}

			else if (type == "MeshRenderer")
			{
				MeshRenderer* mr = obj->AddComponent<MeshRenderer>(c["path"], GlobalShaderProgram);
				mr->Deserialize(c);
			}
			else if (type == "MeshRenderer")
			{
				MeshRenderer* mr = obj->AddComponent<MeshRenderer>(c["path"], GlobalShaderProgram);
				mr->Deserialize(c);
			}
		}
		return obj;
	}
};

